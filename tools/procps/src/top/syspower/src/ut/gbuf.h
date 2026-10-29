/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef UT_GBUF_H__
#define UT_GBUF_H__

// external
#include LIB_INT
#include LIB_STD
#include LIB_DEF
#include LIB_STRING

struct ut_gbuf {
	void **ptr;
	size_t size;
	uint64_t len;
};

static inline struct ut_gbuf *ut_gbuf_salloc(void)
{
	struct ut_gbuf *gbuf = CFG_MALLOC(sizeof(struct ut_gbuf));
	if (!gbuf)
		goto fail_gbuf;

	return gbuf;

fail_gbuf:
	return NULL;
}

static inline int ut_gbuf_ialloc(struct ut_gbuf *gbuf)
{
	if (!gbuf)
		goto fail_struct;

	gbuf->ptr = CFG_MALLOC(2 * sizeof(void *));
	if (!gbuf->ptr)
		goto fail_ptr;

	return 0;

fail_ptr:
	CFG_FREE(gbuf->ptr);
fail_struct:
	return -1;
}

static inline struct ut_gbuf *ut_gbuf_falloc(void)
{
	struct ut_gbuf *gbuf = ut_gbuf_salloc();
	ut_gbuf_ialloc(gbuf);
	return gbuf;
}

static inline int ut_gbuf_init(struct ut_gbuf *gbuf)
{
	if (!gbuf)
		goto fail_struct;
	// only starts buffer with 2 entries
	gbuf->size = 2;
	gbuf->len = 0;

	return 0;

fail_struct:
	return -1;
}

static inline void ut_gbuf_sfree(struct ut_gbuf *gbuf)
{
	CFG_FREE(gbuf);
	return;
}

static inline void ut_gbuf_ifree(struct ut_gbuf *gbuf)
{
	CFG_FREE(gbuf->ptr);
	return;
}

static inline void ut_gbuf_ffree(struct ut_gbuf *gbuf)
{
	ut_gbuf_ifree(gbuf);
	ut_gbuf_sfree(gbuf);
	return;
}

static inline int ut_gbuf_grow(struct ut_gbuf *gbuf)
{
	// doubles buffer every growth
	gbuf->size *= 2;

	void **new_buf = CFG_MALLOC(gbuf->size * sizeof(void *));
	if (!new_buf)
		return -1;

	memcpy(new_buf, gbuf->ptr, gbuf->len * sizeof(void *));

	CFG_FREE(gbuf->ptr);
	gbuf->ptr = new_buf;

	return 0;
}

static inline int ut_gbuf_push(struct ut_gbuf *gbuf, void *val)
{
	// grow if needed
	if (gbuf->len >= gbuf->size)
		if (ut_gbuf_grow(gbuf))
			return -1;

	gbuf->ptr[gbuf->len] = val;
	gbuf->len++;
	return 0;
}

static inline int ut_gbuf_pop(struct ut_gbuf *gbuf)
{
	if (gbuf->len <= 0)
		return -1;

	gbuf->len--;
	return 0;
}

#endif
