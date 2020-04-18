#!/usr/bin/env python
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
""" Copyright (c) 2002-2008 LOGILAB S.A. (Paris, FRANCE).
http://www.logilab.fr/ -- mailto:contact@logilab.fr

Copyright (c) 2012 The Chromium Authors. All rights reserved.
"""
import os
import sys

# Add local modules to the search path.
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(
    __file__)), 'logilab'))

from pylint import lint

args = sys.argv[1:]

# Add support for a custom mode where arguments are fed line by line on
# stdin. This allows us to get around command line length limitations.
ARGS_ON_STDIN = '--args-on-stdin'
if ARGS_ON_STDIN in args:
  args = [arg for arg in args if arg != ARGS_ON_STDIN]
  args.extend(arg.strip() for arg in sys.stdin)

lint.Run(args)
