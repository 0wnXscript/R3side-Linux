#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <dlfcn.h>
#include <errno.h>

pid_t find_process_by_name(const char *name) {
    DIR* d = opendir("/proc");
    if (!d) return -1;

    struct dirent* e;
    while ((e = readdir(d))) {
        if (e->d_type != DT_DIR) continue;
        pid_t pid = atoi(e->d_name);
        if (pid <= 0) continue;

        char path[512], exe[512];
        snprintf(path, sizeof(path), "/proc/%d/exe", pid);
        ssize_t len = readlink(path, exe, sizeof(exe) - 1);
        if (len != -1) {
            exe[len] = '\0';
            if (strstr(exe, name)) {
                closedir(d);
                return pid;
            }
        }
    }
    closedir(d);
    return -1;
}

unsigned long get_module_base(pid_t pid, const char *name) {
    char path[64], line[512];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE *f = fopen(path, "r");
    if (!f) return 0;

    unsigned long addr = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, name)) {
            addr = strtoul(line, NULL, 16);
            break;
        }
    }
    fclose(f);
    return addr;
}

unsigned long get_symbol_offset(const char *libname, const char *sym) {
    void *h = dlopen(libname, RTLD_LAZY);
    if (!h) return 0;
    void *addr = dlsym(h, sym);
    unsigned long offset = (unsigned long)addr - (unsigned long)get_module_base(getpid(), libname);
    dlclose(h);
    return offset;
}

int injector_attach(pid_t pid, const char *lib_path) {
    struct user_regs_struct orig_regs, regs;
    int status, success = 0;

    if (ptrace(PTRACE_ATTACH, pid, 0, 0) < 0) {
        perror("ptrace ATTACH");
        if (errno == EPERM) {
            fprintf(stderr, " → check /proc/sys/kernel/yama/ptrace_scope (often needs =0 or root)\n");
        }
        return -1;
    }

    waitpid(pid, &status, 0);
    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "Target did not stop\n");
        goto detach;
    }

    if (ptrace(PTRACE_GETREGS, pid, NULL, &orig_regs) < 0) {
        perror("GETREGS");
        goto detach;
    }

    regs = orig_regs;

    unsigned long libc_base   = get_module_base(pid, "libc.so.6");
    unsigned long dlopen_off  = get_symbol_offset("libc.so.6", "dlopen");

    unsigned long target_dlopen = 0;
    if (libc_base && dlopen_off) {
        target_dlopen = libc_base + dlopen_off;
    } else {
        unsigned long libdl_base = get_module_base(pid, "libdl.so.2");
        unsigned long libdl_off  = get_symbol_offset("libdl.so.2", "dlopen");
        if (libdl_base && libdl_off) {
            target_dlopen = libdl_base + libdl_off;
        }
    }

    if (!target_dlopen) {
        fputs("[-] Could not find dlopen address\n", stderr);
        goto restore;
    }

    printf("[+] Using dlopen @ 0x%lx\n", target_dlopen);

    regs.rsp -= 0x1000;             
    unsigned long str_addr = regs.rsp;

    size_t len = strlen(lib_path) + 1;
    for (size_t off = 0; off < len; off += sizeof(long)) {
        long word = 0;
        size_t copy = (len - off < sizeof(long)) ? len - off : sizeof(long);
        memcpy(&word, lib_path + off, copy);

        if (ptrace(PTRACE_POKEDATA, pid, str_addr + off, word) < 0) {
            perror("POKEDATA path");
            goto restore;
        }
    }

    regs.rsp -= 8;
    if (ptrace(PTRACE_POKEDATA, pid, regs.rsp, 0UL) < 0) {
        perror("POKEDATA return addr");
        goto restore;
    }

    regs.rdi = str_addr;         
    regs.rsi = RTLD_NOW;       
    regs.rip = target_dlopen;

    if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) < 0) {
        perror("SETREGS");
        goto restore;
    }

    if (ptrace(PTRACE_CONT, pid, 0, 0) < 0) {
        perror("CONT");
        goto restore;
    }

    waitpid(pid, &status, 0);

    struct user_regs_struct after;
    ptrace(PTRACE_GETREGS, pid, NULL, &after);
    if (after.rax == 0) {
        fputs("[-] dlopen returned NULL → injection likely failed\n", stderr);
    } else {
        printf("[+] Success — dlopen returned handle 0x%lx\n", after.rax);
        success = 1;
    }

restore:
    ptrace(PTRACE_SETREGS, pid, NULL, &orig_regs);
detach:
    ptrace(PTRACE_DETACH, pid, 0, 0);
    return success ? 0 : -1;
}