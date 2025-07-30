#include <stdio.h>
#define XTOOLS_IMPLEMENTATION
#include "../xtools.h"

int main() {
	while (1) {
		xt_status_t status = XT_OK;
		char token[4] = {0};
		xt_get_token(token, sizeof(token), &status);
		
		if (status == XT_OK) {printf(">%s<\n", token);}
		else {fprintf(stderr, "ERROR: %s\n", xt_status_str(status));}
		
		if (status == XT_EOF) {break;}
	}
	return 0;
}
