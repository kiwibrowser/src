#!/bin/bash
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Takes a list of URLs (infile), and runs analyse.py on them in  parallel on a
# device and on GCE, in a sychronized manner (the task is started on both
# platforms at the same time).

infile=$1
outdir=$2
instance_ip=$3
repeat_count=$4

for site in $(< $infile); do
 echo $site
 output_subdir=$(echo "$site"|tr "/:" "_")
 echo 'Start remote task'
 cat >urls.json << EOF
 {
  "urls" : [
    "$site"
  ],
  "repeat_count" : "$repeat_count",
  "emulate_device" : "Nexus 4"
 }
EOF

 while [ "$(curl http://$instance_ip:8080/status)" != "Idle" ]; do
   echo 'Waiting for instance to be ready, retry in 5s'
   sleep 5
 done
 curl -X POST -d @urls.json http://$instance_ip:8080/set_tasks

 echo 'Run on device'
 mkdir $outdir/$output_subdir
 for ((run=0;run<$repeat_count;++run)); do
   echo '****'  $run
   tools/android/loading/analyze.py log_requests \
      --devtools_port 9222 \
      --url $site \
      --output $outdir/${output_subdir}/${run}
   if [ $? -ne 0 ]; then
    echo "Analyze failed. Wait a bit for device to recover."
    sleep 3
   fi
 done
done
