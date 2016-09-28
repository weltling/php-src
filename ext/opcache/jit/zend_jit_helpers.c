/*
   +----------------------------------------------------------------------+
   | Zend JIT                                                             |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2016 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Dmitry Stogov <dmitry@zend.com>                             |
   +----------------------------------------------------------------------+
*/

#include "Zend/zend_API.h"

static zend_function* ZEND_FASTCALL zend_jit_find_func_helper(zend_string *name)
{
	zval *func = zend_hash_find(EG(function_table), name);
	zend_function *fbc;

	if (UNEXPECTED(func == NULL)) {
		zend_throw_error(NULL, "Call to undefined function %s()", ZSTR_VAL(name));
		return NULL;
	}
	fbc = Z_FUNC_P(func);
	if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!fbc->op_array.run_time_cache)) {
		fbc->op_array.run_time_cache = zend_arena_alloc(&CG(arena), fbc->op_array.cache_size);
		memset(fbc->op_array.run_time_cache, 0, fbc->op_array.cache_size);
	}
	return fbc;
}

static zend_execute_data* ZEND_FASTCALL zend_jit_extend_stack_helper(uint32_t used_stack, zend_function *fbc)
{
	zend_execute_data *call = (zend_execute_data*)zend_vm_stack_extend(used_stack);
	call->func = fbc;
	ZEND_SET_CALL_INFO(call, 0, ZEND_CALL_NESTED_FUNCTION|ZEND_CALL_ALLOCATED);
	return call;
}

static zval* ZEND_FASTCALL zend_jit_symtable_find(HashTable *ht, zend_string *str)
{
	zend_ulong idx;
	register const char *tmp = str->val;

	do {
		if (*tmp > '9') {
			break;
		} else if (*tmp < '0') {
			if (*tmp != '-') {
				break;
			}
			tmp++;
			if (*tmp > '9' || *tmp < '0') {
				break;
			}
		}
		if (_zend_handle_numeric_str_ex(str->val, str->len, &idx)) {
			return zend_hash_index_find(ht, idx);
		}
	} while (0);

	return zend_hash_find(ht, str);
}

static zval* ZEND_FASTCALL zend_jit_hash_index_lookup_rw(HashTable *ht, zend_long idx)
{
	zval *retval = zend_hash_index_find(ht, idx);

	if (!retval) {
		zend_error(E_NOTICE,"Undefined index: %s", idx);
		retval = zend_hash_index_update(ht, idx, &EG(uninitialized_zval));
	}
	return retval;
}

static zval* ZEND_FASTCALL zend_jit_hash_index_lookup_w(HashTable *ht, zend_long idx)
{
	zval *retval = zend_hash_index_find(ht, idx);

	if (!retval) {
		retval = zend_hash_index_add_new(ht, idx, &EG(uninitialized_zval));
	}
	return retval;
}

static zval* ZEND_FASTCALL zend_jit_hash_lookup_rw(HashTable *ht, zend_string *str)
{
	zval *retval = zend_hash_find(ht, str);

	if (retval) {
		if (UNEXPECTED(Z_TYPE_P(retval) == IS_INDIRECT)) {
			retval = Z_INDIRECT_P(retval);
			if (UNEXPECTED(Z_TYPE_P(retval) == IS_UNDEF)) {
				zend_error(E_NOTICE,"Undefined index: %s", ZSTR_VAL(str));
				ZVAL_NULL(retval);
			}
		}
	} else {
		zend_error(E_NOTICE,"Undefined index: %s", ZSTR_VAL(str));
		retval = zend_hash_update(ht, str, &EG(uninitialized_zval));
	}
	return retval;
}

static zval* ZEND_FASTCALL zend_jit_hash_lookup_w(HashTable *ht, zend_string *str)
{
	zval *retval = zend_hash_find(ht, str);

	if (retval) {
		if (UNEXPECTED(Z_TYPE_P(retval) == IS_INDIRECT)) {
			retval = Z_INDIRECT_P(retval);
			if (UNEXPECTED(Z_TYPE_P(retval) == IS_UNDEF)) {
				ZVAL_NULL(retval);
			}
		}
	} else {
		retval = zend_hash_add_new(ht, str, &EG(uninitialized_zval));
	}
	return retval;
}

static zval* ZEND_FASTCALL zend_jit_symtable_lookup_rw(HashTable *ht, zend_string *str)
{
	zend_ulong idx;
	register const char *tmp = str->val;

	do {
		if (*tmp > '9') {
			break;
		} else if (*tmp < '0') {
			if (*tmp != '-') {
				break;
			}
			tmp++;
			if (*tmp > '9' || *tmp < '0') {
				break;
			}
		}
		if (_zend_handle_numeric_str_ex(str->val, str->len, &idx)) {
			zval *retval = zend_hash_index_find(ht, idx);

			if (!retval) {
				zend_error(E_NOTICE,"Undefined index: %s", ZSTR_VAL(str));
				retval = zend_hash_index_update(ht, idx, &EG(uninitialized_zval));
			}
			return retval;
		}
	} while (0);

	return zend_jit_hash_lookup_rw(ht, str);
}

static zval* ZEND_FASTCALL zend_jit_symtable_lookup_w(HashTable *ht, zend_string *str)
{
	zend_ulong idx;
	register const char *tmp = str->val;

	do {
		if (*tmp > '9') {
			break;
		} else if (*tmp < '0') {
			if (*tmp != '-') {
				break;
			}
			tmp++;
			if (*tmp > '9' || *tmp < '0') {
				break;
			}
		}
		if (_zend_handle_numeric_str_ex(str->val, str->len, &idx)) {
			zval *retval = zend_hash_index_find(ht, idx);

			if (!retval) {
				retval = zend_hash_index_add_new(ht, idx, &EG(uninitialized_zval));
			}
			return retval;
		}
	} while (0);

	return zend_jit_hash_lookup_w(ht, str);
}

static void ZEND_FASTCALL zend_jit_undefined_op_helper(uint32_t var)
{
	const zend_execute_data *execute_data = EG(current_execute_data);
	zend_string *cv = EX(func)->op_array.vars[EX_VAR_TO_NUM(var)];

	zend_error(E_NOTICE, "Undefined variable: %s", ZSTR_VAL(cv));
}

