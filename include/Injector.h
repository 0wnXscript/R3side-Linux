#ifndef INJECTOR_H
#define INJECTOR_H

#include <sys/types.h>

pid_t find_process_by_name(const char *name);

int injector_attach(pid_t pid, const char *lib_path);

#endif