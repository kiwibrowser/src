#!/usr/bin/env python
#
# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

from __future__ import print_function

import argparse
import contextlib
import multiprocessing
import os
import operator
import posixpath
import signal
import subprocess
import sys
import time
import xml.etree.cElementTree as ElementTree

import logging

# Shared functions across gdbclient.py and ndk-gdb.py.
# ndk-gdb is installed to $NDK/prebuilt/<platform>/bin
NDK_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../..'))
sys.path.append(os.path.join(NDK_PATH, "python-packages"))
import gdbrunner


def log(msg):
    logger = logging.getLogger(__name__)
    logger.info(msg)


def error(msg):
    sys.exit("ERROR: {}".format(msg))


class ArgumentParser(gdbrunner.ArgumentParser):
    def __init__(self):
        super(ArgumentParser, self).__init__()
        self.add_argument(
            "--verbose", "-v", action="store_true",
            help="enable verbose mode")

        self.add_argument(
            "--force", "-f", action="store_true",
            help="kill existing debug session if it exists")

        self.add_argument(
            "--port", type=int, nargs="?", default="5039",
            help="override the port used on the host")

        self.add_argument(
            "--delay", type=float, default=0.25,
            help="delay in seconds to wait after starting activity.\n"
                 "defaults to 0.25, higher values may be needed on slower devices.")

        self.add_argument(
            "-p", "--project", dest="project",
            help="specify application project path")

        app_group = self.add_argument_group("target selection")
        start_group = app_group.add_mutually_exclusive_group()

        start_group.add_argument(
            "--attach", nargs='?', dest="package_name", metavar="PKG_NAME",
            help="attach to application (default)\n"
                 "autodetects PKG_NAME if not specified")

        # NB: args.launch can be False (--attach), None (--launch), or a string
        start_group.add_argument(
            "--launch", nargs='?', dest="launch", default=False,
            metavar="ACTIVITY",
            help="launch application activity\n"
                 "launches main activity if ACTIVITY not specified")

        start_group.add_argument(
            "--launch-list", action="store_true",
            help="list all launchable activity names from manifest")

        debug_group = self.add_argument_group("debugging options")
        debug_group.add_argument(
            "-x", "--exec", dest="exec_file",
            help="execute gdb commands in EXEC_FILE after connection")

        debug_group.add_argument(
            "--nowait", action="store_true",
            help="do not wait for debugger to attach (may miss early JNI "
                 "breakpoints)")

        if sys.platform.startswith("win"):
            tui_help = argparse.SUPPRESS
        else:
            tui_help = "use GDB's tui mode"

        debug_group.add_argument(
            "-t", "--tui", action="store_true", dest="tui",
            help=tui_help)

        debug_group.add_argument(
            "--stdcxx-py-pr", dest="stdcxxpypr",
            help="use C++ library pretty-printer",
            choices=["auto", "none", "gnustl", "stlport"],
            default="auto")


def extract_package_name(xmlroot):
    if "package" in xmlroot.attrib:
        return xmlroot.attrib["package"]
    error("Failed to find package name in AndroidManifest.xml")


ANDROID_XMLNS = "{http://schemas.android.com/apk/res/android}"
def extract_launchable(xmlroot):
    '''
    A given application can have several activities, and each activity
    can have several intent filters. We want to only list, in the final
    output, the activities which have a intent-filter that contains the
    following elements:

      <action android:name="android.intent.action.MAIN" />
      <category android:name="android.intent.category.LAUNCHER" />
    '''
    launchable_activities = []
    application = xmlroot.findall("application")[0]

    main_action = "android.intent.action.MAIN"
    launcher_category = "android.intent.category.LAUNCHER"
    name_attrib = "{}name".format(ANDROID_XMLNS)

    for activity in application.iter("activity"):
        if name_attrib not in activity.attrib:
            continue

        for intent_filter in activity.iter("intent-filter"):
            found_action = False
            found_category = False
            for child in intent_filter:
                if child.tag == "action":
                    if not found_action and name_attrib in child.attrib:
                        if child.attrib[name_attrib] == main_action:
                            found_action = True
                if child.tag == "category":
                    if not found_category and name_attrib in child.attrib:
                        if child.attrib[name_attrib] == launcher_category:
                            found_category = True
            if found_action and found_category:
                launchable_activities.append(activity.attrib[name_attrib])
    return launchable_activities


