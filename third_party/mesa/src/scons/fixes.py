import sys

# Monkey patch os.spawnve on windows to become thread safe
if sys.platform == 'win32':
    import os
    import threading
    from os import spawnve as old_spawnve

    spawn_lock = threading.Lock()

    def new_spawnve(mode, file, args, env):
        spawn_lock.acquire()
        try:
            if mode == os.P_WAIT:
                ret = old_spawnve(os.P_NOWAIT, file, args, env)
            else:
                ret = old_spawnve(mode, file, args, env)
        finally:
            spawn_lock.release()
        if mode == os.P_WAIT:
            pid, status = os.waitpid(ret, 0)
            ret = status >> 8
        return ret

    os.spawnve = new_spawnve


