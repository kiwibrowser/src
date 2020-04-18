# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module contains constants used by cbuildbot and related code."""

from __future__ import print_function

import os

# TODO(akeshet): Once constants.py is more completely split, and the callers are
# all updated, remove these *-imports. See crbug.com/746047 for context.
# pylint: disable=wildcard-import
# pylint: disable=unused-wildcard-import
from chromite.lib.const.waterfall import *

def _FindSourceRoot():
  """Try and find the root check out of the chromiumos tree"""
  source_root = path = os.path.realpath(os.path.join(
      os.path.abspath(__file__), '..', '..', '..'))
  while True:
    if os.path.isdir(os.path.join(path, '.repo')):
      return path
    elif path == '/':
      break
    path = os.path.dirname(path)
  return source_root


SOURCE_ROOT = _FindSourceRoot()
CHROOT_SOURCE_ROOT = '/mnt/host/source'
CHROOT_WORKSPACE_ROOT = '/mnt/host/workspace'
CHROOT_CACHE_ROOT = '/var/cache/chromeos-cache'
DEPOT_TOOLS_SUBPATH = 'chromium/tools/depot_tools'

CROSUTILS_DIR = os.path.join(SOURCE_ROOT, 'src/scripts')
CHROMITE_DIR = os.path.realpath(os.path.join(
    os.path.abspath(__file__), '..', '..'))
BOOTSTRAP_DIR = os.path.join(CHROMITE_DIR, 'bootstrap')
DEPOT_TOOLS_DIR = os.path.join(SOURCE_ROOT, DEPOT_TOOLS_SUBPATH)
CHROMITE_BIN_SUBDIR = 'chromite/bin'
CHROMITE_BIN_DIR = os.path.join(CHROMITE_DIR, 'bin')
PATH_TO_CBUILDBOT = os.path.join(CHROMITE_BIN_SUBDIR, 'cbuildbot')
DEFAULT_CHROOT_DIR = 'chroot'

SITE_CONFIG_DIR = os.path.join(CHROMITE_DIR, 'config')
SITE_CONFIG_FILE = os.path.join(SITE_CONFIG_DIR, 'config_dump.json')

CHROMEOS_CONFIG_FILE = os.path.join(CHROMITE_DIR, 'cbuildbot',
                                    'config_dump.json')
WATERFALL_CONFIG_FILE = os.path.join(CHROMITE_DIR, 'cbuildbot',
                                     'waterfall_layout_dump.txt')

GE_BUILD_CONFIG_FILE = os.path.join(
    CHROMITE_DIR, 'cbuildbot', 'ge_build_config.json')

# The following define the location for storing toolchain packages and
# SDK overlay tarballs created during SDK builder runs. The paths are relative
# to the build root's chroot, which guarantees that they are reachable from it
# and get cleaned up when it is removed.
SDK_TOOLCHAINS_OUTPUT = 'tmp/toolchain-pkgs'
SDK_OVERLAYS_OUTPUT = 'tmp/sdk-overlays'

AUTOTEST_BUILD_PATH = 'usr/local/build/autotest'
UNITTEST_PKG_PATH = 'test-packages'

# Path to the lsb-release file on the device.
LSB_RELEASE_PATH = '/etc/lsb-release'

HOME_DIRECTORY = os.path.expanduser('~')

# If cbuiltbot is running on a bot, then the cidb access credentials will be
# available here. This directory will not exist otherwise.
CIDB_PROD_BOT_CREDS = os.path.join(HOME_DIRECTORY, '.cidb_creds',
                                   'prod_cidb_bot')
CIDB_DEBUG_BOT_CREDS = os.path.join(HOME_DIRECTORY, '.cidb_creds',
                                    'debug_cidb_bot')



# Buildbucket build status
BUILDBUCKET_BUILDER_STATUS_SCHEDULED = 'SCHEDULED'
BUILDBUCKET_BUILDER_STATUS_STARTED = 'STARTED'
BUILDBUCKET_BUILDER_STATUS_COMPLETED = 'COMPLETED'

BUILDBUCKET_BUILDER_STATUSES = (BUILDBUCKET_BUILDER_STATUS_SCHEDULED,
                                BUILDBUCKET_BUILDER_STATUS_STARTED,
                                BUILDBUCKET_BUILDER_STATUS_COMPLETED)

BUILDBUCKET_BUILDER_RESULT_SUCCESS = 'SUCCESS'
BUILDBUCKET_BUILDER_RESULT_FAILURE = 'FAILURE'
BUILDBUCKET_BUILDER_RESULT_CANCELED = 'CANCELED'

# Builder status strings
BUILDER_STATUS_FAILED = 'fail'
BUILDER_STATUS_PASSED = 'pass'
BUILDER_STATUS_INFLIGHT = 'inflight'
BUILDER_STATUS_MISSING = 'missing'
BUILDER_STATUS_ABORTED = 'aborted'
# The following statuses are currently only used for build stages.
BUILDER_STATUS_PLANNED = 'planned'
BUILDER_STATUS_WAITING = 'waiting'
BUILDER_STATUS_SKIPPED = 'skipped'
BUILDER_STATUS_FORGIVEN = 'forgiven'
BUILDER_COMPLETED_STATUSES = (BUILDER_STATUS_PASSED,
                              BUILDER_STATUS_FAILED,
                              BUILDER_STATUS_ABORTED,
                              BUILDER_STATUS_SKIPPED,
                              BUILDER_STATUS_FORGIVEN)
BUILDER_ALL_STATUSES = (BUILDER_STATUS_FAILED,
                        BUILDER_STATUS_PASSED,
                        BUILDER_STATUS_INFLIGHT,
                        BUILDER_STATUS_MISSING,
                        BUILDER_STATUS_ABORTED,
                        BUILDER_STATUS_WAITING,
                        BUILDER_STATUS_PLANNED,
                        BUILDER_STATUS_SKIPPED,
                        BUILDER_STATUS_FORGIVEN)
BUILDER_NON_FAILURE_STATUSES = (BUILDER_STATUS_PLANNED,
                                BUILDER_STATUS_PASSED,
                                BUILDER_STATUS_SKIPPED,
                                BUILDER_STATUS_FORGIVEN)

# CL status strings
CL_STATUS_FAILED = BUILDER_STATUS_FAILED
CL_STATUS_INFLIGHT = BUILDER_STATUS_INFLIGHT
CL_STATUS_PASSED = BUILDER_STATUS_PASSED
CL_STATUS_LAUNCHING = 'launching'
CL_STATUS_WAITING = 'waiting'
CL_STATUS_READY_TO_SUBMIT = 'ready-to-submit'
CL_STATUS_FULLY_VERIFIED = 'fully-verified'

# Signer status strings
SIGNER_STATUS_PASSED = 'passed'
SIGNER_STATUS_FAILED = 'failed'

# Change sources
CHANGE_SOURCE_INTERNAL = 'internal'
CHANGE_SOURCE_EXTERNAL = 'external'

# Build failure categories
FAILURE_CATEGORY_BAD_CL = 'bad_cl'
FAILURE_CATEGORY_BUG_IN_TOT = 'bug_in_tot'
FAILURE_CATEGORY_MERGE_CONFLICT = 'merge_conflict'
FAILURE_CATEGORY_TREE_CLOSED = 'tree_closed'
FAILURE_CATEGORY_SCHEDULED_ABORT = 'scheduled_abort'
FAILURE_CATEGORY_CL_NOT_READY = 'cl_not_ready'
FAILURE_CATEGORY_BAD_CHROME = 'bad_chrome'
FAILURE_CATEGORY_INFRA_FAILURE = 'infra_failure'
FAILURE_CATEGORY_TEST_FLAKE = 'test_flake'
FAILURE_CATEGORY_GERRIT_FAILURE = 'gerrit_failure'
FAILURE_CATEGORY_GS_FAILURE = 'gs_failure'
FAILURE_CATEGORY_LAB_FAILURE = 'lab_failure'
FAILURE_CATEGORY_BAD_BINARY_PACKAGE = 'bad_binary_package'
FAILURE_CATEGORY_BUILD_FLAKE = 'build_flake'
FAILURE_CATEGORY_MYSTERY = 'mystery'

