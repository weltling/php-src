/*
  +----------------------------------------------------------------------+
  | Zend Big Integer Support - LibTomMath backend                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2014 The PHP Group                                     |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Andrea Faulds <ajf@ajf.me>                                  |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>

#include "zend.h"
#include "zend_types.h"
#include "zend_bigint.h"
#include "zend_string.h"

/* These custom allocators are commented out since they are useless unless
 * libtommath is built with XMALLOC defined, otherwise it just uses libc
 */

#if 0
/* emalloc/realloc/free are macros, not functions, so we need wrappers to pass
   as our custom allocators */
void* XMALLOC(size_t size)
{
	return emalloc(size);
}

void* XREALLOC(void *ptr, size_t size)
{
	return erealloc(ptr, size);
}

void* XCALLOC(size_t num, size_t size)
{
	return ecalloc(num, size);
}

void XFREE(void *ptr)
{
	efree(ptr);
}
#endif

#include "tommath.h"

struct _zend_bigint {
    zend_refcounted   gc;
    mp_int            mp;
};

/*** INTERNAL MACROS ***/

#define int_abs(n) ((n) >= 0 ? n : -n) 
#define int_sgn(n) ((n) > 0 ? 1 : ((n) < 0 ? -1 : 0))
#define big_sgn(n) (mpz_sgn((n)->mpz))

/*** INTERNAL FUNCTIONS ***/

/* Called by zend_startup */
void zend_startup_bigint(void)
{
}

/*** INITIALISERS ***/

/* Allocates a bigint and returns pointer, does NOT initialise
 * HERE BE DRAGONS: Memory allocated internally by gmp is non-persistent */
ZEND_API zend_bigint* zend_bigint_alloc(void) /* {{{ */
{
	return emalloc(sizeof(zend_bigint));
}
/* }}} */

/* Initialises a bigint
 * HERE BE DRAGONS: Memory allocated internally by gmp is non-persistent */
ZEND_API void zend_bigint_init(zend_bigint *big) /* {{{ */
{
	GC_REFCOUNT(big) = 1;
	GC_TYPE_INFO(big) = IS_BIGINT;
	mp_init(&big->mp);
}
/* }}} */

/* Convenience function: Allocates and initialises a bigint, returns pointer
 * HERE BE DRAGONS: Memory allocated internally by gmp is non-persistent */
ZEND_API zend_bigint* zend_bigint_init_alloc(void) /* {{{ */
{
	zend_bigint *return_value;
	return_value = zend_bigint_alloc();
	zend_bigint_init(return_value);
	return return_value;
}
/* }}} */

/* Initialises a bigint from a string with the specified base (in range 2-36)
 * Returns FAILURE on failure (if the string is not entirely numeric), else SUCCESS
 * HERE BE DRAGONS: Memory allocated internally by gmp is non-persistent */
ZEND_API int zend_bigint_init_from_string(zend_bigint *big, const char *str, int base) /* {{{ */
{
	zend_bigint_init(big);
	if (mp_read_radix(&big->mp, str, base) < 0) {
		mp_clear(&big->mp);
		return FAILURE;
	}
	return SUCCESS;
}
/* }}} */

/* Initialises a bigint from a string with the specified base (in range 2-36)
 * Takes a length - due to an extra memory allocation, this function is slower
 * Returns FAILURE on failure, else SUCCESS
 * HERE BE DRAGONS: Memory allocated internally by gmp is non-persistent */
ZEND_API int zend_bigint_init_from_string_length(zend_bigint *big, const char *str, size_t length, int base) /* {{{ */
{
	char *temp_str = estrndup(str, length);
	zend_bigint_init(big);
	if (mp_read_radix(&big->mp, temp_str, base) < 0) {
		mp_clear(&big->mp);
		efree(temp_str);
		return FAILURE;
	}
	efree(temp_str);
	return SUCCESS;
}
/* }}} */

