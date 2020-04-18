# Adapted from 
# shfuncs : test suite common shell functions
# which was shipped with the TET example code.

. "$XDG_TEST_DIR/include/testfuncs.sh"

## NOTE: Documentation is generated AUTOMATICALLY from this file
## Function usage must immediately follow function delcaration

assert_exit() {
# execute command (saving output) and check exit code
# Usage: assert_exit N command ...
# where N is a number or a literal 'N' (for non-zero)
# command can be an unquoted string, but be careful.
    EXPECT="$1"
    shift 1

    # make sure nothing is hanging around from a prev. test.
    rm -f out.stdout out.stderr

    # $1 is command, $2 is expected exit code (0 or "N" for non-zero)
    ( "$@" > out.stdout 2> out.stderr )
    CODE="$?"

    LASTCOMMAND="$*"

    if [ -z "$EXPECT" ]; then
	EXPECT=0;
    fi
    if [ "$EXPECT" = N -a "$CODE" -eq 0 ]; then
	test_fail "Command ($*) gave exit code $CODE, expected nonzero"
    elif [ "$EXPECT" != N ] && [ "$EXPECT" -ne "$CODE" ]; then
	test_fail "Command ($*) gave exit code $CODE, expected $EXPECT"
    fi
}

assert_interactive_notroot() {
	if [ `whoami` != 'root' ] ; then
		assert_interactive "$@"
	fi
}

assert_interactive() {
# Useage:
# assert_interactive {msg} [y|n|C|s varname]
#
# msg is the text to print.
# y -> expect y for [y/n]
# n -> expect n for [y/n]
# s -> save y or n into varname. Then, (presumably) $varname can be 
#      given in place of y or n in subsequent calls to assert_interactive
# C -> cleanup msg. Always print "msg [enter to continue]" despite test failure.
# if no argument is given after msg, print "msg [enter to continue]"

	query=$1
	expect=$2
# It seems valuable to see what happens even if the test has failed. 
# (eg fail on stdout.)
	if [ "$TEST_STATUS" = 'FAIL' -a "$expect" != C ] ; then
		## Don't waste user's time if test has already failed.
		test_infoline "Test has already failed. Not bothering to ask '$query'" 
		return
	fi

	if [ ! -z "$XDG_TEST_NO_INTERACTIVE" ] ; then
		test_infoline "Assumed '$query' is '$expect'"
	 	return
	fi

	if [  ! -z "$expect" -a "$expect" != C ] ; then
		if [ "$expect" != y -a "$expect" != n -a "$expect" != s -a "$expect" != C ] ; then
			echo "TEST SYNTAX ERROR: interactive assertions require one of (y,n,s,C) as choices. (found '$expect')" >&2
			exit 255
		fi
		unset result
		while [ "$result" != y -a "$result" != n ] ; do 
			echo -ne "\n\t$query [y/n]: " >&2
			read result
		done

		if [ "$expect" = s ] ; then
			if [ -z "$3" ] ; then
				echo "TEST SYNTAX ERROR: 's' requires a variable name"
				exit 255
			fi
			eval "$3=$result"
		elif [ "$result" != "$expect" ] ; then
			test_fail "User indicated '$result' instead of '$expect' in response to '$query'"
		fi
	else
		echo -ne "\n\t$query [enter to continue] " >&2
		read result
	fi
}
		

assert_file_in_path() {
# Assert that some file is present in a path.
# 
# Usage:
# assert_file_in_path FILENAME PATH
# where FILE is the exact name of the file and 
# PATH is a ':' separated list of directories
	search_dirs=`echo "$2" | tr ':' ' '`
	found_files=`find $search_dirs -name "$1" 2>/dev/null`

	if [ -z "$found_files" ] ; then
		test_fail "Did not find '$1' in '$2'"
	fi
}

assert_file_not_in_path() {
# Assert the some file is NOT present in a path.
# Opposite of 'assert_file_in_path'
	search_dirs=`echo "$2" | tr ':' ' '`
	found_files=`find $search_dirs -name "$1" 2>/dev/null`

	if [ ! -z "$found_files" ] ; then
		test_fail "Found '$found_files' in $2"
	fi
}


assert_file() {
# Assert the existance of an exact filename
# Usage: assert_file FILE
	if [ ! -e "$1" ] ; then
		test_fail "'$1' does not exist"
		return 
	elif [ ! -f "$1" ] ; then
		test_fail "'$1' is not a regular file"
		return
	fi
	if [ -f "$2" ] ; then
		compare=`diff -wB "$1" "$2"`
		if [ ! -z "$compare" ] ; then
			test_fail "'$1' is different from '$2'. Diff is:\n$compare"
		fi
	fi
}

assert_nofile() {
# Assert the non existance of an exact filename.
# Opposite of 'assert_file'
	if [ -e "$1" ] ; then
		test_fail "'$1' exists."
	fi
}


assert_nostdout() {
# assert nothing was written to a stdout.
# NOTE: Failing this assertion will WARN rather than FAIL
    if [ -s out.stdout ]
    then
	test_infoline "Unexpected output from '$LASTCOMMAND' written to stdout, as shown below:"
	infofile out.stdout stdout:
	if [ "$TEST_STATUS" = "PASS" ]; then
		test_status WARN
	fi
    fi
}

assert_nostderr() {
# assert nothing was written to stderr.
# NOTE: Failing this assertion will WARN rather than FAIL
    if [ -n "$XDG_UTILS_DEBUG_LEVEL" ] ; then
        if [ -s out.stderr ] ; then
	    infofile out.stderr debug:
	fi
    elif [ -s out.stderr ] ; then
	test_infoline "Unexpected output from '$LASTCOMMAND' written to stderr, as shown below:"
	infofile out.stderr stderr:
	if [ "$TEST_STATUS" = "PASS" ]; then
		test_status WARN
	fi
    fi
}

