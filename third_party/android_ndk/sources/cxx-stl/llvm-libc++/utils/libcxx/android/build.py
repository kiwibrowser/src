import os
import subprocess


def mm(path, android_build_top):
    env = os.environ
    env['ONE_SHOT_MAKEFILE'] = os.path.join(path, 'Android.mk')

    cmd = [
        'make', '-C', android_build_top, '-f', 'build/core/main.mk',
        'MODULES-IN-' + path.replace('/', '-'), '-B'
    ]
    return not subprocess.Popen(cmd, stdout=None, stderr=None, env=env).wait()
