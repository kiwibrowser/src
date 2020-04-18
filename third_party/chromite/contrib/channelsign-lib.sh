#!/bin/bash
##############################################################################
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
##############################################################################
# BASE ENV
##############################################################################
TOOLS=${TOOLS:-$(cd "$(dirname -- "$0")" && pwd)}
PATH="${TOOLS}:${HOME}/depot_tools:/usr/local/google/gsutil:${PATH}"
OVERLAY_DIR="/usr/local/google/dashboards/chromiumos-overlay"
export PATH TOOLS

umask 0022

if [ -t 1 ]; then
  # Set some video text attributes for use in error/warning msgs.
  V_REVERSE='[7m'
  V_UNDERLINE='[4m'
  V_BOLD='[1m'
  [ -f /usr/bin/tput ] && V_BLINK=$(tput blink)
  V_VIDOFF='[m'
fi
export V_REVERSE V_UNDERLINE V_BOLD V_BLINK V_VIDOFF

# Set some usable highlighted keywords for functions like OkFailed().
OK="${V_BOLD}OK${V_VIDOFF}"
FAILED="${V_REVERSE}FAILED${V_VIDOFF}"
WARNING="${V_REVERSE}WARNING${V_VIDOFF}"
YES="${V_BOLD}YES${V_VIDOFF}"
NO="${V_REVERSE}NO${V_VIDOFF}"

# Set REALUSER
#export REALUSER=$(id -nu)

# Set a PID for use throughout.
export PID=$$

# Save original cmd-line.
ORIG_CMDLINE="$*"

# Define some functions when sourced from PROGrams.
#[ -n "${PROG}" ] && . stdlib.sh

# Set a basic trap to capture ^C's and other unexpected exits and do the
# right thing in TrapClean().
trap TrapClean 2 3 15

##############################################################################
# Standard library of useful shell functions and GLOBALS.
##############################################################################