assert_stderr() {
# check that stderr matches expected error
# $1 is file containing regexp for expected error
# if no argument supplied, just check out.stderr is not empty

	if [ ! -s out.stderr ]
	then
	    test_infoline "Expected output from '$LASTCOMMAND' to stderr, but none written"
	    test_fail
	    return
	fi
    if [ ! -z "$1" ] ; then
	expfile="$1"
	OK=Y
	exec 4<&0 0< "$expfile" 3< out.stderr
	while read expline
	do
	    if read line <&3
	    then
		if expr "$line" : "$expline" > /dev/null
		then
		    :
		else
		    OK=N
		    break
		fi
	    else
		OK=N
	    fi
	done
	exec 0<&4 3<&- 4<&-
	if [ "$OK" = N ]
	then
	    test_infoline "Incorrect output from '$LASTCOMMAND' written to stderr, as shown below"
	    infofile "$expfile" "expected stderr:"
	    infofile out.stderr "received stderr:"
	    test_fail
	fi
    fi 
}

assert_stdout() {
# check that stderr matches expected error
# $1 is file containing regexp for expected error
# if no argument supplied, just check out.stderr is not empty

	if [ ! -s out.stdout ]
	then
	    test_infoline "Expected output from '$LASTCOMMAND' to stdout, but none written"
	    test_fail
	    return
	fi
    if [ ! -z "$1" ] ; then 
	expfile="$1"

	if [ ! -e "$expfile" ] ; then
		test_status NORESULT "Could not find file '$expfile' to look up expected pattern!"
		return
	fi
	OK=Y
	exec 4<&0 0< "$expfile" 3< out.stdout
	while read expline
	do
	    if read line <&3
	    then
		if expr "$line" : "$expline" > /dev/null
		then
		    :
		else
		    OK=N
		    break
		fi
	    else
		OK=N
	    fi
	done
	exec 0<&4 3<&- 4<&-
	if [ "$OK" = N ]
	then
	    test_infoline "Incorrect output from '$LASTCOMMAND' written to stdout, as shown below"
	    infofile "$expfile" "expected stdout:"
	    infofile out.stdout "received stdout:"
	    test_fail
	fi
    fi
}

require_interactive() {
# if $XDG_TEST_NO_INTERACTIVE is set, test result becomes UNTESTED
    if [ ! -z "$XDG_TEST_NO_INTERACTIVE" ] ; then
	test_result UNTESTED "XDG_TEST_NO_INTERACTIVE is set, but this test needs interactive"
    fi
}

require_root() {
# if the test is not being run as root, test result is UNTESTED
    if [ `whoami` != 'root' ] ; then
	test_result UNTESTED "not running as root, but test requires root privileges"
    fi
}

require_notroot() {
# if the test is being run as root, the test result is UNTESTED
# opposite of 'require_root'
    if [ `whoami` = 'root' ] ; then
	test_result UNTESTED "running as root, but test must be run as a normal user"
    fi
}

set_no_display() {
# Clear $DISPLAY
	unset DISPLAY
}

assert_display() {
# Assert that the $DISPLAY variable is set.
	if [ -z "$DISPLAY" ] ; then 
		test_fail "DISPLAY not set!"
	fi
}

assert_util_var() {
# Assert that the $XDGUTIL varilable is set.
# DEPRICATED. Only used by generic tests.
if [ "x$XDGUTIL" = x ]; then
	test_fail "XDGUTIL variable not set"
fi
}

use_file() {
# Copy a datafile from it's defult location into the test directory
# Usage:
# use_file ORIG_FILE VAR
# Where ORIG_FILE is the name of the file, and VAR is the name of the
# variable to create that will contain the new (unique) filename.
# DO NOT put a '$' in front of VAR. VAR will be created via eval.
#
# $VAR will be set to 'xdgtestdata-$XDG_TEST_ID-$file' after the 
# directory is stripped from $file.
	src="$1"
	file=${src##/*/}
	varname="$2"

	if [ $# -lt 2 ] ; then
			echo "TEST SYNTAX ERROR: use_file must have two arguments" >&2
		exit 255 
	fi

	assert_file "$src"

	outfile="xdgtestdata-$XDG_TEST_ID-$file"
	eval "$varname=$outfile"
	
	cp "$src" "$XDG_TEST_TMPDIR/$outfile"
}

get_unique_name() {
# Get a unique name for a file, similar to 'use_file'
# except that no file is copied. You are left to create file $VAR.
	varname="$1"
	file="$2"
	if [ -z "$varname" ] ; then
		echo "TEST SYNAX ERROR: get_unique_name requries a variable name"
		exit 255
	fi

	outfile="xdgtestdata-$XDG_TEST_ID-$file"
	eval "$varname=$outfile"
}

edit_file() {
# Edit file via sed. 
# Usage:
# edit_file $FILE origstr VARNAME [newstr]
# Where:
# $FILE    is a file, probably copied from 'use_file' 
# VARNAME  is created via 'eval' to contain the newstr
# newstr   is the optional substitution. If newstr is not present,
#          it will become 'xdgtestdata-$XDG_TEST_ID-$origstr'
	file="$1"
	origstr="$2"
	varname="$3"
	newstr="$4"

	if [ $# -lt 3 ] ; then
		echo "TEST SYNTAX ERROR: edit_file must have at least 3 arguments."
		exit 255
	fi
	
	assert_file "$file"

	if [ -z "$newstr" ] ; then
		newstr="xdgtestdata-$XDG_TEST_ID-$origstr"
	fi

	eval "$varname=\"$newstr\""

	sed -i -e "s|$origstr|$newstr|g" "$file"
}

