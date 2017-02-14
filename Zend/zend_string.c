/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2017 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Dmitry Stogov <dmitry@zend.com>                             |
   +----------------------------------------------------------------------+
*/

/* $Id: $ */

#include "zend.h"
#include "zend_globals.h"

ZEND_API zend_string *(*zend_new_interned_string)(zend_string *str);
ZEND_API void (*zend_interned_strings_snapshot)(void);
ZEND_API void (*zend_interned_strings_restore)(void);

static zend_string *zend_new_interned_string_int(zend_string *str);
static void zend_interned_strings_snapshot_int(void);
static void zend_interned_strings_restore_int(void);

/* Any strings interned in the startup phase. Common to all the threads,
	won't be free'd until process exit. If we want an ability to 
	add permanent strings even after startup, it would be still
	possible on costs of locking in the thread safe builds. */
static HashTable interned_strings_permanent;
static HashTable *interned_strings_permanent_ptr = &interned_strings_permanent;

/* Short living interned strings, these are all those created during
	the request. They should not be marked permanent. Every thread
	has its own copies of transient interned strings, which
	should not be shared between threads. */
ZEND_TLS HashTable interned_strings_volatile;

static zend_string *empty_string;
static zend_bool permanent_snapshot_exists = 0;

ZEND_API zend_ulong zend_hash_func(const char *str, size_t len)
{
	return zend_inline_hash_func(str, len);
}

static void _str_dtor(zval *zv)
{
	zend_string *str = Z_STR_P(zv);
	pefree(str, GC_FLAGS(str) & IS_STR_PERSISTENT);
}

/* Readonly, so assigned also per thread. */
static const zend_string **known_interned_strings = NULL;
static uint32_t known_interned_strings_count = 0;

ZEND_API uint32_t zend_intern_known_strings(const char **strings, uint32_t count)
{
	uint32_t i, old_count = known_interned_strings_count;

	known_interned_strings = perealloc(known_interned_strings, sizeof(char*) * (old_count + count), 1);
	for (i = 0; i < count; i++) {
		zend_string *str = zend_string_init(strings[i], strlen(strings[i]), 1);
		known_interned_strings[known_interned_strings_count + i] =
			zend_new_interned_string_int(str);
	}
	known_interned_strings_count = old_count + count;
	return old_count;
}

static const char *known_strings[] = {
#define _ZEND_STR_DSC(id, str) str,
ZEND_KNOWN_STRINGS(_ZEND_STR_DSC)
#undef _ZEND_STR_DSC
	NULL
};

void zend_known_interned_strings_init(zend_string ***strings, uint32_t *count)
{
	*strings = (zend_string **)known_interned_strings;
	*count   = known_interned_strings_count;
}

static void zend_init_interned_strings_ht(HashTable *interned_strings)
{
	zend_hash_init(interned_strings, 1024, NULL, _str_dtor, 1);

	interned_strings->nTableMask = -interned_strings->nTableSize;
	HT_SET_DATA_ADDR(interned_strings, pemalloc(HT_SIZE(interned_strings), 1));
	HT_HASH_RESET(interned_strings);
	interned_strings->u.flags |= HASH_FLAG_INITIALIZED;
}

void zend_interned_strings_init_thread(void)
{
	zend_init_interned_strings_ht(&interned_strings_volatile);
}

ZEND_API zend_string *zend_interned_strings_get_empty_string(void)
{
	return empty_string;
}

ZEND_API HashTable *zend_interned_strings_get_permanent_storage(void)
{
	return interned_strings_permanent_ptr;
}

ZEND_API void zend_interned_strings_get_permanent_storage(HashTable *new_storage)
{
	/* XXX Cleanup the old storage before, if needed. */
	interned_strings_permanent_ptr = new_storage;
}

void zend_interned_strings_init(void)
{
	zend_string *str;

	zend_init_interned_strings_ht(interned_strings_permanent_ptr);
	zend_init_interned_strings_ht(&interned_strings_volatile);

	/* interned empty string */
	str = zend_string_alloc(sizeof("")-1, 1);
	ZSTR_VAL(str)[0] = '\000';
	empty_string = zend_new_interned_string_int(str);
#ifndef ZTS
	CG(empty_string) = empty_string;
#endif
	/* one char strings (the actual interned strings are going to be created by ext/opcache) */
	memset(CG(one_char_string), 0, sizeof(CG(one_char_string)));

	/* known strings */
	zend_intern_known_strings(known_strings, (sizeof(known_strings) / sizeof(known_strings[0])) - 1);
	zend_known_interned_strings_init(&CG(known_strings), &CG(known_strings_count));

	zend_new_interned_string = zend_new_interned_string_int;
	zend_interned_strings_snapshot = zend_interned_strings_snapshot_int;
	zend_interned_strings_restore = zend_interned_strings_restore_int;
}

