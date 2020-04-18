#!/bin/sh -e

if [ "x$SCANNER" = "x" ] ; then
	echo "No scanner present, test skipped." 1>&2
	exit 77
fi

$SCANNER client-header $1 /dev/null
$SCANNER server-header $1 /dev/null
$SCANNER code $1 /dev/null
