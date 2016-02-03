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

/* This file integrates several modified parts from the libuv project, which
 * is copyrighted to
 *
 * Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */ 

#include <assert.h>
#include <stdlib.h>
#include <direct.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/utime.h>
#include <stdio.h>

#include "php.h"
#include "win32/winutil.h"
#include "win32/time.h"
#include "win32/ioutil.h"

/*
#undef NONLS
#undef _WINNLS_
#include <winnls.h>
*/
PW32IO wchar_t *php_win32_ioutil_mb_to_w(const char* path)
{/*{{{*/
	wchar_t *ret;
	int ret_len, tmp_len;

	if (!path) {
		return NULL;
	}

    ret_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
    if (ret_len == 0) {
		return NULL;
    }

	ret = malloc(ret_len * sizeof(wchar_t));
	if (!ret) {
		return NULL;
	}

	tmp_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, ret, ret_len);

    /* assert(tmp_len == ret_len); */

	return ret;
}/*}}}*/

PW32IO char *php_win32_ioutil_w_to_utf8(wchar_t* w_source_ptr)
{/*{{{*/
	int r;
	int target_len;
	char* target;


	target_len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, w_source_ptr, -1, NULL, 0, NULL, NULL);
	if (target_len == 0) {
		return NULL;
	}

	target = malloc(target_len);
	if (target == NULL) {
		SetLastError(ERROR_OUTOFMEMORY);
		_set_errno(ENOMEM);
		return NULL;
	}

	r = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, w_source_ptr, -1, target, target_len, NULL, NULL);

	/*assert(r == target_len);*/

	return target;
}/*}}}*/


PW32IO BOOL php_win32_ioutil_posix_to_open_opts(int flags, mode_t mode, php_ioutil_open_opts *opts)
{/*{{{*/
	int current_umask;

	opts->attributes = 0;

	/* Obtain the active umask. umask() never fails and returns the previous */
	/* umask. */
	current_umask = umask(0);
	umask(current_umask);

	/* convert flags and mode to CreateFile parameters */
	switch (flags & (_O_RDONLY | _O_WRONLY | _O_RDWR)) {
		case _O_RDONLY:
			opts->access = FILE_GENERIC_READ;
			opts->attributes |= FILE_FLAG_BACKUP_SEMANTICS;
			break;
		case _O_WRONLY:
			opts->access = FILE_GENERIC_WRITE;
			break;
		case _O_RDWR:
			opts->access = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
			break;
		default:
			goto einval;
	}

	if (flags & _O_APPEND) {
		/* XXX this might look wrong, but i just leave it here. Disabling FILE_WRITE_DATA prevents the current truncate behaviors for files opened with "a". */
		/* access &= ~FILE_WRITE_DATA;*/
		opts->access |= FILE_APPEND_DATA;
		opts->attributes &= ~FILE_FLAG_BACKUP_SEMANTICS;
	}

	/*
	* Here is where we deviate significantly from what CRT's _open()
	* does. We indiscriminately use all the sharing modes, to match
	* UNIX semantics. In particular, this ensures that the file can
	* be deleted even whilst it's open.
	*/
	opts->share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

	switch (flags & (_O_CREAT | _O_EXCL | _O_TRUNC)) {
		case 0:
		case _O_EXCL:
			opts->disposition = OPEN_EXISTING;
			break;
		case _O_CREAT:
			opts->disposition = OPEN_ALWAYS;
			break;
		case _O_CREAT | _O_EXCL:
		case _O_CREAT | _O_TRUNC | _O_EXCL:
			opts->disposition = CREATE_NEW;
			break;
		case _O_TRUNC:
		case _O_TRUNC | _O_EXCL:
			opts->disposition = TRUNCATE_EXISTING;
			break;
		case _O_CREAT | _O_TRUNC:
			opts->disposition = CREATE_ALWAYS;
			break;
		default:
			goto einval;
	}

	opts->attributes |= FILE_ATTRIBUTE_NORMAL;
	if (flags & _O_CREAT) {
		if (!((mode & ~current_umask) & _S_IWRITE)) {
			opts->attributes |= FILE_ATTRIBUTE_READONLY;
		}
	}

	if (flags & _O_TEMPORARY ) {
		opts->attributes |= FILE_FLAG_DELETE_ON_CLOSE | FILE_ATTRIBUTE_TEMPORARY;
		opts->access |= DELETE;
	}

	if (flags & _O_SHORT_LIVED) {
		opts->attributes |= FILE_ATTRIBUTE_TEMPORARY;
	}

	switch (flags & (_O_SEQUENTIAL | _O_RANDOM)) {
		case 0:
			break;
		case _O_SEQUENTIAL:
			opts->attributes |= FILE_FLAG_SEQUENTIAL_SCAN;
			break;
		case _O_RANDOM:
			opts->attributes |= FILE_FLAG_RANDOM_ACCESS;
			break;
		default:
			goto einval;
	}

	/* Very compat options */
	/*if (flags & O_ASYNC) {
		opts->attributes |= FILE_FLAG_OVERLAPPED;
	} else if (flags & O_SYNC) {
		opts->attributes &= ~FILE_FLAG_OVERLAPPED;
	}*/

	/* Setting this flag makes it possible to open a directory. */
	opts->attributes |= FILE_FLAG_BACKUP_SEMANTICS;

	return 1;

einval:
	_set_errno(EINVAL);
	return 0;
}/*}}}*/

