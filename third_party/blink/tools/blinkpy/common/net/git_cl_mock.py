# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from blinkpy.common.net.git_cl import CLStatus, GitCL

# pylint: disable=unused-argument

class MockGitCL(object):

    def __init__(
            self, host, try_job_results=None, status='closed',
            issue_number='1234', time_out=False):
        """Constructs a fake GitCL with canned return values.

        Args:
            host: Host object, used for builder names.
            try_job_results: A dict of Build to TryJobStatus.
            status: CL status string.
            issue_number: CL issue number as a string.
            time_out: Whether to simulate timing out while waiting.
        """
        self._builders = host.builders.all_try_builder_names()
        self._status = status
        self._try_job_results = try_job_results
        self._issue_number = issue_number
        self._time_out = time_out
        self.calls = []

    def run(self, args):
        self.calls.append(['git', 'cl'] + args)
        return 'mock output'

    def trigger_try_jobs(self, builders, bucket=None):
        bucket = bucket or 'master.tryserver.blink'
        command = ['try', '-B', bucket]
        for builder in sorted(builders):
            command.extend(['-b', builder])
        self.run(command)

    def get_issue_number(self):
        return self._issue_number

    def try_job_results(self, **_):
        return self._try_job_results

    def wait_for_try_jobs(self, **_):
        if self._time_out:
            return None
        return CLStatus(
            self._status,
            self.filter_latest(self._try_job_results))

    def wait_for_closed_status(self, **_):
        if self._time_out:
            return None
        return 'closed'

    def latest_try_jobs(self, builder_names=None, **_):
        return self.filter_latest(self._try_job_results)

    @staticmethod
    def filter_latest(try_results):
        return GitCL.filter_latest(try_results)

    @staticmethod
    def all_finished(try_results):
        return GitCL.all_finished(try_results)

    @staticmethod
    def all_success(try_results):
        return GitCL.all_success(try_results)

    @staticmethod
    def some_failed(try_results):
        return GitCL.some_failed(try_results)
