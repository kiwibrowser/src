/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/tests/irt_ext/error_report.h"
#include "native_client/tests/irt_ext/file_desc.h"
#include "native_client/tests/irt_ext/libc/libc_test.h"

#define TEST_DIRECTORY "test_directory"
#define TEST_FILE "test_file.txt"
#define TEST_FILE2 "test_file2.txt"
#define TEST_TIME_VALUE 20

static const char TEST_TEXT[] = "test text";

/*
 * TODO(dyen): remove this once this declarations get added to the prebuilt
 * newlib toolchain.
 */
extern int utimes(const char *filename, const struct timeval times[2]);
extern int fchdir(int dir_fd);

#ifdef _NEWLIB_VERSION
/*
 * TODO(dyen): Move the definition and declaration of this function to newlib.
 */
int dirfd(DIR *dir) {
  if (dir == NULL) {
    errno = EINVAL;
    return -1;
  }
  return dir->dd_fd;
}
#endif

typedef int (*TYPE_file_test)(struct file_desc_environment *file_desc_env);

/* Directory tests. */
static int do_mkdir_rmdir_test(struct file_desc_environment *file_desc_env) {
  struct inode_data *parent_dir = NULL;
  struct inode_data *test_dir = NULL;

  if (0 != mkdir(TEST_DIRECTORY, S_IRUSR | S_IWUSR)) {
    irt_ext_test_print("Could not create directory: %s\n",
                       strerror(errno));
    return 1;
  }

  test_dir = find_inode_path(file_desc_env, "/" TEST_DIRECTORY, &parent_dir);
  if (test_dir == NULL) {
    irt_ext_test_print("mkdir: dir was not successfully created.\n");
    return 1;
  }

  if (0 != rmdir(TEST_DIRECTORY))  {
    irt_ext_test_print("Could not remove directory: %s\n",
                       strerror(errno));
    return 1;
  }

  test_dir = find_inode_path(file_desc_env, "/" TEST_DIRECTORY, &parent_dir);
  if (test_dir != NULL) {
    irt_ext_test_print("rmdir: dir was not successfully removed.\n");
    return 1;
  }

  return 0;
}

static int do_chdir_test(struct file_desc_environment *file_desc_env) {
  if (0 != mkdir(TEST_DIRECTORY, S_IRUSR | S_IWUSR)) {
    irt_ext_test_print("Could not create directory: %s\n",
                       strerror(errno));
    return 1;
  }

  if (0 != chdir(TEST_DIRECTORY)) {
    irt_ext_test_print("Could not change directory: %s\n",
                       strerror(errno));
    return 1;
  }
  if (strcmp(file_desc_env->current_dir->name, TEST_DIRECTORY) != 0) {
    irt_ext_test_print("do_chdir_test: directory change failed.\n");
    return 1;
  }

  if (0 != chdir("/")) {
    irt_ext_test_print("Could not change to root directory: %s\n",
                       strerror(errno));
    return 1;
  }
  if (file_desc_env->current_dir->name[0] != '\0') {
    irt_ext_test_print("do_chdir_test: directory was not changed to root.\n");
    return 1;
  }

  return 0;
}

static int do_cwd_test(struct file_desc_environment *file_desc_env) {
  char buffer[512];
  struct inode_data test_dir_node;

  /* Create a dummy directory on the stack to test cwd. */
  init_inode_data(&test_dir_node);
  test_dir_node.mode = S_IFDIR;
  strncpy(test_dir_node.name, TEST_DIRECTORY, sizeof(test_dir_node.name));

  /* Change the current directory to the dummy test directory. */
  test_dir_node.parent_dir = file_desc_env->current_dir;
  file_desc_env->current_dir = &test_dir_node;

  if (buffer != getcwd(buffer, sizeof(buffer)) ||
      strcmp(buffer, "/" TEST_DIRECTORY) != 0) {
    irt_ext_test_print("do_cwd_test: getcwd returned unexpected dir: %s\n",
                       buffer);
    return 1;
  }

  return 0;
}