FAILURE_CATEGORY_ALL_CATEGORIES = (
    FAILURE_CATEGORY_BAD_CL,
    FAILURE_CATEGORY_BUG_IN_TOT,
    FAILURE_CATEGORY_MERGE_CONFLICT,
    FAILURE_CATEGORY_TREE_CLOSED,
    FAILURE_CATEGORY_SCHEDULED_ABORT,
    FAILURE_CATEGORY_CL_NOT_READY,
    FAILURE_CATEGORY_BAD_CHROME,
    FAILURE_CATEGORY_INFRA_FAILURE,
    FAILURE_CATEGORY_TEST_FLAKE,
    FAILURE_CATEGORY_GERRIT_FAILURE,
    FAILURE_CATEGORY_GS_FAILURE,
    FAILURE_CATEGORY_LAB_FAILURE,
    FAILURE_CATEGORY_BAD_BINARY_PACKAGE,
    FAILURE_CATEGORY_BUILD_FLAKE,
    FAILURE_CATEGORY_MYSTERY,
)


# Exception categories, as recorded in cidb
EXCEPTION_CATEGORY_UNKNOWN = 'unknown'
EXCEPTION_CATEGORY_BUILD = 'build'
EXCEPTION_CATEGORY_TEST = 'test'
EXCEPTION_CATEGORY_INFRA = 'infra'
EXCEPTION_CATEGORY_LAB = 'lab'

EXCEPTION_CATEGORY_ALL_CATEGORIES = (
    EXCEPTION_CATEGORY_UNKNOWN,
    EXCEPTION_CATEGORY_BUILD,
    EXCEPTION_CATEGORY_TEST,
    EXCEPTION_CATEGORY_INFRA,
    EXCEPTION_CATEGORY_LAB,
)

# Suspect reasons for rejecting changes in validation_pool.
SUSPECT_REASON_BAD_CHANGE = 'bad_change'
SUSPECT_REASON_INFRA_FAIL = 'infra_fail'
SUSPECT_REASON_BUILD_FAIL = 'build_fail'
SUSPECT_REASON_TEST_FAIL = 'test_fail'
SUSPECT_REASON_OVERLAY_CHANGE = 'overlay_change'
SUSPECT_REASON_UNKNOWN = 'unknown'

# A dict mapping suspect reasons to their blame priorities.
# Lower values have higher blame priorities.
SUSPECT_REASONS = {
    SUSPECT_REASON_BAD_CHANGE: 1,
    SUSPECT_REASON_INFRA_FAIL: 2,
    SUSPECT_REASON_BUILD_FAIL: 3,
    SUSPECT_REASON_TEST_FAIL: 4,
    SUSPECT_REASON_OVERLAY_CHANGE: 5,
    SUSPECT_REASON_UNKNOWN: 6,
}

# Monarch metric names
MON_CQ_WALL_CLOCK_SECS = 'chromeos/cbuildbot/cq_wall_clock_seconds'
MON_CQ_SELF_DESTRUCTION_COUNT = ('chromeos/cbuildbot/build/'
                                 'cq_self_destruction_count')
MON_CQ_BUILD_DURATION = 'chromeos/cbuildbot/build/cq_build_durations'
MON_CL_ACTION = 'chromeos/cbuildbot/cl_action'
MON_LAST_SLAVE = 'chromeos/cbuildbot/last_completed_slave'
MON_PRECQ_LAUNCH_COUNT = 'chromeos/cbuildbot/pre-cq/launch_count'
MON_PRECQ_CL_LAUNCH_COUNT = 'chromeos/cbuildbot/pre-cq/cl_launch_count'
MON_PRECQ_TICK_COUNT = 'chromeos/cbuildbot/pre-cq/tick_count'
MON_BUILD_COMP_COUNT = 'chromeos/cbuildbot/build/completed_count'
MON_BUILD_SANITY_COMP_COUNT = (
    'chromeos/cbuildbot/build/sanity_build_completed_count')
MON_BUILD_SANITY_ID = 'chromeos/cbuildbot/build/sanity_build_id'
MON_BUILD_DURATION = 'chromeos/cbuildbot/build/durations'
MON_STAGE_COMP_COUNT = 'chromeos/cbuildbot/stage/completed_count'
MON_STAGE_DURATION = 'chromeos/cbuildbot/stage/durations'
MON_STAGE_FAILURE_COUNT = 'chromeos/cbuildbot/stage/failure_count'
MON_CL_HANDLE_TIME = 'chromeos/cbuildbot/submitted_change/handling_times'
MON_CL_WALL_CLOCK_TIME = 'chromeos/cbuildbot/submitted_change/wall_clock_times'
MON_CL_PRECQ_TIME = 'chromeos/cbuildbot/submitted_change/precq_times'
MON_CL_WAIT_TIME = 'chromeos/cbuildbot/submitted_change/wait_times'
MON_CL_CQRUN_TIME = 'chromeos/cbuildbot/submitted_change/cq_run_times'
MON_CL_CQ_TRIES = 'chromeos/cbuildbot/submitted_change/cq_attempts'
MON_CL_FALSE_REJ = 'chromeos/cbuildbot/submitted_change/false_rejections'
MON_CL_FALSE_REJ_TOTAL = (
    'chromeos/cbuildbot/submitted_change/false_rejections_total')
MON_CL_FALSE_REJ_COUNT = (
    'chromeos/cbuildbot/submitted_change/false_rejection_count')
MON_CQ_FALSE_REJ_MINUS_EXONERATIONS = (
    'chromeos/cbuildbot/submitted_change/false_rejections_minus_exonerations')
MON_CHROOT_USED = 'chromeos/cbuildbot/chroot_at_version'
MON_REPO_SYNC_COUNT = 'chromeos/cbuildbot/repo/sync_count'
MON_REPO_SYNC_RETRY_COUNT = 'chromeos/cbuildbot/repo/sync_retry_count'
MON_REPO_SELFUPDATE_FAILURE_COUNT = ('chromeos/cbuildbot/repo/'
                                     'selfupdate_failure_count')
MON_REPO_INIT_RETRY_COUNT = 'chromeos/cbuildbot/repo/init_retry_count'
MON_REPO_MANIFEST_FAILURE_COUNT = ('chromeos/cbuildbot/repo/'
                                   'manifest_failure_count')
MON_GIT_FETCH_COUNT = 'chromeos/cbuildbot/git/fetch_count'
MON_BB_RETRY_BUILD_COUNT = ('chromeos/cbuildbot/buildbucket/'
                            'retry_build_count')
MON_BB_CANCEL_BATCH_BUILDS_COUNT = ('chromeos/cbuildbot/buildbucket/'
                                    'cancel_batch_builds_count')
MON_BB_CANCEL_PRE_CQ_BUILD_COUNT = ('chromeos/cbuildbot/buildbucket/'
                                    'cancel_pre_cq_build_count')
MON_EXPORT_TO_GCLOUD = 'chromeos/cbuildbot/export_to_gcloud'
MON_CL_REJECT_COUNT = 'chromeos/cbuildbot/change/rejected_count'
MON_GS_ERROR = 'chromeos/gs/error_count'


# Re-execution API constants.
# Used by --resume and --bootstrap to decipher which options they
# can pass to the target cbuildbot (since it may not have that
# option).
# Format is Major.Minor.  Minor is used for tracking new options added
# that aren't critical to the older version if it's not ran.
# Major is used for tracking heavy API breakage- for example, no longer
# supporting the --resume option.
REEXEC_API_MAJOR = 0
REEXEC_API_MINOR = 8
REEXEC_API_VERSION = '%i.%i' % (REEXEC_API_MAJOR, REEXEC_API_MINOR)

# Support --master-build-id
REEXEC_API_MASTER_BUILD_ID = 3
# Support --git-cache-dir
REEXEC_API_GIT_CACHE_DIR = 4
# Support --goma_dir and --goma_client_json
REEXEC_API_GOMA = 5
# Support --ts-mon-task-num
REEXEC_API_TSMON_TASK_NUM = 6
# Support --sanity-check-build
REEXEC_API_SANITY_CHECK_BUILD = 7
# Support --previous-build-state
REEXEC_API_PREVIOUS_BUILD_STATE = 8

