# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import sys
import getopt
import os

here = os.path.realpath(__file__)
src_path = (os.path.normpath(os.path.join(here, '..', '..', '..')))
sys.path.append(os.path.normpath(os.path.join(src_path, '..', 'depot_tools')))

USAGE = 'The utility uploads .png files to ' \
        'chrome-os-oobe-ui-screenshot-testing Google Storage bucket.\n' \
        '-i:\n\tdirectory with .png files which have to be uploaded\n' \
        '-o (optional):\n\tdirectory to store generated .sha1 files. ' \
        'Is set to chrome/browser/chromeos/login/screenshot_testing' \
        '/golden_screenshots by default\n--help:\n\thelp'


import upload_to_google_storage
import download_from_google_storage

def upload(png_path):

  # Creating a list of files which need to be uploaded to Google Storage:
  # all .png files from the directory containing golden screenshots.
  target = []
  for file in os.listdir(png_path):
    if file.endswith('.png'):
      target.append(os.path.join(png_path, file))

  # Creating a standard gsutil object, assuming there are depot_tools
  # and everything related is set up already.
  gsutil_path = os.path.abspath(os.path.join(src_path, '..', 'depot_tools',
                                             'third_party', 'gsutil',
                                             'gsutil'))
  gsutil = download_from_google_storage.Gsutil(gsutil_path,
                                               boto_path=None,
                                               bypass_prodaccess=True)

  # URL of the bucket used for storing screenshots.
  bucket_url = 'gs://chrome-os-oobe-ui-screenshot-testing'

  # Uploading using the most simple way,
  # see depot_tools/upload_to_google_storage.py to have better understanding
  # of this False and 1 arguments.
  upload_to_google_storage.upload_to_google_storage(target, bucket_url, gsutil,
                                                    False, False, 1, False)

  print 'All images are uploaded to Google Storage.'

def move_sha1(from_path, to_path):
  from shutil import move
  for file in os.listdir(from_path):
    if (file.endswith('.sha1')):
      old_place = os.path.join(from_path, file)
      new_place = os.path.join(to_path, file)
      if not os.path.exists(os.path.dirname(new_place)):
        os.makedirs(os.path.dirname(new_place))
      move(old_place, new_place)

def main(argv):
  png_path = ''
  sha1_path = os.path.join(src_path,
                           'chrome', 'browser', 'chromeos', 'login',
                           'screenshot_testing', 'golden_screenshots')
  try:
    opts, args = getopt.getopt(argv,'i:o:', ['--help'])
  except getopt.GetoptError:
    print USAGE
    sys.exit(1)
  for opt, arg in opts:
    if opt == '--help':
      print USAGE
      sys.exit()
    elif opt == '-i':
      png_path = arg
    elif opt =='-o':
      sha1_path = arg

  if png_path == '':
    print USAGE
    sys.exit(1)

  png_path = os.path.abspath(png_path)
  sha1_path = os.path.abspath(sha1_path)

  upload(png_path)
  move_sha1(png_path, sha1_path)

  # TODO(elizavetai): Can this git stuff be done automatically?
  print 'Please add new .sha1 files from ' \
        + str(sha1_path) + \
        ' to git manually.'

if __name__ == "__main__":
 main(sys.argv[1:])