#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdint.h>

#include "Offset.h"

uintptr_t get_app_base(const char* name) {
    char line[512];
    FILE* f = fopen("/proc/self/maps", "r");
    if (!f)
        return 0;

    uintptr_t addr = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, name)) {
            addr = strtoull(line, NULL, 16);
            break;
        }
    }

    fclose(f);
    return addr;
}

void print_str(const char* message) {
    uintptr_t base = get_app_base(TARGET_LIB);
    if (!base)
        return;

    uintptr_t chat_func_addr =
        base + PRINT_OFFSET;

    GameSendChatFunc send_chat_func = (GameSendChatFunc)(chat_func_addr);

    if (send_chat_func)
        send_chat_func(message, 0); //0 = BasicP, 1 = InfoP, 2 = WarnP, 3 = ErrorP
}

__attribute__((constructor))
void init_lib() {
    print_str("R3side loaded!");
}