def ndk_bin_path():
    return os.path.dirname(os.path.realpath(__file__))


def handle_args():
    def find_program(program, paths):
        '''Find a binary in paths'''
        exts = [""]
        if sys.platform.startswith("win"):
            exts += [".exe", ".bat", ".cmd"]
        for path in paths:
            if os.path.isdir(path):
                for ext in exts:
                    full = path + os.sep + program + ext
                    if os.path.isfile(full):
                        return full
        return None

    # FIXME: This is broken for PATH that contains quoted colons.
    paths = os.environ["PATH"].replace('"', '').split(os.pathsep)

    args = ArgumentParser().parse_args()

    if args.tui and sys.platform.startswith("win"):
        error("TUI is unsupported on Windows.")

    ndk_bin = ndk_bin_path()
    args.make_cmd = find_program("make", [ndk_bin])
    args.jdb_cmd = find_program("jdb", paths)
    if args.make_cmd is None:
        error("Failed to find make in '{}'".format(ndk_bin))
    if args.jdb_cmd is None:
        print("WARNING: Failed to find jdb on your path, defaulting to "
              "--nowait")
        args.nowait = True

    if args.verbose:
        logger = logging.getLogger(__name__)
        handler = logging.StreamHandler(sys.stdout)
        formatter = logging.Formatter()

        handler.setFormatter(formatter)
        logger.addHandler(handler)
        logger.propagate = False

        logger.setLevel(logging.INFO)

    return args


def find_project(args):
    manifest_name = "AndroidManifest.xml"
    if args.project is not None:
        log("Using project directory: {}".format(args.project))
        args.project = os.path.realpath(os.path.expanduser(args.project))
        if not os.path.exists(os.path.join(args.project, manifest_name)):
            msg = "could not find AndroidManifest.xml in '{}'"
            error(msg.format(args.project))
    else:
        # Walk upwards until we find AndroidManifest.xml, or run out of path.
        current_dir = os.getcwdu()
        while not os.path.exists(os.path.join(current_dir, manifest_name)):
            parent_dir = os.path.dirname(current_dir)
            if parent_dir == current_dir:
                error("Could not find AndroidManifest.xml in current"
                      " directory or a parent directory.\n"
                      "       Launch this script from inside a project, or"
                      " use --project=<path>.")
            current_dir = parent_dir
        args.project = current_dir
        log("Using project directory: {} ".format(args.project))
    args.manifest_path = os.path.join(args.project, manifest_name)
    return args.project


def canonicalize_activity(package_name, activity_name):
    if activity_name.startswith("."):
        return "{}{}".format(package_name, activity_name)
    return activity_name


def parse_manifest(args):
    manifest = ElementTree.parse(args.manifest_path)
    manifest_root = manifest.getroot()
    package_name = extract_package_name(manifest_root)
    log("Found package name: {}".format(package_name))

    activities = extract_launchable(manifest_root)
    activities = [canonicalize_activity(package_name, a) for a in activities]

    if args.launch_list:
        print("Launchable activities: {}".format(", ".join(activities)))
        sys.exit(0)

    args.activities = activities
    args.package_name = package_name


def select_target(args):
    assert args.launch != False

    if len(args.activities) == 0:
        error("No launchable activities found.")

    if args.launch is None:
        target = args.activities[0]

        if len(args.activities) > 1:
            print("WARNING: Multiple launchable activities found, choosing"
                  " '{}'.".format(args.activities[0]))
    else:
        activity_name = canonicalize_activity(args.package_name, args.launch)

        if activity_name not in args.activities:
            msg = "Could not find launchable activity: '{}'."
            error(msg.format(activity_name))
        target = activity_name
    return target