static void ZEND_FASTCALL zend_jit_fetch_dim_r_helper(zend_array *ht, zval *dim, zval *result)
{
	zend_long hval;
	zend_string *offset_key;
	zval *retval;

	if (Z_TYPE_P(dim) == IS_REFERENCE) {
		dim = Z_REFVAL_P(dim);
	}

	switch (Z_TYPE_P(dim)) {
		case IS_UNDEF:
			zend_jit_undefined_op_helper(EG(current_execute_data)->opline->op2.var);
			/* break missing intentionally */
		case IS_NULL:
			offset_key = ZSTR_EMPTY_ALLOC();
			goto str_index;
		case IS_DOUBLE:
			hval = zend_dval_to_lval(Z_DVAL_P(dim));
			goto num_index;
		case IS_RESOURCE:
			zend_error(E_NOTICE, "Resource ID#%d used as offset, casting to integer (%d)", Z_RES_HANDLE_P(dim), Z_RES_HANDLE_P(dim));
			hval = Z_RES_HANDLE_P(dim);
			goto num_index;
		case IS_FALSE:
			hval = 0;
			goto num_index;
		case IS_TRUE:
			hval = 1;
			goto num_index;
		default:
			zend_error(E_WARNING, "Illegal offset type");
			ZVAL_NULL(result);
			return;
	}

str_index:
	retval = zend_hash_find(ht, offset_key);
	if (retval) {
		/* support for $GLOBALS[...] */
		if (UNEXPECTED(Z_TYPE_P(retval) == IS_INDIRECT)) {
			retval = Z_INDIRECT_P(retval);
			if (UNEXPECTED(Z_TYPE_P(retval) == IS_UNDEF)) {
				zend_error(E_NOTICE, "Undefined index: %s", ZSTR_VAL(offset_key));
				ZVAL_NULL(result);
				return;
			}
		}
	} else {
		zend_error(E_NOTICE, "Undefined index: %s", ZSTR_VAL(offset_key));
		ZVAL_NULL(result);
		return;
	}
	ZVAL_COPY_UNREF(result, retval);
	return;

num_index:
	ZEND_HASH_INDEX_FIND(ht, hval, retval, num_undef);
	ZVAL_COPY_UNREF(result, retval);
	return;

num_undef:
	zend_error(E_NOTICE,"Undefined offset: " ZEND_LONG_FMT, hval);
	ZVAL_NULL(result);
}

static void ZEND_FASTCALL zend_jit_fetch_dim_is_helper(zend_array *ht, zval *dim, zval *result)
{
	zend_long hval;
	zend_string *offset_key;
	zval *retval;

	if (Z_TYPE_P(dim) == IS_REFERENCE) {
		dim = Z_REFVAL_P(dim);
	}

	switch (Z_TYPE_P(dim)) {
		case IS_UNDEF:
			zend_jit_undefined_op_helper(EG(current_execute_data)->opline->op2.var);
			/* break missing intentionally */
		case IS_NULL:
			offset_key = ZSTR_EMPTY_ALLOC();
			goto str_index;
		case IS_DOUBLE:
			hval = zend_dval_to_lval(Z_DVAL_P(dim));
			goto num_index;
		case IS_RESOURCE:
			zend_error(E_NOTICE, "Resource ID#%d used as offset, casting to integer (%d)", Z_RES_HANDLE_P(dim), Z_RES_HANDLE_P(dim));
			hval = Z_RES_HANDLE_P(dim);
			goto num_index;
		case IS_FALSE:
			hval = 0;
			goto num_index;
		case IS_TRUE:
			hval = 1;
			goto num_index;
		default:
			zend_error(E_WARNING, "Illegal offset type");
			ZVAL_NULL(result);
			return;
	}

str_index:
	retval = zend_hash_find(ht, offset_key);
	if (retval) {
		/* support for $GLOBALS[...] */
		if (UNEXPECTED(Z_TYPE_P(retval) == IS_INDIRECT)) {
			retval = Z_INDIRECT_P(retval);
			if (UNEXPECTED(Z_TYPE_P(retval) == IS_UNDEF)) {
				ZVAL_NULL(result);
				return;
			}
		}
	} else {
		ZVAL_NULL(result);
		return;
	}
	ZVAL_COPY_UNREF(result, retval);
	return;

num_index:
	ZEND_HASH_INDEX_FIND(ht, hval, retval, num_undef);
	ZVAL_COPY_UNREF(result, retval);
	return;

num_undef:
	ZVAL_NULL(result);
}

static zval* ZEND_FASTCALL zend_jit_fetch_dim_rw_helper(zend_array *ht, zval *dim)
{
	zend_long hval;
	zend_string *offset_key;
	zval *retval;

	if (Z_TYPE_P(dim) == IS_REFERENCE) {
		dim = Z_REFVAL_P(dim);
	}

	switch (Z_TYPE_P(dim)) {
		case IS_UNDEF:
			zend_jit_undefined_op_helper(EG(current_execute_data)->opline->op2.var);
			/* break missing intentionally */
		case IS_NULL:
			offset_key = ZSTR_EMPTY_ALLOC();
			goto str_index;
		case IS_DOUBLE:
			hval = zend_dval_to_lval(Z_DVAL_P(dim));
			goto num_index;
		case IS_RESOURCE:
			zend_error(E_NOTICE, "Resource ID#%d used as offset, casting to integer (%d)", Z_RES_HANDLE_P(dim), Z_RES_HANDLE_P(dim));
			hval = Z_RES_HANDLE_P(dim);
			goto num_index;
		case IS_FALSE:
			hval = 0;
			goto num_index;
		case IS_TRUE:
			hval = 1;
			goto num_index;
		default:
			zend_error(E_WARNING, "Illegal offset type");
			return NULL;
	}

str_index:
	retval = zend_hash_find(ht, offset_key);
	if (retval) {
		/* support for $GLOBALS[...] */
		if (UNEXPECTED(Z_TYPE_P(retval) == IS_INDIRECT)) {
			retval = Z_INDIRECT_P(retval);
			if (UNEXPECTED(Z_TYPE_P(retval) == IS_UNDEF)) {
				zend_error(E_NOTICE, "Undefined index: %s", ZSTR_VAL(offset_key));
				ZVAL_NULL(retval);
			}
		}
	} else {
		zend_error(E_NOTICE, "Undefined index: %s", ZSTR_VAL(offset_key));
		retval = zend_hash_update(ht, offset_key, &EG(uninitialized_zval));
	}
	return retval;

num_index:
	ZEND_HASH_INDEX_FIND(ht, hval, retval, num_undef);
	return retval;

num_undef:
	zend_error(E_NOTICE,"Undefined offset: " ZEND_LONG_FMT, hval);
	retval = zend_hash_index_update(ht, hval, &EG(uninitialized_zval));
	return retval;
}

