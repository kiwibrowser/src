# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Baselines for user/group tests."""

from __future__ import print_function

from chromite.lib import cros_build_lib


# firewall:!:236:236:firewall daemon:/dev/null:/bin/false
UserEntry = cros_build_lib.Collection('UserEntry',
                                      user=None, encpasswd='!',
                                      uid=None, gid=None,
                                      home='/dev/null', shell='/bin/false')

# tty:!:5:xorg,power,brltty
GroupEntry = cros_build_lib.Collection('GroupEntry', group=None, encpasswd='!',
                                       gid=None, users=set())

# For users that we allow to login to the system, whitelist a number of
# alternative shells.  These are equivalent from a security POV.
_VALID_LOGIN_SHELLS = set((
    '/bin/sh',
    '/bin/bash',
    '/bin/dash',
))

USER_BASELINE = dict((e.user, e) for e in (
    UserEntry(user='root', encpasswd='x', uid=0, gid=0, home='/root',
              shell=_VALID_LOGIN_SHELLS),
    UserEntry(user='bin', uid=1, gid=1, home='/bin'),
    UserEntry(user='daemon', uid=2, gid=2, home='/sbin'),
    UserEntry(user='adm', uid=3, gid=4, home='/var/adm'),
    UserEntry(user='lp', uid=4, gid=7, home='/var/spool/lpd'),
    UserEntry(user='news', uid=9, gid=13, home='/var/spool/news'),
    UserEntry(user='uucp', uid=10, gid=14, home='/var/spool/uucp'),
    UserEntry(user='portage', uid=250, gid=250, home='/var/tmp/portage'),
    UserEntry(user='nobody', uid=65534, gid=65534, home='/var/empty'),
    UserEntry(user='chronos', encpasswd='x', uid=1000, gid=1000,
              home='/home/chronos/user', shell=_VALID_LOGIN_SHELLS),
    UserEntry(user='chronos-access', uid=1001, gid=1001),
    UserEntry(user='sshd', uid=204, gid=204, home='/var/empty'),
    UserEntry(user='tss', uid=207, gid=207, home='/var/lib/tpm'),
    UserEntry(user='dhcp', uid=224, gid=224,
              home={'/var/lib/dhcp', '/dev/null'}),
    UserEntry(user='goofy', encpasswd='x', uid=248, gid=248,
              home='/home/goofy', shell='/bin/bash'),
    UserEntry(user='android-root', uid=655360, gid=655360),
    UserEntry(user='user-containers', uid=10000, gid=10000),
))

USER_BASELINE_LAKITU = dict((e.user, e) for e in (
    UserEntry(user='systemd-timesync', uid=271, gid=271),
    UserEntry(user='systemd-network', uid=274, gid=274),
    UserEntry(user='systemd-resolve', uid=275, gid=275),
))

USER_BASELINE_JETSTREAM = dict((e.user, e) for e in (
    UserEntry(user='ap-monitor', uid=1102, gid=1103),
))

USER_BASELINE_TERMINA = dict((e.user, e) for e in (
    UserEntry(user='lxd', uid=298, gid=298),
))

USER_BOARD_BASELINES = {
    'lakitu': USER_BASELINE_LAKITU,
    'lakitu-gpu': USER_BASELINE_LAKITU,
    'lakitu-st': USER_BASELINE_LAKITU,
    'lakitu_next': USER_BASELINE_LAKITU,
    'arkham': USER_BASELINE_JETSTREAM,
    'cyclone': USER_BASELINE_JETSTREAM,
    'gale': USER_BASELINE_JETSTREAM,
    'storm': USER_BASELINE_JETSTREAM,
    'whirlwind': USER_BASELINE_JETSTREAM,
    'tael': USER_BASELINE_TERMINA,
    'tatl': USER_BASELINE_TERMINA,
}

