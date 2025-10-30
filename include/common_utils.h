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
#define utils_print(...) do {} while(0)
#endif


/**
 * @brief Print the content of a uint8_t array in the specified format
 * @param BUF Pointer to the start of the uint8_t array (must be a valid pointer)
 * @param SIZE Number of elements in the array (must be a non-negative integer, preferably of type size_t)
 * @param FORMAT printf-style format string (must match the uint8_t type, e.g., "%02hhx ", "%hhu ", "%hho ", "%c")
 * 
 * Notes:
 * 1. The format specifier must use the "hh" length modifier for uint8_t (to avoid sign extension issues):
 *    - Decimal: %hhu (unsigned)
 *    - Hexadecimal: %hhx (lowercase), %hhX (uppercase)
 *    - Octal: %hho
 *    - ASCII: %c (Note: Non-printable characters may display abnormally; handle them as needed)
 * 2. Iterates from index 0 to SIZE-1, outputting each element in the specified FORMAT
 * 3. Automatically adds a newline after output to separate different buffer contents
 */
#define DUMP_BUFFER_CONTENT(BUF, SIZE, FORMAT) do { \
    /* Boundary check: Return directly if SIZE is 0 to avoid invalid loops */ \
    if ((SIZE) == 0) { \
        printf("Buffer is empty (size = 0)\n"); \
        break; \
    } \
    /* Traverse the array and output each element in the specified format */ \
    for (size_t i = 0; i < (SIZE); ++i) { \
        /* Force cast to uint8_t* to ensure type correctness and avoid pointer type mismatch */ \
        printf(FORMAT, ((const uint8_t*)(BUF))[i]); \
    } \
    /* Add a newline after output to distinguish between different buffer outputs */ \
    printf("\n"); \
} while (0)

#endif