@contextlib.contextmanager
def cd(path):
    curdir = os.getcwd()
    os.chdir(path)
    os.environ["PWD"] = path
    try:
        yield
    finally:
        os.environ["PWD"] = curdir
        os.chdir(curdir)


def dump_var(args, variable, abi=None):
    make_args = [args.make_cmd, "--no-print-dir", "-f",
                 os.path.join(NDK_PATH, "build/core/build-local.mk"),
                 "-C", args.project, "DUMP_{}".format(variable)]

    if abi is not None:
        make_args.append("APP_ABI={}".format(abi))

    with cd(args.project):
        try:
            make_output = subprocess.check_output(make_args, cwd=args.project)
        except subprocess.CalledProcessError:
            error("Failed to retrieve application ABI from Android.mk.")
    return make_output.splitlines()[-1]


def get_api_level(device):
    # Check the device API level
    try:
        api_level = int(device.get_prop("ro.build.version.sdk"))
    except (TypeError, ValueError):
        error("Failed to find target device's supported API level.\n"
              "ndk-gdb only supports devices running Android 2.2 or higher.")
    if api_level < 8:
        error("ndk-gdb only supports devices running Android 2.2 or higher.\n"
              "(expected API level 8, actual: {})".format(api_level))

    return api_level


def fetch_abi(args):
    '''
    Figure out the intersection of which ABIs the application is built for and
    which ones the device supports, then pick the one preferred by the device,
    so that we know which gdbserver to push and run on the device.
    '''

    app_abis = dump_var(args, "APP_ABI").split(" ")
    if "all" in app_abis:
        app_abis = dump_var(args, "NDK_ALL_ABIS").split(" ")
    app_abis_msg = "Application ABIs: {}".format(", ".join(app_abis))
    log(app_abis_msg)

    new_abi_props = ["ro.product.cpu.abilist"]
    old_abi_props = ["ro.product.cpu.abi", "ro.product.cpu.abi2"]
    abi_props = new_abi_props
    if args.device.get_prop("ro.product.cpu.abilist") is None:
        abi_props = old_abi_props

    device_abis = []
    for key in abi_props:
        value = args.device.get_prop(key)
        if value is not None:
            device_abis.extend(value.split(","))

    device_abis_msg = "Device ABIs: {}".format(", ".join(device_abis))
    log(device_abis_msg)

    for abi in device_abis:
        if abi in app_abis:
            # TODO(jmgao): Do we expect gdb to work with ARM-x86 translation?
            log("Selecting ABI: {}".format(abi))
            return abi

    msg = "Application cannot run on the selected device."

    # Don't repeat ourselves.
    if not args.verbose:
        msg += "\n{}\n{}".format(app_abis_msg, device_abis_msg)

    error(msg)


def get_run_as_cmd(user, cmd):
    return ["run-as", user] + cmd


def get_app_data_dir(args, package_name):
    cmd = ["/system/bin/sh", "-c", "pwd", "2>/dev/null"]
    cmd = get_run_as_cmd(package_name, cmd)
    (rc, stdout, _) = args.device.shell_nocheck(cmd)
    if rc != 0:
        error("Could not find application's data directory. Are you sure that "
              "the application is installed and debuggable?")
    data_dir = stdout.strip()

    # Applications with minSdkVersion >= 24 will have their data directories
    # created with rwx------ permissions, preventing adbd from forwarding to
    # the gdbserver socket. To be safe, if we're on a device >= 24, always
    # chmod the directory.
    if get_api_level(args.device) >= 24:
        chmod_cmd = ["/system/bin/chmod", "a+x", data_dir]
        chmod_cmd = get_run_as_cmd(package_name, chmod_cmd)
        (rc, _, _) = args.device.shell_nocheck(chmod_cmd)
        if rc != 0:
            error("Failed to make application data directory world executable")

    log("Found application data directory: {}".format(data_dir))
    return data_dir


def abi_to_arch(abi):
    if abi.startswith("armeabi"):
        return "arm"
    elif abi == "arm64-v8a":
        return "arm64"
    else:
        return abi


