/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/untrusted/irt/irt_dev.h"
#include "native_client/src/untrusted/irt/irt_extension.h"
#include "native_client/tests/irt_ext/error_report.h"
#include "native_client/tests/irt_ext/file_desc.h"

#ifdef __GLIBC__
# include "native_client/src/trusted/service_runtime/include/bits/stat.h"
# define STAT_MEMBER(st_foo) nacl_abi_##st_foo
#else
# define STAT_MEMBER(st_foo) st_foo
#endif

/* Non-POSIX flags for open() helps with link logic. */
#ifndef O_NOFOLLOW
# define O_NOFOLLOW 0x8000
#endif

static struct file_desc_environment *g_activated_env = NULL;
static const struct inode_data g_default_inode = {
  .valid = false,
  .mode = S_IRUSR | S_IWUSR,
};
static const struct file_descriptor g_default_fd = {
  .valid = false,
  .data = NULL,
};

static struct inode_data g_root_dir = {
  .valid = true,
  .mode = S_IRUSR | S_IWUSR | S_IFDIR,
};

/* Helper allocation functions. */
struct inode_data *allocate_inode(struct file_desc_environment *env) {
  for (int i = 0; i < NACL_ARRAY_SIZE(env->inode_datas); i++) {
    if (!env->inode_datas[i].valid) {
      init_inode_data(&env->inode_datas[i]);
      env->inode_datas[i].valid = true;
      return &env->inode_datas[i];
    }
  }

  return NULL;
}

int allocate_file_descriptor(struct file_desc_environment *env) {
  for (int i = 0; i < NACL_ARRAY_SIZE(env->file_descs); i++) {
    if (!env->file_descs[i].valid) {
      init_file_descriptor(&env->file_descs[i]);
      env->file_descs[i].valid = true;
      return i;
    }
  }

  return -1;
}

/* NACL_IRT_DEV_FDIO_v0_3 functions */
static int my_close(int fd) {
  if (g_activated_env &&
      fd >= 0 &&
      fd < NACL_ARRAY_SIZE(g_activated_env->file_descs) &&
      g_activated_env->file_descs[fd].valid) {
    g_activated_env->file_descs[fd].valid = false;
    return 0;
  }

  return EBADF;
}

static int my_dup(int fd, int *newfd) {
  if (g_activated_env &&
      fd >= 0 &&
      fd < NACL_ARRAY_SIZE(g_activated_env->file_descs) &&
      g_activated_env->file_descs[fd].valid) {

    int try_fd = allocate_file_descriptor(g_activated_env);
    if (try_fd == -1) {
      return EMFILE;
    }

    g_activated_env->file_descs[try_fd] = g_activated_env->file_descs[fd];
    *newfd = try_fd;
    return 0;
  }

  return EBADF;
}

static int my_dup2(int fd, int newfd) {
  if (g_activated_env &&
      fd >= 0 &&
      fd < NACL_ARRAY_SIZE(g_activated_env->file_descs) &&
      newfd >= 0 &&
      newfd < NACL_ARRAY_SIZE(g_activated_env->file_descs) &&
      g_activated_env->file_descs[fd].valid) {
    if (fd == newfd)
      return 0;

    g_activated_env->file_descs[newfd] = g_activated_env->file_descs[fd];
    return 0;
  }

  return EBADF;
}

static int my_read(int fd, void *buf, size_t count, size_t *nread) {
  if (g_activated_env &&
      fd >= 0 &&
      fd < NACL_ARRAY_SIZE(g_activated_env->file_descs) &&
      g_activated_env->file_descs[fd].valid &&
      g_activated_env->file_descs[fd].data != NULL) {
    struct inode_data *filedata = g_activated_env->file_descs[fd].data;
    size_t read_count = 0;
    if (count <= filedata->size && filedata->size - count >= filedata->position)
      read_count = count;
    else if (filedata->position < filedata->size)
      read_count = filedata->size - filedata->position;

    memcpy(buf, &filedata->content[filedata->position], read_count);
    filedata->atime = g_activated_env->current_time++;
    filedata->position += read_count;

    *nread = read_count;
    return 0;
  }

  return EBADF;
}