# We rely on the (waterfall, builder name, build number) to uniquely identify
# a build. However, future migrations or state wipes of the buildbot master may
# cause it to reset its build number counter. When that happens, this value
# should be incremented, ensuring that (waterfall, builder name, build number,
# buildbot generation) is a unique identifier of builds.
BUILDBOT_GENERATION = 1

ISOLATESERVER = 'https://isolateserver.appspot.com'

GOOGLE_EMAIL = '@google.com'
CHROMIUM_EMAIL = '@chromium.org'

CORP_DOMAIN = 'corp.google.com'
GOLO_DOMAIN = 'golo.chromium.org'
CHROME_DOMAIN = 'chrome.' + CORP_DOMAIN
CHROMEOS_BOT_INTERNAL = 'chromeos-bot.internal'

GOB_HOST = '%s.googlesource.com'

EXTERNAL_GOB_INSTANCE = 'chromium'
EXTERNAL_GERRIT_INSTANCE = 'chromium-review'
EXTERNAL_GOB_HOST = GOB_HOST % EXTERNAL_GOB_INSTANCE
EXTERNAL_GERRIT_HOST = GOB_HOST % EXTERNAL_GERRIT_INSTANCE
EXTERNAL_GOB_URL = 'https://%s' % EXTERNAL_GOB_HOST
EXTERNAL_GERRIT_URL = 'https://%s' % EXTERNAL_GERRIT_HOST

INTERNAL_GOB_INSTANCE = 'chrome-internal'
INTERNAL_GERRIT_INSTANCE = 'chrome-internal-review'
INTERNAL_GOB_HOST = GOB_HOST % INTERNAL_GOB_INSTANCE
INTERNAL_GERRIT_HOST = GOB_HOST % INTERNAL_GERRIT_INSTANCE
INTERNAL_GOB_URL = 'https://%s' % INTERNAL_GOB_HOST
INTERNAL_GERRIT_URL = 'https://%s' % INTERNAL_GERRIT_HOST

# Tests without 'cheets_CTS_', 'cheets_GTS.' prefix will not considered
# as CTS/GTS test in chromite.lib.cts_helper
DEFAULT_CTS_TEST_XML_MAP = {
    'cheets_CTS_': 'test_result.xml',
    'cheets_GTS.': 'test_result.xml',
}
# Google Storage bucket URI to store results in.
DEFAULT_CTS_RESULTS_GSURI = 'gs://chromeos-cts-results/'
DEFAULT_CTS_APFE_GSURI = 'gs://chromeos-cts-apfe/'

ANDROID_INTERNAL_PATTERN = r'\.zip.internal$'
ANDROID_BUCKET_URL = 'gs://android-build-chromeos/builds'
ANDROID_MST_BUILD_BRANCH = 'git_master-arc-dev'
ANDROID_NYC_BUILD_BRANCH = 'git_nyc-mr1-arc-m68'
ANDROID_PI_BUILD_BRANCH = 'git_pi-arc-dev'
ANDROID_GTS_BUILD_TARGETS = {
    # "gts_arm64" is the build maintained by GMS team.
    'XTS': ('linux-gts_arm64', r'\.zip$'),
}
ANDROID_MST_BUILD_TARGETS = {
    'ARM': ('linux-cheets_arm-user', r'\.zip$'),
    'X86': ('linux-cheets_x86-user', r'\.zip$'),
    'X86_64': ('linux-cheets_x86_64-user', r'\.zip$'),
    'ARM_USERDEBUG': ('linux-cheets_arm-userdebug', r'\.zip$'),
    'X86_USERDEBUG': ('linux-cheets_x86-userdebug', r'\.zip$'),
    'X86_64_USERDEBUG': ('linux-cheets_x86_64-userdebug', r'\.zip$'),
}
ANDROID_NYC_BUILD_TARGETS = {
    # TODO(b/29509721): Workaround to roll adb with system image. We want to
    # get rid of this.
    'ARM': ('linux-cheets_arm-user', r'(\.zip|/adb)$'),
    'X86': ('linux-cheets_x86-user', r'\.zip$'),
    'X86_INTERNAL': ('linux-cheets_x86-user', ANDROID_INTERNAL_PATTERN),
    'X86_USERDEBUG': ('linux-cheets_x86-userdebug', r'\.zip$'),
    'AOSP_X86_USERDEBUG': ('linux-aosp_cheets_x86-userdebug', r'\.zip$'),
    'SDK_TOOLS': ('linux-static_sdk_tools', r'/(aapt|adb|zipalign)$'),
    'SDK_GOOGLE_X86_USERDEBUG': ('linux-sdk_google_cheets_x86-userdebug',
                                 r'\.zip$'),
    'SDK_GOOGLE_X86_64_USERDEBUG': ('linux-sdk_google_cheets_x86_64-userdebug',
                                    r'\.zip$'),
    'X86_64': ('linux-cheets_x86_64-user', r'\.zip$'),
    'X86_64_USERDEBUG': ('linux-cheets_x86_64-userdebug', r'\.zip$'),
}
ANDROID_PI_BUILD_TARGETS = {
    'ARM': ('linux-cheets_arm-user', r'\.zip$'),
    'X86': ('linux-cheets_x86-user', r'\.zip$'),
    'X86_NDK_TRANSLATION': ('linux-cheets_x86_ndk_translation-user', r'\.zip$'),
    'X86_64': ('linux-cheets_x86_64-user', r'\.zip$'),
    'ARM_USERDEBUG': ('linux-cheets_arm-userdebug', r'\.zip$'),
    'X86_USERDEBUG': ('linux-cheets_x86-userdebug', r'\.zip$'),
    'X86_NDK_TRANSLATION_USERDEBUG': (
        'linux-cheets_x86_ndk_translation-userdebug', r'\.zip$'
    ),
    'X86_64_USERDEBUG': ('linux-cheets_x86_64-userdebug', r'\.zip$'),
}

ARC_BUCKET_URL = 'gs://chromeos-arc-images/builds'
ARC_BUCKET_ACLS = {
    'ARM': 'googlestorage_acl_arm.txt',
    'X86': 'googlestorage_acl_x86.txt',
    'X86_NDK_TRANSLATION': 'googlestorage_acl_internal.txt',
    'X86_64': 'googlestorage_acl_x86.txt',
    'ARM_USERDEBUG': 'googlestorage_acl_arm.txt',
    'X86_USERDEBUG': 'googlestorage_acl_x86.txt',
    'X86_NDK_TRANSLATION_USERDEBUG': 'googlestorage_acl_internal.txt',
    'X86_64_USERDEBUG': 'googlestorage_acl_x86.txt',
    'AOSP_ARM_USERDEBUG': 'googlestorage_acl_arm.txt',
    'AOSP_X86_USERDEBUG': 'googlestorage_acl_x86.txt',
    'AOSP_X86_64_USERDEBUG': 'googlestorage_acl_x86.txt',
    'SDK_GOOGLE_X86_USERDEBUG': 'googlestorage_acl_x86.txt',
    'SDK_GOOGLE_X86_64_USERDEBUG': 'googlestorage_acl_x86.txt',
    'X86_INTERNAL': 'googlestorage_acl_internal.txt',
    'SDK_TOOLS': 'googlestorage_acl_public.txt',
    'XTS': 'googlestorage_acl_cts.txt',
}
ANDROID_SYMBOLS_URL_TEMPLATE = (
    ARC_BUCKET_URL +
    '/%(branch)s-linux-cheets_%(arch)s-user/%(version)s'
    '/cheets_%(arch)s-symbols-%(version)s.zip')
