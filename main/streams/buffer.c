/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
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

#include "php.h"
#include "php_streams_int.h"

static const php_stream_buffer_strategy_phase exp_adaptive_phase[] = {
	{0, 8192},
	{3, 12288},
	{5, 18432},
	{8, 27648},
	{13, 41472},
	{21, 62208},
	{34, 93312},
	{55, 139968},
	{89, 209952},
	{144, 314928}
#if 0
	{0, 4096},
	{1, 6144},
	{3, 9216},
	{8, 13824},
	{21, 20736},
	{55, 31104},
	{144, 46656},
	{377, 69984},
	{987, 104976},
	{2584, 157464},
	{6765, 236196}
#endif
#if 0
	{0, 4096},
	{2, 6144},
	{5, 9216},
	{13, 13824},
	{34, 20736},
	{89, 31104},
	{233, 46656},
	{610, 69984},
	{1597, 104976},
	{4181, 157464},
	{10946, 236196}
#endif
};

#if 0
static const php_stream_buffer_strategy_phase lin_adaptive_phase[] = {
	{0, 4096},
};
#endif

PHPAPI void _php_stream_buffer_ctor(php_stream *stream, uint8_t strategy)
{
	php_stream_buffer *psb;

	stream->buffer = pemalloc(sizeof(php_stream_buffer), stream->is_persistent);
	psb = stream->buffer;

	switch (strategy) {
		case PHP_STREAM_BUFFER_GROWTH_NONE:
		default:
			psb->chunk_size = psb->readbuflen = 8192;
			psb->strategy = NULL;
		case PHP_STREAM_BUFFER_GROWTH_EXP_ADAPTIVE:
			psb->strategy = pemalloc(sizeof(php_stream_buffer_strategy), stream->is_persistent);
			psb->strategy->phase_data = &exp_adaptive_phase;
			psb->strategy->next_phase = 1;
			psb->strategy->it = 0;
			psb->chunk_size = psb->readbuflen = psb->strategy->phase_data[psb->strategy->next_phase-1].size;
			psb->strategy->strategy = strategy;
			break;
	}

	psb->readbuf = pemalloc(psb->readbuflen, stream->is_persistent);
}

PHPAPI void _php_stream_buffer_dtor(php_stream *stream)
{
	php_stream_buffer *psb = stream->buffer;

	if (!psb) {
		return;
	}

	pefree(psb->readbuf, stream->is_persistent);
	pefree(psb->strategy, stream->is_persistent);
	pefree(psb, stream->is_persistent);
	stream->buffer = NULL;
}

PHPAPI void _php_stream_buffer_extend(php_stream *stream, size_t add_size) 
{
	php_stream_buffer *psb = stream->buffer;
	size_t calc_size, prev_size, size;

	prev_size = psb->chunk_size;

	calc_size = php_stream_buffer_calc_size(psb);

	size = psb->readbuflen + add_size < calc_size ? calc_size : psb->readbuflen + add_size;

	if (size > prev_size) {
		psb->readbuf = perealloc(psb->readbuf, size, stream->is_persistent);
	}
}

PHPAPI size_t _php_stream_buffer_calc_size(php_stream_buffer *psb)
{
	if (!psb->strategy) {
		return psb->readbuflen;
	}

	/* no strategy handling for now. */
	switch (psb->strategy->strategy) {
		case PHP_STREAM_BUFFER_GROWTH_NONE:
		default:
			psb->readbuflen += psb->chunk_size;
			return psb->chunk_size;

		case PHP_STREAM_BUFFER_GROWTH_EXP_ADAPTIVE:
			if (psb->strategy->next_phase >= sizeof(exp_adaptive_phase)/sizeof(php_stream_buffer_strategy_phase)) {
				return psb->chunk_size;
			}
			if (psb->strategy->it >= psb->strategy->phase_data[psb->strategy->next_phase].it) {
				psb->strategy->next_phase++;
				psb->chunk_size = psb->strategy->phase_data[psb->strategy->next_phase - 1].size;
			}
			psb->strategy->it++;
			psb->readbuflen = psb->chunk_size;
			return psb->chunk_size;
	}
}

PHPAPI void _php_stream_buffer_put(php_stream *stream, zend_off_t offset, unsigned char *buf, size_t buf_len)
{
	php_stream_buffer *psb = stream->buffer;
	
	memmove(psb->readbuf + offset, buf, buf_len);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