/* Intialises a bigint from a C-string with the specified base (10 or 16)
 * If endptr is not NULL, it it set to point to first character after number
 * If base is zero, it shall be detected from the prefix: 0x/0X for 16, else 10
 * Leading whitespace is ignored, will take as many valid characters as possible
 * Stops at first non-valid character, else null byte
 * If there are no valid characters, the bigint is initialised to zero
 * This behaviour is supposed to match that of strtol but is not exactly the same
 * HERE BE DRAGONS: Memory allocated internally by gmp is non-persistent */
ZEND_API void zend_bigint_init_strtol(zend_bigint *big, const char *str, char** endptr, int base) /* {{{ */
{
	size_t len = 0;

	/* Skip leading whitespace */
	while (isspace(*str)) {
		str++;
	}

	/* A single sign is valid */
	if (str[0] == '+' || str[0] == '-') {
		len += 1;
	}

	/* detect hex prefix */
	if (base == 0) {
		if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
			base = 16;
			str += 2;
		} else {
			base = 10;
		}
	}

	if (base == 10) {
		while (isdigit(str[len])) {
			len++;
		}
	} else if (base == 16) {
		while (isxdigit(str[len])) {
			len++;
		}
	}

	zend_bigint_init(big);
	if (len) {
		char *temp_str = estrndup(str, len);
		/* we ignore the return value since if it fails it'll just be zero anyway */
		mp_read_radix(&big->mp, temp_str, base);
		efree(temp_str);
	}

	if (endptr) {
		*endptr = (char*)(str + len);
	}
}
/* }}} */

/* Initialises a bigint from a long */
ZEND_API void zend_bigint_init_from_long(zend_bigint *big, zend_long value) /* {{{ */
{
	zend_bigint_init(big);
	/* FIXME: Handle >32-bit longs */
	mp_set_int(&big->mp, int_abs(value));
	if (int_sgn(value) == -1) {
		mp_neg(&big->mp, &big->mp);
	}
}
/* }}} */

/* Initialises a bigint from a double
 * HERE BE DRAGONS: Memory allocated internally by gmp is non-persistent */
ZEND_API void zend_bigint_init_from_double(zend_bigint *big, double value) /* {{{ */
{
	zend_bigint_init(big);
	/* prevents crash and ensures zed_dval_to_lval conformity */
	if (zend_finite(value) && !zend_isnan(value)) {
		/* FIXME: Handle larger doubles */
		zend_bigint_init_from_long(big, zend_dval_to_lval(value));
	}
}
/* }}} */

/* Initialises a bigint and duplicates a bigint to it (copies value)
 * HERE BE DRAGONS: Memory allocated internally by gmp is non-persistent */
ZEND_API void zend_bigint_init_dup(zend_bigint *big, const zend_bigint *source) /* {{{ */
{
	zend_bigint_init(big);
	mp_copy(&source->mp, &big->mp);
}
/* }}} */

/* Destroys a bigint (does NOT deallocate) */
ZEND_API void zend_bigint_dtor(zend_bigint *big) /* {{{ */
{
	mp_clear(&big->mp);
}
/* }}} */

/* Decreases the refcount of a bigint and, if <= 0, destroys and frees it */
ZEND_API void zend_bigint_release(zend_bigint *big) /* {{{ */
{
	if (--GC_REFCOUNT(big) <= 0) {
		zend_bigint_dtor(big);
		efree(big);
	}
}
/* }}} */

/*** INFORMATION ***/

/* Returns true if bigint can fit into an unsigned long without truncation */
ZEND_API zend_bool zend_bigint_can_fit_ulong(const zend_bigint *big) /* {{{ */
{
	/* FIXME: non-stub */
	return 0;
}
/* }}} */

/* Returns true if bigint can fit into a long without truncation */
ZEND_API zend_bool zend_bigint_can_fit_long(const zend_bigint *big) /* {{{ */
{
	/* FIXME: non-stub */
	return 0;
}
/* }}} */

/* Returns sign of bigint (-1 for negative, 0 for zero or 1 for positive) */
ZEND_API int zend_bigint_sign(const zend_bigint *big) /* {{{ */
{
	return SIGN(&big->mp);
}
/* }}} */

/* Returns true if bigint is divisible by a bigint */
ZEND_API zend_bool zend_bigint_divisible(const zend_bigint *num, const zend_bigint *divisor) /* {{{ */
{
	/* FIXME: non-stub */
	return 0;
}
/* }}} */