ANDROID_SYMBOLS_FILE = 'android-symbols.zip'
# x86-user, x86-userdebug and x86-eng builders create build artifacts with the
# same name, e.g. cheets_x86-target_files-${VERSION}.zip. Chrome OS builders
# that need to select x86-user or x86-userdebug artifacts at emerge time need
# the artifacts to have different filenames to avoid checksum failures. These
# targets will have their artifacts renamed when the PFQ copies them from the
# the Android bucket to the ARC++ bucket (b/33072485).
ARC_BUILDS_NEED_ARTIFACTS_RENAMED = {
    'ARM_USERDEBUG',
    'X86_NDK_TRANSLATION',
    'X86_USERDEBUG',
    'X86_NDK_TRANSLATION_USERDEBUG',
    'X86_64_USERDEBUG',
    'AOSP_ARM_USERDEBUG',
    'AOSP_X86_USERDEBUG',
    'AOSP_X86_64_USERDEBUG',
    'SDK_GOOGLE_X86_USERDEBUG',
    'SDK_GOOGLE_X86_64_USERDEBUG',
}
# All builds will have the same name without target prefix.
# Emerge checksum failures will be workarounded by ebuild rename symbol (->).
ARC_ARTIFACTS_RENAME_NOT_NEEDED = [
    'sepolicy.zip',
]

GOB_COOKIE_PATH = os.path.expanduser('~/.git-credential-cache/cookie')
GITCOOKIES_PATH = os.path.expanduser('~/.gitcookies')

# Timestamps in the JSON from GoB's web interface is of the form 'Tue
# Dec 02 17:48:06 2014' and is assumed to be in UTC.
GOB_COMMIT_TIME_FORMAT = '%a %b %d %H:%M:%S %Y'

CHROMITE_PROJECT = 'chromiumos/chromite'
CHROMITE_URL = '%s/%s' % (EXTERNAL_GOB_URL, CHROMITE_PROJECT)
CHROMIUM_SRC_PROJECT = 'chromium/src'
CHROMIUM_GOB_URL = '%s/%s.git' % (EXTERNAL_GOB_URL, CHROMIUM_SRC_PROJECT)
CHROME_INTERNAL_PROJECT = 'chrome/src-internal'
CHROME_INTERNAL_GOB_URL = '%s/%s.git' % (
    INTERNAL_GOB_URL, CHROME_INTERNAL_PROJECT)

DEFAULT_MANIFEST = 'default.xml'
OFFICIAL_MANIFEST = 'official.xml'
LKGM_MANIFEST = 'LKGM/lkgm.xml'

SHARED_CACHE_ENVVAR = 'CROS_CACHEDIR'
PARALLEL_EMERGE_STATUS_FILE_ENVVAR = 'PARALLEL_EMERGE_STATUS_FILE'

# These projects can be responsible for infra failures.
INFRA_PROJECTS = (CHROMITE_PROJECT,)

# The manifest contains extra attributes in the 'project' nodes to determine our
# branching strategy for the project.
#   create: Create a new branch on the project repo for the new CrOS branch.
#           This is the default.
#   pin: On the CrOS branch, pin the project to the current revision.
#   tot: On the CrOS branch, the project still tracks ToT.
MANIFEST_ATTR_BRANCHING = 'branch-mode'
MANIFEST_ATTR_BRANCHING_CREATE = 'create'
MANIFEST_ATTR_BRANCHING_PIN = 'pin'
MANIFEST_ATTR_BRANCHING_TOT = 'tot'
MANIFEST_ATTR_BRANCHING_ALL = (
    MANIFEST_ATTR_BRANCHING_CREATE,
    MANIFEST_ATTR_BRANCHING_PIN,
    MANIFEST_ATTR_BRANCHING_TOT,
)

STREAK_COUNTERS = 'streak_counters'

PATCH_BRANCH = 'patch_branch'
STABLE_EBUILD_BRANCH = 'stabilizing_branch'
MERGE_BRANCH = 'merge_branch'

# These branches are deleted at the beginning of every buildbot run.
CREATED_BRANCHES = [
    PATCH_BRANCH,
    STABLE_EBUILD_BRANCH,
    MERGE_BRANCH
]

# Default OS target packages.
TARGET_OS_PKG = 'virtual/target-os'
TARGET_OS_DEV_PKG = 'virtual/target-os-dev'
TARGET_OS_TEST_PKG = 'virtual/target-os-test'

# Constants for uprevving Chrome

CHROMEOS_BASE = 'chromeos-base'

# Portage category and package name for Chrome.
CHROME_PN = 'chromeos-chrome'
CHROME_CP = 'chromeos-base/%s' % CHROME_PN

# Other packages to uprev while uprevving Chrome.
OTHER_CHROME_PACKAGES = ['chromeos-base/chromium-source']

# Chrome use flags
USE_CHROME_INTERNAL = 'chrome_internal'
USE_AFDO_USE = 'afdo_use'


# Builds and validates _alpha ebuilds.  These builds sync to the latest
# revsion of the Chromium src tree and build with that checkout.
CHROME_REV_TOT = 'tot'

# Builds and validates chrome at a given revision through cbuildbot
# --chrome_version
CHROME_REV_SPEC = 'spec'

# Builds and validates the latest Chromium release as defined by
# ~/trunk/releases in the Chrome src tree.  These ebuilds are suffixed with rc.
CHROME_REV_LATEST = 'latest_release'

# Builds and validates the latest Chromium release for a specific Chromium
# branch that we want to watch.  These ebuilds are suffixed with rc.
CHROME_REV_STICKY = 'stable_release'

# Builds and validates Chromium for a pre-populated directory.
# Also uses _alpha, since portage doesn't have anything lower.
CHROME_REV_LOCAL = 'local'
VALID_CHROME_REVISIONS = [CHROME_REV_TOT, CHROME_REV_LATEST,
                          CHROME_REV_STICKY, CHROME_REV_LOCAL, CHROME_REV_SPEC]


# Constants for uprevving Android.

# Portage package name for Android container.
ANDROID_PACKAGE_NAME = 'android-container'

# Builds and validates the latest Android release.
ANDROID_REV_LATEST = 'latest_release'
VALID_ANDROID_REVISIONS = [ANDROID_REV_LATEST]


# Builder types supported
BAREMETAL_BUILD_SLAVE_TYPE = 'baremetal'
GCE_BEEFY_BUILD_SLAVE_TYPE = 'gce_beefy'
GCE_BUILD_SLAVE_TYPE = 'gce'

VALID_BUILD_SLAVE_TYPES = (
    BAREMETAL_BUILD_SLAVE_TYPE,
    GCE_BEEFY_BUILD_SLAVE_TYPE,
    GCE_BUILD_SLAVE_TYPE,
)

# Build types supported.

# TODO(sosa): Deprecate PFQ type.
# Incremental builds that are built using binary packages when available.
# These builds have less validation than other build types.
INCREMENTAL_TYPE = 'binary'

# These builds serve as PFQ builders.  This is being deprecated.
PFQ_TYPE = 'pfq'

# Hybrid Commit and PFQ type.  Ultimate protection.  Commonly referred to
# as simply "commit queue" now.
PALADIN_TYPE = 'paladin'

# A builder that kicks off Pre-CQ builders that bless the purest CLs.
PRE_CQ_LAUNCHER_TYPE = 'priest'

# Chrome PFQ type.  Incremental build type that builds and validates new
# versions of Chrome.  Only valid if set with CHROME_REV.  See
# VALID_CHROME_REVISIONS for more information.
CHROME_PFQ_TYPE = 'chrome'

# Android PFQ type.  Builds and validates new versions of Android.
ANDROID_PFQ_TYPE = 'android'

# Builds from source and non-incremental.  This builds fully wipe their
# chroot before the start of every build and no not use a BINHOST.
BUILD_FROM_SOURCE_TYPE = 'full'

# Full but with versioned logic.
CANARY_TYPE = 'canary'

# Generate payloads for an already built build/version.
PAYLOADS_TYPE = 'payloads'

# Similar behavior to canary, but used to validate toolchain changes.
TOOLCHAIN_TYPE = 'toolchain'

BRANCH_UTIL_CONFIG = 'branch-util'

# Generic type of tryjob only build configs.
TRYJOB_TYPE = 'tryjob'

# Special build type for Chroot builders.  These builds focus on building
# toolchains and validate that they work.
CHROOT_BUILDER_TYPE = 'chroot'
CHROOT_BUILDER_BOARD = 'amd64-host'

# Use for builds that don't requite a type.
GENERIC_TYPE = 'generic'

