#include "boot_params.h"

boot_params_t * get_boot_params() {
    return UINT2PTR(PADDR_BOOT_PARAMS);
}
