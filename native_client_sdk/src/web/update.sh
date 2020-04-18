#!/bin/sh
for file in index.html manifest.html; do
  gsutil cp -a public-read $file gs://nativeclient-mirror/nacl/nacl_sdk
done