static int do_opendir_test(struct file_desc_environment *file_desc_env) {
  if (0 != mkdir(TEST_DIRECTORY, S_IRUSR | S_IWUSR)) {
    irt_ext_test_print("do_opendir_test: could not create directory - %s\n",
                       strerror(errno));
    return 1;
  }

  struct inode_data *parent_dir = NULL;
  struct inode_data *test_dir = find_inode_path(file_desc_env,
                                                "/" TEST_DIRECTORY,
                                                &parent_dir);
  if (test_dir == NULL) {
    irt_ext_test_print("do_opendir_test: dir was not successfully created.\n");
    return 1;
  }

  DIR *dir = opendir(TEST_DIRECTORY);
  if (NULL == dir) {
    irt_ext_test_print("do_opendir_test: opendir has failed - %s\n",
                       strerror(errno));
    return 1;
  }

  int dir_fd = dirfd(dir);
  if (dir_fd == -1) {
    irt_ext_test_print("do_opendir_test: dirfd has failed - %s\n",
                       strerror(errno));
    return 1;
  }

  if (dir_fd < 0 ||
      dir_fd >= NACL_ARRAY_SIZE(file_desc_env->file_descs) ||
      !file_desc_env->file_descs[dir_fd].valid) {
    irt_ext_test_print("do_opendir_test: directory fd is invalid (%d).\n",
                       dir_fd);
    return 1;
  }

  if (0 != closedir(dir)) {
    irt_ext_test_print("do_opendir_test: closedir failed - %s\n",
                       strerror(errno));
    return 1;
  }

  if (file_desc_env->file_descs[dir_fd].valid) {
    irt_ext_test_print("do_opendir_test: closed directory fd is valid.\n");
    return 1;
  }

  return 0;
}

static int do_fchdir_test(struct file_desc_environment *file_desc_env) {
  if (0 != mkdir(TEST_DIRECTORY, S_IRUSR | S_IWUSR)) {
    irt_ext_test_print("do_fchdir_test: could not create directory - %s\n",
                       strerror(errno));
    return 1;
  }

  DIR *dir = opendir(TEST_DIRECTORY);
  if (NULL == dir) {
    irt_ext_test_print("do_fchdir_test: opendir has failed - %s\n",
                       strerror(errno));
    return 1;
  }

  int dir_fd = dirfd(dir);
  if (dir_fd == -1) {
    irt_ext_test_print("do_fchdir_test: dirfd has failed - %s\n",
                       strerror(errno));
    return 1;
  }

  if (0 != fchdir(dir_fd)) {
    irt_ext_test_print("do_fchdir_test: fchdir has failed - %s\n",
                       strerror(errno));
    return 1;
  }

  if (strcmp(file_desc_env->current_dir->name, TEST_DIRECTORY) != 0) {
    irt_ext_test_print("do_fchdir_test: directory change failed.\n");
    return 1;
  }

  return 0;
}