# Build type of Pre-CQs
PRE_CQ_TYPE = 'pre_cq'

VALID_BUILD_TYPES = (
    PALADIN_TYPE,
    INCREMENTAL_TYPE,
    BUILD_FROM_SOURCE_TYPE,
    CANARY_TYPE,
    CHROOT_BUILDER_TYPE,
    CHROOT_BUILDER_BOARD,
    CHROME_PFQ_TYPE,
    ANDROID_PFQ_TYPE,
    PFQ_TYPE,
    PRE_CQ_LAUNCHER_TYPE,
    PAYLOADS_TYPE,
    TOOLCHAIN_TYPE,
    TRYJOB_TYPE,
    GENERIC_TYPE,
    PRE_CQ_TYPE
)

# The default list of pre-cq configs to use.
PRE_CQ_DEFAULT_CONFIGS = [
    # Betty is the designated board to run vmtest on N.
    'betty-pre-cq',                   # vm board                  vmtest
    'cyan-no-vmtest-pre-cq',          # braswell     kernel 3.18
    'daisy_spring-no-vmtest-pre-cq',  # arm32        kernel 3.8
    'eve-no-vmtest-pre-cq',           # kabylake     kernel 4.4   cheets_user_64
    'kevin-arcnext-no-vmtest-pre-cq', # arm64        kernel 4.4   arcnext
    'nyan_blaze-no-vmtest-pre-cq',    # arm32        kernel 3.10
    'reef-no-vmtest-pre-cq',          # apollolake   kernel 4.4   vulkan
    'samus-no-vmtest-pre-cq',         # broadwell    kernel 3.14
    'whirlwind-no-vmtest-pre-cq',     # brillo
    'zako-no-vmtest-pre-cq',          # haswell      kernel 3.8
]

# The name of the pre-cq launching config.
PRE_CQ_LAUNCHER_CONFIG = 'pre-cq-launcher'

# The name of the Pre-CQ launcher on the waterfall.
# As of crbug.com/591117 this is the same as the config name.
PRE_CQ_LAUNCHER_NAME = PRE_CQ_LAUNCHER_CONFIG

CQ_CONFIG_FILENAME = 'COMMIT-QUEUE.ini'
CQ_CONFIG_SECTION_GENERAL = 'GENERAL'
CQ_CONFIG_IGNORED_STAGES = 'ignored-stages'
CQ_CONFIG_SUBMIT_IN_PRE_CQ = 'submit-in-pre-cq'
CQ_CONFIG_SUBSYSTEM = 'subsystem'
CQ_CONFIG_UNION_PRE_CQ_SUB_CONFIGS = 'union-pre-cq-sub-configs'

# The COMMIT-QUEUE.ini and commit message option that overrides pre-cq configs
# to test with.
CQ_CONFIG_PRE_CQ_CONFIGS = 'pre-cq-configs'
CQ_CONFIG_PRE_CQ_CONFIGS_REGEX = CQ_CONFIG_PRE_CQ_CONFIGS + ':'

# Define pool of machines for Hardware tests.
HWTEST_TRYBOT_NUM = 3
HWTEST_MACH_POOL = 'bvt'
HWTEST_MACH_POOL_UNI = 'bvt-uni'
HWTEST_PALADIN_POOL = 'cq'
HWTEST_TOT_PALADIN_POOL = 'tot-cq'
HWTEST_PFQ_POOL = 'pfq'
HWTEST_SUITES_POOL = 'suites'
HWTEST_CHROME_PERF_POOL = 'chromeperf'
HWTEST_TRYBOT_POOL = HWTEST_SUITES_POOL
HWTEST_WIFICELL_PRE_CQ_POOL = 'wificell-pre-cq'
HWTEST_BLUESTREAK_PRE_CQ_POOL = 'bluestreak-pre-cq'
HWTEST_CONTINUOUS_POOL = 'continuous'
HWTEST_CTS_POOL = 'cts'
HWTEST_GTS_POOL = HWTEST_CTS_POOL


# How many total test retries should be done for a suite.
HWTEST_MAX_RETRIES = 5

# Defines for the various hardware test suites:
#   BVT:  Basic blocking suite to be run against any build that
#       requires a HWTest phase.
#   COMMIT:  Suite of basic tests required for commits to the source
#       tree.  Runs as a blocking suite on the CQ and PFQ; runs as
#       a non-blocking suite on canaries.
#   CANARY:  Non-blocking suite run only against the canaries.
#   AFDO:  Non-blocking suite run only AFDO builders.
#   MOBLAB: Blocking Suite run only on *_moblab builders.
#   INSTALLER: Blocking suite run against all canaries; tests basic installer
#              functionality.
HWTEST_ARC_COMMIT_SUITE = 'bvt-arc'
HWTEST_ARC_CANARY_SUITE = 'arc-bvt-perbuild'
HWTEST_BVT_SUITE = 'bvt-inline'
HWTEST_COMMIT_SUITE = 'bvt-cq'
HWTEST_CANARY_SUITE = 'bvt-perbuild'
HWTEST_INSTALLER_SUITE = 'bvt-installer'
HWTEST_AFDO_SUITE = 'AFDO_record'
HWTEST_JETSTREAM_COMMIT_SUITE = 'jetstream_cq'
HWTEST_MOBLAB_SUITE = 'moblab'
HWTEST_MOBLAB_QUICK_SUITE = 'moblab_quick'
HWTEST_SANITY_SUITE = 'sanity'
HWTEST_TOOLCHAIN_SUITE = 'toolchain-tests'
HWTEST_PROVISION_SUITE = 'provision'
HWTEST_CTS_FOLLOWER_SUITE = 'arc-cts-follower'
HWTEST_CTS_QUAL_SUITE = 'arc-cts-qual'
HWTEST_GTS_QUAL_SUITE = 'arc-gts-qual'
HWTEST_CTS_PRIORITY = 11
HWTEST_GTS_PRIORITY = HWTEST_CTS_PRIORITY
# Non-blocking informational hardware tests for Chrome, run throughout the
# day on tip-of-trunk Chrome rather than on the daily Chrome branch.
HWTEST_CHROME_INFORMATIONAL = 'chrome-informational'

# Additional timeout to wait for autotest to abort a suite if the test takes
# too long to run. This is meant to be overly conservative as a timeout may
# indicate that autotest is at capacity.
HWTEST_TIMEOUT_EXTENSION = 10 * 60

HWTEST_DEFAULT_PRIORITY = 'DEFAULT'
HWTEST_CQ_PRIORITY = 'CQ'
HWTEST_BUILD_PRIORITY = 'Build'
HWTEST_PFQ_PRIORITY = 'PFQ'
HWTEST_POST_BUILD_PRIORITY = 'PostBuild'

# Ordered by priority (first item being lowest).
HWTEST_VALID_PRIORITIES = ['Weekly',
                           'Daily',
                           HWTEST_POST_BUILD_PRIORITY,
                           HWTEST_DEFAULT_PRIORITY,
                           HWTEST_BUILD_PRIORITY,
                           HWTEST_PFQ_PRIORITY,
                           HWTEST_CQ_PRIORITY]

# Creates a mapping of priorities to make easy comparsions.
# Use the same priorities mapping as autotest/client/common_lib/priorities.py
HWTEST_PRIORITIES_MAP = {key: 10 + 10 * index
                         for index, key in enumerate(HWTEST_VALID_PRIORITIES)}


# HWTest result statuses
HWTEST_STATUS_PASS = 'pass'
HWTEST_STATUS_FAIL = 'fail'
HWTEST_STATUS_ABORT = 'abort'
HWTEST_STATUS_OTHER = 'other'
HWTEST_STATUES_NOT_PASSED = frozenset([HWTEST_STATUS_FAIL,
                                       HWTEST_STATUS_ABORT,
                                       HWTEST_STATUS_OTHER])

# Define HWTEST subsystem logic constants.
SUBSYSTEMS = 'subsystems'
SUBSYSTEM_PASS = 'subsystem_pass'
SUBSYSTEM_FAIL = 'subsystem_fail'
SUBSYSTEM_UNUSED = 'subsystem_unused'