GROUP_BASELINE = dict((e.group, e) for e in (
    GroupEntry(group='root', gid=0, users={'root'}),
    GroupEntry(group='bin', gid=1, users={'root', 'bin', 'daemon'}),
    GroupEntry(group='daemon', gid=2, users={'root', 'bin', 'daemon'}),
    GroupEntry(group='sys', gid=3, users={'root', 'bin', 'adm'}),
    GroupEntry(group='adm', gid=4, users={'root', 'adm', 'daemon'}),
    GroupEntry(group='disk', gid=6, users={'root', 'adm', 'cros-disks'}),
    GroupEntry(group='lp', gid=7, users={'lp', 'lpadmin', 'cups', 'chronos'}),
    GroupEntry(group='mem', gid=8),
    GroupEntry(group='kmem', gid=9),
    GroupEntry(group='wheel', gid=10, users={'root'}),
    GroupEntry(group='floppy', gid=11, users={'root'}),
    GroupEntry(group='news', gid=13, users={'news'}),
    GroupEntry(group='uucp', gid=14, users={'uucp', 'gpsd'}),
    GroupEntry(group='console', gid=17),
    GroupEntry(group='audio', gid=18, users={'cras', 'chronos', 'volume',
                                             'midis', 'rtanalytics'}),
    GroupEntry(group='cdrom', gid=19, users={'cros-disks'}),
    GroupEntry(group='tape', gid=26, users={'root'}),
    GroupEntry(group='cdrw', gid=80, users={'cros-disks'}),
    GroupEntry(group='usb', gid=85, users={'mtp', 'brltty', 'dlm', 'modem'}),
    GroupEntry(group='users', gid=100),
    GroupEntry(group='portage', gid=250, users={'portage'}),
    GroupEntry(group='utmp', gid=406),
    GroupEntry(group='nogroup', gid=65533),
    GroupEntry(group='nobody', gid=65534),
    GroupEntry(group='chronos', gid=1000),
    GroupEntry(group='chronos-access', gid=1001,
               users={'root', 'ipsec', 'chronos', 'ntfs-3g', 'avfs',
                      'fuse-exfat', 'chaps', 'cros-disks', 'imageloaderd'}),
    GroupEntry(group='tss', gid=207, users={'root', 'attestation',
                                            'bootlockboxd', 'chaps',
                                            'tpm_manager', 'trunks'}),
    GroupEntry(group='pkcs11', gid=208, users={'root', 'ipsec', 'chronos',
                                               'chaps', 'wpa', 'attestation'}),
    GroupEntry(group='wpa', gid=219, users={'root'}),
    GroupEntry(group='cras', gid=600, users={'chronos', 'power',
                                             'rtanalytics'}),
    GroupEntry(group='wayland', gid=601, users={'chronos', 'crosvm'}),
    GroupEntry(group='arc-bridge', gid=602, users={'chronos'}),
    GroupEntry(group='brltty', gid=240, users={'chronos'}),
    GroupEntry(group='preserve', gid=253, users={'root', 'attestation',
                                                 'tpm_manager'}),
    GroupEntry(group='goofy', gid=248, users={'goofy'}),
    GroupEntry(group='authpolicyd', gid=254, users={'authpolicyd',
                                                    'authpolicyd-exec'}),
    GroupEntry(group='scanner', gid=255, users={'saned'}),
    GroupEntry(group='uinput', gid=258, users={'bluetooth', 'volume'}),
    GroupEntry(group='apmanager', gid=259, users={'apmanager', 'buffet'}),
    GroupEntry(group='peerd', gid=260, users={'buffet', 'chronos', 'peerd'}),
    GroupEntry(group='buffet', gid=264, users={'chronos', 'buffet', 'power'}),
    GroupEntry(group='webservd', gid=266, users={'buffet', 'webservd'}),
    GroupEntry(group='lpadmin', gid=269, users={'cups', 'lpadmin'}),
    GroupEntry(group='policy-readers', gid=303, users={'authpolicyd', 'chronos',
                                                       'u2f', 'shill'}),
    GroupEntry(group='ipsec', gid=212, users={'shill'}),
    GroupEntry(group='debugfs-access', gid=605, users={'shill'}),
    GroupEntry(group='arc-camera', gid=603, users={'chronos'}),
    GroupEntry(group='daemon-store', gid=400, users={'biod', 'chaps',
                                                     'crosvm'}),
    GroupEntry(group='logs-access', gid=401, users={'debugd-logs'}),
    GroupEntry(group='serial', gid=402, users={'uucp'}),
    GroupEntry(group='devbroker-access', gid=403, users={'chronos'}),
    GroupEntry(group='i2c', gid=404, users={'power'}),
    GroupEntry(group='android-root', gid=655360, users={'android-root'}),
    GroupEntry(group='android-everybody', gid=665357, users={'chronos'}),
    GroupEntry(group='user-containers', gid=10000, users={'user-containers'}),
    GroupEntry(group='midis', gid=608, users={'chronos'}),
    GroupEntry(group='avfs', gid=301, users={'cros-disks'}),
    GroupEntry(group='cfm-peripherals', gid=20103,
               users={'cfm-monitor', 'cfm-firmware-updaters'}),
    GroupEntry(group='ippusb', gid=20100, users={'ippusb', 'lp', 'lpadmin',
                                                 'cups'}),
    GroupEntry(group='tun', gid=413, users={'crosvm', 'shill'}),
    GroupEntry(group='gpio', gid=414, users={'modem'}),
))

