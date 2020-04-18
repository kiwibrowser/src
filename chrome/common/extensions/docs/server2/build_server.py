#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script is used to copy all dependencies into the local directory.
# The package of files can then be uploaded to App Engine.
import os
import shutil
import stat
import sys

SRC_DIR = os.path.join(sys.path[0], os.pardir, os.pardir, os.pardir, os.pardir,
    os.pardir)
THIRD_PARTY_DIR = os.path.join(SRC_DIR, 'third_party')
LOCAL_THIRD_PARTY_DIR = os.path.join(sys.path[0], 'third_party')
TOOLS_DIR = os.path.join(SRC_DIR, 'tools')
SCHEMA_COMPILER_FILES = ['memoize.py',
                         'model.py',
                         'idl_schema.py',
                         'schema_util.py',
                         'json_parse.py',
                         'json_schema.py']

def MakeInit(path):
  path = os.path.join(path, '__init__.py')
  with open(os.path.join(path), 'w') as f:
    os.utime(os.path.join(path), None)

def OnError(function, path, excinfo):
  os.chmod(path, stat.S_IWUSR)
  function(path)

def CopyThirdParty(src, dest, files=None, make_init=True):
  dest_path = os.path.join(LOCAL_THIRD_PARTY_DIR, dest)
  if not files:
    shutil.copytree(src, dest_path)
    if make_init:
      MakeInit(dest_path)
    return
  try:
    os.makedirs(dest_path)
  except Exception:
    pass
  if make_init:
    MakeInit(dest_path)
  for filename in files:
    shutil.copy(os.path.join(src, filename), os.path.join(dest_path, filename))

def main():
  if os.path.isdir(LOCAL_THIRD_PARTY_DIR):
    try:
      shutil.rmtree(LOCAL_THIRD_PARTY_DIR, False, OnError)
    except OSError:
      print('*-------------------------------------------------------------*\n'
            '| If you are receiving an upload error, try removing          |\n'
            '| chrome/common/extensions/docs/server2/third_party manually. |\n'
            '*-------------------------------------------------------------*\n')


  CopyThirdParty(os.path.join(THIRD_PARTY_DIR, 'motemplate'), 'motemplate')
  CopyThirdParty(os.path.join(THIRD_PARTY_DIR, 'markdown'), 'markdown',
                 make_init=False)
  CopyThirdParty(os.path.join(SRC_DIR, 'ppapi', 'generators'),
                 'json_schema_compiler')
  CopyThirdParty(os.path.join(THIRD_PARTY_DIR, 'ply'),
                 os.path.join('json_schema_compiler', 'ply'))
  CopyThirdParty(os.path.join(TOOLS_DIR, 'json_schema_compiler'),
                 'json_schema_compiler',
                 SCHEMA_COMPILER_FILES)
  CopyThirdParty(os.path.join(TOOLS_DIR, 'json_comment_eater'),
                 'json_schema_compiler',
                 ['json_comment_eater.py'])
  CopyThirdParty(os.path.join(THIRD_PARTY_DIR, 'simplejson'),
                 os.path.join('json_schema_compiler', 'simplejson'),
                 make_init=False)
  MakeInit(LOCAL_THIRD_PARTY_DIR)

  CopyThirdParty(os.path.join(THIRD_PARTY_DIR, 'google_appengine_cloudstorage',
                 'cloudstorage'), 'cloudstorage')

  # To be able to use the Motemplate class we need this import in __init__.py.
  with open(os.path.join(LOCAL_THIRD_PARTY_DIR,
                         'motemplate',
                         '__init__.py'), 'a') as f:
    f.write('from motemplate import Motemplate\n')

if __name__ == '__main__':
  main()