static int my_write(int fd, const void *buf, size_t count, size_t *nwrote) {
  if (g_activated_env &&
      fd >= 0 &&
      fd < NACL_ARRAY_SIZE(g_activated_env->file_descs) &&
      g_activated_env->file_descs[fd].valid &&
      g_activated_env->file_descs[fd].data != NULL) {
    struct inode_data *filedata = g_activated_env->file_descs[fd].data;
    size_t write_count = 0;
    if (count <= NACL_ARRAY_SIZE(filedata->content) &&
        NACL_ARRAY_SIZE(filedata->content) - count >= filedata->position) {
      write_count = count;
    } else if (filedata->position < NACL_ARRAY_SIZE(filedata->content)) {
      write_count = NACL_ARRAY_SIZE(filedata->content) - filedata->position;
    }

    memcpy(&filedata->content[filedata->position], buf, write_count);
    filedata->mtime = g_activated_env->current_time++;
    filedata->position += write_count;
    if (filedata->size < filedata->position)
      filedata->size = filedata->position;

    *nwrote = write_count;
    return 0;
  }

  return EBADF;
}

static int my_seek(int fd, nacl_irt_off_t offset, int whence,
                  nacl_irt_off_t *new_offset) {
  if (g_activated_env &&
      fd >= 0 &&
      fd < NACL_ARRAY_SIZE(g_activated_env->file_descs) &&
      g_activated_env->file_descs[fd].valid &&
      g_activated_env->file_descs[fd].data != NULL) {
    struct inode_data *filedata = g_activated_env->file_descs[fd].data;
    off_t try_position = -1;
    switch (whence) {
      case SEEK_SET:
        try_position = offset;
        break;
      case SEEK_CUR:
        try_position = filedata->position + offset;
        break;
      case SEEK_END:
        try_position = filedata->size + offset;
        break;
      default:
        return EINVAL;
    }

    if (try_position < 0)
      return EINVAL;

    filedata->position = try_position;
    *new_offset = try_position;
    return 0;
  }

  return EBADF;
}

static int my_fstat(int fd, nacl_irt_stat_t *buf) {
  if (g_activated_env &&
      fd >= 0 &&
      fd < NACL_ARRAY_SIZE(g_activated_env->file_descs) &&
      g_activated_env->file_descs[fd].valid &&
      g_activated_env->file_descs[fd].data != NULL) {
    struct inode_data *filedata = g_activated_env->file_descs[fd].data;
    memset(buf, 0, sizeof(*buf));
    buf->STAT_MEMBER(st_mode) = filedata->mode;
    buf->STAT_MEMBER(st_size) = filedata->size;
    buf->STAT_MEMBER(st_atime) = filedata->atime;
    buf->STAT_MEMBER(st_mtime) = filedata->mtime;
    buf->STAT_MEMBER(st_ctime) = filedata->ctime;
    return 0;
  }

  return EBADF;
}

static int my_getdents(int fd, struct dirent *ents, size_t count,
                       size_t *nread) {
  if (g_activated_env &&
      fd >= 0 &&
      fd < NACL_ARRAY_SIZE(g_activated_env->file_descs) &&
      g_activated_env->file_descs[fd].valid &&
      g_activated_env->file_descs[fd].data != NULL) {
    const struct inode_data *dir_node = g_activated_env->file_descs[fd].data;
    if (!S_ISDIR(dir_node->mode))
      return ENOTDIR;

    int dir_position = g_activated_env->file_descs[fd].dir_position;
    char *ent_iter = (char *) ents;
    char *ent_end = (char *) ents + count;
    for (int i = dir_position;
         i < NACL_ARRAY_SIZE(g_activated_env->inode_datas);
         i++) {
      const struct inode_data *inode_iter = &g_activated_env->inode_datas[i];
      if (inode_iter->valid && inode_iter->parent_dir == dir_node) {
        size_t name_len = strlen(inode_iter->name) + 1;
        size_t ent_len = offsetof(struct dirent, d_name) + name_len;
        char *next_ent = ent_iter + ent_len;
        if (next_ent > ent_end)
          break;

        struct dirent *ent_desc = (struct dirent *) ent_iter;
        memset(ent_desc, 0, ent_len);
        ent_desc->d_ino = i;
        ent_desc->d_reclen = ent_len;
        memcpy(ent_desc->d_name, inode_iter->name, name_len);

        ent_iter = next_ent;
      }

      g_activated_env->file_descs[fd].dir_position++;
    }

    *nread = ent_iter - (char *) ents;
    return 0;
  }

  return EBADF;
}

