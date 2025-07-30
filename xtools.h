#ifndef XTOOLS_H
#define XTOOLS_H

#include <stdint.h>

#define STATUS_CODES \
    X(XT_OK,               "OK") \
    X(XT_EOF,              "End of input") \
    X(XT_ERR_INVALID,      "Invalid input") \
    X(XT_ERR_OUT_OF_RANGE, "Out of range")

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

// --------------------
// COMMON FUNCTIONALITY
// --------------------

#ifdef XTOOLS_IMPLEMENTATION

#define signed_t   int64_t
#define unsigned_t uint64_t
#define floating_t long double

#define FAIL(msg) \
    do { \
        fprintf(stderr, "[ERROR] Function \"%s\": " msg "\n", __func__); \
        exit(EXIT_FAILURE); \
    } while (0)

#define END_IF_STATUS_ERROR \
    do { \
        if (status && *status != XT_OK) return 0; \
    } while (0)

const char * xt_status_str(xt_status_t status) {
    switch (status) {
        #define X(code, msg) case code: return msg;
        STATUS_CODES
        #undef X
        default: return "Unknown error";
    }
}

// ---------------------
// PARSING FUNCTIONALITY
// ---------------------

#include <stdlib.h>
#include <float.h>
#include <errno.h>
#include <ctype.h>

#define parse_signed(str, endptr)   strtoll(str, endptr, 10)
#define parse_unsigned(str, endptr) strtoull(str, endptr, 10)
#define parse_floating(str, endptr) strtold(str, endptr)

#define RANGE_CHECK_CONDITION_signed(value, min, max) ((value) < (min) || (value) > (max))
#define RANGE_CHECK_CONDITION_unsigned(value, min, max) ((value) > (max))
#define RANGE_CHECK_CONDITION_floating(value, min, max) RANGE_CHECK_CONDITION_signed(value, min, max)

#define RANGE_CHECK_CONDITION(variant, value, min, max) RANGE_CHECK_CONDITION_##variant(value, min, max)

#define X(name, variant, type, min, max) \
    type xt_parse_##name (const char * str, xt_status_t * status) {\
        xt_status_t code = XT_OK;\
        type out = 0;\
\
        const char * str_ptr    = str;\
        const char * span_start = NULL;\
        const char * span_end   = NULL;\
        \
        while (*str_ptr != '\0') {\
            if (!isspace(*str_ptr)) {\
                if (!span_start) {span_start = str_ptr;}\
                span_end = str_ptr;\
            }\
            str_ptr += 1;\
        }\
        if (span_start == NULL) {\
            code = XT_ERR_INVALID;\
            goto END;\
        }\
\
        char * parse_end;\
        errno = 0;\
        const variant##_t value = parse_##variant(span_start, &parse_end);\
        if (parse_end <= span_end) {\
            code = XT_ERR_INVALID;\
            goto END;\
        }\
        if (errno == ERANGE || RANGE_CHECK_CONDITION(variant, value, min, max) ) {\
            code = XT_ERR_OUT_OF_RANGE;\
            goto END;\
        }\
\
        out = value;\
\
        END:\
        if (status) *status = code;\
        return out;\
    }
TYPES_LIST
#undef X

// ----------------------
// SCANNING FUNCTIONALITY
// ----------------------

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>


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
    END_IF_STATUS_ERROR;

    xt_status_t code = XT_OK;
    bool xt_newline_found = false;

    size_t tkn_len = 0;
    bool in_tkn = false;
    bool got_token = false;
    while (1) {
        const int c = fgetc(fp);
        if (c == EOF) {
            if (!got_token) {
                code = XT_EOF;
                goto END;
            }
            break;
        }
        if (c == '\n') {
            xt_newline_found = true;
            break;
        }

        if (isspace(c)) {
            if (!in_tkn) {continue;}
            break;
        }
        got_token = true;

        in_tkn = true;
        str[tkn_len++] = (char) c;
        if (tkn_len >= len) {
            code = XT_ERR_OUT_OF_RANGE;
            goto END;
        }
    }

    // Do not null terminate when user requested single char
    if (len != 1) {str[tkn_len] = '\0';}

    END:
    if (status && *status == XT_OK) {
        *status = code;
    }
    return xt_newline_found;
}

bool xt_get_token(char * str, const size_t len, xt_status_t * status) {
    return xt_fget_token(stdin, str, len, status);
}

#define X(name, variant, type, min, max) \
    type xt_fscan_##name (FILE * fp, bool * newline_found, xt_status_t * status) { \
        END_IF_STATUS_ERROR; \
\
        xt_status_t err = XT_OK; \
        type out = 0; \
\
        char buffer[512] = {0}; \
        const bool _newline_found = xt_fget_token(fp, buffer, sizeof(buffer), &err); \
        if (newline_found) {*newline_found = _newline_found;} \
        if (err && err != XT_EOF) { \
            goto END; \
        } \
        if (buffer[0] == '\0') {\
            if (err == XT_EOF) {goto END;} \
            err = XT_ERR_INVALID;\
            goto END;\
        }\
\
        const variant##_t value = xt_parse_##name(buffer, &err);\
        if (err) {\
            goto END;\
        }\
        out = value;\
\
        END:\
        if (status && *status == XT_OK) {*status = err;}\
        return out;\
    } \
    type xt_scan_##name (bool * newline_found, xt_status_t * status) { \
        return xt_fscan_##name (stdin, newline_found, status); \
    }
TYPES_LIST
#undef X

// ----------------
// UNDEF ALL MACROS
// ----------------

#undef FAIL
#undef END_IF_STATUS_ERROR

#undef parse_signed
#undef parse_unsigned
#undef parse_floating

#undef signed_t
#undef unsigned_t
#undef floating_t

#undef RANGE_CHECK_CONDITION_signed
#undef RANGE_CHECK_CONDITION_unsigned
#undef RANGE_CHECK_CONDITION_floating
#undef RANGE_CHECK_CONDITION

#endif // XTOOLS_IMPLEMENTATION

#undef STATUS_CODES
#undef TYPES_LIST

#endif // XTOOLS_H