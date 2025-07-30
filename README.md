# XTools

Single-header library with simple tools to improve your stay in C programming language.

> [!WARNING] 
> Current implementation requires heavy testing. For now, use at your own risk!

## Functionality

Currently, the library offers the following functionality:

- Status codes via `enum`
- Number parsing functions
- Text stream tokenizers

Library supports all basic data types. It uses `stdint.h` for integer types. All floating-point types also supported.

### Status codes

Library defines custom `enum` type `xt_status_t` with the following codes:

- `XT_OK` - Success
- `XT_EOF` - EOF encountered
- `XT_ERR_INVALID` - Indicating un-parsable string
- `XT_ERR_OUT_OF_RANGE` - Indicating a number too large for requested data type

Plus a function for getting text description of each status code:

`const char * xt_status_str(xt_status_t status)`

### Number parsing functions

All number parsing functions have the following signature:

`[type] xt_parse_[name] (const char * str, xt_status_t * status)`

where *type* is 8-64 bit signed/unsigned integer from `stdint.h` or any of the floating-point types. *name* is shortened version of the data type used:

- `i64` for `int64_t`
- `u32` for `uint32_t`
- `ld`  for `long double`
- and so on...

### Text stream tokenizers

> **_NOTE:_** TODO

## Usage

Pick a file which will contain the implementation of all XTools functions. Then define the `XTOOLS_IMPLEMENTATION` flag before including the `xtools.h` library:

```c
#define XTOOLS_IMPLEMENTATION
#include "xtools.h"
...
```

Then, in all other files using XTools functions:

```c
#include "xtools.h"
...
```