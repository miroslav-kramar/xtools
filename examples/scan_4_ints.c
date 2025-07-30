#include <stdio.h>
#define XTOOLS_IMPLEMENTATION
#include "xtools.h"

int main() {
    while (1) {
        xt_status_t status = XT_OK;
        bool newline_found = false;
        int data[4];
        for (int i = 0; i < 4; i++) {
            data[i] = xt_scan_i32(&newline_found, &status);
        }
        xt_clear_input(newline_found);

        if (status == XT_EOF) {
            break;
        }
        else if (status) {
            fprintf(stderr, "ERROR: %s\n", xt_status_str(status));
        }
        else {
            for (int i = 0; i < 4; i++) {
                printf("%d ", data[i]);
            }
            putchar('\n');
        }
    }
    printf("Bye!\n");
}