/* Returns true if bigint is divisible by a long */
ZEND_API zend_bool zend_bigint_divisible_long(const zend_bigint *num, zend_long divisor) /* {{{ */
{
	/* FIXME: non-stub */
	return 0;
}
/* }}} */

/* Returns true if long is divisible by a bigint */
ZEND_API zend_bool zend_bigint_long_divisible(zend_long num, const zend_bigint *divisor) /* {{{ */
{
	/* FIXME: non-stub */
	return 0;
}
/* }}} */

/*** CONVERTORS ***/

/* Converts to long; if it won't fit, wraps around (like zend_dval_to_lval) */
ZEND_API zend_long zend_bigint_to_long(const zend_bigint *big) /* {{{ */
{
	/* FIXME: reimplement dval_to_lval algo and handle larger nos */
	return mp_get_int(&big->mp) * SIGN(&big->mp);
}

/* Converts to long; if it won't fit, saturates (caps at ZEND_LONG_MAX/_MIN) */
ZEND_API zend_long zend_bigint_to_long_saturate(const zend_bigint *big) /* {{{ */
{
	/* FIXME: reimplement saturation algo and handle larger nos */
	return mp_get_int(&big->mp) * SIGN(&big->mp);
}
/* }}} */

/* Converts to long; if it won't fit, may return garbage
 * If it didn't fit, sets overflow to 1, else to 0 */
ZEND_API zend_long zend_bigint_to_long_ex(const zend_bigint *big, zend_bool *overflow) /* {{{ */
{
	/* FIXME: reimplement saturation algo and handle larger nos */
	*overflow = 0;
	return mp_get_int(&big->mp) * SIGN(&big->mp);
}
/* }}} */

/* Converts to unsigned long; this will cap at the max value of an unsigned long */
ZEND_API zend_ulong zend_bigint_to_ulong(const zend_bigint *big) /* {{{ */
{
	/* FIXME: reimplement saturation algo and handle larger nos */
	return mp_get_int(&big->mp);
}

/* Converts to bool */
ZEND_API zend_bool zend_bigint_to_bool(const zend_bigint *big) /* {{{ */
{
	return SIGN(&big->mp);
}
/* }}} */

/* Converts to double; this will lose precision beyond a certain point */
ZEND_API double zend_bigint_to_double(const zend_bigint *big) /* {{{ */
{
	/* TODO: Handle larger numbers */
	return mp_get_int(&big->mp) * SIGN(&big->mp);
}
/* }}} */

/* Converts to decimal C string
 * HERE BE DRAGONS: String allocated is non-persistent */
ZEND_API char* zend_bigint_to_string(const zend_bigint *big) /* {{{ */
{
	int size;
	char *str;
	mp_radix_size(&big->mp, 10, &size);
	str = emalloc(size);
	mp_toradix(&big->mp, str, 10);
	return str;
}
/* }}} */

/* Convenience function: Converts to zend string */
ZEND_API zend_string* zend_bigint_to_zend_string(const zend_bigint *big, int persistent) /* {{{ */
{
	int size;
	zend_string *str;
	str = zend_string_alloc(size - 1, persistent);
	mp_toradix(&big->mp, str->val, 10);
	return str;
}
/* }}} */

/* Converts to C string of arbitrary base */
ZEND_API char* zend_bigint_to_string_base(const zend_bigint *big, int base) /* {{{ */
{
	int size;
	char *str;
	mp_radix_size(&big->mp, base, &size);
	str = emalloc(size);
	mp_toradix(&big->mp, str, base);
	return str;
}
/* }}} */

/*** OPERATIONS **/

/* By the way, in case you're wondering, you can indeed use something as both
 * output and operand. For example, zend_bigint_add_long(foo, foo, 1) is
 * perfectly valid for incrementing foo.
 * This is not advisable, however, because bigints are reference-counted
 * and are copy-on-write so far as userland PHP code cares. Do it sparingly,
 * and never to bigints which have been exposed to userland, unless they only
 * have a single reference. With great power comes great responsibility.
 */