PW32IO int php_win32_ioutil_open_w(const wchar_t *path, int flags, ...)
{/*{{{*/
	php_ioutil_open_opts open_opts;
	HANDLE file;
	int fd;
	mode_t mode = 0;

	if (flags & O_CREAT) {
		va_list arg;

		va_start(arg, flags);
		mode = (mode_t) va_arg(arg, int);
		va_end(arg);
	}

	if (!php_win32_ioutil_posix_to_open_opts(flags, mode, &open_opts)) {
		goto einval;
	}

	/* XXX care about security attributes here if needed, see tsrm_win32_access() */
	file = CreateFileW(path,
		open_opts.access,
		open_opts.share,
		NULL,
		open_opts.disposition,
		open_opts.attributes,
		NULL);

	if (file == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();

		if (error == ERROR_FILE_EXISTS && (flags & _O_CREAT) &&
			!(flags & _O_EXCL)) {
			/* Special case: when ERROR_FILE_EXISTS happens and O_CREAT was */
			/* specified, it means the path referred to a directory. */
			_set_errno(EISDIR);
		} else {
			SET_ERRNO_FROM_WIN32_CODE(error);
		}
		return -1;
	}

	fd = _open_osfhandle((intptr_t) file, flags);
	if (fd < 0) {
		DWORD error = GetLastError();

		/* The only known failure mode for _open_osfhandle() is EMFILE, in which
		 * case GetLastError() will return zero. However we'll try to handle other
		 * errors as well, should they ever occur.
		 */
		if (errno == EMFILE) {
			_set_errno(EMFILE);
		} else if (error != ERROR_SUCCESS) {
			SET_ERRNO_FROM_WIN32_CODE(error);
		}
		CloseHandle(file);
		return -1;
	}

	if (flags & _O_TEXT) {
		_setmode(fd, _O_TEXT);
	} else if (flags & _O_BINARY) {
		_setmode(fd, _O_BINARY);
	}

	return fd;

	einval:
		_set_errno(EINVAL);
		return -1;
}/*}}}*/

PW32IO int php_win32_ioutil_open_a(const char *path, int flags, ...)
{/*{{{*/
	php_ioutil_open_opts open_opts;
	HANDLE file;
	int fd;
	mode_t mode = 0;

	if (flags & O_CREAT) {
		va_list arg;

		va_start(arg, flags);
		mode = (mode_t) va_arg(arg, int);
		va_end(arg);
	}

	if (!php_win32_ioutil_posix_to_open_opts(flags, mode, &open_opts)) {
		goto einval;
	}

	/* XXX care about security attributes here if needed, see tsrm_win32_access() */
	file = CreateFileA(path,
		open_opts.access,
		open_opts.share,
		NULL,
		open_opts.disposition,
		open_opts.attributes,
		NULL);

	if (file == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();

		if (error == ERROR_FILE_EXISTS && (flags & _O_CREAT) &&
			!(flags & _O_EXCL)) {
			/* Special case: when ERROR_FILE_EXISTS happens and O_CREAT was */
			/* specified, it means the path referred to a directory. */
			_set_errno(EISDIR);
		} else {
			SET_ERRNO_FROM_WIN32_CODE(error);
		}
		return -1;
	}

	fd = _open_osfhandle((intptr_t) file, flags);
	if (fd < 0) {
		DWORD error = GetLastError();

		/* The only known failure mode for _open_osfhandle() is EMFILE, in which
		 * case GetLastError() will return zero. However we'll try to handle other
		 * errors as well, should they ever occur.
		 */
		if (errno == EMFILE) {
			_set_errno(EMFILE);
		} else if (error != ERROR_SUCCESS) {
			SET_ERRNO_FROM_WIN32_CODE(error);
		}
		CloseHandle(file);
		return -1;
	}

	if (flags & _O_TEXT) {
		_setmode(fd, _O_TEXT);
	} else if (flags & _O_BINARY) {
		_setmode(fd, _O_BINARY);
	}

	return fd;

	einval:
		_set_errno(EINVAL);
		return -1;
}/*}}}*/

