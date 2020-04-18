#!/bin/bash
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script copies all dependencies required for trace collection.
# Usage:
#   deploy.sh builddir gcs_path
#
# Where:
#   builddir is the build directory for Chrome
#   gcs_path is the Google Storage bucket under which the deployment is
#   installed

builddir=$1
tmpdir=`mktemp -d`
deployment_gcs_path=$2/deployment

# Extract needed sources.
src_suffix=src
tmp_src_dir=$tmpdir/$src_suffix

# Copy files from tools/android/loading.
mkdir -p $tmp_src_dir/tools/android/loading/cloud
cp -r tools/android/loading/cloud/backend \
  $tmp_src_dir/tools/android/loading/cloud/
cp -r tools/android/loading/cloud/common \
  $tmp_src_dir/tools/android/loading/cloud/
cp tools/android/loading/*.py $tmp_src_dir/tools/android/loading
cp tools/android/loading/cloud/*.py $tmp_src_dir/tools/android/loading/cloud

# Copy other dependencies.
mkdir $tmp_src_dir/third_party
rsync -av --exclude=".*" --exclude "*.pyc" --exclude "*.html" --exclude "*.md" \
  third_party/catapult $tmp_src_dir/third_party
mkdir $tmp_src_dir/tools/perf
cp -r tools/perf/chrome_telemetry_build $tmp_src_dir/tools/perf
mkdir -p $tmp_src_dir/build/android
cp build/android/devil_chromium.py $tmp_src_dir/build/android/
cp build/android/video_recorder.py $tmp_src_dir/build/android/
cp build/android/devil_chromium.json $tmp_src_dir/build/android/
cp -r build/android/pylib $tmp_src_dir/build/android/
mkdir -p \
  $tmp_src_dir/third_party/blink/renderer/devtools/front_end/emulated_devices
cp third_party/blink/renderer/devtools/front_end/emulated_devices/module.json \
  $tmp_src_dir/third_party/blink/renderer/devtools/front_end/emulated_devices/

# Tar up the source and copy it to Google Cloud Storage.
source_tarball=$tmpdir/source.tgz
tar -cvzf $source_tarball -C $tmpdir $src_suffix
gsutil cp $source_tarball gs://$deployment_gcs_path/source/

# Copy the chrome executable to Google Cloud Storage.
chrome/tools/build/make_zip.py $builddir chrome/tools/build/linux/FILES.cfg \
  $tmpdir/linux.zip
gsutil cp $tmpdir/linux.zip gs://$deployment_gcs_path/binaries/linux.zip

# Copy the startup script uncompressed so that it can be executed.
gsutil cp tools/android/loading/cloud/backend/startup-script.sh \
  gs://$deployment_gcs_path/

# Generate and upload metadata about this deployment.
CHROMIUM_REV=$(git merge-base HEAD origin/master)
cat >$tmpdir/build_metadata.json << EOF
{
  "chromium_rev": "$CHROMIUM_REV"
}
EOF
gsutil cp $tmpdir/build_metadata.json \
  gs://$deployment_gcs_path/deployment_metadata.json
rm -rf $tmpdir