/* Adds two bigints and stores result in out */
ZEND_API void zend_bigint_add(zend_bigint *out, const zend_bigint *op1, const zend_bigint *op2) /* {{{ */
{
	mp_add(&op1->mp, &op2->mp, &out->mp);
}
/* }}} */

/* Adds a bigint and a long and stores result in out */
ZEND_API void zend_bigint_add_long(zend_bigint *out, const zend_bigint *op1, zend_long op2) /* {{{ */
{
	/* FIXME: Handle large op2 */
	if (int_sgn(op2) >= 1) {
		mp_add_d(&op1->mp, op2, &out->mp);
	} else {
		mp_sub_d(&op1->mp, -op2, &out->mp);
	}
}
/* }}} */

/* Adds a long and a long and stores result in out */
ZEND_API void zend_bigint_long_add_long(zend_bigint *out, zend_long op1, zend_long op2) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Subtracts two bigints and stores result in out */
ZEND_API void zend_bigint_subtract(zend_bigint *out, const zend_bigint *op1, const zend_bigint *op2) /* {{{ */
{
	mp_sub(&op1->mp, &op2->mp, &out->mp);
}
/* }}} */

/* Subtracts a bigint and a long and stores result in out */
ZEND_API void zend_bigint_subtract_long(zend_bigint *out, const zend_bigint *op1, zend_long op2) /* {{{ */
{
	/* FIXME: Handle large op2 */
	if (int_sgn(op2) >= 1) {
		mp_sub_d(&op1->mp, op2, &out->mp);
	} else {
		mp_add_d(&op1->mp, -op2, &out->mp);
	}
}
/* }}} */

/* Subtracts a long and a long and stores result in out */
ZEND_API void zend_bigint_long_subtract_long(zend_bigint *out, zend_long op1, zend_long op2) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Subtracts a long and a bigint and stores result in out */
ZEND_API void zend_bigint_long_subtract(zend_bigint *out, zend_long op1, const zend_bigint *op2) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Multiplies two bigints and stores result in out */
ZEND_API void zend_bigint_multiply(zend_bigint *out, const zend_bigint *op1, const zend_bigint *op2) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Multiplies a bigint and a long and stores result in out */
ZEND_API void zend_bigint_multiply_long(zend_bigint *out, const zend_bigint *op1, zend_long op2) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Multiplies a long and a long and stores result in out */
ZEND_API void zend_bigint_long_multiply_long(zend_bigint *out, zend_long op1, zend_long op2) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Raises a bigint base to an unsigned long power and stores result in out */
ZEND_API void zend_bigint_pow_ulong(zend_bigint *out, const zend_bigint *base, zend_ulong power) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Raises a long base to an unsigned long power and stores result in out */
ZEND_API void zend_bigint_long_pow_ulong(zend_bigint *out, zend_long base, zend_ulong power) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Divides a bigint by a bigint and stores result in out */
ZEND_API void zend_bigint_divide(zend_bigint *out, const zend_bigint *num, const zend_bigint *divisor) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Divides a bigint by a bigint and returns result as a double */
ZEND_API double zend_bigint_divide_as_double(const zend_bigint *num, const zend_bigint *divisor) /* {{{ */
{
	/* FIXME: Non-stub */
	return NAN;
}
/* }}} */

/* Divides a bigint by a long and stores result in out */
ZEND_API void zend_bigint_divide_long(zend_bigint *out, const zend_bigint *num, zend_long divisor) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Divides a bigint by a long and returns result as a double */
ZEND_API double zend_bigint_divide_long_as_double(const zend_bigint *num, zend_long divisor) /* {{{ */
{
	/* FIXME: Non-stub */
	return NAN;
}
/* }}} */

/* Divides a long by a bigint and stores result in out */
ZEND_API void zend_bigint_long_divide(zend_bigint *out, zend_long num, const zend_bigint *divisor) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Divides a long by a bigint and returns result as a double */
ZEND_API double zend_bigint_long_divide_as_double(zend_long num, const zend_bigint *divisor) /* {{{ */
{
	/* FIXME: Non-stub */
	return NAN;
}
/* }}} */