static zval* ZEND_FASTCALL zend_jit_fetch_dim_w_helper(zend_array *ht, zval *dim)
{
	zend_long hval;
	zend_string *offset_key;
	zval *retval;

	if (Z_TYPE_P(dim) == IS_REFERENCE) {
		dim = Z_REFVAL_P(dim);
	}

	switch (Z_TYPE_P(dim)) {
		case IS_UNDEF:
			zend_jit_undefined_op_helper(EG(current_execute_data)->opline->op2.var);
			/* break missing intentionally */
		case IS_NULL:
			offset_key = ZSTR_EMPTY_ALLOC();
			goto str_index;
		case IS_DOUBLE:
			hval = zend_dval_to_lval(Z_DVAL_P(dim));
			goto num_index;
		case IS_RESOURCE:
			zend_error(E_NOTICE, "Resource ID#%d used as offset, casting to integer (%d)", Z_RES_HANDLE_P(dim), Z_RES_HANDLE_P(dim));
			hval = Z_RES_HANDLE_P(dim);
			goto num_index;
		case IS_FALSE:
			hval = 0;
			goto num_index;
		case IS_TRUE:
			hval = 1;
			goto num_index;
		default:
			zend_error(E_WARNING, "Illegal offset type");
			return NULL;
	}

str_index:
	retval = zend_hash_find(ht, offset_key);
	if (retval) {
		/* support for $GLOBALS[...] */
		if (UNEXPECTED(Z_TYPE_P(retval) == IS_INDIRECT)) {
			retval = Z_INDIRECT_P(retval);
			if (UNEXPECTED(Z_TYPE_P(retval) == IS_UNDEF)) {
				ZVAL_NULL(retval);
			}
		}
	} else {
		retval = zend_hash_add_new(ht, offset_key, &EG(uninitialized_zval));
	}
	return retval;

num_index:
	ZEND_HASH_INDEX_FIND(ht, hval, retval, num_undef);
	return retval;

num_undef:
	retval = zend_hash_index_add_new(ht, hval, &EG(uninitialized_zval));
	return retval;
}

static void ZEND_FASTCALL zend_jit_fetch_dim_str_r_helper(zval *container, zval *dim, zval *result)
{
	zend_long offset;

try_string_offset:
	if (UNEXPECTED(Z_TYPE_P(dim) != IS_LONG)) {
		switch (Z_TYPE_P(dim)) {
			/* case IS_LONG: */
			case IS_STRING:
				if (IS_LONG == is_numeric_string(Z_STRVAL_P(dim), Z_STRLEN_P(dim), NULL, NULL, -1)) {
					break;
				}
				zend_error(E_WARNING, "Illegal string offset '%s'", Z_STRVAL_P(dim));
				break;
			case IS_UNDEF:
				zend_jit_undefined_op_helper(EG(current_execute_data)->opline->op2.var);
			case IS_DOUBLE:
			case IS_NULL:
			case IS_FALSE:
			case IS_TRUE:
				zend_error(E_NOTICE, "String offset cast occurred");
				break;
			case IS_REFERENCE:
				dim = Z_REFVAL_P(dim);
				goto try_string_offset;
			default:
				zend_error(E_WARNING, "Illegal offset type");
				break;
		}

		offset = _zval_get_long_func(dim);
	} else {
		offset = Z_LVAL_P(dim);
	}

	if (UNEXPECTED(Z_STRLEN_P(container) < (size_t)((offset < 0) ? -offset : (offset + 1)))) {
		zend_error(E_NOTICE, "Uninitialized string offset: " ZEND_LONG_FMT, offset);
		ZVAL_EMPTY_STRING(result);
	} else {
		zend_uchar c;
		zend_long real_offset;

		real_offset = (UNEXPECTED(offset < 0)) /* Handle negative offset */
			? (zend_long)Z_STRLEN_P(container) + offset : offset;
		c = (zend_uchar)Z_STRVAL_P(container)[real_offset];
			if (CG(one_char_string)[c]) {
			ZVAL_INTERNED_STR(result, CG(one_char_string)[c]);
		} else {
			ZVAL_NEW_STR(result, zend_string_init(Z_STRVAL_P(container) + real_offset, 1, 0));
		}
	}
}

static void ZEND_FASTCALL zend_jit_fetch_dim_str_is_helper(zval *container, zval *dim, zval *result)
{
	zend_long offset;

try_string_offset:
	if (UNEXPECTED(Z_TYPE_P(dim) != IS_LONG)) {
		switch (Z_TYPE_P(dim)) {
			/* case IS_LONG: */
			case IS_STRING:
				if (IS_LONG == is_numeric_string(Z_STRVAL_P(dim), Z_STRLEN_P(dim), NULL, NULL, -1)) {
					break;
				}
				ZVAL_NULL(result);
				return;
			case IS_UNDEF:
				zend_jit_undefined_op_helper(EG(current_execute_data)->opline->op2.var);
			case IS_DOUBLE:
			case IS_NULL:
			case IS_FALSE:
			case IS_TRUE:
				break;
			case IS_REFERENCE:
				dim = Z_REFVAL_P(dim);
				goto try_string_offset;
			default:
				zend_error(E_WARNING, "Illegal offset type");
				break;
		}

		offset = _zval_get_long_func(dim);
	} else {
		offset = Z_LVAL_P(dim);
	}

	if (UNEXPECTED(Z_STRLEN_P(container) < (size_t)((offset < 0) ? -offset : (offset + 1)))) {
		ZVAL_NULL(result);
	} else {
		zend_uchar c;
		zend_long real_offset;

		real_offset = (UNEXPECTED(offset < 0)) /* Handle negative offset */
			? (zend_long)Z_STRLEN_P(container) + offset : offset;
		c = (zend_uchar)Z_STRVAL_P(container)[real_offset];
			if (CG(one_char_string)[c]) {
			ZVAL_INTERNED_STR(result, CG(one_char_string)[c]);
		} else {
			ZVAL_NEW_STR(result, zend_string_init(Z_STRVAL_P(container) + real_offset, 1, 0));
		}
	}
}

