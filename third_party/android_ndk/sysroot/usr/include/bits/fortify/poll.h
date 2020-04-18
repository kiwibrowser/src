/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef _POLL_H_
#error "Never include this file directly; instead, include <poll.h>"
#endif


#if __ANDROID_API__ >= 23
int __poll_chk(struct pollfd*, nfds_t, int, size_t) __INTRODUCED_IN(23);
int __ppoll_chk(struct pollfd*, nfds_t, const struct timespec*, const sigset_t*, size_t)
  __INTRODUCED_IN(23);
#endif /* __ANDROID_API__ >= 23 */


#if defined(__BIONIC_FORTIFY)
#if __ANDROID_API__ >= __ANDROID_API_M__
#if defined(__clang__)
__BIONIC_FORTIFY_INLINE
int poll(struct pollfd* const fds __pass_object_size, nfds_t fd_count, int timeout)
    __overloadable
    __clang_error_if(__bos(fds) != __BIONIC_FORTIFY_UNKNOWN_SIZE &&
                       __bos(fds) < sizeof(*fds) * fd_count,
                     "in call to 'poll', fd_count is larger than the given buffer") {
  size_t bos_fds = __bos(fds);

  if (bos_fds == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
    return __call_bypassing_fortify(poll)(fds, fd_count, timeout);
  }
  return __poll_chk(fds, fd_count, timeout, bos_fds);
}

__BIONIC_FORTIFY_INLINE
int ppoll(struct pollfd* const fds __pass_object_size, nfds_t fd_count, const struct timespec* timeout, const sigset_t* mask)
    __overloadable
    __clang_error_if(__bos(fds) != __BIONIC_FORTIFY_UNKNOWN_SIZE &&
                       __bos(fds) < sizeof(*fds) * fd_count,
                     "in call to 'ppoll', fd_count is larger than the given buffer") {
  size_t bos_fds = __bos(fds);

  if (bos_fds == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
    return __call_bypassing_fortify(ppoll)(fds, fd_count, timeout, mask);
  }
  return __ppoll_chk(fds, fd_count, timeout, mask, bos_fds);
}
#else /* defined(__clang__) */
int __poll_real(struct pollfd*, nfds_t, int) __RENAME(poll);
__errordecl(__poll_too_small_error, "poll: pollfd array smaller than fd count");

int __ppoll_real(struct pollfd*, nfds_t, const struct timespec*, const sigset_t*) __RENAME(ppoll)
  __INTRODUCED_IN(21);
__errordecl(__ppoll_too_small_error, "ppoll: pollfd array smaller than fd count");

__BIONIC_FORTIFY_INLINE
int poll(struct pollfd* fds, nfds_t fd_count, int timeout) {
  if (__bos(fds) != __BIONIC_FORTIFY_UNKNOWN_SIZE) {
    if (!__builtin_constant_p(fd_count)) {
      return __poll_chk(fds, fd_count, timeout, __bos(fds));
    } else if (__bos(fds) / sizeof(*fds) < fd_count) {
      __poll_too_small_error();
    }
  }
  return __poll_real(fds, fd_count, timeout);
}

__BIONIC_FORTIFY_INLINE
int ppoll(struct pollfd* fds, nfds_t fd_count, const struct timespec* timeout,
          const sigset_t* mask) {
  if (__bos(fds) != __BIONIC_FORTIFY_UNKNOWN_SIZE) {
    if (!__builtin_constant_p(fd_count)) {
      return __ppoll_chk(fds, fd_count, timeout, mask, __bos(fds));
    } else if (__bos(fds) / sizeof(*fds) < fd_count) {
      __ppoll_too_small_error();
    }
  }
  return __ppoll_real(fds, fd_count, timeout, mask);
}

#endif /* defined(__clang__) */
#endif /* __ANDROID_API__ >= __ANDROID_API_M__ */
#endif /* defined(__BIONIC_FORTIFY) */