/* Finds the remainder of the division of a bigint by a bigint and stores result in out */
ZEND_API void zend_bigint_modulus(zend_bigint *out, const zend_bigint *num, const zend_bigint *divisor) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Finds the remainder of the division of a bigint by a long and stores result in out */
ZEND_API void zend_bigint_modulus_long(zend_bigint *out, const zend_bigint *num, zend_long divisor) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Finds the remainder of the division of a long by a bigint and stores result in out */
ZEND_API void zend_bigint_long_modulus(zend_bigint *out, zend_long num, const zend_bigint *divisor) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Finds the one's complement of a bigint and stores result in out */
ZEND_API void zend_bigint_ones_complement(zend_bigint *out, const zend_bigint *op) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Finds the bitwise OR of a bigint and a bigint and stores result in out */
ZEND_API void zend_bigint_or(zend_bigint *out, const zend_bigint *op1, const zend_bigint *op2) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Finds the bitwise OR of a bigint and a long and stores result in out */
ZEND_API void zend_bigint_or_long(zend_bigint *out, const zend_bigint *op1, zend_long op2) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Finds the bitwise OR of a long and a bigint and stores result in out */
ZEND_API void zend_bigint_long_or(zend_bigint *out, zend_long op1, const zend_bigint *op2) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Finds the bitwise AND of a bigint and a bigint and stores result in out */
ZEND_API void zend_bigint_and(zend_bigint *out, const zend_bigint *op1, const zend_bigint *op2) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Finds the bitwise AND of a bigint and a long and stores result in out */
ZEND_API void zend_bigint_and_long(zend_bigint *out, const zend_bigint *op1, zend_long op2) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Finds the bitwise AND of a long and a bigint and stores result in out */
ZEND_API void zend_bigint_long_and(zend_bigint *out, zend_long op1, const zend_bigint *op2) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Finds the bitwise XOR of a bigint and a bigint and stores result in out */
ZEND_API void zend_bigint_xor(zend_bigint *out, const zend_bigint *op1, const zend_bigint *op2) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Finds the bitwise XOR of a bigint and a long and stores result in out */
ZEND_API void zend_bigint_xor_long(zend_bigint *out, const zend_bigint *op1, zend_long op2) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Finds the bitwise XOR of a long and a bigint and stores result in out */
ZEND_API void zend_bigint_long_xor(zend_bigint *out, zend_long op1, const zend_bigint *op2) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Shifts a bigint left by an unsigned long and stores result in out */
ZEND_API void zend_bigint_shift_left_ulong(zend_bigint *out, const zend_bigint *num, zend_ulong shift) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Shifts a long left by an unsigned long and stores result in out */
ZEND_API void zend_bigint_long_shift_left_ulong(zend_bigint *out, zend_long num, zend_ulong shift) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Shifts a bigint right by an unsigned long and stores result in out */
ZEND_API void zend_bigint_shift_right_ulong(zend_bigint *out, const zend_bigint *num, zend_ulong shift) /* {{{ */
{
	/* FIXME: Non-stub */
}
/* }}} */

/* Compares a bigint and a bigint and returns result (negative if op1 > op2, zero if op1 == op2, positive if op1 < op2) */
ZEND_API int zend_bigint_cmp(const zend_bigint *op1, const zend_bigint *op2) /* {{{ */
{
	/* FIXME: Non-stub */
	return -1;
}
/* }}} */

/* Compares a bigint and a long and returns result (negative if op1 > op2, zero if op1 == op2, positive if op1 < op2) */
ZEND_API int zend_bigint_cmp_long(const zend_bigint *op1, zend_long op2) /* {{{ */
{
	/* FIXME: Non-stub */
	return -1;
}
/* }}} */

/* Compares a bigint and a double and returns result (negative if op1 > op2, zero if op1 == op2, positive if op1 < op2) */
ZEND_API int zend_bigint_cmp_double(const zend_bigint *op1, double op2) /* {{{ */
{
	/* FIXME: Non-stub */
	return -1;
}
/* }}} */

/* Finds the absolute value of a bigint and stores result in out */
ZEND_API void zend_bigint_abs(zend_bigint *out, const zend_bigint *big) /* {{{ */
{
	/* FIXME: Non-stub */
}