static void ZEND_FASTCALL zend_jit_fetch_dim_obj_r_helper(zval *container, zval *dim, zval *result)
{
	if (UNEXPECTED(Z_TYPE_P(dim) == IS_UNDEF)) {
		zend_jit_undefined_op_helper(EG(current_execute_data)->opline->op2.var);
		dim = &EG(uninitialized_zval);
	}
	if (!Z_OBJ_HT_P(container)->read_dimension) {
		zend_throw_error(NULL, "Cannot use object as array");
		ZVAL_NULL(result);
	} else {
		zval *retval = Z_OBJ_HT_P(container)->read_dimension(container, dim, BP_VAR_R, result);

		if (retval) {
			if (result != retval) {
				ZVAL_COPY(result, retval);
			}
		} else {
			ZVAL_NULL(result);
		}
	}
}

static void ZEND_FASTCALL zend_jit_fetch_dim_obj_is_helper(zval *container, zval *dim, zval *result)
{
	if (UNEXPECTED(Z_TYPE_P(dim) == IS_UNDEF)) {
		zend_jit_undefined_op_helper(EG(current_execute_data)->opline->op2.var);
		dim = &EG(uninitialized_zval);
	}
	if (!Z_OBJ_HT_P(container)->read_dimension) {
		zend_throw_error(NULL, "Cannot use object as array");
		ZVAL_NULL(result);
	} else {
		zval *retval = Z_OBJ_HT_P(container)->read_dimension(container, dim, BP_VAR_IS, result);

		if (retval) {
			if (result != retval) {
				ZVAL_COPY(result, retval);
			}
		} else {
			ZVAL_NULL(result);
		}
	}
}

static zval* ZEND_FASTCALL zend_jit_fetch_dimension_rw_long_helper(HashTable *ht, zend_long hval)
{
	zend_error(E_NOTICE,"Undefined offset: " ZEND_LONG_FMT, hval);
	return zend_hash_index_update(ht, hval, &EG(uninitialized_zval));
}

static zend_never_inline zend_long zend_check_string_offset(zval *dim, int type)
{
	zend_long offset;

try_again:
	if (UNEXPECTED(Z_TYPE_P(dim) != IS_LONG)) {
		switch(Z_TYPE_P(dim)) {
			case IS_STRING:
				if (IS_LONG == is_numeric_string(Z_STRVAL_P(dim), Z_STRLEN_P(dim), NULL, NULL, -1)) {
					break;
				}
				if (type != BP_VAR_UNSET) {
					zend_error(E_WARNING, "Illegal string offset '%s'", Z_STRVAL_P(dim));
				}
				break;
			case IS_UNDEF:
				zend_jit_undefined_op_helper(EG(current_execute_data)->opline->op2.var);
			case IS_DOUBLE:
			case IS_NULL:
			case IS_FALSE:
			case IS_TRUE:
				zend_error(E_NOTICE, "String offset cast occurred");
				break;
			case IS_REFERENCE:
				dim = Z_REFVAL_P(dim);
				goto try_again;
			default:
				zend_error(E_WARNING, "Illegal offset type");
				break;
		}

		offset = _zval_get_long_func(dim);
	} else {
		offset = Z_LVAL_P(dim);
	}

	return offset;
}

static zend_never_inline ZEND_COLD void zend_wrong_string_offset(void)
{
	const char *msg = NULL;
	const zend_op *opline = EG(current_execute_data)->opline;
	const zend_op *end;
	uint32_t var;

	switch (opline->opcode) {
		case ZEND_ASSIGN_ADD:
		case ZEND_ASSIGN_SUB:
		case ZEND_ASSIGN_MUL:
		case ZEND_ASSIGN_DIV:
		case ZEND_ASSIGN_MOD:
		case ZEND_ASSIGN_SL:
		case ZEND_ASSIGN_SR:
		case ZEND_ASSIGN_CONCAT:
		case ZEND_ASSIGN_BW_OR:
		case ZEND_ASSIGN_BW_AND:
		case ZEND_ASSIGN_BW_XOR:
		case ZEND_ASSIGN_POW:
			msg = "Cannot use assign-op operators with string offsets";
			break;
		case ZEND_FETCH_DIM_W:
		case ZEND_FETCH_DIM_RW:
		case ZEND_FETCH_DIM_FUNC_ARG:
		case ZEND_FETCH_DIM_UNSET:
			/* TODO: Encode the "reason" into opline->extended_value??? */
			var = opline->result.var;
			opline++;
			end = EG(current_execute_data)->func->op_array.opcodes +
				EG(current_execute_data)->func->op_array.last;
			while (opline < end) {
				if (opline->op1_type == IS_VAR && opline->op1.var == var) {
					switch (opline->opcode) {
						case ZEND_ASSIGN_ADD:
						case ZEND_ASSIGN_SUB:
						case ZEND_ASSIGN_MUL:
						case ZEND_ASSIGN_DIV:
						case ZEND_ASSIGN_MOD:
						case ZEND_ASSIGN_SL:
						case ZEND_ASSIGN_SR:
						case ZEND_ASSIGN_CONCAT:
						case ZEND_ASSIGN_BW_OR:
						case ZEND_ASSIGN_BW_AND:
						case ZEND_ASSIGN_BW_XOR:
						case ZEND_ASSIGN_POW:
							if (opline->extended_value == ZEND_ASSIGN_OBJ) {
								msg = "Cannot use string offset as an object";
							} else if (opline->extended_value == ZEND_ASSIGN_DIM) {
								msg = "Cannot use string offset as an array";
							} else {
								msg = "Cannot use assign-op operators with string offsets";
							}
							break;
						case ZEND_PRE_INC_OBJ:
						case ZEND_PRE_DEC_OBJ:
						case ZEND_POST_INC_OBJ:
						case ZEND_POST_DEC_OBJ:
						case ZEND_PRE_INC:
						case ZEND_PRE_DEC:
						case ZEND_POST_INC:
						case ZEND_POST_DEC:
							msg = "Cannot increment/decrement string offsets";
							break;
						case ZEND_FETCH_DIM_W:
						case ZEND_FETCH_DIM_RW:
						case ZEND_FETCH_DIM_FUNC_ARG:
						case ZEND_FETCH_DIM_UNSET:
						case ZEND_ASSIGN_DIM:
							msg = "Cannot use string offset as an array";
							break;
						case ZEND_FETCH_OBJ_W:
						case ZEND_FETCH_OBJ_RW:
						case ZEND_FETCH_OBJ_FUNC_ARG:
						case ZEND_FETCH_OBJ_UNSET:
						case ZEND_ASSIGN_OBJ:
							msg = "Cannot use string offset as an object";
							break;
						case ZEND_ASSIGN_REF:
						case ZEND_ADD_ARRAY_ELEMENT:
						case ZEND_INIT_ARRAY:
						case ZEND_MAKE_REF:
							msg = "Cannot create references to/from string offsets";
							break;
						case ZEND_RETURN_BY_REF:
						case ZEND_VERIFY_RETURN_TYPE:
							msg = "Cannot return string offsets by reference";
							break;
						case ZEND_UNSET_DIM:
						case ZEND_UNSET_OBJ:
							msg = "Cannot unset string offsets";
							break;
						case ZEND_YIELD:
							msg = "Cannot yield string offsets by reference";
							break;
						case ZEND_SEND_REF:
						case ZEND_SEND_VAR_EX:
							msg = "Only variables can be passed by reference";
							break;
						EMPTY_SWITCH_DEFAULT_CASE();
					}
					break;
				}
				if (opline->op2_type == IS_VAR && opline->op2.var == var) {
					ZEND_ASSERT(opline->opcode == ZEND_ASSIGN_REF);
					msg = "Cannot create references to/from string offsets";
					break;
				}
			}
			break;
		EMPTY_SWITCH_DEFAULT_CASE();
	}
	ZEND_ASSERT(msg != NULL);
	zend_throw_error(NULL, msg);
}