def get_gdbserver_path(args, package_name, app_data_dir, arch):
    app_gdbserver_path = "{}/lib/gdbserver".format(app_data_dir)
    cmd = ["ls", app_gdbserver_path, "2>/dev/null"]
    cmd = get_run_as_cmd(package_name, cmd)
    (rc, _, _) = args.device.shell_nocheck(cmd)
    if rc == 0:
        log("Found app gdbserver: {}".format(app_gdbserver_path))
        return app_gdbserver_path

    # We need to upload our gdbserver
    log("App gdbserver not found at {}, uploading.".format(app_gdbserver_path))
    local_path = "{}/prebuilt/android-{}/gdbserver/gdbserver"
    local_path = local_path.format(NDK_PATH, arch)
    remote_path = "/data/local/tmp/{}-gdbserver".format(arch)
    args.device.push(local_path, remote_path)

    # Copy gdbserver into the data directory on M+, because selinux prevents
    # execution of binaries directly from /data/local/tmp.
    if get_api_level(args.device) >= 23:
        destination = "{}/{}-gdbserver".format(app_data_dir, arch)
        log("Copying gdbserver to {}.".format(destination))
        cmd = ["cat", remote_path, "|", "run-as", package_name,
               "sh", "-c", "'cat > {}'".format(destination)]
        (rc, _, _) = args.device.shell_nocheck(cmd)
        if rc != 0:
            error("Failed to copy gdbserver to {}.".format(destination))
        (rc, _, _) = args.device.shell_nocheck(["run-as", package_name,
                                                "chmod", "700", destination])
        if rc != 0:
            error("Failed to chmod gdbserver at {}.".format(destination))

        remote_path = destination

    log("Uploaded gdbserver to {}".format(remote_path))
    return remote_path


def pull_binaries(device, out_dir, app_64bit):
    required_files = []
    libraries = ["libc.so", "libm.so", "libdl.so"]

    if app_64bit:
        required_files = ["/system/bin/app_process64", "/system/bin/linker64"]
        library_path = "/system/lib64"
    else:
        required_files = ["/system/bin/linker"]
        library_path = "/system/lib"

    for library in libraries:
        required_files.append(posixpath.join(library_path, library))

    for required_file in required_files:
        # os.path.join not used because joining absolute paths will pick the last one
        local_path = os.path.realpath(out_dir + required_file)
        local_dirname = os.path.dirname(local_path)
        if not os.path.isdir(local_dirname):
            os.makedirs(local_dirname)
        log("Pulling '{}' to '{}'".format(required_file, local_path))
        device.pull(required_file, local_path)

    # /system/bin/app_process is 32-bit on 32-bit devices, but a symlink to
    # app_process64 on 64-bit. If we need the 32-bit version, try to pull
    # app_process32, and if that fails, pull app_process.
    if not app_64bit:
        destination = os.path.realpath(out_dir + "/system/bin/app_process")
        try:
            device.pull("/system/bin/app_process32", destination)
        except:
            device.pull("/system/bin/app_process", destination)