#if 0
PW32IO int php_win32_ioutil_open(const char *path, int flags, ...)
{/*{{{*/
	php_ioutil_open_opts open_opts;
	HANDLE file;
	int fd;
	mode_t mode = 0;
	wchar_t *pathw = php_win32_ioutil_mb_to_w(path);
	BOOL use_a = 0;

#if PHP_WIN32_IOUTIL_ANSI_COMPAT_MODE
	if (!pathw) {
		/* Falling back to the path literally given */
		use_a = 1;
	}
#else
	if (!pathw) {
		goto einval;
	}
#endif

	if (flags & O_CREAT) {
		va_list arg;

		va_start(arg, flags);
		mode = (mode_t) va_arg(arg, int);
		va_end(arg);
	}

	if (!php_win32_ioutil_posix_to_open_opts(flags, mode, &open_opts)) {
		goto einval;
	}

	/* XXX care about security attributes here if needed, see tsrm_win32_access() */
	if (!use_a) {
		file = CreateFileW(pathw,
			open_opts.access,
			open_opts.share,
			NULL,
			open_opts.disposition,
			open_opts.attributes,
			NULL);
#if PHP_WIN32_IOUTIL_ANSI_COMPAT_MODE
	} else {
		file = CreateFileA(path,
			open_opts.access,
			open_opts.share,
			NULL,
			open_opts.disposition,
			open_opts.attributes,
			NULL);
#endif
	}

	if (file == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();

		if (error == ERROR_FILE_EXISTS && (flags & _O_CREAT) &&
			!(flags & _O_EXCL)) {
			/* Special case: when ERROR_FILE_EXISTS happens and O_CREAT was */
			/* specified, it means the path referred to a directory. */
			_set_errno(EISDIR);
		} else {
			SET_ERRNO_FROM_WIN32_CODE(error);
		}
		return -1;
	}

	fd = _open_osfhandle((intptr_t) file, flags);
	if (fd < 0) {
		DWORD error = GetLastError();

		/* The only known failure mode for _open_osfhandle() is EMFILE, in which
		 * case GetLastError() will return zero. However we'll try to handle other
		 * errors as well, should they ever occur.
		 */
		if (errno == EMFILE) {
			_set_errno(EMFILE);
		} else if (error != ERROR_SUCCESS) {
			SET_ERRNO_FROM_WIN32_CODE(error);
		}
		CloseHandle(file);
		return -1;
	}

	if (pathw) {
		free(pathw);
	}

	if (flags & _O_TEXT) {
		_setmode(fd, _O_TEXT);
	} else if (flags & _O_BINARY) {
		_setmode(fd, _O_BINARY);
	}

	return fd;

	einval:
		if (pathw) {
			free(pathw);
		}
		_set_errno(EINVAL);
		return -1;
}/*}}}*/
#endif

PW32IO int php_win32_ioutil_close(int fd)
{/*{{{*/
	int result = -1;

	if (-1 == fd) {
		_set_errno(EBADF);
		return result;
	}

	if (fd > 2) {
		result = _close(fd);
	} else {
		result = 0;
	}

	/* _close doesn't set _doserrno on failure, but it does always set errno
	* to EBADF on failure.
	*/
	if (result == -1) {
		_set_errno(EBADF);
	}

	return result;
}/*}}}*/

#if PHP_WIN32_IOUTIL_ANSI_COMPAT_MODE
PW32IO int php_win32_ioutil_mkdir_a(const char *path, mode_t mode)
{/*{{{*/
	int ret = -1;
	DWORD err = 0;

	/* TODO extend with mode usage */
	if (CreateDirectoryA(path, NULL)) {
		ret = 0;
	} else {
		err = GetLastError();
	}

	if (0 > ret) {
		SET_ERRNO_FROM_WIN32_CODE(err);
	}

	return ret;
}/*}}}*/
#endif