# Build messages
MESSAGE_TYPE_IGNORED_REASON = 'ignored_reason'
MESSAGE_TYPE_ANNOTATIONS_FINALIZED = 'annotations_finalized'
# MESSSGE_TYPE_IGNORED_REASON messages store the affected build as
# the CIDB column message_value.
MESSAGE_SUBTYPE_SELF_DESTRUCTION = 'self_destruction'

# Define HWTEST job_keyvals
JOB_KEYVAL_DATASTORE_PARENT_KEY = 'datastore_parent_key'
JOB_KEYVAL_CIDB_BUILD_ID = 'cidb_build_id'
JOB_KEYVAL_CIDB_BUILD_STAGE_ID = 'cidb_build_stage_id'


# How many total test retries should be done for a suite.
VM_TEST_MAX_RETRIES = 5
# Defines VM Test types.
FULL_AU_TEST_TYPE = 'full_suite'
SIMPLE_AU_TEST_TYPE = 'pfq_suite'
VM_SUITE_TEST_TYPE = 'vm_suite'
GCE_SUITE_TEST_TYPE = 'gce_suite'
CROS_VM_TEST_TYPE = 'cros_vm_test'
DEV_MODE_TEST_TYPE = 'dev_mode_test'
VALID_VM_TEST_TYPES = [FULL_AU_TEST_TYPE, SIMPLE_AU_TEST_TYPE,
                       VM_SUITE_TEST_TYPE, GCE_SUITE_TEST_TYPE,
                       CROS_VM_TEST_TYPE, DEV_MODE_TEST_TYPE]
VALID_GCE_TEST_SUITES = ['gce-smoke', 'gce-sanity']
# MoblabVM tests are suites of tests used to validate a moblab image via
# VMTests.
MOBLAB_VM_SMOKE_TEST_TYPE = 'moblab_smoke_test'

CHROMIUMOS_OVERLAY_DIR = 'src/third_party/chromiumos-overlay'
VERSION_FILE = os.path.join(CHROMIUMOS_OVERLAY_DIR,
                            'chromeos/config/chromeos_version.sh')
SDK_VERSION_FILE = os.path.join(CHROMIUMOS_OVERLAY_DIR,
                                'chromeos/binhost/host/sdk_version.conf')
SDK_GS_BUCKET = 'chromiumos-sdk'

PUBLIC = 'public'
PRIVATE = 'private'

BOTH_OVERLAYS = 'both'
PUBLIC_OVERLAYS = PUBLIC
PRIVATE_OVERLAYS = PRIVATE
VALID_OVERLAYS = [BOTH_OVERLAYS, PUBLIC_OVERLAYS, PRIVATE_OVERLAYS, None]

# Common default logging settings for use with the logging module.
LOGGER_FMT = '%(asctime)s: %(levelname)s: %(message)s'
LOGGER_DATE_FMT = '%H:%M:%S'

# Used by remote patch serialization/deserialzation.
INTERNAL_PATCH_TAG = 'i'
EXTERNAL_PATCH_TAG = 'e'
PATCH_TAGS = (INTERNAL_PATCH_TAG, EXTERNAL_PATCH_TAG)

# Tree status strings
TREE_OPEN = 'open'
TREE_THROTTLED = 'throttled'
TREE_CLOSED = 'closed'
TREE_MAINTENANCE = 'maintenance'
# The statuses are listed in the order of increasing severity.
VALID_TREE_STATUSES = (TREE_OPEN, TREE_THROTTLED, TREE_CLOSED, TREE_MAINTENANCE)


# Common parts of query used for CQ, THROTTLED_CQ, and PRECQ.
# "NOT is:draft" in this query doesn't work, it finds any non-draft revision.
# We want to match drafts anyway, so we can comment on them.
_QUERIES = {
    # CLs that are open and not vetoed.
    'open': 'status:open AND -label:CodeReview=-2 AND -label:Verified=-1',

    # CLs that are approved and verified.
    'approved': 'label:Code-Review=+2 AND label:Verified=+1',
}

#
# Please note that requiring the +2 code review (or Trybot-Ready) for all CQ
# and PreCQ runs is a security requirement. Otherwise arbitrary people can
# run code on our servers.
#
# The Verified and Commit-Queue flags can be set by any registered user (you
# don't need commit access to set them.)
#


# Default gerrit query used to find changes for CQ.
CQ_READY_QUERY = (
    '%(open)s AND %(approved)s AND label:Commit-Queue>=1' % _QUERIES,
    lambda change: change.IsMergeable())

# The PreCQ does not require the CQ bit to be set if it's a recent CL, or if
# the Trybot-Ready flag has been set.
PRECQ_READY_QUERY = (
    '%(open)s AND (%(approved)s AND label:Commit-Queue>=1 OR '
    'label:Code-Review=+2 AND -age:2h OR label:Trybot-Ready=+1)' % _QUERIES,
    lambda change: (not change.IsBeingMerged() and
                    change.HasApproval('CRVW', '2') or
                    change.HasApproval('TRY', '1')))

GERRIT_ON_BORG_LABELS = {
    'Code-Review': 'CRVW',
    'Commit-Queue': 'COMR',
    'Verified': 'VRIF',
    'Trybot-Ready': 'TRY',
}

# Actions that a CQ run can take on a CL
CL_ACTION_PICKED_UP = 'picked_up'         # CL picked up in CommitQueueSync
CL_ACTION_SUBMITTED = 'submitted'         # CL submitted successfully
CL_ACTION_KICKED_OUT = 'kicked_out'       # CL CQ-Ready value set to zero
CL_ACTION_SUBMIT_FAILED = 'submit_failed' # CL submitted but submit failed
CL_ACTION_VERIFIED = 'verified'           # CL was verified by the builder
CL_ACTION_FORGIVEN = 'forgiven'           # Build failed, but CL not kicked out
CL_ACTION_EXONERATED = 'exonerated'       # CL was kicked out, then exonerated.

# Actions the Pre-CQ Launcher can take on a CL
# See cbuildbot/stages/sync_stages.py:PreCQLauncherStage for more info
CL_ACTION_PRE_CQ_INFLIGHT = 'pre_cq_inflight'
CL_ACTION_PRE_CQ_PASSED = 'pre_cq_passed'
CL_ACTION_PRE_CQ_FAILED = 'pre_cq_failed'
CL_ACTION_PRE_CQ_LAUNCHING = 'pre_cq_launching'
CL_ACTION_PRE_CQ_WAITING = 'pre_cq_waiting'
CL_ACTION_PRE_CQ_FULLY_VERIFIED = 'pre_cq_fully_verified'
CL_ACTION_PRE_CQ_READY_TO_SUBMIT = 'pre_cq_ready_to_submit'
# Recording this action causes the pre-cq status and all per-config statuses to
# be reset.
CL_ACTION_PRE_CQ_RESET = 'pre_cq_reset'

# Miscellaneous actions

# Recorded by pre-cq launcher for a change when it is noticed that a previously
# rejected change is again in the queue.
# This is a best effort detection for developers re-marking their changes, to
# help calculate true CQ handling time. It is susceptible to developers
# un-marking their change after is requeued or to the CQ picking up a CL before
# it is seen by the pre-cq-launcher.
CL_ACTION_REQUEUED = 'requeued'

# Recorded by pre-cq launcher when it begins handling a change that isn't marked
# as CQ+1. This indicates that all actions between this and the next
# CL_ACTION_REQUEUED action have occured on a non-CQ+1 change.
CL_ACTION_SPECULATIVE = 'speculative'

# Recorded by pre-cq launcher when it has screened a change for necessary
# tryjobs
CL_ACTION_SCREENED_FOR_PRE_CQ = 'screened_for_pre_cq'
# Recorded by pre-cq launcher for each tryjob config necessary to validate
# a change, with |reason| field specifying the config.
CL_ACTION_VALIDATION_PENDING_PRE_CQ = 'validation_pending_pre_cq'

# Recorded by CQ slaves builds when a picked-up CL is determined to be
# irrelevant to that slave build.
CL_ACTION_IRRELEVANT_TO_SLAVE = 'irrelevant_to_slave'

