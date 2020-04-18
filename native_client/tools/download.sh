#!/bin/bash
# Copyright 2008 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


set -o nounset
set -o errexit

######################################################################
# Helper functions
######################################################################

ReadKey() {
  read
}

Banner() {
  echo "######################################################################"
  echo $*
  echo "######################################################################"
}

Download() {
  Banner "downloading $1"
  if which wget ; then
    wget $1 -O $2
  elif which curl ; then
    curl --url $1 -o $2
  else
    echo
    echo "Problem encountered"
    echo
    echo "Please install curl or wget and rerun this script"
    echo "or manually download $1 to $2"
    echo
    echo "press any key when done"
    ReadKey
  fi

  if [ ! -s $2 ] ; then
    echo "ERROR: could not find $2"
    exit -1
  fi
}


if [ $# -ne 2 ] ; then
  echo "usage: download.sh <url> <localfilename>"
  exit -1
fi



Download $1 $2
