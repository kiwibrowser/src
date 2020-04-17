#!/bin/bash

# Copyright 2018 Google LLC
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     https://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#
# Render a trace using gnuplot into a png file.
#

if [ -z "$1" ]; then
  echo "Usage: time_sequence_gnuplot.sh path/to/trace output.png"
  exit 1
fi

if [ ! -f "$1" ]; then
  echo "File $1 does not exist or is not a file"
  exit 1
fi

if [ -z "$2" ]; then
  echo "Output file not specified"
  exit 1
fi

if [ ! -x "$QUIC_TRACE_CONV_BIN" ]; then
  QUIC_TRACE_CONV_BIN="$0.runfiles/com_google_quic_trace/tools/quic_trace_to_time_sequence_gnuplot"
fi

if [ ! -x "$QUIC_TRACE_CONV_BIN" ]; then
  echo "Cannot find conversion tool binary"
  exit 1
fi

TMPDIR="$(mktemp -d)"
for event_type in send ack loss; do
  "$QUIC_TRACE_CONV_BIN" --sequence=$event_type < "$1" > "$TMPDIR/$event_type.txt"
done

if [ -z "$GNUPLOT_SIZE" ]; then
  GNUPLOT_SIZE="7680,4320"
fi
if [ -z "$GNUPLOT_TERMINAL" ]; then
  GNUPLOT_TERMINAL="png size $GNUPLOT_SIZE"
fi


gnuplot <<EOF
set terminal $GNUPLOT_TERMINAL
set output "$2"
plot \
   "$TMPDIR/send.txt" with lines lt rgb "blue" title "Sent", \
   "$TMPDIR/ack.txt" with lines lt rgb "green" title "Ack", \
   "$TMPDIR/loss.txt" with lines lt rgb "red" title "Loss"
EOF

rm -rf "$TMPDIR"