# Recorded by CQ slaves builds when a picked-up CL is determined to be
# relevant to that slave build.
CL_ACTION_RELEVANT_TO_SLAVE = 'relevant_to_slave'

# Recorded by pre-cq-launcher when it launches a tryjob with a particular
# config. The |reason| field of the action will be the config.
CL_ACTION_TRYBOT_LAUNCHING = 'trybot_launching'

# Recorded by pre-cq-launcher when it cancels a trybot.
CL_ACTION_TRYBOT_CANCELLED = 'trybot_cancelled'


CL_ACTIONS = (CL_ACTION_PICKED_UP,
              CL_ACTION_SUBMITTED,
              CL_ACTION_KICKED_OUT,
              CL_ACTION_SUBMIT_FAILED,
              CL_ACTION_VERIFIED,
              CL_ACTION_PRE_CQ_INFLIGHT,
              CL_ACTION_PRE_CQ_PASSED,
              CL_ACTION_PRE_CQ_FAILED,
              CL_ACTION_PRE_CQ_LAUNCHING,
              CL_ACTION_PRE_CQ_WAITING,
              CL_ACTION_PRE_CQ_READY_TO_SUBMIT,
              CL_ACTION_REQUEUED,
              CL_ACTION_SCREENED_FOR_PRE_CQ,
              CL_ACTION_VALIDATION_PENDING_PRE_CQ,
              CL_ACTION_IRRELEVANT_TO_SLAVE,
              CL_ACTION_RELEVANT_TO_SLAVE,
              CL_ACTION_TRYBOT_LAUNCHING,
              CL_ACTION_SPECULATIVE,
              CL_ACTION_FORGIVEN,
              CL_ACTION_EXONERATED,
              CL_ACTION_PRE_CQ_FULLY_VERIFIED,
              CL_ACTION_PRE_CQ_RESET,
              CL_ACTION_TRYBOT_CANCELLED)

# Actions taken by a builder when making a decision about a CL.
CL_DECISION_ACTIONS = (
    CL_ACTION_SUBMITTED,
    CL_ACTION_KICKED_OUT,
    CL_ACTION_SUBMIT_FAILED,
    CL_ACTION_VERIFIED,
    CL_ACTION_FORGIVEN
)

# Per-config status strings for a CL.
CL_PRECQ_CONFIG_STATUS_PENDING = 'pending'
CL_PRECQ_CONFIG_STATUS_LAUNCHED = 'launched'
CL_PRECQ_CONFIG_STATUS_INFLIGHT = CL_STATUS_INFLIGHT
CL_PRECQ_CONFIG_STATUS_FAILED = BUILDER_STATUS_FAILED
CL_PRECQ_CONFIG_STATUS_VERIFIED = CL_ACTION_VERIFIED
CL_PRECQ_CONFIG_STATUSES = (CL_PRECQ_CONFIG_STATUS_PENDING,
                            CL_PRECQ_CONFIG_STATUS_LAUNCHED,
                            CL_PRECQ_CONFIG_STATUS_INFLIGHT,
                            CL_PRECQ_CONFIG_STATUS_FAILED,
                            CL_PRECQ_CONFIG_STATUS_VERIFIED)

# CL submission, rejection, or forgiven reasons (i.e. strategies).
STRATEGY_CQ_SUCCESS = 'strategy:cq-success'
STRATEGY_PRECQ_SUBMIT = 'strategy:pre-cq-submit'
STRATEGY_NONMANIFEST = 'strategy:non-manifest-submit'

# Strategy for CQ partial pool submission
STRATEGY_CQ_PARTIAL_NOT_TESTED = 'strategy:cq-submit-partial-pool-not-tested'
STRATEGY_CQ_PARTIAL_CQ_HISTORY = 'strategy:cq-submit-partial-pool-cq-history'
STRATEGY_CQ_PARTIAL_IGNORED_STAGES = (
    'strategy:cq-submit-partial-pool-ignored-stages')
STRATEGY_CQ_PARTIAL_SUBSYSTEM = 'strategy:cq-submit-partial-pool-pass-subsystem'
STRATEGY_CQ_PARTIAL_BUILDS_PASSED = (
    'strategy:cq-submit-partial-pool-builds-passed')

# A dict mapping CQ partial pool submission strategies to their priorities;
# lower values have higher priorities.
STRATEGY_CQ_PARTIAL_REASONS = {
    STRATEGY_CQ_PARTIAL_NOT_TESTED: 1,
    STRATEGY_CQ_PARTIAL_CQ_HISTORY: 2,
    STRATEGY_CQ_PARTIAL_IGNORED_STAGES: 3,
    STRATEGY_CQ_PARTIAL_SUBSYSTEM: 4,
    STRATEGY_CQ_PARTIAL_BUILDS_PASSED: 5
}

# CQ types.
CQ = 'cq'
PRE_CQ = 'pre-cq'

# Environment variables that should be exposed to all children processes
# invoked via cros_build_lib.RunCommand.
ENV_PASSTHRU = ('CROS_SUDO_KEEP_ALIVE', SHARED_CACHE_ENVVAR,
                PARALLEL_EMERGE_STATUS_FILE_ENVVAR)

# List of variables to proxy into the chroot from the host, and to
# have sudo export if existent. Anytime this list is modified, a new
# chroot_version_hooks.d upgrade script that symlinks to 45_rewrite_sudoers.d
# should be created.
CHROOT_ENVIRONMENT_WHITELIST = (
    'CHROMEOS_OFFICIAL',
    'CHROMEOS_VERSION_AUSERVER',
    'CHROMEOS_VERSION_DEVSERVER',
    'CHROMEOS_VERSION_TRACK',
    'GCC_GITHASH',
    'GIT_AUTHOR_EMAIL',
    'GIT_AUTHOR_NAME',
    'GIT_COMMITTER_EMAIL',
    'GIT_COMMITTER_NAME',
    'GIT_PROXY_COMMAND',
    'GIT_SSH',
    'RSYNC_PROXY',
    'SSH_AGENT_PID',
    'SSH_AUTH_SOCK',
    'TMUX',
    'USE',
    'all_proxy',
    'ftp_proxy',
    'http_proxy',
    'https_proxy',
    'no_proxy',
)

# Paths for Chrome LKGM which are relative to the Chromium base url.
CHROME_LKGM_FILE = 'CHROMEOS_LKGM'
PATH_TO_CHROME_LKGM = 'chromeos/%s' % CHROME_LKGM_FILE
# Path for the Chrome LKGM's closest OWNERS file.
PATH_TO_CHROME_CHROMEOS_OWNERS = 'chromeos/OWNERS'

# Cache constants.
COMMON_CACHE = 'common'

# Artifact constants.
def _SlashToUnderscore(string):
  return string.replace('/', '_')

# GCE tar ball constants.
def ImageBinToGceTar(image_bin):
  assert image_bin.endswith('.bin'), ('Filename %s does not end with ".bin"' %
                                      image_bin)
  return '%s_gce.tar.gz' % os.path.splitext(image_bin)[0]

RELEASE_BUCKET = 'gs://chromeos-releases'
TRASH_BUCKET = 'gs://chromeos-throw-away-bucket'
CHROME_SYSROOT_TAR = 'sysroot_%s.tar.xz' % _SlashToUnderscore(CHROME_CP)
CHROME_ENV_TAR = 'environment_%s.tar.xz' % _SlashToUnderscore(CHROME_CP)
CHROME_ENV_FILE = 'environment'
BASE_IMAGE_NAME = 'chromiumos_base_image'
BASE_IMAGE_TAR = '%s.tar.xz' % BASE_IMAGE_NAME
BASE_IMAGE_BIN = '%s.bin' % BASE_IMAGE_NAME
BASE_IMAGE_GCE_TAR = ImageBinToGceTar(BASE_IMAGE_BIN)
IMAGE_SCRIPTS_NAME = 'image_scripts'
IMAGE_SCRIPTS_TAR = '%s.tar.xz' % IMAGE_SCRIPTS_NAME
TARGET_SYSROOT_TAR = 'sysroot_%s.tar.xz' % _SlashToUnderscore(TARGET_OS_PKG)
VM_IMAGE_NAME = 'chromiumos_qemu_image'
VM_IMAGE_BIN = '%s.bin' % VM_IMAGE_NAME
VM_IMAGE_TAR = '%s.tar.xz' % VM_IMAGE_NAME
VM_DISK_PREFIX = 'chromiumos_qemu_disk.bin'
VM_MEM_PREFIX = 'chromiumos_qemu_mem.bin'
VM_NUM_RETRIES = 1
TAST_VM_TEST_RESULTS = 'tast_vm_test_results_%(attempt)s'