static zend_never_inline void zend_assign_to_string_offset(zval *str, zval *dim, zval *value, zval *result)
{
	zend_string *old_str;
	zend_uchar c;
	size_t string_len;
	zend_long offset;

	offset = zend_check_string_offset(dim, BP_VAR_W);
	if (offset < -(zend_long)Z_STRLEN_P(str)) {
		/* Error on negative offset */
		zend_error(E_WARNING, "Illegal string offset:  " ZEND_LONG_FMT, offset);
		if (result) {
			ZVAL_NULL(result);
		}
		return;
	}

	if (Z_TYPE_P(value) != IS_STRING) {
		/* Convert to string, just the time to pick the 1st byte */
		zend_string *tmp = zval_get_string(value);

		string_len = ZSTR_LEN(tmp);
		c = (zend_uchar)ZSTR_VAL(tmp)[0];
		zend_string_release(tmp);
	} else {
		string_len = Z_STRLEN_P(value);
		c = (zend_uchar)Z_STRVAL_P(value)[0];
	}

	if (string_len == 0) {
		/* Error on empty input string */
		zend_error(E_WARNING, "Cannot assign an empty string to a string offset");
		if (result) {
			ZVAL_NULL(result);
		}
		return;
	}

	if (offset < 0) { /* Handle negative offset */
		offset += (zend_long)Z_STRLEN_P(str);
	}

	if ((size_t)offset >= Z_STRLEN_P(str)) {
		/* Extend string if needed */
		zend_long old_len = Z_STRLEN_P(str);
		Z_STR_P(str) = zend_string_extend(Z_STR_P(str), offset + 1, 0);
		Z_TYPE_INFO_P(str) = IS_STRING_EX;
		memset(Z_STRVAL_P(str) + old_len, ' ', offset - old_len);
		Z_STRVAL_P(str)[offset+1] = 0;
	} else if (!Z_REFCOUNTED_P(str)) {
		old_str = Z_STR_P(str);
		Z_STR_P(str) = zend_string_init(Z_STRVAL_P(str), Z_STRLEN_P(str), 0);
		Z_TYPE_INFO_P(str) = IS_STRING_EX;
		zend_string_release(old_str);
	} else {
		SEPARATE_STRING(str);
		zend_string_forget_hash_val(Z_STR_P(str));
	}

	Z_STRVAL_P(str)[offset] = c;

	if (result) {
		/* Return the new character */
		if (CG(one_char_string)[c]) {
			ZVAL_INTERNED_STR(result, CG(one_char_string)[c]);
		} else {
			ZVAL_NEW_STR(result, zend_string_init(Z_STRVAL_P(str) + offset, 1, 0));
		}
	}
}

static void ZEND_FASTCALL zend_jit_assign_dim_helper(zval *object_ptr, zval *dim, zval *value)
{
	if (EXPECTED(Z_TYPE_P(object_ptr) == IS_OBJECT)) {

		if (UNEXPECTED(!Z_OBJ_HT_P(object_ptr)->write_dimension)) {
			zend_throw_error(NULL, "Cannot use object as array");
		} else {
			Z_OBJ_HT_P(object_ptr)->write_dimension(object_ptr, dim, value);
//???		if (UNEXPECTED(RETURN_VALUE_USED(opline)) && EXPECTED(!EG(exception))) {
//???			ZVAL_COPY(EX_VAR(opline->result.var), value);
//???		}
		}
	} else if (EXPECTED(Z_TYPE_P(object_ptr) == IS_STRING)) {
		if (dim) {
			zend_throw_error(NULL, "[] operator not supported for strings");
		} else {
			zend_assign_to_string_offset(object_ptr, dim, value, /*???UNEXPECTED(RETURN_VALUE_USED(opline)) ? EX_VAR(opline->result.var) :*/ NULL);
		}
	} else {
//???		if (OP1_TYPE != IS_VAR || EXPECTED(!Z_ISERROR_P(object_ptr))) {
			zend_error(E_WARNING, "Cannot use a scalar value as an array");
//???		}
//???		if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
//???				ZVAL_NULL(EX_VAR(opline->result.var));
//???		}
	}
}

