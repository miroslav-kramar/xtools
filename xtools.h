#ifndef XTOOLS_H
#define XTOOLS_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

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
	X(i64,   signed,   int64_t,      INT64_MIN, INT64_MAX)  \
	X(i32,   signed,   int32_t,      INT32_MIN, INT32_MAX)  \
	X(i16,   signed,   int16_t,      INT16_MIN, INT16_MAX)  \
	X(i8,    signed,   int8_t,       INT8_MIN,  INT8_MAX)   \
	X(u64,   unsigned, uint64_t,     0,         UINT64_MAX) \
	X(u32,   unsigned, uint32_t,     0,         UINT32_MAX) \
	X(u16,   unsigned, uint16_t,     0,         UINT16_MAX) \
	X(u8,    unsigned, uint8_t,      0,         UINT8_MAX)  \
	X(ldbl,  floating, long double, -LDBL_MAX,  LDBL_MAX)   \
	X(dbl,   floating, double,      -DBL_MAX,   DBL_MAX)    \
	X(flt,   floating, float,       -FLT_MAX,   FLT_MAX)

typedef struct {
    FILE * stream;
	const char * delim;
	xt_status_t status;
	bool newline_found;
} xt_scanner_t;

xt_scanner_t xt_set_scanner(FILE * stream, const char * delim);

// ---------------------
// FUNCTION DECLARATIONS
// ---------------------

// parsing
#define X(name, variant, type, min, max) \
    type xt_parse_##name (const char * str, xt_status_t * status);
TYPES_LIST
#undef X

// scanning
const char * xt_status_str(xt_status_t status);

#define X(name, variant, type, min, max) \
    type xt_scan_##name(xt_scanner_t * sc);
TYPES_LIST
#undef X

void xt_clear_input(xt_scanner_t * sc);
size_t xt_get_token(xt_scanner_t * sc, char * buffer, size_t len);

char * xt_get_line(xt_scanner_t * sc);

#ifdef XTOOLS_IMPLEMENTATION

// --------------------
// COMMON FUNCTIONALITY
// --------------------

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
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

xt_scanner_t xt_set_scanner(FILE * stream, const char * delim) {
    const char * const DEFAULT_DELIM = " \t";
    xt_scanner_t out;
    out.stream = stream;
    out.status = XT_OK;
    out.newline_found = false;
    if (delim == NULL) {out.delim = DEFAULT_DELIM;}
    else {out.delim = delim;}
    return out;
}

void xt_clear_input(xt_scanner_t * sc) {
    if (sc->newline_found) {return;}
    int c;
    do {
        c = fgetc(sc->stream);
        if (c == EOF) {goto EOF_FOUND;}
    } while (c != '\n');
    sc->newline_found = true;
    return;

    EOF_FOUND:
    sc->status = XT_EOF;
}

size_t xt_get_token(xt_scanner_t * sc, char * buffer, const size_t len) {
    if (sc->status != XT_OK) {return 0;}

    if (sc->newline_found) {sc->newline_found = false;}

    size_t tkn_len = 0;
    while (1) {
        const int c = fgetc(sc->stream);
        if (c == EOF) {
            if (tkn_len == 0) {goto EOF_NO_CHAR;}
            break;
        }
        if (c == '\n') {
            sc->newline_found = true;
            break;
        }
        if (strchr(sc->delim, c) != NULL) {
            if (tkn_len == 0) {continue;}
            break;
        }

        if (tkn_len < len - 1 || (tkn_len == 0 && len == 1)) {
        	buffer[tkn_len++] = (char) c;
        }
        else {
            sc->status = XT_OUT_OF_RANGE;
        }
    }

    // If not single char
    if (len != 1) {buffer[tkn_len] = '\0';}
    return tkn_len;

    EOF_NO_CHAR:
    sc->status = XT_EOF;
    return 0;
}

#define X(name, variant, type, min, max) \
    type xt_scan_##name (xt_scanner_t * sc) { \
        if (sc->status != XT_OK) {return (type)0;} \
\
        char buffer[512] = {0}; \
        const size_t tkn_len = xt_get_token(sc, buffer, sizeof(buffer)); \
        if (sc->status != XT_OK) {goto TOKENIZATION_ERROR;}\
        if (tkn_len == 0) {goto INVALID_TOKEN;}\
\
        const type value = xt_parse_##name(buffer, &sc->status);\
        if (sc->status != XT_OK) {goto PARSING_ERROR;}\
\
        return value;\
\
        INVALID_TOKEN: \
        sc->status = XT_INVALID_INPUT;\
        PARSING_ERROR: \
        TOKENIZATION_ERROR: \
        return 0;\
    }
TYPES_LIST
#undef X

char * xt_get_line(xt_scanner_t * sc) {
	size_t line_cap = 16;
	size_t line_len = 0;
	char * line = malloc(line_cap);
	if (line == NULL) {goto ERR_ALLOC;}

	while (1) {
		const char c = fgetc(sc->stream);
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
	sc->status = XT_INTERNAL_ERROR;
	return NULL;

	EOF_NO_CHAR:
	free(line);
	sc->status =XT_EOF;
	return NULL;
}

// ----------------
// UNDEF ALL MACROS
// ----------------

#undef FAIL

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
