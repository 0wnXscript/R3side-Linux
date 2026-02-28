#ifndef OFFSET_H
#define OFFSET_H

#include <stdint.h>

#define TARGET_LIB "libroblox.so"

#define PRINT_OFFSET 0x20E534A

typedef void (*GameSendChatFunc)(const char* msg, int type);

#endif 