static int my_fchdir(int fd) {
  if (g_activated_env &&
      fd >= 0 &&
      fd < NACL_ARRAY_SIZE(g_activated_env->file_descs) &&
      g_activated_env->file_descs[fd].valid &&
      g_activated_env->file_descs[fd].data != NULL) {
    if (!S_ISDIR(g_activated_env->file_descs[fd].data->mode))
      return ENOTDIR;

    g_activated_env->current_dir = g_activated_env->file_descs[fd].data;
    return 0;
  }

  return EBADF;
}

static int my_fchmod(int fd, mode_t mode) {
  if (g_activated_env &&
      fd >= 0 &&
      fd < NACL_ARRAY_SIZE(g_activated_env->file_descs) &&
      g_activated_env->file_descs[fd].valid &&
      g_activated_env->file_descs[fd].data != NULL) {
    struct inode_data *filedata = g_activated_env->file_descs[fd].data;

    /* For the purposes of tests, only the owner permission bits can be set. */
    filedata->mode = (filedata->mode & (~S_IRWXU)) | (mode & S_IRWXU);
    return 0;
  }

  return EBADF;
}

static int my_fsync(int fd) {
  if (g_activated_env &&
      fd >= 0 &&
      fd < NACL_ARRAY_SIZE(g_activated_env->file_descs) &&
      g_activated_env->file_descs[fd].valid) {
    g_activated_env->file_descs[fd].fsync = true;
    g_activated_env->file_descs[fd].fdatasync = true;
    return 0;
  }
  return EBADF;
}

static int my_fdatasync(int fd) {
  if (g_activated_env &&
      fd >= 0 &&
      fd < NACL_ARRAY_SIZE(g_activated_env->file_descs) &&
      g_activated_env->file_descs[fd].valid) {
    g_activated_env->file_descs[fd].fdatasync = true;
    return 0;
  }
  return EBADF;
}

static int my_ftruncate(int fd, nacl_irt_off_t length) {
  if (g_activated_env &&
      fd >= 0 &&
      fd < NACL_ARRAY_SIZE(g_activated_env->file_descs) &&
      g_activated_env->file_descs[fd].valid &&
      g_activated_env->file_descs[fd].data != NULL) {
    struct inode_data *filedata = g_activated_env->file_descs[fd].data;
    if (length < 0)
      return EINVAL;
    else if (length > BUFFER_SIZE)
      return EFBIG;

    if (length > filedata->size)
      memset(&filedata->content[filedata->size], 0, length - filedata->size);

    if (length != filedata->size) {
      filedata->size = length;
      filedata->ctime = g_activated_env->current_time++;
      filedata->mtime = filedata->ctime;
    }

    return 0;
  }

  return EBADF;
}

static int my_isatty(int fd, int *result) {
  if (g_activated_env &&
      fd >= 0 &&
      fd < NACL_ARRAY_SIZE(g_activated_env->file_descs) &&
      g_activated_env->file_descs[fd].valid &&
      g_activated_env->file_descs[fd].data != NULL) {
    *result = (fd >= 0 && fd < 3);
    return 0;
  }

  return EBADF;
}

