/****************************************************************************
 * Copyright 2018,2020 Thomas E. Dickey                                     *
 * Copyright 2011-2014,2017 Free Software Foundation, Inc.                  *
 *                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a  *
 * copy of this software and associated documentation files (the            *
 * "Software"), to deal in the Software without restriction, including      *
 * without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute, distribute with modifications, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is    *
 * furnished to do so, subject to the following conditions:                 *
 *                                                                          *
 * The above copyright notice and this permission notice shall be included  *
 * in all copies or substantial portions of the Software.                   *
 *                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                          *
 * Except as contained in this notice, the name(s) of the above copyright   *
 * holders shall not be used in advertising or otherwise to promote the     *
 * sale, use or other dealings in this Software without prior written       *
 * authorization.                                                           *
 ****************************************************************************/

/****************************************************************************
 *  Author: Thomas E. Dickey                        2011                    *
 ****************************************************************************/

/* $Id: nc_termios.h,v 1.8 2020/08/29 20:53:19 tom Exp $ */

#ifndef NC_TERMIOS_included
#define NC_TERMIOS_included 1

#include <ncurses_cfg.h>

#if HAVE_TERMIOS_H && HAVE_TCGETATTR

#else /* !HAVE_TERMIOS_H */

#if HAVE_TERMIO_H

/*
 * Transitional POSIX/Unix compatibility: make termio provide the small
 * termios spelling used by the surviving tty path.  Modern Linux and Android
 * use termios directly, but retaining this block is harmless archaeology for
 * Unix-like systems while the build world is narrowed further.
 */
#ifndef TCSADRAIN
#define TCSADRAIN TCSETAW
#endif
#ifndef TCSAFLUSH
#define TCSAFLUSH TCSETAF
#endif
#ifndef tcsetattr
#define tcsetattr(fd, cmd, arg) ioctl(fd, cmd, arg)
#endif
#ifndef tcgetattr
#define tcgetattr(fd, arg) ioctl(fd, TCGETA, arg)
#endif
#ifndef cfgetospeed
#define cfgetospeed(t) ((t)->c_cflag & CBAUD)
#endif
#ifndef TCIFLUSH
#define TCIFLUSH 0
#endif
#ifndef tcflush
#define tcflush(fd, arg) ioctl(fd, TCFLSH, arg)
#endif

#endif /* HAVE_TERMIO_H */

/*
 * Removed here: the synthetic Win32 termios layer and its MinGW wrappers.
 *
 * That compatibility path supported the native Windows/MinGW backend removed
 * from this archaeology branch.  Linux/glibc and Android/Bionic provide the
 * POSIX termios interface used by the surviving tty implementation.
 */

#endif /* HAVE_TERMIOS_H */

#endif /* NC_TERMIOS_included */
