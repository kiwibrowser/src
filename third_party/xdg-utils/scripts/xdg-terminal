#!/bin/sh
#---------------------------------------------
#   xdg-terminal
#
#   Utility script to open the registered terminal emulator
#
#   Refer to the usage() function below for usage.
#
#   Copyright 2009-2010, Fathi Boudra <fabo@freedesktop.org>
#   Copyright 2009-2010, Rex Dieter <rdieter@fedoraproject.org>
#   Copyright 2006, Kevin Krammer <kevin.krammer@gmx.at>
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

manualpage()
{
cat << _MANUALPAGE
Name

xdg-terminal - opens the user's preferred terminal emulator application

Synopsis

xdg-terminal [command]

xdg-terminal { --help | --manual | --version }

Description

xdg-terminal opens the user's preferred terminal emulator application. If a
command is provided the command will be executed by the shell within the newly
opened terminal window.

xdg-terminal is for use inside a desktop session only. It is not recommended to
use xdg-terminal as root.

Options

--help
    Show command synopsis.
--manual
    Show this manualpage.
--version
    Show the xdg-utils version information.

Exit Codes

An exit code of 0 indicates success while a non-zero exit code indicates
failure. The following failure codes can be returned:

1
    Error in command line syntax.
3
    A required tool could not be found.
4
    The action failed.

Examples

xdg-terminal

Opens the user's default terminal emulator, just starting an interactive shell.

xdg-terminal top

Opens the user's default terminal emulator and lets it run the top executable.

_MANUALPAGE
}

usage()
{
cat << _USAGE
xdg-terminal - opens the user's preferred terminal emulator application

Synopsis

xdg-terminal [command]

xdg-terminal { --help | --manual | --version }

_USAGE
}

#@xdg-utils-common@

#----------------------------------------------------------------------------
#   Common utility functions included in all XDG wrapper scripts
#----------------------------------------------------------------------------

DEBUG()
{
  [ -z "${XDG_UTILS_DEBUG_LEVEL}" ] && return 0;
  [ ${XDG_UTILS_DEBUG_LEVEL} -lt $1 ] && return 0;
  shift
  echo "$@" >&2
}

#-------------------------------------------------------------
# Exit script on successfully completing the desired operation