/* NACL_IRT_DEV_FILENAME_v0_3 functions */
static int my_open(const char *pathname, int oflag, mode_t cmode, int *newfd) {
  if (g_activated_env) {
    struct inode_data *parent_dir = NULL;
    struct inode_data *path_item = find_inode_path(g_activated_env,
                                                   pathname, &parent_dir);
    const char *filename = strrchr(pathname, '/');
    if (filename == NULL)
      filename = pathname;
    else
      filename = filename + 1;

    /* If parent directory is invalid, must not have been a directory. */
    if (parent_dir == NULL) {
      return ENOTDIR;
    }

    bool write_access = false;
    switch (oflag & O_ACCMODE) {
      case O_WRONLY:
      case O_RDWR:
        /* Both write and readwrite access has special checks later. */
        write_access = true;

      case O_RDONLY:
        break;
      default:
        return EINVAL;
    }

    /* If requested write/read write access, path cannot be a directory. */
    if (write_access && path_item && S_ISDIR(path_item->mode))
      return EISDIR;

    if (oflag & O_CREAT) {
      if (path_item == NULL) {
        if (0 == (parent_dir->mode & S_IWUSR))
          return EACCES;

        path_item = allocate_inode(g_activated_env);
        if (path_item == NULL) {
          return ENOSPC;
        }
        path_item->ctime = g_activated_env->current_time++;
        path_item->atime = path_item->ctime;
        path_item->mtime = path_item->ctime;
        path_item->mode = cmode;
        path_item->parent_dir = parent_dir;

        size_t file_len = strlen(filename);
        if (file_len >= NACL_ARRAY_SIZE(path_item->name))
          return ENAMETOOLONG;

        memcpy(path_item->name, filename, file_len + 1);
        path_item->valid = true;
      } else if (oflag & O_EXCL) {
        return EEXIST;
      }
    }

    if (path_item) {
      int follow_count = 0;
      while (path_item->link) {
        if (S_ISLNK(path_item->mode) && (0 != (oflag & O_NOFOLLOW)))
          break;

        path_item = path_item->link;

        follow_count++;
        if (follow_count > MAX_INODES) {
          return ELOOP;
        }
      }
    }

    if (write_access && (oflag & O_TRUNC) && path_item) {
      path_item->position = 0;
      path_item->size = 0;
    }

    int try_fd = allocate_file_descriptor(g_activated_env);
    if (try_fd == -1) {
      return ENFILE;
    }

    g_activated_env->file_descs[try_fd].oflag = oflag;
    g_activated_env->file_descs[try_fd].data = path_item;
    *newfd = try_fd;
    return 0;
  }

  return ENOSYS;
}

static int my_stat(const char *pathname, nacl_irt_stat_t *buf) {
  if (g_activated_env) {
    int fd = -1;
    int ret = 0;

    ret = my_open(pathname, O_RDONLY, 0, &fd);
    if (ret != 0)
      return ret;

    ret = my_fstat(fd, buf);
    if (ret != 0)
      return ret;

    return my_close(fd);
  }

  return EIO;
}

static int my_mkdir(const char *pathname, mode_t mode) {
  if (g_activated_env) {
    struct inode_data *parent_dir = NULL;
    struct inode_data *path_item = find_inode_path(g_activated_env,
                                                   pathname, &parent_dir);
    struct inode_data *new_dir = NULL;
    const char *dirname = strrchr(pathname, '/');
    if (dirname == NULL)
      dirname = pathname;
    else
      dirname = dirname + 1;

    /* If parent directory is invalid, must not have been a directory. */
    if (parent_dir == NULL)
      return ENOTDIR;

    /* Sub-directories are not allowed. */
    if (parent_dir != &g_root_dir)
      return ENOSPC;

    if (path_item)
      return EEXIST;

    new_dir = allocate_inode(g_activated_env);
    if (new_dir == NULL) {
      return ENOSPC;
    }

    new_dir->mode = mode | S_IFDIR;
    new_dir->parent_dir = parent_dir;
    new_dir->name[NACL_ARRAY_SIZE(new_dir->name)-1] = '\0';
    strncpy(new_dir->name, dirname, NACL_ARRAY_SIZE(new_dir->name));
    if (new_dir->name[NACL_ARRAY_SIZE(new_dir->name)-1] != '\0') {
      return ENAMETOOLONG;
    }
    new_dir->valid = true;

    return 0;
  }

  return ENOSYS;
}

static int my_rmdir(const char *pathname) {
  if (g_activated_env) {
    struct inode_data *parent_dir = NULL;
    struct inode_data *path_item = find_inode_path(g_activated_env,
                                                   pathname, &parent_dir);

    if (path_item == NULL || !S_ISDIR(path_item->mode))
      return ENOTDIR;

    /* Make sure the directory is empty. */
    for (int i = 0; i < NACL_ARRAY_SIZE(g_activated_env->inode_datas); i++) {
      if (path_item->valid &&
          path_item == g_activated_env->inode_datas[i].parent_dir) {
        return ENOTEMPTY;
      }
    }

    path_item->valid = false;
    return 0;
  }

  return ENOSYS;
}

