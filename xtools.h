#ifndef XTOOLS_H
#define XTOOLS_H

#include <stdint.h>

#define STATUS_CODES \
    X(XT_OK,             "OK") \
    X(XT_EOF,            "End of input") \
    X(XT_INVALID_INPUT,  "Invalid input") \
    X(XT_OUT_OF_RANGE,   "Out of range") \
    X(XT_INTERNAL_ERROR, "Internal error")

typedef enum {
    #define X(code, msg) code,
    STATUS_CODES
    #undef X
} xt_status_t;

#define TYPES_LIST \
	X(i64, signed,   int64_t,      INT64_MIN, INT64_MAX)  \
	X(i32, signed,   int32_t,      INT32_MIN, INT32_MAX)  \
	X(i16, signed,   int16_t,      INT16_MIN, INT16_MAX)  \
	X(i8,  signed,   int8_t,       INT8_MIN,  INT8_MAX)   \
	X(u64, unsigned, uint64_t,     0,         UINT64_MAX) \
	X(u32, unsigned, uint32_t,     0,         UINT32_MAX) \
	X(u16, unsigned, uint16_t,     0,         UINT16_MAX) \
	X(u8,  unsigned, uint8_t,      0,         UINT8_MAX)  \
	X(ld,  floating, long double, -LDBL_MAX,  LDBL_MAX)   \
	X(d,   floating, double,      -DBL_MAX,   DBL_MAX)    \
	X(f,   floating, float,       -FLT_MAX,   FLT_MAX)

// ---------------------
// FUNCTION DECLARATIONS
// ---------------------

#include <stddef.h>
#include <stdbool.h>

const char * xt_status_str(xt_status_t status);

// parsing
#define X(name, variant, type, min, max) type xt_parse_##name (const char * str, xt_status_t * status);
TYPES_LIST
#undef X

// scanning
#define X(name, variant, type, min, max) \
    type xt_fscan_##name(FILE * fp, bool * newline_found, xt_status_t * status); \
    type xt_scan_##name(bool * newline_found, xt_status_t * status);
TYPES_LIST
#undef X

void xt_clear_input(bool newline_found);
bool xt_fget_token(FILE * fp, char * str, size_t len, xt_status_t * status);
bool xt_get_token(char * str, const size_t len, xt_status_t * status);

char * xt_fget_line(FILE * fp, xt_status_t * status);

#ifdef XTOOLS_IMPLEMENTATION

// --------------------
// COMMON FUNCTIONALITY
// --------------------

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <float.h>
#include <errno.h>

#define signed_t   int64_t
#define unsigned_t uint64_t
#define floating_t long double

#define FAIL(msg) \
    do { \
        fprintf(stderr, "[ERROR] Function \"%s\": " msg "\n", __func__); \
        exit(EXIT_FAILURE); \
    } while (0)

#define END_IF_STATUS_NOT_OK \
    do { \
        if (status && *status != XT_OK) return 0; \
    } while (0)

#define SET_STATUS(status, code) \
	do { \
		if (status && *status == XT_OK) {*status = code;} \
	} while (0)

const char * xt_status_str(xt_status_t status) {
    switch (status) {
        #define X(code, msg) case code: return msg;
        STATUS_CODES
        #undef X
        default: return "Unknown";
    }
}

// ---------------------
// PARSING FUNCTIONALITY
// ---------------------

#define parse_signed(str, endptr)   strtoll(str, endptr, 10)
#define parse_unsigned(str, endptr) strtoull(str, endptr, 10)
#define parse_floating(str, endptr) strtold(str, endptr)

#define CHECK_BAD_RANGE_signed(value, min, max) ((value) < (min) || (value) > (max))
#define CHECK_BAD_RANGE_unsigned(value, min, max) ((value) > (max))
#define CHECK_BAD_RANGE_floating(value, min, max) CHECK_BAD_RANGE_signed(value, min, max)

#define CHECK_BAD_RANGE(variant, value, min, max) CHECK_BAD_RANGE_##variant(value, min, max)

#define X(name, variant, type, min, max) \
    type xt_parse_##name (const char * str, xt_status_t * status) {\
\
        const char * tkn_start = NULL;\
        const char * tkn_end   = NULL;\
        \
        for (size_t i = 0; *(str+i) != '\0'; i++) {\
            if (!isspace(str[i])) {\
                if (!tkn_start) {tkn_start = str + i;}\
                tkn_end = str + i;\
            }\
        }\
        if (tkn_start == NULL)\
            {goto INVALID_TOKEN;}\
\
        char * parse_end;\
        errno = 0;\
        const variant##_t value = parse_##variant(tkn_start, &parse_end);\
        if (parse_end <= tkn_end)\
            {goto INVALID_TOKEN;}\
        if (errno == ERANGE || CHECK_BAD_RANGE(variant, value, min, max)) \
            {goto OUT_OF_RANGE;}\