PW32IO int php_win32_ioutil_mkdir_w(const wchar_t *path, mode_t mode)
{/*{{{*/
	int ret = -1;
	DWORD err = 0;

	/* TODO extend with mode usage */
	if (CreateDirectoryW(path, NULL)) {
		ret = 0;
	} else {
		err = GetLastError();
	}

	if (0 > ret) {
		SET_ERRNO_FROM_WIN32_CODE(err);
	}

	return ret;
}/*}}}*/


PW32IO int php_win32_ioutil_mkdir(const char *path, mode_t mode)
{/*{{{*/
	wchar_t *pathw = php_win32_ioutil_mb_to_w(path);
	int ret = -1;
	DWORD err = 0;

	/* TODO extend with mode usage */
	if (pathw) {
		if (CreateDirectoryW(pathw, NULL)) {
			ret = 0;
		} else {
			err = GetLastError();
		}
		free(pathw);
	} else {
#if PHP_WIN32_IOUTIL_ANSI_COMPAT_MODE
		if (CreateDirectoryA(path, NULL)) {
			ret = 0;
		} else {
			err = GetLastError();
		}
#else
		SET_ERRNO_FROM_WIN32_CODE(ERROR_INVALID_PARAMETER);
		return ret;
#endif
	}

	if (0 > ret) {
		SET_ERRNO_FROM_WIN32_CODE(err);
	}

	return ret;
}/*}}}*/

#if PHP_WIN32_IOUTIL_ANSI_COMPAT_MODE
PW32IO int php_win32_ioutil_unlink_a(const char *path)
{
	int ret = 0;
	DWORD err = 0;

	if (!DeleteFileA(path)) {
		err = GetLastError();
		ret = -1;
	}

	if (0 > ret) {
		SET_ERRNO_FROM_WIN32_CODE(err);
	}

	return ret;
}
#endif

PW32IO int php_win32_ioutil_unlink_w(const wchar_t *path)
{
	int ret = 0;
	DWORD err = 0;

	if (!DeleteFileW(path)) {
		err = GetLastError();
		ret = -1;
	}

	if (0 > ret) {
		SET_ERRNO_FROM_WIN32_CODE(err);
	}

	return ret;
}

#if PHP_WIN32_IOUTIL_ANSI_COMPAT_MODE
PW32IO int php_win32_ioutil_rmdir_a(const char *path)
{
	int ret = 0;
	DWORD err = 0;

	if (!RemoveDirectoryA(path)) {
		err = GetLastError();
		ret = -1;
	}

	if (0 > ret) {
		SET_ERRNO_FROM_WIN32_CODE(err);
	}

	return ret;
}
#endif

PW32IO int php_win32_ioutil_rmdir_w(const wchar_t *path)
{
	int ret = 0;
	DWORD err = 0;

	if (!RemoveDirectoryW(path)) {
		err = GetLastError();
		ret = -1;
	}

	if (0 > ret) {
		SET_ERRNO_FROM_WIN32_CODE(err);
	}

	return ret;
}

/* an extended version could be implemented, for now direct functions can be used. */
#if 0
#if PHP_WIN32_IOUTIL_ANSI_COMPAT_MODE
PW32IO int php_win32_ioutil_access_a(const char *path, mode_t mode)
{
	return _access(path, mode);
}
#endif

PW32IO int php_win32_ioutil_access_w(const wchar_t *path, mode_t mode)
{
	return _waccess(path, mode);
}
#endif

#if 0
PW32IO HANDLE php_win32_ioutil_findfirstfile_w(char *path, WIN32_FIND_DATA *data)
{
	HANDLE ret = INVALID_HANDLE_VALUE;
	DWORD err;

	if (!path) {
		SET_ERRNO_FROM_WIN32_CODE(ERROR_INVALID_PARAMETER);
		return ret;
	}

	pathw = php_win32_ioutil_mb_to_w(path);

	if (!pathw) {
		err = GetLastError();
		SET_ERRNO_FROM_WIN32_CODE(ret);
		return ret;
	}

		ret = FindFirstFileW(pathw, data);
	
	if (INVALID_HANDLE_VALUE == ret && path) {
		ret = FindFirstFileA(path, data);
	}

	/* XXX set errno */
	return ret;
}
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
