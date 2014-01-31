/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2014 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Anatol Belski <ab@php.net>                                   |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_SIZE_INT_COMPAT_H
#define PHP_SIZE_INT_COMPAT_H

/* XXX change the check accordingly to the vote results */
#define PHP_NEED_STRSIZE_COMPAT (PHP_MAJOR_VERSION < 5) || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 6) || defined(PHP_FORCE_STRSIZE_COMPAT)

#if PHP_NEED_STRSIZE_COMPAT

typedef long zend_int_t;
typedef unsigned long zend_uint_t;
typedef long zend_off_t;
typedef int zend_size_t;
typedef struct stat zend_stat_t;
# define ZEND_INT_MAX LONG_MAX
# define ZEND_INT_MIN LONG_MIN
# define ZEND_UINT_MAX ULONG_MAX
# define ZEND_SIZE_MAX SIZE_MAX
# define Z_I(i) i
# define Z_UI(i) i
# define SIZEOF_ZEND_INT SIZEOF_LONG
# define ZEND_STRTOL(s0, s1, base) strtol((s0), (s1), (base))
# define ZEND_STRTOUL(s0, s1, base) strtoul((s0), (s1), (base))
# ifdef PHP_WIN32
#  define ZEND_ITOA(i, s, len) _ltoa_s((i), (s), (len), 10)
#  define ZEND_ATOI(i, s) i = atol((s))
# else
#  define ZEND_ITOA(i, s, len) \
	do { \
		int st = snprintf((s), (len), "%ld", (i)); \
		(s)[st] = '\0'; \
 	} while (0)
#  define ZEND_ATOI(i, s) (i) = atol((s))
# endif
# define ZEND_INT_FMT "%ld"
# define ZEND_UINT_FMT "%lu"
# define ZEND_INT_FMT_SPEC "ld"
# define ZEND_UINT_FMT_SPEC "lu"
# define ZEND_STRTOL_PTR strtol
# define ZEND_STRTOUL_PTR strtoul
# define ZEND_ABS abs
# define zend_fseek fseek
# define zend_ftell ftell
# define zend_lseek lseek
# define zend_fstat fstat
# define zend_stat stat
# define php_fstat fstat
# define php_stat_fn stat

#define php_size_t zend_size_t
#define php_int_t zend_int_t
#define php_uint_t zend_uint_t
#define php_stat_t zend_stat_t
#define PHP_INT_MAX ZEND_INT_MAX
#define PHP_INT_MIN ZEND_INT_MIN
#define PHP_UINT_MAX ZEND_UINT_MAX
#define PHP_SIZE_MAX ZEND_SIZE_MAX

# define MAX_LENGTH_OF_LONG                     MAX_LENGTH_OF_LONG
# define Z_STRLEN                                  Z_STRLEN
# define Z_STRLEN_P                                Z_STRLEN_P
# define Z_STRLEN_PP                               Z_STRLEN_PP
# define Z_LVAL                                     Z_LVAL
# define Z_LVAL_P                                   Z_LVAL_P
# define Z_LVAL_PP                                  Z_LVAL_PP
# define IS_LONG                                     IS_LONG
# define ZVAL_LONG                                   ZVAL_LONG
# define RETVAL_LONG                                 RETVAL_LONG
# define RETURN_LONG                                 RETURN_LONG
# define LITERAL_LONG                                LITERAL_LONG
# define REGISTER_LONG_CONSTANT                      REGISTER_LONG_CONSTANT
# define REGISTER_NS_LONG_CONSTANT                   REGISTER_NS_LONG_CONSTANT
# define REGISTER_MAIN_LONG_CONSTANT                 REGISTER_MAIN_LONG_CONSTANT
# define HASH_KEY_IS_LONG                            HASH_KEY_IS_LONG
# define convert_to_long                             convert_to_long
# define convert_to_long_ex                          convert_to_long_ex
# define add_assoc_long                              add_assoc_long
# define add_property_long                           add_property_long
# define zendi_convert_to_long                       zendi_convert_to_long
# define zend_ini_long                               zend_ini_long
# define ZEND_SIGNED_MULTIPLY_LONG                   ZEND_SIGNED_MULTIPLY_LONG
# define zend_update_property_long                   zend_update_property_long
# define SET_VAR_LONG                                SET_VAR_LONG
# define zend_declare_class_constant_long            zend_declare_class_constant_long
# define add_get_index_long                          add_get_index_long
# define add_next_index_long                         add_next_index_long
# define zend_update_static_property_long            zend_update_static_property_long
# define zend_register_long_constant                 zend_register_long_constant
# define add_index_long                              add_index_long
# define zend_declare_property_long                  zend_declare_property_long
# define ZEND_MM_LONG_CONST                          ZEND_MM_LONG_CONST
# define REGISTER_PDO_CLASS_CONST_LONG               REGISTER_PDO_CLASS_CONST_LONG
# define PDO_LONG_PARAM_CHECK                        PDO_LONG_PARAM_CHECK
# define REGISTER_XMLREADER_CLASS_CONST_LONG         REGISTER_XMLREADER_CLASS_CONST_LONG
# define CALENDAR_DECL_LONG_CONST                    CALENDAR_DECL_LONG_CONST
# define TIMEZONE_DECL_LONG_CONST                    TIMEZONE_DECL_LONG_CONST
# define BREAKITER_DECL_LONG_CONST                   BREAKITER_DECL_LONG_CONST
# define REGISTER_SPL_CLASS_CONST_LONG               REGISTER_SPL_CLASS_CONST_LONG
# define REGISTER_REFLECTION_CLASS_CONST_LONG        REGISTER_REFLECTION_CLASS_CONST_LONG
# define REGISTER_PHAR_CLASS_CONST_LONG              REGISTER_PHAR_CLASS_CONST_LONG
# define MAKE_LONG_ZVAL_INCREF                       MAKE_LONG_ZVAL_INCREF
# define MYSQLI_RETURN_LONG_INT                      MYSQLI_RETURN_LONG_LONG
# define MYSQLI_MAP_PROPERTY_FUNC_LONG               MYSQLI_MAP_PROPERTY_FUNC_LONG
# define REGISTER_ZIP_CLASS_CONST_LONG               REGISTER_ZIP_CLASS_CONST_LONG
# define REGISTER_SNMP_CLASS_CONST_LONG              REGISTER_SNMP_CLASS_CONST_LONG
# define PHP_SNMP_LONG_PROPERTY_READER_FUNCTION      PHP_SNMP_LONG_PROPERTY_READER_FUNCTION
# define ISC_LONG_MIN                                ISC_LONG_MIN
# define ISC_LONG_MAX                                ISC_LONG_MAX
# define long_min_digits                             long_min_digits
# define zend_dval_to_lval                          zend_dval_to_lval
# define ZEND_DVAL_TO_LVAL_CAST_OK                  ZEND_DVAL_TO_LVAL_CAST_OK
# define smart_str_append_long                       smart_str_append_long
# define cfg_get_long                                cfg_get_long
# define pdo_attr_lval                              pdo_attr_lval

# define ZPP_FMT_COMPAT(old_fmt, new_fmt) old_fmt

#else

# define ZPP_FMT_COMPAT(old_fmt, new_fmt) new_fmt

#endif

#endif /* PHP_SIZE_INT_COMPAT_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
