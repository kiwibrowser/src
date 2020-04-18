#-------------------------------------------------------------------------------
# test/utils.py
#
# Some common utils for tests
#
# Eli Bendersky (eliben@gmail.com)
# This code is in the public domain
#-------------------------------------------------------------------------------
import os, sys, subprocess, tempfile

# This module should not import elftools before setup_syspath() is called!
# See the Hacking Guide in the documentation for more details.

def setup_syspath():
    """ Setup sys.path so that tests pick up local pyelftools before the
        installed one when run from development directory.
    """
    if sys.path[0] != '.':
        sys.path.insert(0, '.')


def run_exe(exe_path, args=[]):
    """ Runs the given executable as a subprocess, given the
        list of arguments. Captures its return code (rc) and stdout and
        returns a pair: rc, stdout_str
    """
    popen_cmd = [exe_path] + args
    if os.path.splitext(exe_path)[1] == '.py':
        popen_cmd.insert(0, sys.executable)
    proc = subprocess.Popen(popen_cmd, stdout=subprocess.PIPE)
    proc_stdout = proc.communicate()[0]
    from elftools.common.py3compat import bytes2str
    return proc.returncode, bytes2str(proc_stdout)


def is_in_rootdir():
    """ Check whether the current dir is the root dir of pyelftools
    """
    return os.path.isdir('test') and os.path.isdir('elftools')


def dump_output_to_temp_files(testlog, *args):
    """ Dumps the output strings given in 'args' to temp files: one for each
        arg.
    """
    for i, s in enumerate(args):
        fd, path = tempfile.mkstemp(
                prefix='out' + str(i + 1) + '_',
                suffix='.stdout')
        file = os.fdopen(fd, 'w')
        file.write(s)
        file.close()
        testlog.info('@@ Output #%s dumped to file: %s' % (i + 1, path))

