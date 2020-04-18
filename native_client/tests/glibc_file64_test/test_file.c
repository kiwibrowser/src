/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * The type of the arguments to the comparison function changed
 * after glibc-2.9 to match the POSIX.1-2008 specification.
 */
#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 10
typedef const void *scandir_cmp_arg_t;
#else
typedef const struct dirent **scandir_cmp_arg_t;
#endif

int ftw_stub(const char* fpath, const struct stat* sb, int typeflag) {
    return 0;
}

int nftw_stub(const char* fpath, const struct stat* sb, int typeflag,
              struct FTW* ftwbuf) {
    return 0;
}

int scandir_filter_stub(const struct dirent* pdirent) {
    return 0;
}
int scandir_compare_stub(scandir_cmp_arg_t entry1,
                         scandir_cmp_arg_t entry2) {
    return 0;
}

int glob_stub(const char* epath, int err) {
    return 0;
}

int main(void) {
    /* dummy variables to prevent null warnings */
    char path[] = "";
    char mode[] = "";
    char buf[1];
    struct stat st;
    struct statfs stfs;
    struct statvfs stvfs;
    fpos_t fpos;
    off_t off;
    struct rlimit rlim;
    glob_t glb;
    int result;
    DIR* dir;
    struct dirent direntry;
    struct dirent* pdirentry;
    struct dirent** ppdirentry;
    const struct dirent *pconstdirentry;
    dir = 0;
    result = alphasort(&pconstdirentry, &pconstdirentry);
    creat(path, 0);
    fgetpos(0, &fpos);
    fopen(path, mode);
    freopen(path, mode, 0);
    fseeko(0, 0, 0);
    fsetpos(0, &fpos);
    fstat(0, &st);
    fstatat(0, path, &st, 0);
    fstatfs(0, &stfs);
    fstatvfs(0, &stvfs);
    ftello(0);
    ftruncate(0, 0);
    ftw(path, ftw_stub, 0);
    getdirentries(0, buf, 0, &off);
    getrlimit(0, &rlim);
    glob(path, 0, glob_stub, &glb);
    lockf(0, 0, 0);
    lseek(0, 0, 0);
    lstat(path, &st);
    mkostemp(path, 0);
    mkstemp(path);
    mmap(buf, 0, 0, 0, 0, 0);
    nftw(path, nftw_stub, 0, 0);
    open(path, 0);
    open(path, 0, 0);
    openat(0, path, 0);
    openat(0, path, 0, 0);
    posix_fadvise(0, 0, 0, 0);
    posix_fallocate(0, 0, 0);
    pread(0, buf, 0, 0);
    pwrite(0, buf, 0, 0);
    readdir(dir);
    readdir_r(dir, &direntry, &pdirentry);
    scandir(path, &ppdirentry, scandir_filter_stub, scandir_compare_stub);
    sendfile(0, 0, &off, 0);
    setrlimit(0, &rlim);
    stat(path, &st);
    statfs(path, &stfs);
    statvfs(path, &stvfs);
    tmpfile();
    truncate(path, 0);
    result = versionsort(&pconstdirentry, &pconstdirentry);
    return result;
}
