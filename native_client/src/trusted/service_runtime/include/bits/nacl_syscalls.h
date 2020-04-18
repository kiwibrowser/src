/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl kernel / service run-time system call numbers
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_BITS_NACL_SYSCALLS_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_BITS_NACL_SYSCALLS_H_

/* intentionally not using zero */

/*
 * TODO(bsy,sehr): these identifiers should be NACL_ABI_SYS_<name>.
 */

#define NACL_sys_null                    1

#define NACL_sys_dup                     8
#define NACL_sys_dup2                    9
#define NACL_sys_open                   10
#define NACL_sys_close                  11
#define NACL_sys_read                   12
#define NACL_sys_write                  13
#define NACL_sys_lseek                  14
/* 15 used to be ioctl */
#define NACL_sys_stat                   16
#define NACL_sys_fstat                  17
#define NACL_sys_chmod                  18
#define NACL_sys_isatty                 19

#define NACL_sys_brk                    20
#define NACL_sys_mmap                   21
#define NACL_sys_munmap                 22

#define NACL_sys_getdents               23

#define NACL_sys_mprotect               24

#define NACL_sys_list_mappings          25

#define NACL_sys_fsync                  26
#define NACL_sys_fdatasync              27
#define NACL_sys_fchmod                 28

#define NACL_sys_exit                   30
#define NACL_sys_getpid                 31
#define NACL_sys_sched_yield            32
#define NACL_sys_sysconf                33

#define NACL_sys_gettimeofday           40
#define NACL_sys_clock                  41
#define NACL_sys_nanosleep              42
#define NACL_sys_clock_getres           43
#define NACL_sys_clock_gettime          44

#define NACL_sys_mkdir                  45
#define NACL_sys_rmdir                  46
#define NACL_sys_chdir                  47
#define NACL_sys_getcwd                 48
#define NACL_sys_unlink                 49
#define NACL_sys_fchdir                 50

#define NACL_sys_ftruncate              52

/* 50-58 previously used for multimedia syscalls */

#define NACL_sys_imc_makeboundsock      60
#define NACL_sys_imc_accept             61
#define NACL_sys_imc_connect            62
#define NACL_sys_imc_sendmsg            63
#define NACL_sys_imc_recvmsg            64
#define NACL_sys_imc_mem_obj_create     65
#define NACL_sys_imc_socketpair         66

#define NACL_sys_mutex_create           70
#define NACL_sys_mutex_lock             71
#define NACL_sys_mutex_trylock          72
#define NACL_sys_mutex_unlock           73
#define NACL_sys_cond_create            74
#define NACL_sys_cond_wait              75
#define NACL_sys_cond_signal            76
#define NACL_sys_cond_broadcast         77
#define NACL_sys_cond_timed_wait_abs    79

#define NACL_sys_thread_create          80
#define NACL_sys_thread_exit            81
#define NACL_sys_tls_init               82
#define NACL_sys_thread_nice            83
#define NACL_sys_tls_get                84
#define NACL_sys_second_tls_set         85
#define NACL_sys_second_tls_get         86
#define NACL_sys_exception_handler      87
#define NACL_sys_exception_stack        88
#define NACL_sys_exception_clear_flag   89

#define NACL_sys_sem_create             100
#define NACL_sys_sem_wait               101
#define NACL_sys_sem_post               102
#define NACL_sys_sem_get_value          103

#define NACL_sys_dyncode_create         104
#define NACL_sys_dyncode_modify         105
#define NACL_sys_dyncode_delete         106

#define NACL_sys_test_infoleak          109
#define NACL_sys_test_crash             110

/*
 * These syscall numbers are set aside for use in tests that add
 * syscalls that must coexist with the normal syscalls.
 */
#define NACL_sys_test_syscall_1         111
#define NACL_sys_test_syscall_2         112

#define NACL_sys_futex_wait_abs         120
#define NACL_sys_futex_wake             121

#define NACL_sys_pread                  130
#define NACL_sys_pwrite                 131

#define NACL_sys_truncate               140
#define NACL_sys_lstat                  141
#define NACL_sys_link                   142
#define NACL_sys_rename                 143
#define NACL_sys_symlink                144
#define NACL_sys_access                 145
#define NACL_sys_readlink               146
#define NACL_sys_utimes                 147

#define NACL_sys_get_random_bytes       150

#define NACL_MAX_SYSCALLS               151

#endif
