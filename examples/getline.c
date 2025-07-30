#define XTOOLS_IMPLEMENTATION
#include "../xtools.h"

int main() {
	xt_status_t status = XT_OK;
	char * line = xt_fget_line(stdin, &status);
	if (status != XT_OK) {fprintf(stderr, "ERROR: %s\n", xt_status_str(status));}
	else {printf(">%s<\n", line);}
	free(line);
	return 0;
}
