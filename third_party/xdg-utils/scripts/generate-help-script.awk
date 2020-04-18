#!/usr/bin/awk -f
#---------------------------------------------
#
#   generate-script.awk
#
#   Simple AWK script to generate the XDG scripts, substituting the
#   necessary text from other source files.
#
#   Copyright 2006, Benedikt Meurer <benny@xfce.org>
#
#   LICENSE:
#
#   Permission is hereby granted, free of charge, to any person obtaining a
#   copy of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom the
#   Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
#   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
#   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#   OTHER DEALINGS IN THE SOFTWARE.
#
#---------------------------------------------


# All lines from the input file should be printed
{
	print
}


# The text from ../LICENSE should be inserted after
# the "#   LICENSE:" line
/^#   LICENSE:/ {
	while (getline < "../LICENSE")
		print
	close ("../LICENSE")
}


# Insert the examples text from the .txt file
# after the "cat << _MANUALPAGE" line
/^cat << _MANUALPAGE/ {
	# determine the name of the .txt file
	txtfile = FILENAME
	sub(/\.in$/, ".txt", txtfile)

	# read the .txt file content
	for (txtfile_print = 0; getline < txtfile; ) {
#		if (match ($0, /^Examples/) != 0) {
#			# print everything starting at the "Examples" line
#			txtfile_print = 1
#		}
#		if (txtfile_print != 0) {
#			print $0
#		}
                gsub("`","'")
                gsub("—","-")
                print $0
	}
	close (txtfile)
}


# Insert the usage text from the .txt file
# after the "cat << _USAGE" line
/^cat << _USAGE/ {
	# determine the name of the .txt file
	txtfile = FILENAME
	sub(/\.in$/, ".txt", txtfile)

	# read the .txt file content
	for (txtfile_print = 0; getline < txtfile; ) {
		if (match ($0, /^Name/) != 0) {
			# skip empty line after "Name"
			getline < txtfile

			# from now on, print everything
			txtfile_print = 1
		}
		else if (match ($0, /^Description/) != 0) {
			# stop at "Description"
			break
		}
		else if (txtfile_print != 0) {
	                gsub("—","-")
			print $0
		}
	}
	close (txtfile)
}


# Insert the xdg-utils-common.in content after
# the "#@xdg-utils-common@" line
/^#@xdg-utils-common@/ {
	while (getline < "xdg-utils-common.in")
		print
	close ("xdg-utils-common.in")
}


