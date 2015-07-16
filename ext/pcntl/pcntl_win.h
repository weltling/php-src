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

#ifndef PHP_PCNTL_WIN_H
#define PHP_PCNTL_WIN_H

#define HAVE_GETPRIORITY 1
#define HAVE_SETPRIORITY 1

#define WIFEXITED(stat)   (((*((int *)&(stat))) & 0xff) == 0)
#define WEXITSTATUS(stat) (((*((int *)&(stat))) >> 8) & 0xff)
#define WIFSIGNALED(stat) (((*((int *)&(stat)))) && ((*((int *)&(stat))) == ((*((int *)&(stat))) & 0x00ff)))
#define WTERMSIG(stat)    ((*((int *)&(stat))) & 0x7f)
#define WIFSTOPPED(stat)  (((*((int *)&(stat))) & 0xff) == 0177)
#define WSTOPSIG(stat)    (((*((int *)&(stat))) >> 8) & 0xff)

#define WNOHANG 1
#define WUNTRACED 2

#define PRIO_PROCESS 0
#define PRIO_PGRP 1
#define PRIO_USER 2

#ifndef intptr_t
typedef INT_PTR intptr_t;
#endif

/* mask_all is ignored on Windows */
#define php_signal4(signo, func, restart, mask_all) php_signal(signo, func, restart)

pid_t waitpid(pid_t pid, int *stat_loc, int options);
pid_t wait(int *stat_loc);
unsigned int alarm(unsigned int seconds);
int setpriority(int which, int who, int prio);
int getpriority(int which, int who);

#endif