#  TimeStamp  < begin | end | done > [ section ]
#+ DESCRIPTION
#+     TimeStamp begin is run at the beginning of the script to display a
#+     'begin' marker and TimeStamp end is run at the end of the script to
#+     display an 'end' marker.
#+     TimeStamp will auto-scope to the current shell or you can optionally
#+     pass in a second argument as a section identifier if you wish to track
#+     multiple times within the same shell.
#+
#+     For example, if you call TimeStamp begin and end within one script and then
#+     call it again in another script (new shell), you can just use begin and end
#+     But if you want to call it twice within the same script use it like this:
#+
TimeStamp() {
  # Always set trace (set -x) back
  #set +x
  #
  local action=$1
  local section=${2:-run}
  # convert / to underscore
  section=${section//\//_}
  # convert : to underscore
  section=${section//:/_}
  # convert . to underscore
  section=${section//./_}
  local start_var="${section}start_seconds"
  local end_var="${section}end_seconds"

  case ${action} in
  begin)
    # Get time(date) for display and calc.
    eval ${start_var}=$(date '+%s')

    # Print BEGIN message for $PROG.
    echo "BEGIN ${section} on ${HOSTNAME%%.*} $(date)"
    [[ ${section} == "run" ]] && echo
    ;;
  end|done)
    # Check for "START" values before calcing.
    if [[ -z ${!start_var} ]]; then
      #display_time="EE:EE:EE - 'end' run without 'begin' in this scope or sourced script using TimeStamp"
      return 1
    fi

    # Get time(date) for display and calc.
    eval ${end_var}=$(date '+%s')

    local elapsed=$(( ${!end_var} - ${!start_var} ))
    local d=$(( elapsed / 86400 ))
    local h=$(( (elapsed % 86400) / 3600 ))
    local m=$(( (elapsed % 3600) / 60 ))
    local s=$(( elapsed % 60 ))
    [ "$d" -gt 0 ] && local prettyd="${d}d"
    [ "$h" -gt 0 ] && local prettyh="${h}h"
    [ "$m" -gt 0 ] && local prettym="${m}m"
    [ "$s" -gt 0 ] && local prettys="${s}s"
    local pretty="$prettyd$prettyh$prettym$prettys"

    [[ ${section} == "run" ]] && echo
    echo "${PROG}: DONE ${section} on ${HOSTNAME%%.*} $(date) in ${pretty}"

    # NULL these local vars now that we've displayed things.
    unset ${start_var} ${end_var}
    ;;
  esac
}

TrapClean() {
  # If user ^C's at read then tty is hosed, so make it sane again.
  stty sane
  echo
  echo
  echo "^C caught!"
  Exit 1 "Exiting..."
}

CleanExit() {
  # cleanup CLEANEXIT_RM.
  # Sanity check the list.
  # Should we test that each arg is a absolute path?
  if echo "${CLEANEXIT_RM}" | fgrep -qw /; then
    echo "/ found in CLEANEXIT_RM.  Skipping Cleanup..."
  else
    if [[ -n ${CLEANEXIT_RM} ]]; then
      echo "Cleaning up: ${CLEANEXIT_RM}"
   else
      : echo "Cleaning up..."
   fi
      # cd somewhere relatively safe and run cleanup the $CLEANEXIT_RM stack
      # + some usual suspects...
      cd /tmp && rm -rf ${CLEANEXIT_RM} ${TMPFILE1} ${TMPFILE2} ${TMPFILE3} \
                                        ${TMPDIR1}  ${TMPDIR2}  ${TMPDIR3}
  fi
  # Display end timestamp when an existing TimeStamp begin was run.
  [[ -n ${runstart_seconds} ]] && TimeStamp end
  exit ${1:-0}
}

# Requires 2 args
# - Exit code
# - message
Exit() {
  local etype=${1:-0}
  shift
  echo
  echo "$@"
  CleanExit ${etype}
}

# Output misc debug related information -- format not meant for machines!
DumpRunTimeEnv() {
  local info=(
    "PROG=${PROG}"
    "USER=${USER:-$(id)}"
    "HOSTNAME=${HOSTNAME:-$(hostname)}"
    "GIT_REV=${CROSTOOLS_GIT_REV}"
  )
  printf '%s\n' "${info[@]}"
}

# Returns a full path to gsutil or 1 if nothing found.
FindGSUTIL() {
  local lf
  for lf in ${CROSTOOLS_GSUTIL} /b/build/third_party/gsutil/gsutil \
            /home/chromeos-re/gsutil/gsutil gsutil; do
    type -P ${lf} && return 0
  done
  return 1
}

# It creates a temporary file to expand the environment variables.
ManHelp() {
  # Whatever caller is.
  local lprog=$0
  local ltmpfile="/tmp/${PROG}-manhelp.$$"

  [[ ${usage}    == "yes" ]] && set -- -usage
  [[ ${man}      == "yes" ]] && set -- -man
  [[ ${comments} == "yes" ]] && set -- -comments

  case $1 in
  # Standard usage function
  -usage|--usage|"-?")
    (
    echo 'cat << EOFCAT'
    echo "Usage:"
    sed -n '/#+ SYNOPSIS/,/^#+ DESCRIPTION/p' "${lprog}" | \
      sed -e 's,^#+ ,,g' -e 's,^#+$,,g' -e '/^DESCRIPTION/d'
    echo 'EOFCAT'
    ) > "${ltmpfile}"
    . "${ltmpfile}"
    rm -f "${ltmpfile}"
    exit 1
    ;;
  # Standard man function
  -man|--man|-h|-help|--help)
    (
    echo 'cat << EOFCAT'
    grep "^#+" "${lprog}" | cut -c4-
    echo 'EOFCAT'
    ) > "${ltmpfile}"
    . "${ltmpfile}" | ${PAGE:-"less"}
    rm -f "${ltmpfile}"
    exit 1
    ;;
  # Standard comments function
  -comments)
    echo
    egrep "^#\-" "${lprog}" | sed -e 's,^#- *,,g'
    exit 1
    ;;
  esac
}

# parse cmdline
namevalue() {
  local arg namex valuex
  for arg in "$@"; do
    case $arg in
    *=*) # Strip off any leading -*
         arg=$(echo $arg |sed 's,^-*,,g')
         # "set" any arguments that have = in them
         namex=${arg%%=*}
         # change -'s to _ for legal vars in bash
         namex=${namex//-/_}
         valuex=${arg#*=}
         eval export $namex=\""$valuex"\"
         ;;
      *) LOOSEARGS="$LOOSEARGS $arg"
         ;;
    esac
  done
}
namevalue "$@"

# Run ManHelp to show usage and man pages
ManHelp "$@"

# Keep crostools up to date
# symlink may be setting by caller or use $0
# Try it using std git pull or repo sync
if ! ${CROSTOOLS_SKIP_SYNC:-false}; then
  CROSTOOLS_GIT_REV=$(
    cd "$(dirname "${symlink:-$0}")" || exit 0
    git pull -q >&/dev/null || repo sync -q . >&/dev/null
    git rev-parse HEAD
  )
fi
