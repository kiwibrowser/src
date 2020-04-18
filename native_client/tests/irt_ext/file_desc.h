/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This module is not intended to emulate a full fledged file system and
 * terminal, but simply attempting to implement the minimal amount of
 * functionality so that a standard library can be tested to show that the
 * supplied functions for file access calls through irt_extension are
 * called correctly. One big advantage of this is that the module does not need
 * to account for every edge case, only the calls within the unit tests for
 * irt_ext will call these functions.
 *
 * Once the module is initialized, all file system calls will interact with
 * the currently activated file_desc_environment. Environments are meant to be
 * activated and deactivated for each test that gets run. This makes each test
 * run begin with a clean state.
 *
 * One of the simplifications this module makes is sub-directories are not
 * allowed, directories cannot exist in other directories. Another
 * simplification is that only the owner permission bits are set, the unit
 * tests should only test and set these sets of flags (IE S_IRWXU).
 *
 * This code also does not try to be thread-safe although real implementations
 * will need to be. Because these tests are only testing if the functions are
 * hooked into the standard libraries properly, it is expected that all the
 * tests will run on a single thread.
 *
 * In order to reuse the logic in creating and opening files, many of the
 * file path functions simply create a file descriptor using the internal open()
 * function. In a real file system implementation this would not be acceptable
 * since many of the functions must work when the file descriptor table is
 * already full, but since this is just a test harness we are accepting this
 * simplification.
 */

#ifndef NATIVE_CLIENT_TESTS_IRT_EXT_FILE_DESC_H
#define NATIVE_CLIENT_TESTS_IRT_EXT_FILE_DESC_H

#include <stdbool.h>
#include <sys/stat.h>

#define MAX_DIRS_PER_DIR 8
#define MAX_FILES_PER_DIR 16
#define MAX_INODES 32
#define MAX_FILE_DESCS 32
#define NAME_SIZE 64
#define BUFFER_SIZE 128

struct inode_data {
  bool valid;

  /* Check S_IFDIR for directories. */
  mode_t mode;
  off_t position;
  off_t size;
  time_t atime;
  time_t mtime;
  time_t ctime;

  /*
   * Symbolic Links will have S_IFLNK be set along with link.
   * Hard links will just have link set.
   */
  struct inode_data *link;
  struct inode_data *parent_dir;

  char name[NAME_SIZE];
  char content[BUFFER_SIZE];
};

struct file_descriptor {
  bool valid;
  bool fsync;
  bool fdatasync;
  int oflag;
  int dir_position;
  struct inode_data *data;
};

struct file_desc_environment {
  time_t current_time;
  struct inode_data *current_dir;
  struct inode_data inode_datas[MAX_INODES];
  struct file_descriptor file_descs[MAX_FILE_DESCS];
};

void init_file_desc_module(void);

void init_inode_data(struct inode_data *inode_data);
void init_file_descriptor(struct file_descriptor *file_desc);
void init_file_desc_environment(struct file_desc_environment *env);

struct inode_data *find_inode_name(struct file_desc_environment *env,
                                   struct inode_data *parent_dir,
                                   const char *name, size_t name_len);

struct inode_data *find_inode_path(struct file_desc_environment *env,
                                   const char *path,
                                   struct inode_data **parent_dir);

void activate_file_desc_env(struct file_desc_environment *env);
void deactivate_file_desc_env(void);

#endif /* NATIVE_CLIENT_TESTS_IRT_EXT_FILE_DESC_H */