static void ZEND_FASTCALL zend_jit_assign_dim_add_helper(zval *container, zval *dim, zval *value)
{
	if (EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
		zval *object = container;
		zval *property = dim;
		zval *z;
		zval rv, res;

		if (Z_OBJ_HT_P(object)->read_dimension &&
			(z = Z_OBJ_HT_P(object)->read_dimension(object, property, BP_VAR_R, &rv)) != NULL) {

			if (Z_TYPE_P(z) == IS_OBJECT && Z_OBJ_HT_P(z)->get) {
				zval rv2;
				zval *value = Z_OBJ_HT_P(z)->get(z, &rv2);

				if (z == &rv) {
					zval_ptr_dtor(&rv);
				}
				ZVAL_COPY_VALUE(z, value);
			}
			add_function(&res, Z_ISREF_P(z) ? Z_REFVAL_P(z) : z, value);
			Z_OBJ_HT_P(object)->write_dimension(object, property, &res);
			if (z == &rv) {
				zval_ptr_dtor(&rv);
			}
//???			if (retval) {
//???				ZVAL_COPY(retval, &res);
//???			}
			zval_ptr_dtor(&res);
		} else {
			zend_error(E_WARNING, "Attempt to assign property of non-object");
//???			if (retval) {
//???				ZVAL_NULL(retval);
//???			}
		}
	} else {
		if (UNEXPECTED(Z_TYPE_P(container) == IS_STRING)) {
			if (!dim) {
				zend_throw_error(NULL, "[] operator not supported for strings");
			} else {
				zend_check_string_offset(dim, BP_VAR_RW);
				zend_wrong_string_offset();
			}
//???		} else if (EXPECTED(Z_TYPE_P(container) <= IS_FALSE)) {
//???			ZEND_VM_C_GOTO(assign_dim_op_convert_to_array);
		} else {
//???			if (UNEXPECTED(OP1_TYPE != IS_VAR || EXPECTED(!Z_ISERROR_P(container)))) {
				zend_error(E_WARNING, "Cannot use a scalar value as an array");
//???			}
//???			if (retval) {
//???				ZVAL_NULL(retval);
//???			}
		}
	}
}

static void ZEND_FASTCALL zend_jit_assign_dim_sub_helper(zval *container, zval *dim, zval *value)
{
	if (EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
		zval *object = container;
		zval *property = dim;
		zval *z;
		zval rv, res;

		if (Z_OBJ_HT_P(object)->read_dimension &&
			(z = Z_OBJ_HT_P(object)->read_dimension(object, property, BP_VAR_R, &rv)) != NULL) {

			if (Z_TYPE_P(z) == IS_OBJECT && Z_OBJ_HT_P(z)->get) {
				zval rv2;
				zval *value = Z_OBJ_HT_P(z)->get(z, &rv2);

				if (z == &rv) {
					zval_ptr_dtor(&rv);
				}
				ZVAL_COPY_VALUE(z, value);
			}
			sub_function(&res, Z_ISREF_P(z) ? Z_REFVAL_P(z) : z, value);
			Z_OBJ_HT_P(object)->write_dimension(object, property, &res);
			if (z == &rv) {
				zval_ptr_dtor(&rv);
			}
//???			if (retval) {
//???				ZVAL_COPY(retval, &res);
//???			}
			zval_ptr_dtor(&res);
		} else {
			zend_error(E_WARNING, "Attempt to assign property of non-object");
//???			if (retval) {
//???				ZVAL_NULL(retval);
//???			}
		}
	} else {
		if (UNEXPECTED(Z_TYPE_P(container) == IS_STRING)) {
			if (!dim) {
				zend_throw_error(NULL, "[] operator not supported for strings");
			} else {
				zend_check_string_offset(dim, BP_VAR_RW);
				zend_wrong_string_offset();
			}
//???		} else if (EXPECTED(Z_TYPE_P(container) <= IS_FALSE)) {
//???			ZEND_VM_C_GOTO(assign_dim_op_convert_to_array);
		} else {
//???			if (UNEXPECTED(OP1_TYPE != IS_VAR || EXPECTED(!Z_ISERROR_P(container)))) {
				zend_error(E_WARNING, "Cannot use a scalar value as an array");
//???			}
//???			if (retval) {
//???				ZVAL_NULL(retval);
//???			}
		}
	}
}

static void ZEND_FASTCALL zend_jit_assign_dim_mul_helper(zval *container, zval *dim, zval *value)
{
	if (EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
		zval *object = container;
		zval *property = dim;
		zval *z;
		zval rv, res;

		if (Z_OBJ_HT_P(object)->read_dimension &&
			(z = Z_OBJ_HT_P(object)->read_dimension(object, property, BP_VAR_R, &rv)) != NULL) {

			if (Z_TYPE_P(z) == IS_OBJECT && Z_OBJ_HT_P(z)->get) {
				zval rv2;
				zval *value = Z_OBJ_HT_P(z)->get(z, &rv2);

				if (z == &rv) {
					zval_ptr_dtor(&rv);
				}
				ZVAL_COPY_VALUE(z, value);
			}
			mul_function(&res, Z_ISREF_P(z) ? Z_REFVAL_P(z) : z, value);
			Z_OBJ_HT_P(object)->write_dimension(object, property, &res);
			if (z == &rv) {
				zval_ptr_dtor(&rv);
			}
//???			if (retval) {
//???				ZVAL_COPY(retval, &res);
//???			}
			zval_ptr_dtor(&res);
		} else {
			zend_error(E_WARNING, "Attempt to assign property of non-object");
//???			if (retval) {
//???				ZVAL_NULL(retval);
//???			}
		}
	} else {
		if (UNEXPECTED(Z_TYPE_P(container) == IS_STRING)) {
			if (!dim) {
				zend_throw_error(NULL, "[] operator not supported for strings");
			} else {
				zend_check_string_offset(dim, BP_VAR_RW);
				zend_wrong_string_offset();
			}
//???		} else if (EXPECTED(Z_TYPE_P(container) <= IS_FALSE)) {
//???			ZEND_VM_C_GOTO(assign_dim_op_convert_to_array);
		} else {
//???			if (UNEXPECTED(OP1_TYPE != IS_VAR || EXPECTED(!Z_ISERROR_P(container)))) {
				zend_error(E_WARNING, "Cannot use a scalar value as an array");
//???			}
//???			if (retval) {
//???				ZVAL_NULL(retval);
//???			}
		}
	}
}

