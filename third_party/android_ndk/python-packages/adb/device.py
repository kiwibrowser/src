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
import atexit
import base64
import logging
import os
import re
import subprocess


class FindDeviceError(RuntimeError):
    pass


class DeviceNotFoundError(FindDeviceError):
    def __init__(self, serial):
        self.serial = serial
        super(DeviceNotFoundError, self).__init__(
            'No device with serial {}'.format(serial))


class NoUniqueDeviceError(FindDeviceError):
    def __init__(self):
        super(NoUniqueDeviceError, self).__init__('No unique device')


class ShellError(RuntimeError):
    def __init__(self, cmd, stdout, stderr, exit_code):
        super(ShellError, self).__init__(
            '`{0}` exited with code {1}'.format(cmd, exit_code))
        self.cmd = cmd
        self.stdout = stdout
        self.stderr = stderr
        self.exit_code = exit_code


def get_devices(adb_path='adb'):
    with open(os.devnull, 'wb') as devnull:
        subprocess.check_call([adb_path, 'start-server'], stdout=devnull,
                              stderr=devnull)
    out = split_lines(subprocess.check_output([adb_path, 'devices']))

    # The first line of `adb devices` just says "List of attached devices", so
    # skip that.
    devices = []
    for line in out[1:]:
        if not line.strip():
            continue
        if 'offline' in line:
            continue

        serial, _ = re.split(r'\s+', line, maxsplit=1)
        devices.append(serial)
    return devices


def _get_unique_device(product=None, adb_path='adb'):
    devices = get_devices(adb_path=adb_path)
    if len(devices) != 1:
        raise NoUniqueDeviceError()
    return AndroidDevice(devices[0], product, adb_path)


def _get_device_by_serial(serial, product=None, adb_path='adb'):
    for device in get_devices(adb_path=adb_path):
        if device == serial:
            return AndroidDevice(serial, product, adb_path)
    raise DeviceNotFoundError(serial)


def get_device(serial=None, product=None, adb_path='adb'):
    """Get a uniquely identified AndroidDevice if one is available.

    Raises:
        DeviceNotFoundError:
            The serial specified by `serial` or $ANDROID_SERIAL is not
            connected.

        NoUniqueDeviceError:
            Neither `serial` nor $ANDROID_SERIAL was set, and the number of
            devices connected to the system is not 1. Having 0 connected
            devices will also result in this error.

    Returns:
        An AndroidDevice associated with the first non-None identifier in the
        following order of preference:

        1) The `serial` argument.
        2) The environment variable $ANDROID_SERIAL.
        3) The single device connnected to the system.
    """
    if serial is not None:
        return _get_device_by_serial(serial, product, adb_path)

    android_serial = os.getenv('ANDROID_SERIAL')
    if android_serial is not None:
        return _get_device_by_serial(android_serial, product, adb_path)

    return _get_unique_device(product, adb_path=adb_path)


def _get_device_by_type(flag, adb_path):
    with open(os.devnull, 'wb') as devnull:
        subprocess.check_call([adb_path, 'start-server'], stdout=devnull,
                              stderr=devnull)
    try:
        serial = subprocess.check_output(
            [adb_path, flag, 'get-serialno']).strip()
    except subprocess.CalledProcessError:
        raise RuntimeError('adb unexpectedly returned nonzero')
    if serial == 'unknown':
        raise NoUniqueDeviceError()
    return _get_device_by_serial(serial, adb_path=adb_path)


def get_usb_device(adb_path='adb'):
    """Get the unique USB-connected AndroidDevice if it is available.

    Raises:
        NoUniqueDeviceError:
            0 or multiple devices are connected via USB.

    Returns:
        An AndroidDevice associated with the unique USB-connected device.
    """
    return _get_device_by_type('-d', adb_path=adb_path)


def get_emulator_device(adb_path='adb'):
    """Get the unique emulator AndroidDevice if it is available.

    Raises:
        NoUniqueDeviceError:
            0 or multiple emulators are running.

    Returns:
        An AndroidDevice associated with the unique running emulator.
    """
    return _get_device_by_type('-e', adb_path=adb_path)


