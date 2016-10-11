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

/* Legacy, constant buffer size as in previous PHP versions. */
#define PHP_STREAM_BUFFER_GROWTH_NONE 0x0
#define PHP_STREAM_BUFFER_GROWTH_LINEAR 0x1
#define PHP_STREAM_BUFFER_GROWTH_EXP 0x2
#define PHP_STREAM_BUFFER_GROWTH_EXP_ADAPTIVE 0x4

#define PHP_STREAM_BUFFER_CHUNK_DEFAULT 8192

#define PHP_STREAM_BUFFER_CHUNK_LINEAR_MIN 8192
#define PHP_STREAM_BUFFER_CHUNK_LINEAR_MAX 262144

typedef struct _php_stream_buffer_strategy_phase {
	size_t it;
	size_t size;
} php_stream_buffer_strategy_phase;

typedef struct _php_stream_buffer_strategy {
	/* Current buffer growth strategy. */
	uint8_t strategy;
	/* Strategy growth phase. Used for adaptive growth. */
	uint8_t next_phase;
	uint32_t it;
	php_stream_buffer_strategy_phase *phase_data;
} php_stream_buffer_strategy;

typedef struct _php_stream_buffer {

	unsigned char *readbuf;
	size_t readbuflen;

	/* how much data to read when filling buffer */
	size_t chunk_size;

	php_stream_buffer_strategy *strategy;

} php_stream_buffer;

PHPAPI void _php_stream_buffer_ctor(php_stream *, uint8_t);
//#define php_stream_buffer_ctor(stream) _php_stream_buffer_ctor((stream), PHP_STREAM_BUFFER_GROWTH_NONE)
#define php_stream_buffer_ctor(stream) _php_stream_buffer_ctor((stream), PHP_STREAM_BUFFER_GROWTH_EXP_ADAPTIVE)
#define php_stream_buffer_ctor_strategy(stream, strategy) _php_stream_buffer_ctor((stream), (strategy))

PHPAPI void _php_stream_buffer_dtor(php_stream *);
#define php_stream_buffer_dtor(stream) _php_stream_buffer_dtor((stream))

PHPAPI void _php_stream_buffer_extend(php_stream *, size_t);
#define php_stream_buffer_extend(stream) _php_stream_buffer_extend((stream), 0)
#define php_stream_buffer_extend_size(stream, size) _php_stream_buffer_extend((stream), (size))

PHPAPI size_t _php_stream_buffer_calc_size(php_stream_buffer *);
#define php_stream_buffer_calc_size(psb) _php_stream_buffer_calc_size((psb))

#define PHP_STREAM_BUF_LEN(stream) (stream)->buffer->readbuflen
#define PHP_STREAM_BUF_CHUNK_LEN(stream) (stream)->buffer->chunk_size
#define PHP_STREAM_BUF(stream) (stream)->buffer->readbuf
//#define php_stream_set_strategy(strategy) stream->buffer->strategy = strategy

PHPAPI void _php_stream_buffer_put(php_stream *, zend_off_t, unsigned char *, size_t);
#define php_stream_buffer_put(stream, offset, buf, buf_len) _php_stream_buffer_put((stream), (offset), (buf), (buf_len))






/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