static void ZEND_FASTCALL zend_jit_assign_dim_div_helper(zval *container, zval *dim, zval *value)
{
	if (EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
		zval *object = container;
		zval *property = dim;
		zval *z;
		zval rv, res;

		if (Z_OBJ_HT_P(object)->read_dimension &&
			(z = Z_OBJ_HT_P(object)->read_dimension(object, property, BP_VAR_R, &rv)) != NULL) {

			if (Z_TYPE_P(z) == IS_OBJECT && Z_OBJ_HT_P(z)->get) {
				zval rv2;
				zval *value = Z_OBJ_HT_P(z)->get(z, &rv2);

				if (z == &rv) {
					zval_ptr_dtor(&rv);
				}
				ZVAL_COPY_VALUE(z, value);
			}
			div_function(&res, Z_ISREF_P(z) ? Z_REFVAL_P(z) : z, value);
			Z_OBJ_HT_P(object)->write_dimension(object, property, &res);
			if (z == &rv) {
				zval_ptr_dtor(&rv);
			}
//???			if (retval) {
//???				ZVAL_COPY(retval, &res);
//???			}
			zval_ptr_dtor(&res);
		} else {
			zend_error(E_WARNING, "Attempt to assign property of non-object");
//???			if (retval) {
//???				ZVAL_NULL(retval);
//???			}
		}
	} else {
		if (UNEXPECTED(Z_TYPE_P(container) == IS_STRING)) {
			if (!dim) {
				zend_throw_error(NULL, "[] operator not supported for strings");
			} else {
				zend_check_string_offset(dim, BP_VAR_RW);
				zend_wrong_string_offset();
			}
//???		} else if (EXPECTED(Z_TYPE_P(container) <= IS_FALSE)) {
//???			ZEND_VM_C_GOTO(assign_dim_op_convert_to_array);
		} else {
//???			if (UNEXPECTED(OP1_TYPE != IS_VAR || EXPECTED(!Z_ISERROR_P(container)))) {
				zend_error(E_WARNING, "Cannot use a scalar value as an array");
//???			}
//???			if (retval) {
//???				ZVAL_NULL(retval);
//???			}
		}
	}
}

static void ZEND_FASTCALL zend_jit_zval_copy_unref_helper(zval *dst, zval *src)
{
	ZVAL_UNREF(src);
	ZVAL_COPY(dst, src);
}

static zval* ZEND_FASTCALL zend_jit_new_ref_helper(zval *value)
{
	zend_reference *ref = (zend_reference*)emalloc(sizeof(zend_reference));
	GC_REFCOUNT(ref) = 1;
	GC_TYPE_INFO(ref) = IS_REFERENCE;
	ZVAL_COPY_VALUE(&ref->val, value);
	Z_REF_P(value) = ref;
	Z_TYPE_INFO_P(value) = IS_REFERENCE_EX;

	return value;
}

static zval* ZEND_FASTCALL zend_jit_fetch_global_helper(zend_execute_data *execute_data, zval *varname)
{
	uint32_t idx;
	zval *value = zend_hash_find(&EG(symbol_table), Z_STR_P(varname));

	if (UNEXPECTED(value == NULL)) {
		value = zend_hash_add_new(&EG(symbol_table), Z_STR_P(varname), &EG(uninitialized_zval));
		idx = ((char*)value - (char*)EG(symbol_table).arData) / sizeof(Bucket);
		/* Store "hash slot index" + 1 (NULL is a mark of uninitialized cache slot) */
		CACHE_PTR(Z_CACHE_SLOT_P(varname), (void*)(uintptr_t)(idx + 1));
	} else {
		idx = ((char*)value - (char*)EG(symbol_table).arData) / sizeof(Bucket);
		/* Store "hash slot index" + 1 (NULL is a mark of uninitialized cache slot) */
		CACHE_PTR(Z_CACHE_SLOT_P(varname), (void*)(uintptr_t)(idx + 1));
		/* GLOBAL variable may be an INDIRECT pointer to CV */
		if (UNEXPECTED(Z_TYPE_P(value) == IS_INDIRECT)) {
			value = Z_INDIRECT_P(value);
			if (UNEXPECTED(Z_TYPE_P(value) == IS_UNDEF)) {
				ZVAL_NULL(value);
			}
		}
	}

	if (UNEXPECTED(!Z_ISREF_P(value))) {
		return zend_jit_new_ref_helper(value);
	}

	return value;
}

static int is_null_constant(zend_class_entry *scope, zval *default_value)
{
	if (Z_CONSTANT_P(default_value)) {
		zval constant;

		ZVAL_COPY(&constant, default_value);
		if (UNEXPECTED(zval_update_constant_ex(&constant, scope) != SUCCESS)) {
			return 0;
		}
		if (Z_TYPE(constant) == IS_NULL) {
			return 1;
		}
		zval_ptr_dtor(&constant);
	}
	return 0;
}

static zend_bool zend_verify_weak_scalar_type_hint(zend_uchar type_hint, zval *arg)
{
    switch (type_hint) {
        case _IS_BOOL: {
            zend_bool dest;

            if (!zend_parse_arg_bool_weak(arg, &dest)) {
                return 0;
            }
            zval_ptr_dtor(arg);
            ZVAL_BOOL(arg, dest);
            return 1;
        }
        case IS_LONG: {
            zend_long dest;

            if (!zend_parse_arg_long_weak(arg, &dest)) {
                return 0;
            }
            zval_ptr_dtor(arg);
            ZVAL_LONG(arg, dest);
            return 1;
        }
        case IS_DOUBLE: {
            double dest;

            if (!zend_parse_arg_double_weak(arg, &dest)) {
                return 0;
            }
            zval_ptr_dtor(arg);
            ZVAL_DOUBLE(arg, dest);
            return 1;
        }
        case IS_STRING: {
            zend_string *dest;

            /* on success "arg" is converted to IS_STRING */
            if (!zend_parse_arg_str_weak(arg, &dest)) {
                return 0;
			}
            return 1;
        }
        default:
            return 0;
    }
}

static zend_bool zend_verify_scalar_type_hint(zend_uchar type_hint, zval *arg, zend_bool strict)
{
	if (UNEXPECTED(strict)) {
		/* SSTH Exception: IS_LONG may be accepted as IS_DOUBLE (converted) */
		if (type_hint != IS_DOUBLE || Z_TYPE_P(arg) != IS_LONG) {
			return 0;
		}
	} else if (UNEXPECTED(Z_TYPE_P(arg) == IS_NULL)) {
		/* NULL may be accepted only by nullable hints (this is already checked) */
		return 0;
	}
	return zend_verify_weak_scalar_type_hint(type_hint, arg);
}

