#!/bin/sh

# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Version = @@VERSION@@

HELPERTOOLS=/Library/PrivilegedHelperTools
SERVICE_NAME=org.chromium.chromoting
CONFIG_FILE="$HELPERTOOLS/$SERVICE_NAME.json"
SCRIPT_FILE="$HELPERTOOLS/$SERVICE_NAME.me2me.sh"
USERS_TMP_FILE="$SCRIPT_FILE.users"
PLIST=/Library/LaunchAgents/org.chromium.chromoting.plist
ENABLED_FILE="$HELPERTOOLS/$SERVICE_NAME.me2me_enabled"
ENABLED_FILE_BACKUP="$ENABLED_FILE.backup"
PREF_PANE=/Library/PreferencePanes/ChromeRemoteDesktop.prefPane

# In case of errors, log the fact, but continue to unload launchd jobs as much
# as possible. When finished, this preflight script should exit successfully in
# case this is a Keystone-triggered update which must be allowed to proceed.
function on_error {
  logger An error occurred during Chrome Remote Desktop setup.
}

function find_users_with_active_hosts {
  ps -eo uid,command | awk -v script="$SCRIPT_FILE" '
    $2 == "/bin/sh" && $3 == script && $4 == "--run-from-launchd" {
      print $1
    }'
}

function find_login_window_for_user {
  # This function mimics the behaviour of pgrep, which may not be installed
  # on Mac OS X.
  local user=$1
  ps -ec -u "$user" -o comm,pid | awk '$1 == "loginwindow" { print $2; exit }'
}

# Return 0 (true) if the current OS is El Capitan (OS X 10.11) or newer.
function is_el_capitan_or_newer {
  local full_version=$(sw_vers -productVersion)

  # Split the OS version into an array.
  local version
  IFS='.' read -a version <<< "${full_version}"
  local v0="${version[0]}"
  local v1="${version[1]}"
  if [[ $v0 -gt 10 || ( $v0 -eq 10 && $v1 -ge 11 ) ]]; then
    return 0
  else
    return 1
  fi
}

trap on_error ERR

logger Running Chrome Remote Desktop preflight script @@VERSION@@

# If there is an _enabled file, rename it while upgrading.
if [[ -f "$ENABLED_FILE" ]]; then
  logger Moving _enabled file
  mv "$ENABLED_FILE" "$ENABLED_FILE_BACKUP"
fi

# Stop and unload the service for each user currently running the service, and
# record the user IDs so the service can be restarted for the same users in the
# postflight script.
rm -f "$USERS_TMP_FILE"

for uid in $(find_users_with_active_hosts); do
  logger Unloading service for user "$uid"
  if [[ -n "$uid" ]]; then
    echo "$uid" >> "$USERS_TMP_FILE"
    if [[ "$uid" = "0" ]]; then
      context="LoginWindow"
    else
      context="Aqua"
    fi

    stop="launchctl stop $SERVICE_NAME"
    unload="launchctl unload -w -S $context $PLIST"

    if is_el_capitan_or_newer; then
      bootstrap_user="launchctl asuser $uid"
    else
      # Load the launchd agent in the bootstrap context of user $uid's
      # graphical session, so that screen-capture and input-injection can
      # work. To do this, find the PID of a process which is running in that
      # context. The loginwindow process is a good candidate since the user
      # (if logged in to a session) will definitely be running it.
      pid="$(find_login_window_for_user "$uid")"
      if [[ ! -n "$pid" ]]; then
        exit 1
      fi
      sudo_user="sudo -u #$uid"
      bootstrap_user="launchctl bsexec $pid"
    fi

    logger $bootstrap_user $sudo_user $stop
    $bootstrap_user $sudo_user $stop
    logger $bootstrap_user $sudo_user $unload
    $bootstrap_user $sudo_user $unload
  fi
done

# The installer no longer includes a preference-pane applet, so remove any
# pref-pane from a previous installation.
rm -rf "$PREF_PANE"

exit 0
