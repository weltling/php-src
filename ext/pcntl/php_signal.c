/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2015 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Jason Greene <jason@inetgurus.net>                           |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "TSRM.h"
#include "php_signal.h"
#include "Zend/zend.h"
#include "Zend/zend_signal.h"

#ifdef PHP_WIN32
static _invalid_parameter_handler old_invalid_parameter_handler;

void swallow_invalid_parameter_handler(const wchar_t * expression, const wchar_t *function, const wchar_t *file, unsigned int line, uintptr_t pReserved)
{
		/* Do nothing */
}

/* Windows does not HAVE sigaction available to it, instead signal must be used */
Sigfunc *php_signal(int signo, Sigfunc *func, int restart)
{
	Sigfunc *value;

	/* Bad voodoo - PHP has a CRT invalid parameter handler, that's nice, but we'll be 
	   checking for errors in a moment and don't need double warnings! */
	old_invalid_parameter_handler = _set_invalid_parameter_handler(swallow_invalid_parameter_handler);

	/* Do the call */
	value = signal(signo, func);

	/* Restore the previous handler - my word this is ugly */
	if (old_invalid_parameter_handler != NULL) {
		_set_invalid_parameter_handler(old_invalid_parameter_handler);
	}

	return value;
}
#else

/* php_signal using sigaction is derived from Advanced Programing
 * in the Unix Environment by W. Richard Stevens p 298. */
Sigfunc *php_signal4(int signo, Sigfunc *func, int restart, int mask_all)
{
	struct sigaction act,oact;

	act.sa_handler = func;
	if (mask_all) {
		sigfillset(&act.sa_mask);
	} else {
		sigemptyset(&act.sa_mask);
	}
	act.sa_flags = 0;
	if (signo == SIGALRM || (! restart)) {
#ifdef SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT; /* SunOS */
#endif
	} else {
#ifdef SA_RESTART
		act.sa_flags |= SA_RESTART; /* SVR4, 4.3+BSD */
#endif
	}
#ifdef ZEND_SIGNALS
	if (zend_sigaction(signo, &act, &oact) < 0)
#else
	if (sigaction(signo, &act, &oact) < 0)
#endif
	{
		return SIG_ERR;
	}

	return oact.sa_handler;
}

Sigfunc *php_signal(int signo, Sigfunc *func, int restart)
{
	return php_signal4(signo, func, restart, 0);
}

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
