#!/bin/sh

# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

NAME=org.chromium.chromoting
HOST_BUNDLE_NAME=@@HOST_BUNDLE_NAME@@
CONFIG_DIR=/Library/PrivilegedHelperTools
ENABLED_FILE=$CONFIG_DIR/$NAME.me2me_enabled
CONFIG_FILE=$CONFIG_DIR/$NAME.json
HOST_EXE=$CONFIG_DIR/$HOST_BUNDLE_NAME/Contents/MacOS/remoting_me2me_host
PLIST_FILE=$CONFIG_DIR/$HOST_BUNDLE_NAME/Contents/Info.plist

# The exit code returned by 'wait' when a process is terminated by SIGTERM.
SIGTERM_EXIT_CODE=143

# Range of exit codes returned by the host to indicate that a permanent error
# has occurred and that the host should not be restarted. Please, keep these
# constants in sync with remoting/host/host_exit_codes.h.
MIN_PERMANENT_ERROR_EXIT_CODE=100
MAX_PERMANENT_ERROR_EXIT_CODE=105

# Constants controlling the host process relaunch throttling.
MINIMUM_RELAUNCH_INTERVAL=60
MAXIMUM_HOST_FAILURES=10

# Exit code 126 is defined by Posix to mean "Command found, but not
# executable", and is returned if the process cannot be launched due to
# parental control.
PERMISSION_DENIED_PARENTAL_CONTROL=126

HOST_PID=0
SIGNAL_WAS_TRAPPED=0

# This script works as a proxy between launchd and the host. Signals of
# interest to the host must be forwarded.
SIGNAL_LIST="SIGHUP SIGINT SIGQUIT SIGILL SIGTRAP SIGABRT SIGEMT \
      SIGFPE SIGKILL SIGBUS SIGSEGV SIGSYS SIGPIPE SIGALRM SIGTERM SIGURG \
      SIGSTOP SIGTSTP SIGCONT SIGCHLD SIGTTIN SIGTTOU SIGIO SIGXCPU SIGXFSZ \
      SIGVTALRM SIGPROF SIGWINCH SIGINFO SIGUSR1 SIGUSR2"

handle_signal() {
  SIGNAL_WAS_TRAPPED=1
}

run_host() {
  local host_failure_count=0
  local host_start_time=0

  while true; do
    if [[ ! -f "$ENABLED_FILE" ]]; then
      echo "Daemon is disabled."
      exit 0
    fi

    # If this is not the first time the host has run, make sure we don't
    # relaunch it too soon.
    if [[ "$host_start_time" -gt 0 ]]; then
      local host_lifetime=$(($(date +%s) - $host_start_time))
      echo "Host ran for ${host_lifetime}s"
      if [[ "$host_lifetime" -lt "$MINIMUM_RELAUNCH_INTERVAL" ]]; then
        # If the host didn't run for very long, assume it crashed. Relaunch only
        # after a suitable delay and increase the failure count.
        host_failure_count=$(($host_failure_count + 1))
        echo "Host failure count $host_failure_count/$MAXIMUM_HOST_FAILURES"
        if [[ "$host_failure_count" -ge "$MAXIMUM_HOST_FAILURES" ]]; then
          echo "Too many host failures. Giving up."
          exit 1
        fi
        local relaunch_in=$(($MINIMUM_RELAUNCH_INTERVAL - $host_lifetime))
        echo "Relaunching in ${relaunch_in}s"
        sleep "$relaunch_in"
      else
        # If the host ran for long enough, reset the crash counter.
        host_failure_count=0
      fi
    fi

    # Execute the host asynchronously and forward signals to it.
    trap "handle_signal" $SIGNAL_LIST
    host_start_time=$(date +%s)
    "$HOST_EXE" --host-config="$CONFIG_FILE" \
                --ssh-auth-sockname="/tmp/chromoting.$USER.ssh_auth_sock" &
    HOST_PID="$!"

    # Wait for the host to return and process its exit code.
    while true; do
      wait "$HOST_PID"
      EXIT_CODE="$?"
      if [[ $SIGNAL_WAS_TRAPPED -eq 1 ]]; then
        # 'wait' returned as the result of a trapped signal and the exit code is
        # the signal that was trapped + 128. Forward the signal to the host.
        SIGNAL_WAS_TRAPPED=0
        local SIGNAL=$(($EXIT_CODE - 128))
        echo "Forwarding signal $SIGNAL to host"
        kill -$SIGNAL "$HOST_PID"
      elif [[ "$EXIT_CODE" -eq "0" ||
              "$EXIT_CODE" -eq "$SIGTERM_EXIT_CODE" ||
              "$EXIT_CODE" -eq "$PERMISSION_DENIED_PARENTAL_CONTROL" ||
              ("$EXIT_CODE" -ge "$MIN_PERMANENT_ERROR_EXIT_CODE" && \
              "$EXIT_CODE" -le "$MAX_PERMANENT_ERROR_EXIT_CODE") ]]; then
        echo "Host returned permanent exit code $EXIT_CODE at ""$(date)"""
        if [[ "$EXIT_CODE" -eq 101 ]]; then
          # Exit code 101 is "hostID deleted", which indicates that the host
          # was taken off-line remotely. To prevent the host being restarted
          # when the login context changes, try to delete the "enabled" file.
          # Since this requires root privileges, this is only possible when
          # this script is launched in the "login" context. In the "aqua"
          # context, just exit and try again next time.
          echo "Host id deleted - disabling"
          rm -f "$ENABLED_FILE" 2>/dev/null
        fi
        exit "$EXIT_CODE"
      else
        # Ignore non-permanent error-code and launch host again. Stop handling
        # signals temporarily in case the script has to sleep to throttle host
        # relaunches. While throttling, there is no host process to which to
        # forward the signal, so the default behaviour should be restored.
        echo "Host returned non-permanent exit code $EXIT_CODE at ""$(date)"""
        trap - $SIGNAL_LIST
        HOST_PID=0
        break
      fi
    done
  done
}

if [[ "$1" = "--disable" ]]; then
  # This script is executed from base::mac::ExecuteWithPrivilegesAndWait(),
  # which requires the child process to write its PID to stdout before
  # anythine else. See base/mac/authorization_util.h for details.
  echo $$
  rm -f "$ENABLED_FILE"
elif [[ "$1" = "--enable" ]]; then
  echo $$
  # Ensure the config file is private whilst being written.
  rm -f "$CONFIG_FILE"
  umask 0077
  cat > "$CONFIG_FILE"
  # Ensure the config is readable by the user registering the host.
  chmod +a "$USER:allow:read" "$CONFIG_FILE"
  touch "$ENABLED_FILE"
elif [[ "$1" = "--save-config" ]]; then
  echo $$
  cat > "$CONFIG_FILE"
elif [[ "$1" = "--host-version" ]]; then
  /usr/libexec/PlistBuddy -c "Print CFBundleVersion" "$PLIST_FILE"
elif [[ "$1" = "--run-from-launchd" ]]; then
  echo Host started for user $USER at $"$(date)"
  run_host
else
  echo $$
  exit 1
fi
