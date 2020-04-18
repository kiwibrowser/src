import os
import subprocess
import sys

def GetObjcopyCmd(target):
    """Return a suitable objcopy command."""
    if target == 'mips32':
      return 'mipsel-nacl-objcopy'
    return 'arm-nacl-objcopy'

def GetObjdumpCmd(target):
    """Return a suitable objdump command."""
    if target == 'mips32':
      return 'mipsel-nacl-objdump'
    return 'arm-nacl-objdump'

def shellcmd(command, echo=True):
    if not isinstance(command, str):
        command = ' '.join(command)

    if echo:
      print >> sys.stderr, '[cmd]'
      print >> sys.stderr,  command
      print >> sys.stderr

    stdout_result = subprocess.check_output(command, shell=True)
    if echo: sys.stdout.write(stdout_result)
    return stdout_result

def FindBaseNaCl():
    """Find the base native_client/ directory."""
    nacl = 'native_client'
    path_list = os.getcwd().split(os.sep)
    """Use the executable path if cwd does not contain 'native_client' """
    path_list = path_list if nacl in path_list else sys.argv[0].split(os.sep)
    if nacl not in path_list:
        print "Script must be executed from within 'native_client' directory"
        exit(1)
    last_index = len(path_list) - path_list[::-1].index(nacl)
    return os.sep.join(path_list[:last_index])

def get_sfi_string(args, sb_ret, nonsfi_ret, native_ret):
    """Return a value depending on args.sandbox and args.nonsfi."""
    if args.sandbox:
        assert(not args.nonsfi)
        return sb_ret
    if args.nonsfi:
        return nonsfi_ret
    return native_ret