# If necessary, modifies subprocess.check_output() or subprocess.Popen() args
# to run the subprocess via Windows PowerShell to work-around an issue in
# Python 2's subprocess class on Windows where it doesn't support Unicode.
def _get_subprocess_args(args):
    # Only do this slow work-around if Unicode is in the cmd line on Windows.
    # PowerShell takes 600-700ms to startup on a 2013-2014 machine, which is
    # very slow.
    if os.name != 'nt' or all(not isinstance(arg, unicode) for arg in args[0]):
        return args

    def escape_arg(arg):
        # Escape for the parsing that the C Runtime does in Windows apps. In
        # particular, this will take care of double-quotes.
        arg = subprocess.list2cmdline([arg])
        # Escape single-quote with another single-quote because we're about
        # to...
        arg = arg.replace(u"'", u"''")
        # ...put the arg in a single-quoted string for PowerShell to parse.
        arg = u"'" + arg + u"'"
        return arg

    # Escape command line args.
    argv = map(escape_arg, args[0])
    # Cause script errors (such as adb not found) to stop script immediately
    # with an error.
    ps_code = u'$ErrorActionPreference = "Stop"\r\n'
    # Add current directory to the PATH var, to match cmd.exe/CreateProcess()
    # behavior.
    ps_code += u'$env:Path = ".;" + $env:Path\r\n'
    # Precede by &, the PowerShell call operator, and separate args by space.
    ps_code += u'& ' + u' '.join(argv)
    # Make the PowerShell exit code the exit code of the subprocess.
    ps_code += u'\r\nExit $LastExitCode'
    # Encode as UTF-16LE (without Byte-Order-Mark) which Windows natively
    # understands.
    ps_code = ps_code.encode('utf-16le')

    # Encode the PowerShell command as base64 and use the special
    # -EncodedCommand option that base64 decodes. Base64 is just plain ASCII,
    # so it should have no problem passing through Win32 CreateProcessA()
    # (which python erroneously calls instead of CreateProcessW()).
    return (['powershell.exe', '-NoProfile', '-NonInteractive',
             '-EncodedCommand', base64.b64encode(ps_code)],) + args[1:]


# Call this instead of subprocess.check_output() to work-around issue in Python
# 2's subprocess class on Windows where it doesn't support Unicode.
def _subprocess_check_output(*args, **kwargs):
    try:
        return subprocess.check_output(*_get_subprocess_args(args), **kwargs)
    except subprocess.CalledProcessError as e:
        # Show real command line instead of the powershell.exe command line.
        raise subprocess.CalledProcessError(e.returncode, args[0],
                                            output=e.output)


# Call this instead of subprocess.Popen(). Like _subprocess_check_output().
def _subprocess_Popen(*args, **kwargs):
    return subprocess.Popen(*_get_subprocess_args(args), **kwargs)


def split_lines(s):
    """Splits lines in a way that works even on Windows and old devices.

    Windows will see \r\n instead of \n, old devices do the same, old devices
    on Windows will see \r\r\n.
    """
    # rstrip is used here to workaround a difference between splineslines and
    # re.split:
    # >>> 'foo\n'.splitlines()
    # ['foo']
    # >>> re.split(r'\n', 'foo\n')
    # ['foo', '']
    return re.split(r'[\r\n]+', s.rstrip())


def version(adb_path=None):
    """Get the version of adb (in terms of ADB_SERVER_VERSION)."""

    adb_path = adb_path if adb_path is not None else ['adb']
    version_output = subprocess.check_output(adb_path + ['version'])
    pattern = r'^Android Debug Bridge version 1.0.(\d+)$'
    result = re.match(pattern, version_output.splitlines()[0])
    if not result:
        return 0
    return int(result.group(1))