static int my_chdir(const char *pathname) {
  if (g_activated_env) {
    struct inode_data *parent_dir = NULL;
    struct inode_data *path_item = find_inode_path(g_activated_env,
                                                   pathname, &parent_dir);

    if (path_item == NULL || !S_ISDIR(path_item->mode))
      return ENOTDIR;

    g_activated_env->current_dir = path_item;
    return 0;
  }

  return ENOSYS;
}

static int my_getcwd(char *pathname, size_t len) {
  if (g_activated_env) {
    const struct inode_data *current_dir = g_activated_env->current_dir;

    if (len == 0) {
      return pathname ? EINVAL : 0;
    }

    if (current_dir == &g_root_dir) {
      if (len < 2)
        return ENAMETOOLONG;
      pathname[0] = '/';
      pathname[1] = '\0';
    } else {
      int total_len = snprintf(pathname, len, "/%s", current_dir->name);
      if (total_len >= len)
        return ENAMETOOLONG;
    }

    return 0;
  }

  return ENOSYS;
}

static int my_unlink(const char *pathname) {
  if (g_activated_env) {
    struct inode_data *parent_dir = NULL;
    struct inode_data *path_item = find_inode_path(g_activated_env,
                                                   pathname, &parent_dir);
    if (parent_dir == NULL)
      return ENOTDIR;

    if (path_item == NULL)
      return ENOENT;

    if (S_ISDIR(path_item->mode))
      return EISDIR;

    path_item->valid = false;
    return 0;
  }
  return ENOSYS;
}

static int my_truncate(const char *pathname, nacl_irt_off_t length) {
  if (g_activated_env) {
    int fd = -1;
    int ret = my_open(pathname, O_WRONLY, 0, &fd);
    if (ret != 0)
      return ret;

    ret = my_ftruncate(fd, length);
    if (ret != 0)
      return ret;

    return my_close(fd);
  }

  return ENOSYS;
}

static int my_lstat(const char *pathname, nacl_irt_stat_t *buf) {
  if (g_activated_env) {
    int fd = -1;
    int ret = 0;

    ret = my_open(pathname, O_RDONLY | O_NOFOLLOW, 0, &fd);
    if (ret != 0)
      return ret;

    ret = my_fstat(fd, buf);
    if (ret != 0)
      return ret;

    return my_close(fd);
  }
  return ENOSYS;
}

static int my_link(const char *oldpath, const char *newpath) {
  if (g_activated_env) {
    int old_fd = -1;
    int new_fd = -1;
    int ret = 1;
    int old_close_ret = 0;
    int new_close_ret = 0;

    struct file_descriptor *fd_table = g_activated_env->file_descs;
    int try_fd;
    ret = my_open(oldpath, O_RDONLY, 0, &try_fd);
    if (ret != 0)
      goto cleanup;

    old_fd = try_fd;
    struct inode_data *old_path_item = fd_table[old_fd].data;

    if (S_ISDIR(old_path_item->mode)) {
      ret = EPERM;
      goto cleanup;
    }

    ret = my_open(newpath, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR,
                  &try_fd);
    if (ret != 0)
      goto cleanup;

    new_fd = try_fd;
    struct inode_data *new_path_item = fd_table[new_fd].data;

    new_path_item->link = old_path_item;

 cleanup:
    if (old_fd != -1) {
      old_close_ret = my_close(old_fd);
    }

    if (new_fd != -1) {
      new_close_ret = my_close(new_fd);
    }

    if (ret != 0)
      return ret;
    else if (old_close_ret != 0)
      return old_close_ret;
    else if (new_close_ret != 0)
      return new_close_ret;

    return 0;
  }

  return ENOSYS;
}

static int my_rename(const char *oldpath, const char *newpath) {
  if (g_activated_env) {
    struct inode_data *old_parent = NULL;
    struct inode_data *old_path_item = find_inode_path(g_activated_env,
                                                       oldpath, &old_parent);

    struct inode_data *new_parent = NULL;
    struct inode_data *new_path_item = find_inode_path(g_activated_env,
                                                       newpath, &new_parent);

    const char *filename = strrchr(newpath, '/');
    if (filename == NULL)
      filename = newpath;
    else
      filename = filename + 1;

    if (old_parent == NULL || new_parent == NULL)
      return ENOTDIR;

    if (old_path_item == NULL)
      return EACCES;

    if (S_ISDIR(old_path_item->mode))
      return EISDIR;

    if (new_path_item)
      return EEXIST;

    const size_t file_len = strlen(filename) + 1;
    if (file_len > NACL_ARRAY_SIZE(old_path_item->name))
      return ENAMETOOLONG;

    old_path_item->parent_dir = new_parent;
    memcpy(old_path_item->name, filename, file_len);
    return 0;
  }
  return ENOSYS;
}

