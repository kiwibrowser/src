#!/bin/bash

# This file is part of Wireless Display Software for Linux OS
#
# Copyright (C) 2016 Intel Corporation.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301 USA

BASEDIR=$(dirname $0)

xterm -geometry 80x25 -e "$1 -i $BASEDIR/seed/header/ -x $BASEDIR/dict/wfd_header.dict -o $2_header -T header $BASEDIR/wdsfuzzer --num-lines 6 --header" &
xterm -geometry 80x25 -e "$1 -i $BASEDIR/seed/payload_reply/ -x $BASEDIR/dict/wfd_payload_reply.dict -o $2_payload_reply -T payload_reply $BASEDIR/wdsfuzzer --num-lines 8 --payload-reply" &
xterm -geometry 80x25 -e "$1 -i $BASEDIR/seed/payload_request/ -x $BASEDIR/dict/wfd_payload_request.dict -o $2_payload_request -T payload_request $BASEDIR/wdsfuzzer --num-lines 8 --payload-request" &
xterm -geometry 80x25 -e "$1 -i $BASEDIR/seed/payload_error/ -x $BASEDIR/dict/wfd_payload_error.dict -o $2_payload_error -T payload_error $BASEDIR/wdsfuzzer --num-lines 8 --payload-error" &
