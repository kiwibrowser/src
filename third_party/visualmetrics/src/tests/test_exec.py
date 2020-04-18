#!/usr/bin/python
"""
Copyright (c) 2014, Google Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of the <ORGANIZATION> nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""
import os
import shutil
import subprocess
import sys
import tempfile
import unittest


class TestExec(unittest.TestCase):
    temp_dir = None

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp(prefix='test_visualmetrics', dir='/tmp')

    def tearDown(self):
        if self.temp_dir:
            shutil.rmtree(self.temp_dir, True)
            self.temp_dir = None

    def _GetResourceFilename(self, name):
        self_dir = os.path.dirname(os.path.abspath(__file__))
        ret = os.path.abspath(os.path.join(self_dir, name))
        if not os.path.isfile(ret):
            raise IOError('File not found: {0}'.format(name))
        return ret

    def _Popen(self, args, **kwargs):
        command = self._GetResourceFilename('../visualmetrics.py')
        return subprocess.Popen(['python', command] + args, **kwargs)

    def _runTest(self, data_dir, check, add_args=None):
        video_path = self._GetResourceFilename(os.path.join(data_dir, 'video.mp4'))
        timeline_path = self._GetResourceFilename(os.path.join(data_dir, 'timeline.json'))
        test_stdout_path = self._GetResourceFilename(os.path.join(data_dir, check))
        args = [
            '--force',
            '-vv',
            '--orange',
            '--dir', self.temp_dir,
            '--video', video_path,
            '--timeline', timeline_path]
        if add_args:
            args.append(add_args)
        process = self._Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        sys.stdout.write(stdout)
        sys.stderr.write(stderr)
        retcode = process.poll()
        if retcode != 0 and not stdout and not stderr:
            process = self._Popen(['--check'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout, stderr = process.communicate()
            sys.stdout.write(stdout)
            sys.stderr.write(stderr)
        self.assertEqual(retcode, 0)
        # self.assertEqual(stderr, '')
        with open(test_stdout_path) as logfile:
            expected_stdout = logfile.read()
        self.assertEqual(stdout, expected_stdout)

    def test_lemons(self):
        self._runTest('data/lemons', check='test_stdout.txt')

    def test_lemons_perceptual(self):
        self._runTest('data/lemons', check='test_stdout_perceptual.txt', add_args='--perceptual')


if __name__ == '__main__':
    unittest.main()
