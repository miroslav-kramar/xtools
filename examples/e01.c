#include <stdio.h>
#define XTOOLS_IMPLEMENTATION
#include "../xtools.h"

int main() {
    xt_scanner_t sc = xt_set_scanner(stdin, NULL);

    while (1) {
        printf(">> ");
        const int n = xt_scan_i32(&sc);
        xt_clear_input(&sc);

        if (sc.status == XT_EOF) {
            break;
        }
        if (sc.status != XT_OK) {
            printf("ERROR: %s\n", xt_status_str(sc.status));
            sc.status = XT_OK;
            continue;
        }
        printf("%d\n", n);
    }
    printf("\nBye!\n");

    return 0;
}