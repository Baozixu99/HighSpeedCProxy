#include "common_utils.h"

void error_print(char *error){
    printf("%s\n", error);
}


int debug_print(const char *format, ...){
#if UTILS_ENABLE_DEBUG
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#endif 
}