TEST_IMAGE_NAME = 'chromiumos_test_image'
TEST_IMAGE_TAR = '%s.tar.xz' % TEST_IMAGE_NAME
TEST_IMAGE_BIN = '%s.bin' % TEST_IMAGE_NAME
TEST_IMAGE_GCE_TAR = ImageBinToGceTar(TEST_IMAGE_BIN)
TEST_KEY_PRIVATE = 'id_rsa'
TEST_KEY_PUBLIC = 'id_rsa.pub'

DEV_IMAGE_NAME = 'chromiumos_image'
DEV_IMAGE_BIN = '%s.bin' % DEV_IMAGE_NAME

RECOVERY_IMAGE_NAME = 'recovery_image'
RECOVERY_IMAGE_BIN = '%s.bin' % RECOVERY_IMAGE_NAME
RECOVERY_IMAGE_TAR = '%s.tar.xz' % RECOVERY_IMAGE_NAME

# Image type constants.
IMAGE_TYPE_BASE = 'base'
IMAGE_TYPE_DEV = 'dev'
IMAGE_TYPE_TEST = 'test'
IMAGE_TYPE_RECOVERY = 'recovery'
IMAGE_TYPE_FACTORY = 'factory'
IMAGE_TYPE_FIRMWARE = 'firmware'
# NVidia Tegra SoC resume firmware blob.
IMAGE_TYPE_NV_LP0_FIRMWARE = 'nv_lp0_firmware'
# USB PD accessory microcontroller firmware (e.g. power brick, display dongle).
IMAGE_TYPE_ACCESSORY_USBPD = 'accessory_usbpd'
# Standalone accessory microcontroller firmware (e.g. wireless keyboard).
IMAGE_TYPE_ACCESSORY_RWSIG = 'accessory_rwsig'

IMAGE_TYPE_TO_NAME = {
    IMAGE_TYPE_BASE: BASE_IMAGE_BIN,
    IMAGE_TYPE_DEV: DEV_IMAGE_BIN,
    IMAGE_TYPE_RECOVERY: RECOVERY_IMAGE_BIN,
    IMAGE_TYPE_TEST: TEST_IMAGE_BIN,
}
IMAGE_NAME_TO_TYPE = dict((v, k) for k, v in IMAGE_TYPE_TO_NAME.iteritems())

METADATA_JSON = 'metadata.json'
PARTIAL_METADATA_JSON = 'partial-metadata.json'
METADATA_TAGS = 'tags'
DELTA_SYSROOT_TAR = 'delta_sysroot.tar.xz'
DELTA_SYSROOT_BATCH = 'batch'

# Global configuration constants.
CHROMITE_CONFIG_DIR = os.path.expanduser('~/.chromite')
CHROME_SDK_BASHRC = os.path.join(CHROMITE_CONFIG_DIR, 'chrome_sdk.bashrc')
SYNC_RETRIES = 4
SLEEP_TIMEOUT = 30

# Lab status url.
LAB_STATUS_URL = 'http://chromiumos-lab.appspot.com/current?format=json'

GOLO_SMTP_SERVER = 'mail.golo.chromium.org'

# Valid sherrif types.
CHROME_GARDENER = 'chrome'

# URLs to retrieve sheriff names from the waterfall.
CHROME_GARDENER_URL = '%s/sheriff_cr_cros_gardeners.js' % (BUILD_DASHBOARD)

SHERIFF_TYPE_TO_URL = {
    CHROME_GARDENER: (CHROME_GARDENER_URL,)
}


# Useful config targets.
CQ_MASTER = 'master-paladin'
CANARY_MASTER = 'master-release'
PFQ_MASTER = 'master-chromium-pfq'
BINHOST_PRE_CQ = 'binhost-pre-cq'
WIFICELL_PRE_CQ = 'wificell-pre-cq'
BLUESTREAK_PRE_CQ = 'bluestreak-pre-cq'
MST_ANDROID_PFQ_MASTER = 'master-mst-android-pfq'
NYC_ANDROID_PFQ_MASTER = 'master-nyc-android-pfq'
PI_ANDROID_PFQ_MASTER = 'master-pi-android-pfq'
TOOLCHAIN_MASTTER = 'master-toolchain'


# Email validation regex. Not quite fully compliant with RFC 2822, but good
# approximation.
EMAIL_REGEX = r'[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,4}'

# Blacklist of files not allowed to be uploaded into the Partner Project Google
# Storage Buckets:
# debug.tgz contains debug symbols.
# manifest.xml exposes all of our repo names.
# vm_test_results can contain symbolicated crash dumps.
EXTRA_BUCKETS_FILES_BLACKLIST = [
    'debug.tgz',
    'manifest.xml',
    'vm_test_results_*'
]

# AFDO common constants.
# How long does the AFDO_record autotest have to generate the AFDO perf data.
AFDO_GENERATE_TIMEOUT = 100 * 60

# Manual Uprev PFQ constants.
STAGING_PFQ_BRANCH_PREFIX = 'staging_pfq_branch_'
PFQ_REF = 'pfq'

# Gmail Credentials.
GMAIL_TOKEN_CACHE_FILE = os.path.expanduser('~/.gmail_credentials')
GMAIL_TOKEN_JSON_FILE = '/creds/refresh_tokens/chromeos_gmail_alerts'

# Maximum number of boards per release group builder. This should be
# chosen/adjusted based on expected release build times such that successive
# builds don't overlap and create a backlog.
MAX_RELEASE_GROUP_BOARDS = 4

CHROMEOS_SERVICE_ACCOUNT = os.path.join('/', 'creds', 'service_accounts',
                                        'service-account-chromeos.json')

# Buildbucket buckets
CHROMEOS_RELEASE_BUILDBUCKET_BUCKET = 'master.chromeos_release'
CHROMEOS_BUILDBUCKET_BUCKET = 'master.chromeos'
CHROMIUMOS_BUILDBUCKET_BUCKET = 'master.chromiumos'
INTERNAL_SWARMING_BUILDBUCKET_BUCKET = 'luci.chromeos.general'

ACTIVE_BUCKETS = [
    CHROMEOS_RELEASE_BUILDBUCKET_BUCKET,
    CHROMEOS_BUILDBUCKET_BUCKET,
    CHROMIUMOS_BUILDBUCKET_BUCKET,
    INTERNAL_SWARMING_BUILDBUCKET_BUCKET,
]

# Build retry limit on buildbucket
BUILDBUCKET_BUILD_RETRY_LIMIT = 2

# TODO(nxia): consolidate all run.metadata key constants,
# add a unit test to avoid duplicated keys in run_metadata

# Builder_run metadata keys
METADATA_SCHEDULED_IMPORTANT_SLAVES = 'scheduled_important_slaves'
METADATA_SCHEDULED_EXPERIMENTAL_SLAVES = 'scheduled_experimental_slaves'
METADATA_UNSCHEDULED_SLAVES = 'unscheduled_slaves'
# List of builders marked as experimental through the tree status, not all the
# experimental builders for a run.
METADATA_EXPERIMENTAL_BUILDERS = 'experimental_builders'

# Metadata key to indicate whether a build is self-destructed.
SELF_DESTRUCTED_BUILD = 'self_destructed_build'

# Metadata key to indicate whether a build is self-destructed with success.
SELF_DESTRUCTED_WITH_SUCCESS_BUILD = 'self_destructed_with_success_build'

# The path to update_payload in the update_engine.
UPDATE_ENGINE_SCRIPTS_PATH = os.path.join(SOURCE_ROOT, 'src', 'aosp', 'system',
                                          'update_engine', 'scripts')

# Chroot snapshot names
CHROOT_SNAPSHOT_CLEAN = 'clean-chroot'
