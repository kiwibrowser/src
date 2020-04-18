/*
 * Copyright (C) 2016 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _BITS_TERMIOS_INLINES_H_
#define _BITS_TERMIOS_INLINES_H_

#include <errno.h>
#include <sys/cdefs.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <linux/termios.h>

#if !defined(__BIONIC_TERMIOS_INLINE)
#define __BIONIC_TERMIOS_INLINE static __inline
#endif

__BEGIN_DECLS

static __inline speed_t cfgetspeed(const struct termios* s) {
  return __BIONIC_CAST(static_cast, speed_t, s->c_cflag & CBAUD);
}

__BIONIC_TERMIOS_INLINE speed_t cfgetispeed(const struct termios* s) {
  return cfgetspeed(s);
}

__BIONIC_TERMIOS_INLINE speed_t cfgetospeed(const struct termios* s) {
  return cfgetspeed(s);
}

__BIONIC_TERMIOS_INLINE void cfmakeraw(struct termios* s) {
  s->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
  s->c_oflag &= ~OPOST;
  s->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
  s->c_cflag &= ~(CSIZE|PARENB);
  s->c_cflag |= CS8;
}

__BIONIC_TERMIOS_INLINE int cfsetspeed(struct termios* s, speed_t speed) {
  // TODO: check 'speed' is valid.
  s->c_cflag = (s->c_cflag & ~CBAUD) | (speed & CBAUD);
  return 0;
}

__BIONIC_TERMIOS_INLINE int cfsetispeed(struct termios* s, speed_t speed) {
  return cfsetspeed(s, speed);
}

__BIONIC_TERMIOS_INLINE int cfsetospeed(struct termios* s, speed_t speed) {
  return cfsetspeed(s, speed);
}

__BIONIC_TERMIOS_INLINE int tcdrain(int fd) {
  // A non-zero argument to TCSBRK means "don't send a break".
  // The drain is a side-effect of the ioctl!
  return ioctl(fd, TCSBRK, __BIONIC_CAST(static_cast, unsigned long, 1));
}

__BIONIC_TERMIOS_INLINE int tcflow(int fd, int action) {
  return ioctl(fd, TCXONC, __BIONIC_CAST(static_cast, unsigned long, action));
}

__BIONIC_TERMIOS_INLINE int tcflush(int fd, int queue) {
  return ioctl(fd, TCFLSH, __BIONIC_CAST(static_cast, unsigned long, queue));
}

__BIONIC_TERMIOS_INLINE int tcgetattr(int fd, struct termios* s) {
  return ioctl(fd, TCGETS, s);
}

__BIONIC_TERMIOS_INLINE pid_t tcgetsid(int fd) {
  pid_t sid;
  return (ioctl(fd, TIOCGSID, &sid) == -1) ? -1 : sid;
}

__BIONIC_TERMIOS_INLINE int tcsendbreak(int fd, int duration) {
  return ioctl(fd, TCSBRKP, __BIONIC_CAST(static_cast, unsigned long, duration));
}

__BIONIC_TERMIOS_INLINE int tcsetattr(int fd, int optional_actions, const struct termios* s) {
  int cmd;
  switch (optional_actions) {
    case TCSANOW: cmd = TCSETS; break;
    case TCSADRAIN: cmd = TCSETSW; break;
    case TCSAFLUSH: cmd = TCSETSF; break;
    default: errno = EINVAL; return -1;
  }
  return ioctl(fd, cmd, s);
}

__END_DECLS

#endif