static int do_readdir_test(struct file_desc_environment *file_desc_env) {
  if (0 != mkdir(TEST_DIRECTORY, S_IRUSR | S_IWUSR)) {
    irt_ext_test_print("do_readdir_test: could not create directory - %s\n",
                       strerror(errno));
    return 1;
  }

  FILE *fp = fopen(TEST_DIRECTORY "/" TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_readdir_test: fopen 1 failed - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (0 != fclose(fp)) {
    irt_ext_test_print("do_readdir_test: fclose 1 failed - %s.\n",
                       strerror(errno));
    return 1;
  }

  fp = fopen(TEST_DIRECTORY "/" TEST_FILE2, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_readdir_test: fopen 2 failed - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (0 != fclose(fp)) {
    irt_ext_test_print("do_readdir_test: fclose 2 failed - %s.\n",
                       strerror(errno));
    return 1;
  }

  DIR *dir = opendir(TEST_DIRECTORY);
  if (NULL == dir) {
    irt_ext_test_print("do_readdir_test: opendir has failed - %s\n",
                       strerror(errno));
    return 1;
  }

  /*
   * A real readdir will include the contents as well as "." and ".." entries,
   * our own test harness only returns the list of files contained within the
   * directory. This strengthens our test that these results are coming from
   * our test harness.
   */
  int items_read = 0;
  errno = 0;
  for (struct dirent *dp = readdir(dir); dp != NULL; dp = readdir(dir)) {
    items_read++;
    if (strcmp(dp->d_name, TEST_FILE) != 0 &&
        strcmp(dp->d_name, TEST_FILE2) != 0) {
      irt_ext_test_print("do_readdir_test: unexpected directory listing - %s\n",
                         dp->d_name);
      return 1;
    }
  }

  if (errno != 0) {
    irt_ext_test_print("do_readdir_test: readdir has failed - %s\n",
                       strerror(errno));
    return 1;
  }

  if (items_read != 2) {
    irt_ext_test_print("do_readdir_test: unexpected number of items read.\n"
                       "  Expected 2 items, retrieved %d.\n",
                       items_read);
    return 1;
  }

  return 0;
}

/* File IO tests. */
static int do_fopenclose_test(struct file_desc_environment *file_desc_env) {
  FILE *fp = NULL;
  int fd = -1;
  struct inode_data *file_node_parent = NULL;
  struct inode_data *file_node = NULL;

  fp = fopen(TEST_FILE, "w");
  if (fp == NULL) {
    irt_ext_test_print("do_fopenclose_test: fopen was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  file_node = find_inode_path(file_desc_env, TEST_FILE, &file_node_parent);
  if (file_node == NULL) {
    irt_ext_test_print("do_fopenclose_test: did not create inode.\n");
    return 1;
  }

  fd = fileno(fp);
  if (fd < 0 ||
      fd >= NACL_ARRAY_SIZE(file_desc_env->file_descs) ||
      !file_desc_env->file_descs[fd].valid ||
      file_desc_env->file_descs[fd].data != file_node) {
    irt_ext_test_print("do_fopenclose_test: file descriptor (%d) invalid.\n",
                       fd);
    return 1;
  }

  if (0 != fclose(fp)) {
    irt_ext_test_print("do_fopenclose_test: fclose was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (file_desc_env->file_descs[fd].valid) {
    irt_ext_test_print("do_fopenclose_test: did not close file descriptor.\n");
    return 1;
  }

  return 0;
}

static int do_fwriteread_test(struct file_desc_environment *file_desc_env) {
  char buffer[512];
  size_t num_bytes;
  FILE *fp = NULL;
  struct inode_data *file_node_parent = NULL;
  struct inode_data *file_node = NULL;

  fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_fwriteread_test: fopen was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  file_node = find_inode_path(file_desc_env, TEST_FILE, &file_node_parent);
  if (file_node == NULL) {
    irt_ext_test_print("do_fwriteread_test: did not create inode.\n");
    return 1;
  }

  num_bytes = fwrite(TEST_TEXT, sizeof(TEST_TEXT[0]), sizeof(TEST_TEXT), fp);
  if (num_bytes != sizeof(TEST_TEXT)) {
    irt_ext_test_print("do_fwriteread_test: fwrite was not successful.\n");
    return 1;
  }

  if (0 != fseek(fp, 0, SEEK_SET)) {
    irt_ext_test_print("do_fwriteread_test: fseek was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  num_bytes = fread(buffer, sizeof(TEST_TEXT[0]), sizeof(TEST_TEXT), fp);
  if (num_bytes != sizeof(TEST_TEXT)) {
    irt_ext_test_print("do_fwriteread_test: fread was not successful.\n");
    return 1;
  }

  if (strcmp(buffer, TEST_TEXT) != 0) {
    irt_ext_test_print("do_fwriteread_test: read/write text does not match.\n");
    return 1;
  }

  if (strcmp(file_node->content, TEST_TEXT) != 0) {
    irt_ext_test_print("do_fwriteread_test: inode content does not match.\n");
    return 1;
  }

  return 0;
}

static int do_truncate_test(struct file_desc_environment *file_desc_env) {
  FILE *fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_truncate_test: fopen was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  struct inode_data *file_node_parent = NULL;
  struct inode_data *file_node = find_inode_path(file_desc_env, TEST_FILE,
                                                 &file_node_parent);
  if (file_node == NULL) {
    irt_ext_test_print("do_truncate_test: did not create inode.\n");
    return 1;
  }

  size_t num_bytes = fwrite(TEST_TEXT, sizeof(TEST_TEXT[0]),
                            sizeof(TEST_TEXT), fp);
  if (num_bytes != sizeof(TEST_TEXT)) {
    irt_ext_test_print("do_truncate_test: fwrite was not successful - %s.\n");
    return 1;
  }

  if (0 != truncate(TEST_FILE, num_bytes - 1)) {
    irt_ext_test_print("do_truncate_test: truncate was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (file_node->size != (num_bytes - 1)) {
    irt_ext_test_print("do_truncate_test: truncate did not affect env.\n");
    return 1;
  }

  if (0 != ftruncate(fileno(fp), num_bytes - 2)) {
    irt_ext_test_print("do_truncate_test: ftruncate was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (file_node->size != (num_bytes - 2)) {
    irt_ext_test_print("do_truncate_test: ftruncate did not affect env.\n");
    return 1;
  }

  return 0;
}

/* Standard stream tests. */
static int do_isatty_test(struct file_desc_environment *file_desc_env) {
  if (!isatty(STDIN_FILENO) ||
      !isatty(STDOUT_FILENO) ||
      !isatty(STDERR_FILENO)) {
    irt_ext_test_print("do_istty_test: not all standard streams are a tty.\n");
    return 1;
  }

  deactivate_file_desc_env();
  if (isatty(STDIN_FILENO) ||
      isatty(STDOUT_FILENO) ||
      isatty(STDERR_FILENO)) {
    irt_ext_test_print("do_istty_test: valid tty after deactivating env.\n");
    return 1;
  }

  return 0;
}

static int do_printf_stream_test(struct file_desc_environment *file_desc_env) {
  const struct inode_data *stdout_data =
      file_desc_env->file_descs[STDOUT_FILENO].data;

  printf(TEST_TEXT);
  fflush(stdout);
  if (strncmp(stdout_data->content, TEST_TEXT,
              NACL_ARRAY_SIZE(TEST_TEXT) - 1) != 0) {
    irt_ext_test_print("do_printf_test: printf did not output to test env.\n");
    return 1;
  }

  return 0;
}

static int do_fprintf_stream_test(struct file_desc_environment *file_desc_env) {
  const struct inode_data *stdout_data =
      file_desc_env->file_descs[STDOUT_FILENO].data;
  const struct inode_data *stderr_data =
      file_desc_env->file_descs[STDERR_FILENO].data;

  fprintf(stdout, TEST_TEXT);
  fflush(stdout);
  if (strncmp(stdout_data->content, TEST_TEXT,
              NACL_ARRAY_SIZE(TEST_TEXT) - 1) != 0) {
    irt_ext_test_print("do_fprintf_stream_test: fprintf(stdout) did not output"
                       " to test env.\n");
    return 1;
  }

  fprintf(stderr, TEST_TEXT);
  if (strncmp(stderr_data->content, TEST_TEXT,
              NACL_ARRAY_SIZE(TEST_TEXT) - 1) != 0) {
    irt_ext_test_print("do_fprintf_stream_test: fprintf(stderr) did not output"
                       " to test env.\n");
    return 1;
  }

  return 0;
}

static int do_fread_stream_test(struct file_desc_environment *file_desc_env) {
  struct inode_data *stdin_data = file_desc_env->file_descs[STDIN_FILENO].data;
  char buffer[512];

  strcpy(stdin_data->content, TEST_TEXT);
  stdin_data->size = NACL_ARRAY_SIZE(TEST_TEXT);

  fread(buffer, sizeof(buffer[0]), sizeof(buffer), stdin);

  if (strcmp(buffer, TEST_TEXT) != 0) {
    irt_ext_test_print("do_fread_stream_test: fread(stdin) did not match"
                       " expected test text.\n");
    return 1;
  }

  return 0;
}

/* File stat tests. */
static int do_stat_test(struct file_desc_environment *file_desc_env) {
  file_desc_env->current_time = TEST_TIME_VALUE;

  FILE *fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_stat_test: fopen was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  struct stat stat_result;
  if (0 != stat(TEST_FILE, &stat_result)) {
    irt_ext_test_print("do_stat_test: stat was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (stat_result.st_ctime != TEST_TIME_VALUE) {
    irt_ext_test_print("do_stat_test: stat creation time not expected value:\n"
                       "  Expected value: %d. Returned value: %d.\n",
                       TEST_TIME_VALUE, (int) stat_result.st_ctime);
    return 1;
  }

  struct stat fstat_result;
  if (0 != fstat(fileno(fp), &fstat_result)) {
    irt_ext_test_print("do_stat_test: fstat was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (fstat_result.st_ctime != TEST_TIME_VALUE) {
    irt_ext_test_print("do_stat_test: fstat creation time not expected value:\n"
                       "  Expected value: %d. Returned value: %d.\n",
                       TEST_TIME_VALUE, (int) fstat_result.st_ctime);
    return 1;
  }

  if (file_desc_env->current_time <= TEST_TIME_VALUE) {
    irt_ext_test_print("do_stat_test: file env time was not touched.\n");
    return 1;
  }

  return 0;
}

static int do_chmod_test(struct file_desc_environment *file_desc_env) {
  FILE *fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_chmod_test: fopen was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  struct inode_data *file_node_parent = NULL;
  struct inode_data *file_node = find_inode_path(file_desc_env, TEST_FILE,
                                                 &file_node_parent);
  if (file_node == NULL) {
    irt_ext_test_print("do_chmod_test: did not create inode.\n");
    return 1;
  }

  if (0 == (file_node->mode & S_IRWXU)) {
    irt_ext_test_print("do_chmod_test: created inode mode is 0.\n");
    return 1;
  }

  mode_t original_mode = file_node->mode;
  if (0 != chmod(TEST_FILE, 0)) {
    irt_ext_test_print("do_chmod_test: chmod was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (0 != (file_node->mode & S_IRWXU)) {
    irt_ext_test_print("do_chmod_test: chmod did not modify file inode.\n");
    return 1;
  }

  file_node->mode = original_mode;
  if (0 != fchmod(fileno(fp), 0)) {
    irt_ext_test_print("do_chmod_test: fchmod was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (0 != (file_node->mode & S_IRWXU)) {
    irt_ext_test_print("do_chmod_test: fchmod did not modify file inode.\n");
    return 1;
  }

  return 0;
}

static int do_access_test(struct file_desc_environment *file_desc_env) {
  FILE *fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_access_test: fopen was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  struct inode_data *file_node_parent = NULL;
  struct inode_data *file_node = find_inode_path(file_desc_env, TEST_FILE,
                                                 &file_node_parent);
  if (file_node == NULL) {
    irt_ext_test_print("do_access_test: did not create inode.\n");
    return 1;
  }

  file_node->mode &= ~S_IXUSR;
  if (0 == access(TEST_FILE, X_OK)) {
    irt_ext_test_print("do_access_test: access executable incorrect.\n");
    return 1;
  }

  file_node->mode |= S_IXUSR;
  if (0 != access(TEST_FILE, X_OK)) {
    irt_ext_test_print("do_access_test: access not executable incorrect.\n");
    return 1;
  }

  return 0;
}

static int do_utimes_test(struct file_desc_environment *file_desc_env) {
  file_desc_env->current_time = TEST_TIME_VALUE;

  FILE *fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_utimes_test: fopen was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  struct inode_data *file_node_parent = NULL;
  struct inode_data *file_node = find_inode_path(file_desc_env, TEST_FILE,
                                                 &file_node_parent);
  if (file_node == NULL) {
    irt_ext_test_print("do_utimes_test: did not create inode.\n");
    return 1;
  }

  if (file_node->atime != TEST_TIME_VALUE ||
      file_node->mtime != TEST_TIME_VALUE) {
    irt_ext_test_print("do_utimes_test: inode has unexpected time stats:\n"
                       "  Expected time: %d\n"
                       "  atime: %d\n"
                       "  mtime: %d\n",
                       TEST_TIME_VALUE,
                       (int) file_node->atime,
                       (int) file_node->mtime);
    return 1;
  }

  if (file_desc_env->current_time <= TEST_TIME_VALUE) {
    irt_ext_test_print("do_utimes_test: file env time was not touched.\n");
    return 1;
  }

  struct timeval times[2];
  memset(times, sizeof(times), 0);
  times[0].tv_sec = file_desc_env->current_time;
  times[1].tv_sec = file_desc_env->current_time;

  if (0 != utimes(TEST_FILE, times)) {
    irt_ext_test_print("do_utimes_test: utimes was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (file_node->atime != file_desc_env->current_time ||
      file_node->mtime != file_desc_env->current_time) {
    irt_ext_test_print("do_utimes_test: file data was not updated correctly:\n"
                       "  Expected time: %d\n"
                       "  atime: %d\n"
                       "  mtime: %d\n",
                       (int) file_desc_env->current_time,
                       (int) file_node->atime,
                       (int) file_node->mtime);
    return 1;
  }

  return 0;
}

/* Rename test. */
static int do_rename_test(struct file_desc_environment *file_desc_env) {
  FILE *fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_rename_test: fopen was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }
  fclose(fp);

  if (0 != rename(TEST_FILE, TEST_FILE2)) {
    irt_ext_test_print("do_rename_test: rename was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  struct inode_data *file_node_parent = NULL;
  struct inode_data *file_node = find_inode_path(file_desc_env, TEST_FILE,
                                                 &file_node_parent);
  if (file_node != NULL) {
    irt_ext_test_print("do_rename_test: old file still exists.\n");
    return 1;
  }

  file_node = find_inode_path(file_desc_env, TEST_FILE2,
                              &file_node_parent);
  if (file_node == NULL) {
    irt_ext_test_print("do_rename_test: renamed file does not exists.\n");
    return 1;
  }

  return 0;
}

/* Link tests. */
static int do_link_test(struct file_desc_environment *file_desc_env) {
  FILE *fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_link_test: fopen was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  struct inode_data *file_node_parent = NULL;
  struct inode_data *file_node = find_inode_path(file_desc_env, TEST_FILE,
                                                 &file_node_parent);
  if (file_node == NULL) {
    irt_ext_test_print("do_link_test: did not create inode.\n");
    return 1;
  }

  if(0 != link(TEST_FILE, TEST_FILE2)) {
    irt_ext_test_print("do_link_test: link was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  struct inode_data *link_node_parent = NULL;
  struct inode_data *link_node = find_inode_path(file_desc_env, TEST_FILE2,
                                                 &link_node_parent);
  if (link_node == NULL) {
    irt_ext_test_print("do_link_test: did not create link inode.\n");
    return 1;
  }

  if (link_node->link != file_node) {
    irt_ext_test_print("do_link_test: link was not established.\n");
    return 1;
  }

  if (S_ISLNK(link_node->mode)) {
    irt_ext_test_print("do_link_test: hard link was marked as symlink.\n");
    return 1;
  }

  return 0;
}

static int do_symlink_test(struct file_desc_environment *file_desc_env) {
  FILE *fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_symlink_test: fopen was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  struct inode_data *file_node_parent = NULL;
  struct inode_data *file_node = find_inode_path(file_desc_env, TEST_FILE,
                                                 &file_node_parent);
  if (file_node == NULL) {
    irt_ext_test_print("do_symlink_test: did not create inode.\n");
    return 1;
  }

  if(0 != symlink(TEST_FILE, TEST_FILE2)) {
    irt_ext_test_print("do_symlink_test: symlink was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  struct inode_data *link_node_parent = NULL;
  struct inode_data *link_node = find_inode_path(file_desc_env, TEST_FILE2,
                                                 &link_node_parent);
  if (link_node == NULL) {
    irt_ext_test_print("do_symlink_test: did not create symlink inode.\n");
    return 1;
  }

  if (link_node->link != file_node) {
    irt_ext_test_print("do_symlink_test: symlink was not established.\n");
    return 1;
  }

  if (!S_ISLNK(link_node->mode)) {
    irt_ext_test_print("do_symlink_test: symlink was marked as hard link.\n");
    return 1;
  }

  return 0;
}

static int do_lstat_test(struct file_desc_environment *file_desc_env) {
  file_desc_env->current_time = TEST_TIME_VALUE;

  FILE *fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_lstat_test: fopen was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  if(0 != symlink(TEST_FILE, TEST_FILE2)) {
    irt_ext_test_print("do_lstat_test: symlink was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  struct inode_data *link_node_parent = NULL;
  struct inode_data *link_node = find_inode_path(file_desc_env, TEST_FILE2,
                                                 &link_node_parent);
  if (link_node == NULL) {
    irt_ext_test_print("do_lstat_test: did not create symlink inode.\n");
    return 1;
  }

  struct stat stat_result;
  if (0 != lstat(TEST_FILE2, &stat_result)) {
    irt_ext_test_print("do_lstat_test: lstat was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (!S_ISLNK(stat_result.st_mode)) {
    irt_ext_test_print("do_lstat_test: lstat did not retrieve symlink stat.\n");
    return 1;
  }

  if (stat_result.st_ctime == TEST_TIME_VALUE) {
    irt_ext_test_print("do_lstat_test: lstat returned original file time.\n");
    return 1;
  }

  if (stat_result.st_ctime != link_node->ctime) {
    irt_ext_test_print("do_lstat_test: lstat did not return link file time.\n"
                       "  Expected time: %d\n"
                       "  Retrieved time: %d\n",
                       (int) link_node->ctime,
                       (int) stat_result.st_ctime);
    return 1;
  }

  return 0;
}

static int do_readlink_test(struct file_desc_environment *file_desc_env) {
  FILE *fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_readlink_test: fopen was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  if(0 != symlink(TEST_FILE, TEST_FILE2)) {
    irt_ext_test_print("do_readlink_test: symlink was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  char buffer[32] = {'\0'};
  ssize_t len = readlink(TEST_FILE2, buffer, sizeof(buffer));
  if (len < 0) {
    irt_ext_test_print("do_readlink_test: readlink was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (strcmp(buffer, "/" TEST_FILE) != 0) {
    irt_ext_test_print("do_readlink_test: readlink returned unexpected value:\n"
                       "  Expected: /" TEST_FILE "\n"
                       "  Retrieved: %s\n",
                       buffer);
    return 1;
  }

  return 0;
}

static int do_unlink_test(struct file_desc_environment *file_desc_env) {
  FILE *fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_unlink_test: fopen was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }
  fclose(fp);

  if (0 != unlink(TEST_FILE)) {
    irt_ext_test_print("do_unlink_test: unlink was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  struct inode_data *file_node_parent = NULL;
  struct inode_data *file_node = find_inode_path(file_desc_env,
                                                 TEST_FILE, &file_node_parent);
  if (file_node != NULL) {
    irt_ext_test_print("do_unlink_test: inode was not deleted.\n");
    return 1;
  }

  return 0;
}

static int do_remove_test(struct file_desc_environment *file_desc_env) {
  FILE *fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_remove_test: fopen was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }
  fclose(fp);

  if (0 != remove(TEST_FILE)) {
    irt_ext_test_print("do_remove_test: file remove was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  struct inode_data *file_node_parent = NULL;
  struct inode_data *file_node = find_inode_path(file_desc_env,
                                                 TEST_DIRECTORY "/" TEST_FILE,
                                                 &file_node_parent);
  if (file_node != NULL) {
    irt_ext_test_print("do_remove_test: did not delete file.\n");
    return 1;
  }

  return 0;
}

/* Duplicate file descriptor tests. */
static int do_dup_test(struct file_desc_environment *file_desc_env) {
  FILE *fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_dup_test: fopen was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  int old_fd = fileno(fp);
  if (old_fd < 0 ||
      old_fd >= NACL_ARRAY_SIZE(file_desc_env->file_descs) ||
      !file_desc_env->file_descs[old_fd].valid) {
    irt_ext_test_print("do_dup_test: created invalid file descriptor: %d\n",
                       old_fd);
    return 1;
  }

  int dup_fd = dup(old_fd);
  if (dup_fd == -1) {
    irt_ext_test_print("do_dup_test: dup was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (dup_fd < 0 ||
      dup_fd >= NACL_ARRAY_SIZE(file_desc_env->file_descs) ||
      !file_desc_env->file_descs[dup_fd].valid) {
    irt_ext_test_print("do_dup_test: duplicated invalid file descriptor: %d\n",
                       dup_fd);
    return 1;
  }

  const struct file_descriptor *fd_table = file_desc_env->file_descs;
  if (fd_table[old_fd].data != fd_table[dup_fd].data) {
    irt_ext_test_print("do_dup_test: file descriptors do not match.\n");
    return 1;
  }

  return 0;
}

static int do_dup2_new_test(struct file_desc_environment *file_desc_env) {
  FILE *fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_dup2_new_test: fopen was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  int old_fd = fileno(fp);
  if (old_fd < 0 ||
      old_fd >= NACL_ARRAY_SIZE(file_desc_env->file_descs) ||
      !file_desc_env->file_descs[old_fd].valid) {
    irt_ext_test_print("do_dup2_new_test: created invalid fd: %d\n",
                       old_fd);
    return 1;
  }

  /* Test dup2 using an unused file descriptor. */
  int test_fd = old_fd + 1;
  if (test_fd < 0 ||
      test_fd >= NACL_ARRAY_SIZE(file_desc_env->file_descs) ||
      file_desc_env->file_descs[test_fd].valid) {
    irt_ext_test_print("do_dup2_new_test: invalid test file descriptor: %d\n",
                       test_fd);
    return 1;
  }

  if (test_fd != dup2(old_fd, test_fd)) {
    irt_ext_test_print("do_dup2_new_test: dup2 was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (!file_desc_env->file_descs[test_fd].valid) {
    irt_ext_test_print("do_dup2_new_test: dup file descriptor not valid.\n");
    return 1;
  }

  const struct file_descriptor *fd_table = file_desc_env->file_descs;
  if (fd_table[old_fd].data != fd_table[test_fd].data) {
    irt_ext_test_print("do_dup2_new_test: file descriptors do not match.\n");
    return 1;
  }

  return 0;
}

static int do_dup2_used_test(struct file_desc_environment *file_desc_env) {
  FILE *fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_dup2_used_test: fopen 1 was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  int old_fd = fileno(fp);
  if (old_fd < 0 ||
      old_fd >= NACL_ARRAY_SIZE(file_desc_env->file_descs) ||
      !file_desc_env->file_descs[old_fd].valid) {
    irt_ext_test_print("do_dup2_used_test: created invalid fd 1: %d\n",
                       old_fd);
    return 1;
  }

  /* Test dup2 using an open file descriptor. */
  FILE *fp2 = fopen(TEST_FILE2, "w+");
  if (fp2 == NULL) {
    irt_ext_test_print("do_dup2_used_test: fopen 2 was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  int new_fd = fileno(fp2);
  if (new_fd < 0 ||
      new_fd >= NACL_ARRAY_SIZE(file_desc_env->file_descs) ||
      !file_desc_env->file_descs[new_fd].valid) {
    irt_ext_test_print("do_dup2_used_test: created invalid fd 2: %d\n",
                       new_fd);
    return 1;
  }

  const struct file_descriptor *fd_table = file_desc_env->file_descs;
  if (fd_table[old_fd].data == fd_table[new_fd].data) {
    irt_ext_test_print("do_dup2_used_test: file descriptors already match.\n");
    return 1;
  }

  if (new_fd != dup2(old_fd, new_fd)) {
    irt_ext_test_print("do_dup2_used_test: dup2 was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (!file_desc_env->file_descs[new_fd].valid) {
    irt_ext_test_print("do_dup2_used_test: new file descriptor not valid.\n");
    return 1;
  }

  if (fd_table[old_fd].data != fd_table[new_fd].data) {
    irt_ext_test_print("do_dup2_used_test: file descriptors do not match.\n");
    return 1;
  }

  return 0;
}

/* File sync tests. */
static int do_fsync_test(struct file_desc_environment *file_desc_env) {
  FILE *fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_fsync_test: fopen was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  const int fd = fileno(fp);
  if (fd < 0 ||
      fd >= NACL_ARRAY_SIZE(file_desc_env->file_descs) ||
      !file_desc_env->file_descs[fd].valid) {
    irt_ext_test_print("do_fsync_test: created invalid fd: %d\n",
                       fd);
    return 1;
  }

  if (file_desc_env->file_descs[fd].fsync ||
      file_desc_env->file_descs[fd].fdatasync) {
    irt_ext_test_print("do_fsync_test: file desc not initialized properly.\n");
    return 1;
  }

  if (0 != fsync(fd)) {
    irt_ext_test_print("do_fsync_test: fsync was not successful - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (!file_desc_env->file_descs[fd].fsync ||
      !file_desc_env->file_descs[fd].fdatasync) {
    irt_ext_test_print("do_fsync_test: fsync did not modify file desc.\n");
    return 1;
  }

  return 0;
}

static int do_fdatasync_test(struct file_desc_environment *file_desc_env) {
  FILE *fp = fopen(TEST_FILE, "w+");
  if (fp == NULL) {
    irt_ext_test_print("do_fdatasync_test: fopen failed - %s.\n",
                       strerror(errno));
    return 1;
  }

  const int fd = fileno(fp);
  if (fd < 0 ||
      fd >= NACL_ARRAY_SIZE(file_desc_env->file_descs) ||
      !file_desc_env->file_descs[fd].valid) {
    irt_ext_test_print("do_fdatasync_test: created invalid fd: %d\n",
                       fd);
    return 1;
  }

  if (file_desc_env->file_descs[fd].fdatasync) {
    irt_ext_test_print("do_fdatasync_test: descriptor not initialized.\n");
    return 1;
  }

  if (0 != fdatasync(fd)) {
    irt_ext_test_print("do_fdatasync_test: fdatasync failed - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (!file_desc_env->file_descs[fd].fdatasync) {
    irt_ext_test_print("do_fdatasync_test: fdatasync did not modify file"
                       " descriptor.\n");
    return 1;
  }

  return 0;
}

/*
 * These tests should not be in alphabetical order but ordered by complexity,
 * simpler tests should break first. For example, changing to a directory
 * depends on being able to make a directory first, so the make directory test
 * should be run first.
 */
static const TYPE_file_test g_file_tests[] = {
  /* Directory tests. */
  do_mkdir_rmdir_test,
  do_chdir_test,
  do_cwd_test,
  do_opendir_test,
  do_fchdir_test,
  do_readdir_test,

  /* File IO tests. */
  do_fopenclose_test,
  do_fwriteread_test,
  do_truncate_test,

  /* Standard stream tests. */
  do_isatty_test,
  do_printf_stream_test,
  do_fprintf_stream_test,
  do_fread_stream_test,

  /* File stat tests. */
  do_stat_test,
  do_chmod_test,
  do_access_test,
  do_utimes_test,

  /* Rename test. */
  do_rename_test,

  /* Link tests. */
  do_link_test,
  do_symlink_test,
  do_lstat_test,
  do_readlink_test,
  do_unlink_test,
  do_remove_test,

  /* Duplicate file descriptor tests. */
  do_dup_test,
  do_dup2_new_test,
  do_dup2_used_test,

  /* File sync tests. */
  do_fsync_test,
  do_fdatasync_test,
};

static void setup(struct file_desc_environment *file_desc_env) {
  init_file_desc_environment(file_desc_env);
  activate_file_desc_env(file_desc_env);
}

static void teardown(void) {
  deactivate_file_desc_env();
}

DEFINE_TEST(File, g_file_tests, struct file_desc_environment, setup, teardown)