\
        return value;\
\
        INVALID_TOKEN:\
        SET_STATUS(status, XT_INVALID_INPUT);\
        return 0;\
\
        OUT_OF_RANGE:\
        SET_STATUS(status, XT_OUT_OF_RANGE);\
        return 0;\
    }
TYPES_LIST
#undef X

// ----------------------
// SCANNING FUNCTIONALITY
// ----------------------

void xt_clear_input(bool newline_found) {
    if (newline_found) {return;}
    int c;
    do {
        c = getchar();
    } while (c != '\n' && c != EOF);
}

bool xt_fget_token(FILE * fp, char * str, const size_t len, xt_status_t * status) {
    if (fp  == NULL)   {FAIL("fp can not be NULL!");}
    if (str == NULL)   {FAIL("str can not be NULL!");}
    if (len == 0)      {FAIL("len can not be 0!");}
    END_IF_STATUS_NOT_OK;

    bool newline_found = false;

    size_t tkn_len = 0;
    while (1) {
        const int c = fgetc(fp);
        if (c == EOF) {
            if (tkn_len == 0) {goto EOF_NO_CHAR;}
            break;
        }
        if (c == '\n') {
            newline_found = true;
            break;
        }
        if (isspace(c)) {
            if (tkn_len == 0) {continue;}
            break;
        }

        if (tkn_len < len - 1 || (tkn_len == 0 && len == 1)) {
        	str[tkn_len++] = (char) c;
        }
        else {
            SET_STATUS(status, XT_OUT_OF_RANGE);
        }
    }

    // If not single char
    if (len != 1) {str[tkn_len] = '\0';}
    return newline_found;

    EOF_NO_CHAR:
    SET_STATUS(status, XT_EOF);
    return false;
}

bool xt_get_token(char * str, const size_t len, xt_status_t * status) {
    return xt_fget_token(stdin, str, len, status);
}

#define X(name, variant, type, min, max) \
    type xt_fscan_##name (FILE * fp, bool * newline_found, xt_status_t * status) { \
        if (status && *status != XT_OK) {return 0;} \
\
        xt_status_t code = XT_OK; \
\
        char buffer[512] = {0}; \
        const bool _newline_found = xt_fget_token(fp, buffer, sizeof(buffer), &code); \
        if (newline_found) {*newline_found = _newline_found;}\
        if (code != XT_OK) {goto TOKENIZATION_ERROR;}\
        if (buffer[0] == '\0') {goto INVALID_TOKEN;}\
\
        const type value = xt_parse_##name(buffer, &code);\
        if (code != XT_OK) {goto PARSING_ERROR;}\
\
        return value;\
\
        TOKENIZATION_ERROR: \
        PARSING_ERROR: \
        SET_STATUS(status, code); \
        return 0; \
\
        INVALID_TOKEN: \
        SET_STATUS(status, XT_INVALID_INPUT);\
        return 0;\
    } \
\
    type xt_scan_##name (bool * newline_found, xt_status_t * status) { \
        return xt_fscan_##name (stdin, newline_found, status); \
    }
TYPES_LIST
#undef X

char * xt_fget_line(FILE * fp, xt_status_t * status) {
	size_t line_cap = 16;
	size_t line_len = 0;
	char * line = malloc(line_cap);
	if (line == NULL) {goto ERR_ALLOC;}

	while (1) {
		const char c = fgetc(fp);
		if (c == EOF && line_len == 0) {goto EOF_NO_CHAR;}
		if (c == '\n' || c == EOF) {break;}

		if (line_len == line_cap - 1) {
			const size_t new_cap = line_cap * 2;
			char * tmp_line = realloc(line, new_cap);
			if (tmp_line == NULL) {goto ERR_REALLOC;}
			line = tmp_line;
			line_cap = new_cap;
		}

		line[line_len++] = c;
	}

	line[line_len] = '\0';
	return line;

	ERR_REALLOC:
	free(line);
	ERR_ALLOC:
	SET_STATUS(status, XT_INTERNAL_ERROR);
	return NULL;

	EOF_NO_CHAR:
	free(line);
	SET_STATUS(status, XT_EOF);
	return NULL;
}

// ----------------
// UNDEF ALL MACROS
// ----------------

#undef FAIL
#undef END_IF_STATUS_NOT_OK

#undef parse_signed
#undef parse_unsigned
#undef parse_floating

#undef signed_t
#undef unsigned_t
#undef floating_t

#undef CHECK_BAD_RANGE_signed
#undef CHECK_BAD_RANGE_unsigned
#undef CHECK_BAD_RANGE_floating
#undef CHECK_BAD_RANGE

#undef SET_STATUS

#endif // XTOOLS_IMPLEMENTATION

#undef STATUS_CODES
#undef TYPES_LIST

#endif // XTOOLS_H