void zend_interned_strings_dtor(void)
{
	/* ??? still tsrm shutdown is an issue? */
	zend_hash_destroy(interned_strings_permanent_ptr);

	free(CG(known_strings));
	CG(known_strings) = NULL;
	CG(known_strings_count) = 0;
	known_interned_strings = NULL;
	known_interned_strings_count = 0;
}

static zend_string *zend_new_interned_string_int(zend_string *str)
{
	zend_ulong h;
	uint32_t nIndex;
	uint32_t idx;
	Bucket *p;
	HashTable *interned_strings;

	if (ZSTR_IS_INTERNED(str)) {
		return str;
	}

	if (permanent_snapshot_exists) {
		interned_strings = &interned_strings_volatile;
	} else {
		interned_strings = interned_strings_permanent_ptr;
	}

	h = zend_string_hash_val(str);
	nIndex = h | interned_strings->nTableMask;
	idx = HT_HASH(interned_strings, nIndex);
	while (idx != HT_INVALID_IDX) {
		p = HT_HASH_TO_BUCKET(interned_strings, idx);
		if ((p->h == h) && (ZSTR_LEN(p->key) == ZSTR_LEN(str))) {
			if (!memcmp(ZSTR_VAL(p->key), ZSTR_VAL(str), ZSTR_LEN(str))) {
				zend_string_release(str);
				return p->key;
			}
		}
		idx = Z_NEXT(p->val);
	}

	GC_REFCOUNT(str) = 1;
	GC_FLAGS(str) |= IS_STR_INTERNED;

	if (interned_strings->nNumUsed >= interned_strings->nTableSize) {
		if (interned_strings->nTableSize < HT_MAX_SIZE) {	/* Let's double the table size */
			void *new_data;
			void *old_data = HT_GET_DATA_ADDR(interned_strings);
			Bucket *old_buckets = interned_strings->arData;

			interned_strings->nTableSize += interned_strings->nTableSize;
			interned_strings->nTableMask = -interned_strings->nTableSize;
			new_data = malloc(HT_SIZE(interned_strings));

			if (new_data) {
				HT_SET_DATA_ADDR(interned_strings, new_data);
				memcpy(interned_strings->arData, old_buckets, sizeof(Bucket) * interned_strings->nNumUsed);
				free(old_data);
				zend_hash_rehash(interned_strings);
			} else {
				interned_strings->nTableSize = interned_strings->nTableSize >> 1;
				interned_strings->nTableMask = -interned_strings->nTableSize;
			}
		}
	}

	idx = interned_strings->nNumUsed++;
	interned_strings->nNumOfElements++;
	p = interned_strings->arData + idx;
	p->h = h;
	p->key = str;
	Z_STR(p->val) = str;
	Z_TYPE_INFO(p->val) = IS_INTERNED_STRING_EX;
	nIndex = h | interned_strings->nTableMask;
	Z_NEXT(p->val) = HT_HASH(interned_strings, nIndex);
	HT_HASH(interned_strings, nIndex) = HT_IDX_TO_HASH(idx);

	return str;
}

static void zend_interned_strings_snapshot_int(void)
{
	uint32_t idx;
	Bucket *p;
	HashTable *interned_strings = interned_strings_permanent_ptr;

	/* Ensure the function is called only once during startup.
		All the followup strings will be in the per thread table. */
	ZEND_ASSERT(0 == permanent_snapshot_exists);

	idx = interned_strings->nNumUsed;
	while (idx > 0) {
		idx--;
		p = interned_strings->arData + idx;
		ZEND_ASSERT(GC_FLAGS(p->key) & IS_STR_PERSISTENT);
		GC_FLAGS(p->key) |= IS_STR_PERMANENT;
	}

	permanent_snapshot_exists = 1;
}

static void zend_interned_strings_restore_int(void)
{
	uint32_t nIndex;
	uint32_t idx;
	Bucket *p;
	HashTable *interned_strings = &interned_strings_volatile;

	idx = interned_strings->nNumUsed;
	while (idx > 0) {
		idx--;
		p = interned_strings->arData + idx;
		/*if (GC_FLAGS(p->key) & IS_STR_PERMANENT) break;*/
		interned_strings->nNumUsed--;
		interned_strings->nNumOfElements--;

		GC_FLAGS(p->key) &= ~IS_STR_INTERNED;
		GC_REFCOUNT(p->key) = 1;
		zend_string_free(p->key);

		nIndex = p->h | interned_strings->nTableMask;
		if (HT_HASH(interned_strings, nIndex) == HT_IDX_TO_HASH(idx)) {
			HT_HASH(interned_strings, nIndex) = Z_NEXT(p->val);
		} else {
			uint32_t prev = HT_HASH(interned_strings, nIndex);
			while (Z_NEXT(HT_HASH_TO_BUCKET(interned_strings, prev)->val) != idx) {
				prev = Z_NEXT(HT_HASH_TO_BUCKET(interned_strings, prev)->val);
 			}
			Z_NEXT(HT_HASH_TO_BUCKET(interned_strings, prev)->val) = Z_NEXT(p->val);
 		}
 	}
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
