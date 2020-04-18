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

"""Helpers used by both gdbclient.py and ndk-gdb.py."""

import adb
import argparse
import atexit
import os
import subprocess
import sys
import tempfile

class ArgumentParser(argparse.ArgumentParser):
    """ArgumentParser subclass that provides adb device selection."""

    def __init__(self):
        super(ArgumentParser, self).__init__()
        self.add_argument(
            "--adb", dest="adb_path",
            help="use specific adb command")

        group = self.add_argument_group(title="device selection")
        group = group.add_mutually_exclusive_group()
        group.add_argument(
            "-a", action="store_const", dest="device", const="-a",
            help="directs commands to all interfaces")
        group.add_argument(
            "-d", action="store_const", dest="device", const="-d",
            help="directs commands to the only connected USB device")
        group.add_argument(
            "-e", action="store_const", dest="device", const="-e",
            help="directs commands to the only connected emulator")
        group.add_argument(
            "-s", metavar="SERIAL", action="store", dest="serial",
            help="directs commands to device/emulator with the given serial")

    def parse_args(self, args=None, namespace=None):
        result = super(ArgumentParser, self).parse_args(args, namespace)

        adb_path = result.adb_path or "adb"

        # Try to run the specified adb command
        try:
            subprocess.check_output([adb_path, "version"],
                                    stderr=subprocess.STDOUT)
        except (OSError, subprocess.CalledProcessError):
            msg = "ERROR: Unable to run adb executable (tried '{}')."
            if not result.adb_path:
                msg += "\n       Try specifying its location with --adb."
            sys.exit(msg.format(adb_path))

        try:
            if result.device == "-a":
                result.device = adb.get_device(adb_path=adb_path)
            elif result.device == "-d":
                result.device = adb.get_usb_device(adb_path=adb_path)
            elif result.device == "-e":
                result.device = adb.get_emulator_device(adb_path=adb_path)
            else:
                result.device = adb.get_device(result.serial, adb_path=adb_path)
        except (adb.DeviceNotFoundError, adb.NoUniqueDeviceError, RuntimeError):
            # Don't error out if we can't find a device.
            result.device = None

        return result


def get_processes(device):
    """Return a dict from process name to list of running PIDs on the device."""

    # Some custom ROMs use busybox instead of toolbox for ps. Without -w,
    # busybox truncates the output, and very long package names like
    # com.exampleisverylongtoolongbyfar.plasma exceed the limit.
    #
    # Perform the check for this on the device to avoid an adb roundtrip
    # Some devices might not have readlink or which, so we need to handle
    # this as well.
    #
    # Gracefully handle [ or readlink being missing by always using `ps` if
    # readlink is missing. (API 18 has [, but not readlink).

    ps_script = """
        if $(ls /system/bin/readlink >/dev/null 2>&1); then
          if [ $(readlink /system/bin/ps) == "toolbox" ]; then
            ps;
          else
            ps -w;
          fi
        else
          ps;
        fi
    """
    ps_script = " ".join([line.strip() for line in ps_script.splitlines()])

    output, _ = device.shell([ps_script])
    return parse_ps_output(output)

def parse_ps_output(output):
    processes = dict()
    output = adb.split_lines(output.replace("\r", ""))
    columns = output.pop(0).split()
    try:
        pid_column = columns.index("PID")
    except ValueError:
        pid_column = 1
    while output:
        columns = output.pop().split()
        process_name = columns[-1]
        pid = int(columns[pid_column])
        if process_name in processes:
            processes[process_name].append(pid)
        else:
            processes[process_name] = [pid]

    return processes


def get_pids(device, process_name):
    processes = get_processes(device)
    return processes.get(process_name, [])


def start_gdbserver(device, gdbserver_local_path, gdbserver_remote_path,
                    target_pid, run_cmd, debug_socket, port, run_as_cmd=None):
    """Start gdbserver in the background and forward necessary ports.

    Args:
        device: ADB device to start gdbserver on.
        gdbserver_local_path: Host path to push gdbserver from, can be None.
        gdbserver_remote_path: Device path to push gdbserver to.
        target_pid: PID of device process to attach to.
        run_cmd: Command to run on the device.
        debug_socket: Device path to place gdbserver unix domain socket.
        port: Host port to forward the debug_socket to.
        run_as_cmd: run-as or su command to prepend to commands.

    Returns:
        Popen handle to the `adb shell` process gdbserver was started with.
    """

    assert target_pid is None or run_cmd is None

    # Push gdbserver to the target.
    if gdbserver_local_path is not None:
        device.push(gdbserver_local_path, gdbserver_remote_path)

    # Run gdbserver.
    gdbserver_cmd = [gdbserver_remote_path, "--once",
                     "+{}".format(debug_socket)]

    if target_pid is not None:
        gdbserver_cmd += ["--attach", str(target_pid)]
    else:
        gdbserver_cmd += run_cmd

    forward_gdbserver_port(device, local=port, remote="localfilesystem:{}".format(debug_socket))

    if run_as_cmd:
        gdbserver_cmd = run_as_cmd + gdbserver_cmd

    gdbserver_output_path = os.path.join(tempfile.gettempdir(),
                                         "gdbclient.log")
    print("Redirecting gdbserver output to {}".format(gdbserver_output_path))
    gdbserver_output = file(gdbserver_output_path, 'w')
    return device.shell_popen(gdbserver_cmd, stdout=gdbserver_output,
                              stderr=gdbserver_output)