def generate_gdb_script(args, sysroot, binary_path, app_64bit, connect_timeout=5):
    if sys.platform.startswith("win"):
        # GDB expects paths to use forward slashes.
        sysroot = sysroot.replace("\\", "/")
        binary_path = binary_path.replace("\\", "/")

    gdb_commands = "set osabi GNU/Linux\n"
    gdb_commands += "file '{}'\n".format(binary_path)

    solib_search_path = [sysroot, "{}/system/bin".format(sysroot)]
    if app_64bit:
        solib_search_path.append("{}/system/lib64".format(sysroot))
    else:
        solib_search_path.append("{}/system/lib".format(sysroot))
    solib_search_path = os.pathsep.join(solib_search_path)
    gdb_commands += "set solib-absolute-prefix {}\n".format(sysroot)
    gdb_commands += "set solib-search-path {}\n".format(solib_search_path)

    # Try to connect for a few seconds, sometimes the device gdbserver takes
    # a little bit to come up, especially on emulators.
    gdb_commands += """
python

def target_remote_with_retry(target, timeout_seconds):
  import time
  end_time = time.time() + timeout_seconds
  while True:
    try:
      gdb.execute('target remote ' + target)
      return True
    except gdb.error as e:
      time_left = end_time - time.time()
      if time_left < 0 or time_left > timeout_seconds:
        print("Error: unable to connect to device.")
        print(e)
        return False
      time.sleep(min(0.25, time_left))

target_remote_with_retry(':{}', {})

end
""".format(args.port, connect_timeout)

    # Set up the pretty printer if needed
    if args.pypr_dir is not None and args.pypr_fn is not None:
        gdb_commands += """
python
import sys
sys.path.append("{pypr_dir}")
from printers import {pypr_fn}
{pypr_fn}(None)
end""".format(pypr_dir=args.pypr_dir.replace("\\", "/"), pypr_fn=args.pypr_fn)

    if args.exec_file is not None:
        try:
            exec_file = open(args.exec_file, "r")
        except IOError:
            error("Failed to open GDB exec file: '{}'.".format(args.exec_file))

        with exec_file:
            gdb_commands += exec_file.read()

    return gdb_commands


def detect_stl_pretty_printer(args):
    stl = dump_var(args, "APP_STL")
    if not stl:
        detected = "none"
        if args.stdcxxpypr == "auto":
            log("APP_STL not found, disabling pretty printer")
    elif stl.startswith("stlport"):
        detected = "stlport"
    elif stl.startswith("gnustl"):
        detected = "gnustl"
    else:
        detected = "none"

    if args.stdcxxpypr == "auto":
        log("Detected pretty printer: {}".format(detected))
        return detected
    if detected != args.stdcxxpypr and args.stdcxxpypr != "none":
        print("WARNING: detected APP_STL ('{}') does not match pretty printer".format(detected))
    log("Using specified pretty printer: {}".format(args.stdcxxpypr))
    return args.stdcxxpypr


def find_pretty_printer(pretty_printer):
    if pretty_printer == "gnustl":
        path = os.path.join("libstdcxx", "gcc-4.9")
        function = "register_libstdcxx_printers"
    elif pretty_printer == "stlport":
        path = os.path.join("stlport", "stlport")
        function = "register_stlport_printers"
    pp_path = os.path.join(
        ndk_bin_path(), "..", "share", "pretty-printers", path)
    return pp_path, function


def start_jdb(args, pid):
    log("Starting jdb to unblock application.")

    # Do setup stuff to keep ^C in the parent from killing us.
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    windows = sys.platform.startswith("win")
    if not windows:
        os.setpgrp()

    # Wait until gdbserver has interrupted the program.
    time.sleep(0.5)

    jdb_port = 65534
    args.device.forward("tcp:{}".format(jdb_port), "jdwp:{}".format(pid))
    jdb_cmd = [args.jdb_cmd, "-connect",
               "com.sun.jdi.SocketAttach:hostname=localhost,port={}".format(jdb_port)]

    flags = subprocess.CREATE_NEW_PROCESS_GROUP if windows else 0
    jdb = subprocess.Popen(jdb_cmd,
                           stdin=subprocess.PIPE,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT,
                           creationflags=flags)

    # Wait until jdb can communicate with the app. Once it can, the app will
    # start polling for a Java debugger (e.g. every 200ms). We need to wait
    # a while longer then so that the app notices jdb.
    jdb_magic = "__verify_jdb_has_started__"
    jdb.stdin.write('print "{}"\n'.format(jdb_magic))
    saw_magic_str = False
    while True:
        line = jdb.stdout.readline()
        if line == "":
            break
        log("jdb output: " + line.rstrip())
        if jdb_magic in line and not saw_magic_str:
            saw_magic_str = True
            time.sleep(0.3)
            jdb.stdin.write("exit\n")
    jdb.wait()
    if saw_magic_str:
        log("JDB finished unblocking application.")
    else:
        log("error: did not find magic string in JDB output.")