static int my_symlink(const char *oldpath, const char *newpath) {
  /*
   * For the purposes of this test harness, the only difference between link()
   * and symlink() is that symlnk() sets S_IFLNK.
   */
  if (g_activated_env) {
    int ret = my_link(oldpath, newpath);
    if (ret != 0)
      return ret;

    struct inode_data *link_node_parent = NULL;
    struct inode_data *link_node = find_inode_path(g_activated_env, newpath,
                                                   &link_node_parent);

    link_node->mode |= S_IFLNK;
    return 0;
  }

  return ENOSYS;
}

static int my_chmod(const char *path, mode_t mode) {
  if (g_activated_env) {
    int fd = -1;
    int ret = 0;

    ret = my_open(path, O_RDONLY, 0, &fd);
    if (ret != 0)
      return ret;

    ret = my_fchmod(fd, mode);
    if (ret != 0)
      return ret;

    return my_close(fd);
  }

  return ENOSYS;
}

static int my_access(const char *path, int amode) {
  /* amode must be F_OK or a mask of R_OK, W_OK, and X_OK. */
  if (0 != amode &&
      F_OK != amode &&
      0 != (amode & (~(R_OK | W_OK | X_OK)))) {
    return EINVAL;
  }

  nacl_irt_stat_t path_stat;
  int ret = my_stat(path, &path_stat);
  if (0 != ret) {
    return ret;
  } else if (F_OK == amode) {
    return 0;
  }

  if (((amode & R_OK) &&
       S_IRUSR != (path_stat.STAT_MEMBER(st_mode) & S_IRUSR)) ||
      ((amode & W_OK) &&
       S_IWUSR != (path_stat.STAT_MEMBER(st_mode) & S_IWUSR)) ||
      ((amode & X_OK) &&
       S_IXUSR != (path_stat.STAT_MEMBER(st_mode) & S_IXUSR))) {
    return EACCES;
  }

  return 0;
}

static int my_readlink(const char *pathname, char *buf, size_t count,
                       size_t *nread) {
  if (g_activated_env) {
    int fd;
    int ret = my_open(pathname, O_RDONLY, 0, &fd);
    if (ret != 0)
      return ret;

    struct inode_data *file_item = g_activated_env->file_descs[fd].data;

    int written_len;
    if (file_item->parent_dir == &g_root_dir) {
      written_len = snprintf(buf, count, "/%s", file_item->name);
    } else {
      written_len = snprintf(buf, count, "/%s/%s",
                             file_item->parent_dir->name, file_item->name);
    }

    if (written_len >= count) {
      return ENAMETOOLONG;
    }

    *nread = written_len;
    return my_close(fd);
  }
  return ENOSYS;
}

static int my_utimes(const char *path, const struct timeval *times) {
  if (times == NULL)
    return EINVAL;

  if (g_activated_env) {
    int fd = -1;
    int ret = 0;

    ret = my_open(path, O_WRONLY, 0, &fd);
    if (ret != 0)
      return ret;

    struct inode_data *path_item = g_activated_env->file_descs[fd].data;
    path_item->atime = times[0].tv_sec;
    path_item->mtime = times[1].tv_sec;

    return my_close(fd);
  }

  return ENOSYS;
}