class AndroidDevice(object):
    # Delimiter string to indicate the start of the exit code.
    _RETURN_CODE_DELIMITER = 'x'

    # Follow any shell command with this string to get the exit
    # status of a program since this isn't propagated by adb.
    #
    # The delimiter is needed because `printf 1; echo $?` would print
    # "10", and we wouldn't be able to distinguish the exit code.
    _RETURN_CODE_PROBE = [';', 'echo', '{0}$?'.format(_RETURN_CODE_DELIMITER)]

    # Maximum search distance from the output end to find the delimiter.
    # adb on Windows returns \r\n even if adbd returns \n. Some old devices
    # seem to actually return \r\r\n.
    _RETURN_CODE_SEARCH_LENGTH = len(
        '{0}255\r\r\n'.format(_RETURN_CODE_DELIMITER))

    def __init__(self, serial, product=None, adb_path='adb'):
        self.serial = serial
        self.product = product
        self.adb_cmd = [adb_path]

        if self.serial is not None:
            self.adb_cmd.extend(['-s', serial])
        if self.product is not None:
            self.adb_cmd.extend(['-p', product])
        self._linesep = None
        self._features = None

    @property
    def linesep(self):
        if self._linesep is None:
            self._linesep = subprocess.check_output(self.adb_cmd +
                                                    ['shell', 'echo'])
        return self._linesep

    @property
    def features(self):
        if self._features is None:
            try:
                self._features = split_lines(self._simple_call(['features']))
            except subprocess.CalledProcessError:
                self._features = []
        return self._features

    def has_shell_protocol(self):
        return version(self.adb_cmd) >= 35 and 'shell_v2' in self.features

    def _make_shell_cmd(self, user_cmd):
        command = self.adb_cmd + ['shell'] + user_cmd
        if not self.has_shell_protocol():
            command += self._RETURN_CODE_PROBE
        return command

    def _parse_shell_output(self, out):
        """Finds the exit code string from shell output.

        Args:
            out: Shell output string.

        Returns:
            An (exit_code, output_string) tuple. The output string is
            cleaned of any additional stuff we appended to find the
            exit code.

        Raises:
            RuntimeError: Could not find the exit code in |out|.
        """
        search_text = out
        if len(search_text) > self._RETURN_CODE_SEARCH_LENGTH:
            # We don't want to search over massive amounts of data when we know
            # the part we want is right at the end.
            search_text = search_text[-self._RETURN_CODE_SEARCH_LENGTH:]
        partition = search_text.rpartition(self._RETURN_CODE_DELIMITER)
        if partition[1] == '':
            raise RuntimeError('Could not find exit status in shell output.')
        result = int(partition[2])
        # partition[0] won't contain the full text if search_text was
        # truncated, pull from the original string instead.
        out = out[:-len(partition[1]) - len(partition[2])]
        return result, out

    def _simple_call(self, cmd):
        logging.info(' '.join(self.adb_cmd + cmd))
        return _subprocess_check_output(
            self.adb_cmd + cmd, stderr=subprocess.STDOUT)

    def shell(self, cmd):
        """Calls `adb shell`

        Args:
            cmd: command to execute as a list of strings.

        Returns:
            A (stdout, stderr) tuple. Stderr may be combined into stdout
            if the device doesn't support separate streams.

        Raises:
            ShellError: the exit code was non-zero.
        """
        exit_code, stdout, stderr = self.shell_nocheck(cmd)
        if exit_code != 0:
            raise ShellError(cmd, stdout, stderr, exit_code)
        return stdout, stderr

    def shell_nocheck(self, cmd):
        """Calls `adb shell`

        Args:
            cmd: command to execute as a list of strings.

        Returns:
            An (exit_code, stdout, stderr) tuple. Stderr may be combined
            into stdout if the device doesn't support separate streams.
        """
        cmd = self._make_shell_cmd(cmd)
        logging.info(' '.join(cmd))
        p = _subprocess_Popen(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        if self.has_shell_protocol():
            exit_code = p.returncode
        else:
            exit_code, stdout = self._parse_shell_output(stdout)
        return exit_code, stdout, stderr

    def shell_popen(self, cmd, kill_atexit=True, preexec_fn=None,
                    creationflags=0, **kwargs):
        """Calls `adb shell` and returns a handle to the adb process.

        This function provides direct access to the subprocess used to run the
        command, without special return code handling. Users that need the
        return value must retrieve it themselves.

        Args:
            cmd: Array of command arguments to execute.
            kill_atexit: Whether to kill the process upon exiting.
            preexec_fn: Argument forwarded to subprocess.Popen.
            creationflags: Argument forwarded to subprocess.Popen.
            **kwargs: Arguments forwarded to subprocess.Popen.

        Returns:
            subprocess.Popen handle to the adb shell instance
        """

        command = self.adb_cmd + ['shell'] + cmd

        # Make sure a ctrl-c in the parent script doesn't kill gdbserver.
        if os.name == 'nt':
            creationflags |= subprocess.CREATE_NEW_PROCESS_GROUP
        else:
            if preexec_fn is None:
                preexec_fn = os.setpgrp
            elif preexec_fn is not os.setpgrp:
                fn = preexec_fn
                def _wrapper():
                    fn()
                    os.setpgrp()
                preexec_fn = _wrapper

        p = _subprocess_Popen(command, creationflags=creationflags,
                              preexec_fn=preexec_fn, **kwargs)

        if kill_atexit:
            atexit.register(p.kill)

        return p

    def install(self, filename, replace=False):
        cmd = ['install']
        if replace:
            cmd.append('-r')
        cmd.append(filename)
        return self._simple_call(cmd)

    def push(self, local, remote, sync=False):
        """Transfer a local file or directory to the device.

        Args:
            local: The local file or directory to transfer.
            remote: The remote path to which local should be transferred.
            sync: If True, only transfers files that are newer on the host than
                  those on the device. If False, transfers all files.

        Returns:
            Exit status of the push command.
        """
        cmd = ['push']
        if sync:
            cmd.append('--sync')
        cmd.extend([local, remote])
        return self._simple_call(cmd)

    def pull(self, remote, local):
        return self._simple_call(['pull', remote, local])

    def sync(self, directory=None):
        cmd = ['sync']
        if directory is not None:
            cmd.append(directory)
        return self._simple_call(cmd)

    def tcpip(self, port):
        return self._simple_call(['tcpip', port])

    def usb(self):
        return self._simple_call(['usb'])

    def reboot(self):
        return self._simple_call(['reboot'])

    def remount(self):
        return self._simple_call(['remount'])

    def root(self):
        return self._simple_call(['root'])

    def unroot(self):
        return self._simple_call(['unroot'])

    def connect(self, host):
        return self._simple_call(['connect', host])

    def disconnect(self, host):
        return self._simple_call(['disconnect', host])

    def forward(self, local, remote):
        return self._simple_call(['forward', local, remote])

    def forward_list(self):
        return self._simple_call(['forward', '--list'])

    def forward_no_rebind(self, local, remote):
        return self._simple_call(['forward', '--no-rebind', local, remote])

    def forward_remove(self, local):
        return self._simple_call(['forward', '--remove', local])

    def forward_remove_all(self):
        return self._simple_call(['forward', '--remove-all'])

    def reverse(self, remote, local):
        return self._simple_call(['reverse', remote, local])

    def reverse_list(self):
        return self._simple_call(['reverse', '--list'])

    def reverse_no_rebind(self, local, remote):
        return self._simple_call(['reverse', '--no-rebind', local, remote])

    def reverse_remove_all(self):
        return self._simple_call(['reverse', '--remove-all'])

    def reverse_remove(self, remote):
        return self._simple_call(['reverse', '--remove', remote])

    def wait(self):
        return self._simple_call(['wait-for-device'])

    def get_prop(self, prop_name):
        output = split_lines(self.shell(['getprop', prop_name])[0])
        if len(output) != 1:
            raise RuntimeError('Too many lines in getprop output:\n' +
                               '\n'.join(output))
        value = output[0]
        if not value.strip():
            return None
        return value

    def set_prop(self, prop_name, value):
        self.shell(['setprop', prop_name, value])