def main():
    args = handle_args()
    device = args.device

    if device is None:
        error("Could not find a unique connected device/emulator.")

    adb_version = subprocess.check_output(device.adb_cmd + ["version"])
    log("ADB command used: '{}'".format(" ".join(device.adb_cmd)))
    log("ADB version: {}".format(" ".join(adb_version.splitlines())))

    project = find_project(args)
    if args.package_name:
        log("Attaching to specified package: {}".format(args.package_name))
    else:
        parse_manifest(args)

    pkg_name = args.package_name

    if args.launch is False:
        log("Attaching to existing application process.")
    else:
        args.launch = select_target(args)
        log("Selected target activity: '{}'".format(args.launch))

    abi = fetch_abi(args)

    out_dir = os.path.join(project, (dump_var(args, "TARGET_OUT", abi)))
    out_dir = os.path.realpath(out_dir)

    pretty_printer = detect_stl_pretty_printer(args)
    if pretty_printer != "none":
        (args.pypr_dir, args.pypr_fn) = find_pretty_printer(pretty_printer)
    else:
        (args.pypr_dir, args.pypr_fn) = (None, None)

    app_data_dir = get_app_data_dir(args, pkg_name)
    arch = abi_to_arch(abi)
    gdbserver_path = get_gdbserver_path(args, pkg_name, app_data_dir, arch)

    # Kill the process and gdbserver if requested.
    if args.force:
        kill_pids = gdbrunner.get_pids(device, gdbserver_path)
        if args.launch:
            kill_pids += gdbrunner.get_pids(device, pkg_name)
        kill_pids = map(str, kill_pids)
        if kill_pids:
            log("Killing processes: {}".format(", ".join(kill_pids)))
            device.shell_nocheck(["run-as", pkg_name, "kill", "-9"] + kill_pids)

    # Launch the application if needed, and get its pid
    if args.launch:
        am_cmd = ["am", "start"]
        if not args.nowait:
            am_cmd.append("-D")
        component_name = "{}/{}".format(pkg_name, args.launch)
        am_cmd.append(component_name)
        log("Launching activity {}...".format(component_name))
        (rc, _, _) = device.shell_nocheck(am_cmd)
        if rc != 0:
            error("Failed to start {}".format(component_name))

        if args.delay > 0.0:
            log("Sleeping for {} seconds.".format(args.delay))
            time.sleep(args.delay)

    pids = gdbrunner.get_pids(device, pkg_name)
    if len(pids) == 0:
        error("Failed to find running process '{}'".format(pkg_name))
    if len(pids) > 1:
        error("Multiple running processes named '{}'".format(pkg_name))
    pid = pids[0]

    # Pull the linker, zygote, and notable system libraries
    app_64bit = "64" in abi
    pull_binaries(device, out_dir, app_64bit)
    if app_64bit:
        zygote_path = os.path.join(out_dir, "system", "bin", "app_process64")
    else:
        zygote_path = os.path.join(out_dir, "system", "bin", "app_process")

    # Start gdbserver.
    debug_socket = posixpath.join(app_data_dir, "debug_socket")
    log("Starting gdbserver...")
    gdbrunner.start_gdbserver(
        device, None, gdbserver_path,
        target_pid=pid, run_cmd=None, debug_socket=debug_socket,
        port=args.port, run_as_cmd=["run-as", pkg_name])

    gdb_path = os.path.join(ndk_bin_path(), "gdb")

    # Start jdb to unblock the application if necessary.
    if args.launch and not args.nowait:
        # Do this in a separate process before starting gdb, since jdb won't
        # connect until gdb connects and continues.
        jdb_process = multiprocessing.Process(target=start_jdb, args=(args, pid))
        jdb_process.start()

    # Start gdb.
    gdb_commands = generate_gdb_script(args, out_dir, zygote_path, app_64bit)
    gdb_flags = []
    if args.tui:
        gdb_flags.append("--tui")
    gdbrunner.start_gdb(gdb_path, gdb_commands, gdb_flags)

if __name__ == "__main__":
    main()