/* Module file_desc functions. */
void init_file_desc_module(void) {
  struct nacl_irt_dev_fdio fdio = {
    my_close,
    my_dup,
    my_dup2,
    my_read,
    my_write,
    my_seek,
    my_fstat,
    my_getdents,
    my_fchdir,
    my_fchmod,
    my_fsync,
    my_fdatasync,
    my_ftruncate,
    my_isatty,
  };

  size_t size = nacl_interface_ext_supply(NACL_IRT_DEV_FDIO_v0_3, &fdio,
                                          sizeof(fdio));
  IRT_EXT_ASSERT_MSG(size == sizeof(fdio),
                     "Could not supply interface: " NACL_IRT_DEV_FDIO_v0_3);

  /* nacl_irt_fdio shares the same prefix as nacl_irt_dev_fdio. */
  size = nacl_interface_ext_supply(NACL_IRT_FDIO_v0_1, &fdio,
                                   sizeof(struct nacl_irt_fdio));
  IRT_EXT_ASSERT_MSG(size == sizeof(struct nacl_irt_fdio),
                     "Could not supply interface: " NACL_IRT_FDIO_v0_1);

  struct nacl_irt_dev_filename fname = {
    my_open,
    my_stat,
    my_mkdir,
    my_rmdir,
    my_chdir,
    my_getcwd,
    my_unlink,
    my_truncate,
    my_lstat,
    my_link,
    my_rename,
    my_symlink,
    my_chmod,
    my_access,
    my_readlink,
    my_utimes,
  };

  size = nacl_interface_ext_supply(NACL_IRT_DEV_FILENAME_v0_3, &fname,
                                   sizeof(fname));
  IRT_EXT_ASSERT_MSG(size == sizeof(fname),
                     "Could not supply interface: " NACL_IRT_DEV_FILENAME_v0_3);
}

void init_inode_data(struct inode_data *inode_data) {
  *inode_data = g_default_inode;
}

void init_file_descriptor(struct file_descriptor *file_desc) {
  *file_desc = g_default_fd;
}

void init_file_desc_environment(struct file_desc_environment *env) {
  env->current_dir = &g_root_dir;
  for (int i = 0; i < NACL_ARRAY_SIZE(env->inode_datas); i++) {
    init_inode_data(&env->inode_datas[i]);
  }
  for (int i = 0; i < NACL_ARRAY_SIZE(env->file_descs); i++) {
    init_file_descriptor(&env->file_descs[i]);
  }

  /* Initialize STDIN, STDOUT, STDERR. */
  for (int i = 0; i < 3; i++) {
    env->file_descs[i].valid = true;
    env->file_descs[i].data = allocate_inode(env);
  }
}

struct inode_data *find_inode_name(struct file_desc_environment *env,
                                   struct inode_data *parent_dir,
                                   const char *name, size_t name_len) {
  for (int i = 0; i < NACL_ARRAY_SIZE(env->inode_datas); i++) {
    if (env->inode_datas[i].valid &&
        parent_dir == env->inode_datas[i].parent_dir &&
        strncmp(env->inode_datas[i].name, name, name_len) == 0 &&
        env->inode_datas[i].name[name_len] == '\0') {
      return &env->inode_datas[i];
    }
  }

  return NULL;
}

struct inode_data *find_inode_path(struct file_desc_environment *env,
                                   const char *path,
                                   struct inode_data **parent_dir) {
  const char *path_iter = path;
  const char *slash_loc;
  struct inode_data *current_dir = env->current_dir;
  if (*path_iter == '/') {
    current_dir = &g_root_dir;
    path_iter++;
  }

  /* See if there is a directory in the path. */
  if (current_dir == &g_root_dir) {
    slash_loc = strchr(path_iter, '/');
    if (slash_loc != NULL) {
      const size_t dir_len = slash_loc - path_iter;
      struct inode_data *dir_item = find_inode_name(env, current_dir,
                                                    path_iter, dir_len);
      if (dir_item == NULL || !S_ISDIR(dir_item->mode)) {
        *parent_dir = NULL;
        return NULL;
      }

      current_dir = dir_item;
      path_iter = slash_loc + 1;
    }
  }

  /* See if there is another slash, sub-directories are not allowed. */
  slash_loc = strchr(path_iter, '/');
  if (slash_loc != NULL) {
    /* sub-directories are not allowed. */
    *parent_dir = NULL;
    return NULL;
  }

  /* When last character is a slash, found path is the current directory. */
  if (*path_iter == '\0') {
    *parent_dir = current_dir->parent_dir;
    return current_dir;
  }

  /* path_iter now points to last item in the path. */
  *parent_dir = current_dir;
  return find_inode_name(env, current_dir, path_iter, strlen(path_iter));
}

void activate_file_desc_env(struct file_desc_environment *env) {
  g_activated_env = env;
}

void deactivate_file_desc_env(void) {
  g_activated_env = NULL;
}
