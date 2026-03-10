#include "common_utils.h"

void error_print(char *error){
    printf("%s\n", error);
}


/**
 * @brief Print debug information with variable arguments
 * @param format  Format string (same syntax as printf), must not be NULL
 * @param ...     Variable arguments matching the format string
 * @return        On success: number of characters printed (if debug enabled) or 0 (if debug disabled)
 *                On failure: -1 (e.g., NULL format string or vprintf error)
 * @note          The function behavior depends on UTILS_ENABLE_DEBUG macro:
 *                - Enabled: forward to vprintf and return its result
 *                - Disabled: do nothing and return 0
 */
int debug_print(const char *format, ...) {
    // Return value variable, initialized to 0 (default success)
    int ret = 0;

#if UTILS_ENABLE_DEBUG
    // Validate input parameter: return error if format is NULL
    if (format == NULL) {
        ret = -1;  // Mark error status
        return ret;
    }

    va_list args;
    va_start(args, format);
    // Call vprintf and get its return value: success=char count, failure=-1
    ret = vprintf(format, args);
    va_end(args);
#else
    // Do nothing when debug is disabled, return 0 (indicates "no operation success")
    ret = 0;
#endif

    // Unified return statement to ensure non-void function always has return value
    return ret;
}