def forward_gdbserver_port(device, local, remote):
    """Forwards local TCP port `port` to `remote` via `adb forward`."""
    device.forward("tcp:{}".format(local), remote)
    atexit.register(lambda: device.forward_remove("tcp:{}".format(local)))


def find_file(device, executable_path, sysroot, run_as_cmd=None):
    """Finds a device executable file.

    This function first attempts to find the local file which will
    contain debug symbols. If that fails, it will fall back to
    downloading the stripped file from the device.

    Args:
      device: the AndroidDevice object to use.
      executable_path: absolute path to the executable or symlink.
      sysroot: absolute path to the built symbol sysroot.
      run_as_cmd: if necessary, run-as or su command to prepend

    Returns:
      A tuple containing (<open file object>, <was found locally>).

    Raises:
      RuntimeError: could not find the executable binary.
      ValueError: |executable_path| is not absolute.
    """
    if not os.path.isabs(executable_path):
        raise ValueError("'{}' is not an absolute path".format(executable_path))

    def generate_files():
        """Yields (<file name>, <found locally>) tuples."""
        # First look locally to avoid shelling into the device if possible.
        # os.path.join() doesn't combine absolute paths, use + instead.
        yield (sysroot + executable_path, True)

        # Next check if the path is a symlink.
        try:
            target = device.shell(['readlink', '-e', '-n', executable_path])[0]
            yield (sysroot + target, True)
        except adb.ShellError:
            pass

        # Last, download the stripped executable from the device if necessary.
        file_name = "gdbclient-binary-{}".format(os.getppid())
        remote_temp_path = "/data/local/tmp/{}".format(file_name)
        local_path = os.path.join(tempfile.gettempdir(), file_name)

        cmd = ["cat", executable_path, ">", remote_temp_path]
        if run_as_cmd:
            cmd = run_as_cmd + cmd

        try:
            device.shell(cmd)
        except adb.ShellError:
            raise RuntimeError("Failed to copy '{}' to temporary folder on "
                               "device".format(executable_path))
        device.pull(remote_temp_path, local_path)
        yield (local_path, False)

    for path, found_locally in generate_files():
        if os.path.isfile(path):
            return (open(path, "r"), found_locally)
    raise RuntimeError('Could not find executable {}'.format(executable_path))

def find_executable_path(device, executable_name, run_as_cmd=None):
    """Find a device executable from its name

    This function calls which on the device to retrieve the absolute path of
    the executable.

    Args:
      device: the AndroidDevice object to use.
      executable_name: the name of the executable to find.
      run_as_cmd: if necessary, run-as or su command to prepend

    Returns:
      The absolute path of the executable.

    Raises:
      RuntimeError: could not find the executable.
    """
    cmd = ["which", executable_name]
    if run_as_cmd:
        cmd = run_as_cmd + cmd

    try:
        output, _ = device.shell(cmd)
        return output
    except adb.ShellError:
        raise  RuntimeError("Could not find executable '{}' on "
                            "device".format(executable_name))

def find_binary(device, pid, sysroot, run_as_cmd=None):
    """Finds a device executable file corresponding to |pid|."""
    return find_file(device, "/proc/{}/exe".format(pid), sysroot, run_as_cmd)


def get_binary_arch(binary_file):
    """Parse a binary's ELF header for arch."""
    try:
        binary_file.seek(0)
        binary = binary_file.read(0x14)
    except IOError:
        raise RuntimeError("failed to read binary file")
    ei_class = ord(binary[0x4]) # 1 = 32-bit, 2 = 64-bit
    ei_data = ord(binary[0x5]) # Endianness

    assert ei_class == 1 or ei_class == 2
    if ei_data != 1:
        raise RuntimeError("binary isn't little-endian?")

    e_machine = ord(binary[0x13]) << 8 | ord(binary[0x12])
    if e_machine == 0x28:
        assert ei_class == 1
        return "arm"
    elif e_machine == 0xB7:
        assert ei_class == 2
        return "arm64"
    elif e_machine == 0x03:
        assert ei_class == 1
        return "x86"
    elif e_machine == 0x3E:
        assert ei_class == 2
        return "x86_64"
    elif e_machine == 0x08:
        if ei_class == 1:
            return "mips"
        else:
            return "mips64"
    else:
        raise RuntimeError("unknown architecture: 0x{:x}".format(e_machine))


def start_gdb(gdb_path, gdb_commands, gdb_flags=None):
    """Start gdb in the background and block until it finishes.

    Args:
        gdb_path: Path of the gdb binary.
        gdb_commands: Contents of GDB script to run.
        gdb_flags: List of flags to append to gdb command.
    """

    # Windows disallows opening the file while it's open for writing.
    gdb_script_fd, gdb_script_path = tempfile.mkstemp()
    os.write(gdb_script_fd, gdb_commands)
    os.close(gdb_script_fd)
    gdb_args = [gdb_path, "-x", gdb_script_path] + (gdb_flags or [])

    kwargs = {}
    if sys.platform.startswith("win"):
        kwargs["creationflags"] = subprocess.CREATE_NEW_CONSOLE

    gdb_process = subprocess.Popen(gdb_args, **kwargs)
    while gdb_process.returncode is None:
        try:
            gdb_process.communicate()
        except KeyboardInterrupt:
            pass

    os.unlink(gdb_script_path)