static ZEND_COLD void zend_verify_type_error_common(
		const zend_function *zf, const zend_arg_info *arg_info,
		const zend_class_entry *ce, zval *value,
		const char **fname, const char **fsep, const char **fclass,
		const char **need_msg, const char **need_kind, const char **need_or_null,
		const char **given_msg, const char **given_kind)
{
	zend_bool is_interface = 0;
	*fname = ZSTR_VAL(zf->common.function_name);

	if (zf->common.scope) {
		*fsep =  "::";
		*fclass = ZSTR_VAL(zf->common.scope->name);
	} else {
		*fsep =  "";
		*fclass = "";
	}

	switch (arg_info->type_hint) {
		case IS_OBJECT:
			if (ce) {
				if (ce->ce_flags & ZEND_ACC_INTERFACE) {
					*need_msg = "implement interface ";
					is_interface = 1;
				} else {
					*need_msg = "be an instance of ";
				}
				*need_kind = ZSTR_VAL(ce->name);
			} else {
				/* We don't know whether it's a class or interface, assume it's a class */
				*need_msg = "be an instance of ";
				*need_kind = zf->common.type == ZEND_INTERNAL_FUNCTION
					? ((zend_internal_arg_info *) arg_info)->class_name
					: ZSTR_VAL(arg_info->class_name);
			}
			break;
		case IS_CALLABLE:
			*need_msg = "be callable";
			*need_kind = "";
			break;
		case IS_ITERABLE:
			*need_msg = "be iterable";
			*need_kind = "";
			break;
		default:
			*need_msg = "be of the type ";
			*need_kind = zend_get_type_by_const(arg_info->type_hint);
			break;
	}

	if (arg_info->allow_null) {
		*need_or_null = is_interface ? " or be null" : " or null";
	} else {
		*need_or_null = "";
	}

	if (value) {
		if (arg_info->type_hint == IS_OBJECT && Z_TYPE_P(value) == IS_OBJECT) {
			*given_msg = "instance of ";
			*given_kind = ZSTR_VAL(Z_OBJCE_P(value)->name);
		} else {
			*given_msg = zend_zval_type_name(value);
			*given_kind = "";
		}
	} else {
		*given_msg = "none";
		*given_kind = "";
	}
}

static ZEND_COLD void zend_verify_arg_error(
		const zend_function *zf, const zend_arg_info *arg_info,
		int arg_num, const zend_class_entry *ce, zval *value)
{
	zend_execute_data *ptr = EG(current_execute_data)->prev_execute_data;
	const char *fname, *fsep, *fclass;
	const char *need_msg, *need_kind, *need_or_null, *given_msg, *given_kind;

	if (value) {
		zend_verify_type_error_common(
				zf, arg_info, ce, value,
				&fname, &fsep, &fclass, &need_msg, &need_kind, &need_or_null, &given_msg, &given_kind);

		if (zf->common.type == ZEND_USER_FUNCTION) {
			if (ptr && ptr->func && ZEND_USER_CODE(ptr->func->common.type)) {
				zend_type_error("Argument %d passed to %s%s%s() must %s%s%s, %s%s given, called in %s on line %d",
						arg_num, fclass, fsep, fname, need_msg, need_kind, need_or_null, given_msg, given_kind,
						ZSTR_VAL(ptr->func->op_array.filename), ptr->opline->lineno);
			} else {
				zend_type_error("Argument %d passed to %s%s%s() must %s%s%s, %s%s given", arg_num, fclass, fsep, fname, need_msg, need_kind, need_or_null, given_msg, given_kind);
			}
		} else {
			zend_type_error("Argument %d passed to %s%s%s() must %s%s%s, %s%s given", arg_num, fclass, fsep, fname, need_msg, need_kind, need_or_null, given_msg, given_kind);
		}
	} else {
		zend_missing_arg_error(ptr);
	}
}

static void ZEND_FASTCALL zend_jit_verify_arg_object(zval *arg, zend_op_array *op_array, uint32_t arg_num, zend_arg_info *arg_info, void **cache_slot)
{
	zend_class_entry *ce;
	if (EXPECTED(*cache_slot)) {
		ce = (zend_class_entry *)*cache_slot;
	} else {
		ce = zend_fetch_class(arg_info->class_name, (ZEND_FETCH_CLASS_AUTO | ZEND_FETCH_CLASS_NO_AUTOLOAD));
		if (UNEXPECTED(!ce)) {
			zend_verify_arg_error((zend_function*)op_array, arg_info, arg_num, NULL, arg);
			return;
		}
		*cache_slot = (void *)ce;
	}
	if (UNEXPECTED(!instanceof_function(Z_OBJCE_P(arg), ce))) {
		zend_verify_arg_error((zend_function*)op_array, arg_info, arg_num, ce, arg);
	}
}

static void ZEND_FASTCALL zend_jit_verify_arg_slow(zval *arg, zend_op_array *op_array, uint32_t arg_num, zend_arg_info *arg_info, void **cache_slot, zval *default_value)
{
	zend_class_entry *ce = NULL;

	if (Z_TYPE_P(arg) == IS_NULL &&
		(arg_info->allow_null || (default_value && is_null_constant(op_array->scope, default_value)))) {
		/* Null passed to nullable type */
		return;
	}

	if (UNEXPECTED(arg_info->class_name)) {
		/* This is always an error - we fetch the class name for the error message here */
		if (EXPECTED(*cache_slot)) {
			ce = (zend_class_entry *) *cache_slot;
		} else {
			ce = zend_fetch_class(arg_info->class_name, (ZEND_FETCH_CLASS_AUTO | ZEND_FETCH_CLASS_NO_AUTOLOAD));
			if (ce) {
				*cache_slot = (void *)ce;
			}
		}
		goto err;
	} else if (arg_info->type_hint == IS_CALLABLE) {
		if (zend_is_callable(arg, IS_CALLABLE_CHECK_SILENT, NULL) == 0) {
			goto err;
		}
	} else if (arg_info->type_hint == IS_ITERABLE) {
		if (zend_is_iterable(arg) == 0) {
			goto err;
		}
	} else if (arg_info->type_hint == _IS_BOOL &&
			EXPECTED(Z_TYPE_P(arg) == IS_FALSE || Z_TYPE_P(arg) == IS_TRUE)) {
		return;
	} else {
		if (zend_verify_scalar_type_hint(arg_info->type_hint, arg, ZEND_RET_USES_STRICT_TYPES()) == 0) {
			goto err;
		}
	}
	return;
err:
	zend_verify_arg_error((zend_function*)op_array, arg_info, arg_num, ce, arg);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