exit_success()
{
    if [ $# -gt 0 ]; then
        echo "$@"
        echo
    fi

    exit 0
}


#-----------------------------------------
# Exit script on malformed arguments, not enough arguments
# or missing required option.
# prints usage information

exit_failure_syntax()
{
    if [ $# -gt 0 ]; then
        echo "xdg-terminal: $@" >&2
        echo "Try 'xdg-terminal --help' for more information." >&2
    else
        usage
        echo "Use 'man xdg-terminal' or 'xdg-terminal --manual' for additional info."
    fi

    exit 1
}

#-------------------------------------------------------------
# Exit script on missing file specified on command line

exit_failure_file_missing()
{
    if [ $# -gt 0 ]; then
        echo "xdg-terminal: $@" >&2
    fi

    exit 2
}

#-------------------------------------------------------------
# Exit script on failure to locate necessary tool applications

exit_failure_operation_impossible()
{
    if [ $# -gt 0 ]; then
        echo "xdg-terminal: $@" >&2
    fi

    exit 3
}

#-------------------------------------------------------------
# Exit script on failure returned by a tool application

exit_failure_operation_failed()
{
    if [ $# -gt 0 ]; then
        echo "xdg-terminal: $@" >&2
    fi

    exit 4
}

#------------------------------------------------------------
# Exit script on insufficient permission to read a specified file

exit_failure_file_permission_read()
{
    if [ $# -gt 0 ]; then
        echo "xdg-terminal: $@" >&2
    fi

    exit 5
}

#------------------------------------------------------------
# Exit script on insufficient permission to write a specified file

exit_failure_file_permission_write()
{
    if [ $# -gt 0 ]; then
        echo "xdg-terminal: $@" >&2
    fi

    exit 6
}

check_input_file()
{
    if [ ! -e "$1" ]; then
        exit_failure_file_missing "file '$1' does not exist"
    fi
    if [ ! -r "$1" ]; then
        exit_failure_file_permission_read "no permission to read file '$1'"
    fi
}

check_vendor_prefix()
{
    file_label="$2"
    [ -n "$file_label" ] || file_label="filename"
    file=`basename "$1"`
    case "$file" in
       [a-zA-Z]*-*)
         return
         ;;
    esac

    echo "xdg-terminal: $file_label '$file' does not have a proper vendor prefix" >&2
    echo 'A vendor prefix consists of alpha characters ([a-zA-Z]) and is terminated' >&2
    echo 'with a dash ("-"). An example '"$file_label"' is '"'example-$file'" >&2
    echo "Use --novendor to override or 'xdg-terminal --manual' for additional info." >&2
    exit 1
}

check_output_file()
{
    # if the file exists, check if it is writeable
    # if it does not exists, check if we are allowed to write on the directory
    if [ -e "$1" ]; then
        if [ ! -w "$1" ]; then
            exit_failure_file_permission_write "no permission to write to file '$1'"
        fi
    else
        DIR=`dirname "$1"`
        if [ ! -w "$DIR" -o ! -x "$DIR" ]; then
            exit_failure_file_permission_write "no permission to create file '$1'"
        fi
    fi
}

#----------------------------------------
# Checks for shared commands, e.g. --help

check_common_commands()
{
    while [ $# -gt 0 ] ; do
        parm="$1"
        shift

        case "$parm" in
            --help)
            usage
            echo "Use 'man xdg-terminal' or 'xdg-terminal --manual' for additional info."
            exit_success
            ;;

            --manual)
            manualpage
            exit_success
            ;;

            --version)
            echo "xdg-terminal 1.0.2"
            exit_success
            ;;
        esac
    done
}

check_common_commands "$@"

[ -z "${XDG_UTILS_DEBUG_LEVEL}" ] && unset XDG_UTILS_DEBUG_LEVEL;
if [ ${XDG_UTILS_DEBUG_LEVEL-0} -lt 1 ]; then
    # Be silent
    xdg_redirect_output=" > /dev/null 2> /dev/null"
else
    # All output to stderr
    xdg_redirect_output=" >&2"
fi

#--------------------------------------
# Checks for known desktop environments
# set variable DE to the desktop environments name, lowercase

detectDE()
{
    if [ x"$KDE_FULL_SESSION" = x"true" ]; then DE=kde;
    elif [ x"$GNOME_DESKTOP_SESSION_ID" != x"" ]; then DE=gnome;
    elif `dbus-send --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.GetNameOwner string:org.gnome.SessionManager > /dev/null 2>&1` ; then DE=gnome;
    elif xprop -root _DT_SAVE_MODE 2> /dev/null | grep ' = \"xfce4\"$' >/dev/null 2>&1; then DE=xfce;
    elif [ x"$DESKTOP_SESSION" == x"LXDE" ]; then DE=lxde;
    else DE=""
    fi
}

#----------------------------------------------------------------------------
# kfmclient exec/openURL can give bogus exit value in KDE <= 3.5.4
# It also always returns 1 in KDE 3.4 and earlier
# Simply return 0 in such case

kfmclient_fix_exit_code()
{
    version=`kde${KDE_SESSION_VERSION}-config --version 2>/dev/null | grep '^KDE'`
    major=`echo $version | sed 's/KDE.*: \([0-9]\).*/\1/'`
    minor=`echo $version | sed 's/KDE.*: [0-9]*\.\([0-9]\).*/\1/'`
    release=`echo $version | sed 's/KDE.*: [0-9]*\.[0-9]*\.\([0-9]\).*/\1/'`
    test "$major" -gt 3 && return $1
    test "$minor" -gt 5 && return $1
    test "$release" -gt 4 && return $1
    return 0
}

terminal_kde()
{
    terminal=`kreadconfig --file kdeglobals --group General --key TerminalApplication --default konsole`

    terminal_exec=`which $terminal 2>/dev/null`

    if [ -x "$terminal_exec" ]; then
        if [ x"$1" == x"" ]; then
            $terminal_exec
        else
            $terminal_exec -e "$1"
        fi

        if [ $? -eq 0 ]; then
            exit_success
        else
            exit_failure_operation_failed
        fi
    else
        exit_failure_operation_impossible "configured terminal program '$terminal' not found or not executable"
    fi
}

terminal_gnome()
{
    term_exec_key="/desktop/gnome/applications/terminal/exec"
    term_exec_arg_key="/desktop/gnome/applications/terminal/exec_arg"

    term_exec=`gconftool-2 --get ${term_exec_key}`
    term_exec_arg=`gconftool-2 --get ${term_exec_arg_key}`

    terminal_exec=`which $term_exec 2>/dev/null`

    if [ -x "$terminal_exec" ]; then
        if [ x"$1" == x"" ]; then
            $terminal_exec
        else
            if [ x"$term_exec_arg" == x"" ]; then
                $terminal_exec "$1"
            else
                $terminal_exec "$term_exec_arg" "$1"
            fi
        fi

        if [ $? -eq 0 ]; then
            exit_success
        else
            exit_failure_operation_failed
        fi
    else
        exit_failure_operation_impossible "configured terminal program '$term_exec' not found or not executable"
    fi
}

terminal_xfce()
{
    if [ x"$1" == x"" ]; then
        exo-open --launch TerminalEmulator
    else
        exo-open --launch TerminalEmulator "$1"
    fi

    if [ $? -eq 0 ]; then
        exit_success
    else
        exit_failure_operation_failed
    fi
}

terminal_generic()
{
    # if $TERM is not set, try xterm
    if [ x"$TERM" == x"" ]; then
        TERM=xterm
    fi

    terminal_exec=`which $TERM >/dev/null 2>/dev/null`

    if [ -x "$terminal_exec" ]; then
        if [ $? -eq 0 ]; then
            exit_success
        else
            exit_failure_operation_failed
        fi
    else
        exit_failure_operation_impossible "configured terminal program '$TERM' not found or not executable"
    fi
}

terminal_lxde()
{
    if which lxterminal &>/dev/null; then
        if [ x"$1" == x"" ]; then
            lxterminal
        else
            lxterminal -e "$1"
        fi
    else
        terminal_generic "$1"
    fi
}

#[ x"$1" != x"" ] || exit_failure_syntax

command=
while [ $# -gt 0 ] ; do
    parm="$1"
    shift

    case "$parm" in
      -*)
        exit_failure_syntax "unexpected option '$parm'"
        ;;

      *)
        if [ -n "$command" ] ; then
            exit_failure_syntax "unexpected argument '$parm'"
        fi
        command="$parm"
        ;;
    esac
done

detectDE

if [ x"$DE" = x"" ]; then
    # if TERM variable is not set, try xterm
    if [ x"$TERM" = x"" ]; then
        TERM=xterm
    fi
    DE=generic
fi

case "$DE" in
    kde)
    terminal_kde "$command"
    ;;

    gnome)
    terminal_gnome "$command"
    ;;

    xfce)
    terminal_xfce "$command"
    ;;

    lxde)
    terminal_lxde "$command"
    ;;

    generic)
    terminal_generic "$command"
    ;;

    *)
    exit_failure_operation_impossible "no terminal emulator available"
    ;;
esac