GROUP_BASELINE_FREON = dict((e.group, e) for e in (
    GroupEntry(group='tty', gid=5, users={'power', 'brltty'}),
    GroupEntry(group='video', gid=27, users={'root', 'chronos', 'arc-camera',
                                             'dlm', 'rtanalytics', 'crosvm'}),
    GroupEntry(group='input', gid=222, users={'cras', 'power', 'chronos'}),
))

GROUP_BASELINE_XORG = dict((e.group, e) for e in (
    GroupEntry(group='tty', gid=5, users={'xorg', 'power', 'brltty'}),
    GroupEntry(group='video', gid=27, users={'root', 'chronos', 'arc-camera',
                                             'xorg', 'dlm', 'rtanalytics',
                                             'crosvm'}),
    GroupEntry(group='input', gid=222, users={'cras', 'xorg', 'power'}),
))

GROUP_BASELINE_LAKITU = GROUP_BASELINE_XORG.copy()
GROUP_BASELINE_LAKITU.update(dict((e.group, e) for e in (
    GroupEntry(group='systemd-journal', gid=270),
    GroupEntry(group='systemd-timesync', gid=271),
    GroupEntry(group='systemd-network', gid=274),
    GroupEntry(group='systemd-resolve', gid=275),
    GroupEntry(group='docker', gid=412),
    GroupEntry(group='google-sudoers', encpasswd='x', gid=1002),
)))

GROUP_BASELINE_JETSTREAM = GROUP_BASELINE_XORG.copy()
GROUP_BASELINE_JETSTREAM.update(dict((e.group, e) for e in (
    GroupEntry(group='leds', gid=1102, users={'ap-controller'}),
    GroupEntry(group='wpa_supplicant', gid=1114,
               users={'ap-wifi-diagnostics', 'wpa_supplicant',
                      'ap-wifi-manager', 'ap-hal'}),
    GroupEntry(group='hostapd', gid=1106,
               users={'hostapd', 'ap-wireless-optimizer', 'ap-monitor',
                      'ap-wifi-manager', 'ap-wifi-diagnostics', 'ap-hal'}),
)))

# rialtod:!:400:rialto
GROUP_BASELINE_RIALTO = dict((e.group, e) for e in (
    GroupEntry(group='rialtod', gid=400, users={'rialto'}),
))

GROUP_BASELINE_TERMINA = dict((e.group, e) for e in (
    GroupEntry(group='lxd', gid=298, users={'lxd', 'chronos'}),
))

GROUP_BOARD_BASELINES = {
    'lakitu': GROUP_BASELINE_LAKITU,
    'lakitu-gpu': GROUP_BASELINE_LAKITU,
    'lakitu-st': GROUP_BASELINE_LAKITU,
    'lakitu_next': GROUP_BASELINE_LAKITU,
    'arkham': GROUP_BASELINE_JETSTREAM,
    'cyclone': GROUP_BASELINE_JETSTREAM,
    'gale': GROUP_BASELINE_JETSTREAM,
    'storm': GROUP_BASELINE_JETSTREAM,
    'whirlwind': GROUP_BASELINE_JETSTREAM,
    'veyron_rialto': GROUP_BASELINE_RIALTO,
    'tael': GROUP_BASELINE_TERMINA,
    'tatl': GROUP_BASELINE_TERMINA,
}
