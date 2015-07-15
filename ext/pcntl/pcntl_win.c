/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2009 The PHP Group                                |
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

#include "php.h"
#include "pcntl_win.h"

/* Implementation of waitpid - only supports WNOHANG for options */
pid_t waitpid (pid_t pid, int *stat_loc, int options) {
    DWORD timeout;

    if (options == WNOHANG) {
		timeout = 0;
    } else {
		timeout = INFINITE;
    }
    if (WaitForSingleObject((HANDLE) pid, timeout) == WAIT_OBJECT_0) {
		pid = _cwait(stat_loc, pid, 0);
		return pid;
    }
    return 0;
}

/* Implementation of wait - only supports processes created via the pcntl_spawn method */
pid_t wait (int *stat_loc) {
	/*
     TODO: make spawn store the process handles on nowait
	 put process handle hashtable in globals
	 make waitpid and wait remove process handles
	 similiar to perl's handling of the matter ;)
	*/
    return 0;
}

/* Implementation of alarm using timers */
unsigned int alarm(unsigned int seconds) {
}

/* Implementation of setpriority using SetPriorityClass */
int setpriority(int which, int who, int prio) {
	HANDLE process;
	zend_bool close_handle = 0;
	DWORD dwFlag = NORMAL_PRIORITY_CLASS;
	BOOL success;

	/* On windows, which can only be 0 */
	if (which != 0) {
		errno = EINVAL; /* Invalid which value */
		return -1;
	}

	/* If who is 0, get the current process handle */
	if (who == 0) {
		process = GetCurrentProcess();
	} else {
	/* Take the pid provided and open a process handle to use */
		process = OpenProcess(PROCESS_SET_INFORMATION, FALSE, who);
		if (!process) {
			errno = ESRCH; /* Invalid identifier */
			return -1;
		} else {
			close_handle = 1;
		}
	}

	/* Translate priorities into windows priority constants
	   range is guessed at -15 to 15 - 0 is included in normal */
	/* anything less then -10 is idle */
	if (prio < -10) {
		dwFlag = IDLE_PRIORITY_CLASS;
	/* -10 to -5 is below normal */
	} else if (prio < -5 && prio > -11) {
		dwFlag = BELOW_NORMAL_PRIORITY_CLASS;
	/* -5 to 0 is normal */
	} else if (prio > -6 && prio < 1) {
		dwFlag = NORMAL_PRIORITY_CLASS;
	/* 1 to 5 is above normal */
	} else if (prio > 0 && prio < 6) {
		dwFlag = ABOVE_NORMAL_PRIORITY_CLASS;
	/* 5 to 10 is high */
	} else if (prio > 5 && prio < 11) {
		dwFlag = HIGH_PRIORITY_CLASS;
	/* anything above 10 is realtime */
	} else if (prio > 10) {
		dwFlag = REALTIME_PRIORITY_CLASS;
	}

	/* do the setpriority call */
	success = SetPriorityClass(process, dwFlag);

	/* close any handles */
	if (close_handle) {
		CloseHandle(process);
	}

	/* return values are -1 for an error or zero for worked */
	if (success == 0) {
		/* windows uses 0 for fail */
		errno = GetLastError();
		return -1;
	} else {
		return 0;
	}
}

/* Implementation of getpriority using GetPriorityClass */
int getpriority(int which, int who) {
	HANDLE process;
	zend_bool close_handle = 0;
	DWORD value;

	/* On windows, which can only be 0 */
	if (which != 0) {
		errno = EINVAL; /* Invalid which value */
		return -1;
	}

	/* If who is 0, get the current process handle */
	if (who == 0) {
		process = GetCurrentProcess();
	} else {
	/* Take the pid provided and open a process handle to use */
		process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, who);
		if (!process) {
			errno = ESRCH; /* Invalid identifier */
			return -1;
		} else {
			close_handle = 1;
		}
	}

	/* do the getpriority call */
	value = GetPriorityClass(process);

	/* close any handles */
	if (close_handle) {
		CloseHandle(process);
	}

	/* Translate the windows priority constants into simple ints
	   range is guessed at -15 to 15 - 0 is included in normal */
	/* anything less then -10 is idle */
	switch(value) {
		case IDLE_PRIORITY_CLASS:
			return -11;
		case BELOW_NORMAL_PRIORITY_CLASS:
			return -6;
		case NORMAL_PRIORITY_CLASS:
			return 0;
		case ABOVE_NORMAL_PRIORITY_CLASS:
			return 4;
		case HIGH_PRIORITY_CLASS:
			return 9;
		case REALTIME_PRIORITY_CLASS:
			return 15;
		default:
			return 1;
	}
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */