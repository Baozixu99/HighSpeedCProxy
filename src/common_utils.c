#include "common_utils.h"

void error_print(char *error){
#if UTILS_ENABLE_DEBUG
    printf("%s\n", error);
#endif
}
