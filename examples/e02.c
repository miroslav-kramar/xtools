#include <stdio.h>
#define XTOOLS_IMPLEMENTATION
#include "../xtools.h"

int main() {
    xt_scanner_t sc = xt_set_scanner(stdin, ",");
    while (1) {
        int height, width, depth, density;
        height  = xt_scan_i32(&sc);
        width   = xt_scan_i32(&sc);
        depth   = xt_scan_i32(&sc);
        density = xt_scan_i32(&sc);
        xt_clear_input(&sc);
        if (sc.status == XT_EOF) {
            break;
        }
        if (sc.status != XT_OK) {
            printf("ERROR: %s\n", xt_status_str(sc.status));
            sc.status = XT_OK;
        }
        printf("Height: %d, width: %d, depth: %d, density: %d\n", height, width, depth, density);
    }
    printf("Bye!\n");
    return 0;
}