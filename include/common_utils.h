#ifndef COMMON_UTILS_H_
#define COMMON_UTILS_H_

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#define UTILS_ENABLE_DEBUG                        1

void error_print(char *error);

int debug_print(const char *format, ...);

#if UTILS_ENABLE_DEBUG
#define utils_print(...) printf(__VA_ARGS__)
#else
#define utils_print(...) do {} while(0)  // 空操作
#endif

#endif