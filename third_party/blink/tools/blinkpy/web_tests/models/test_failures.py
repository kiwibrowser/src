# Copyright (C) 2010 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import cPickle

from blinkpy.web_tests.models import test_expectations


def is_reftest_failure(failure_list):
    input_failure_types = {type(f) for f in failure_list}
    reftest_failure_types = {
        FailureReftestMismatch,
        FailureReftestMismatchDidNotOccur,
        FailureReftestNoImageGenerated,
        FailureReftestNoReferenceImageGenerated
    }
    return bool(input_failure_types & reftest_failure_types)


def has_failure_type(failure_type, failure_list):
    return any(isinstance(failure, failure_type) for failure in failure_list)


# FIXME: This is backwards.  Each TestFailure subclass should know what
# test_expectation type it corresponds too.  Then this method just
# collects them all from the failure list and returns the worst one.


def determine_result_type(failure_list):
    """Takes a set of test_failures and returns which result type best fits
    the list of failures. "Best fits" means we use the worst type of failure.

    Returns:
      one of the test_expectations result types - PASS, FAIL, CRASH, etc.
    """

    if not failure_list or len(failure_list) == 0:
        return test_expectations.PASS

    if has_failure_type(FailureCrash, failure_list):
        return test_expectations.CRASH
    elif has_failure_type(FailureLeak, failure_list):
        return test_expectations.LEAK
    elif has_failure_type(FailureTimeout, failure_list):
        return test_expectations.TIMEOUT
    elif has_failure_type(FailureEarlyExit, failure_list):
        return test_expectations.SKIP
    elif (has_failure_type(FailureMissingResult, failure_list) or
          has_failure_type(FailureMissingImage, failure_list) or
          has_failure_type(FailureMissingImageHash, failure_list) or
          has_failure_type(FailureMissingAudio, failure_list)):
        return test_expectations.MISSING
    else:
        is_text_failure = (has_failure_type(FailureTextMismatch, failure_list) or
                           has_failure_type(FailureTestHarnessAssertion, failure_list))
        is_image_failure = (has_failure_type(FailureImageHashIncorrect, failure_list) or
                            has_failure_type(FailureImageHashMismatch, failure_list) or
                            is_reftest_failure(failure_list))
        is_audio_failure = has_failure_type(FailureAudioMismatch, failure_list)
        if is_text_failure and is_image_failure:
            return test_expectations.IMAGE_PLUS_TEXT
        elif is_text_failure:
            return test_expectations.TEXT
        elif is_image_failure:
            return test_expectations.IMAGE
        elif is_audio_failure:
            return test_expectations.AUDIO
        else:
            failure_types = [type(failure) for failure in failure_list]
            raise ValueError('unclassifiable set of failures: '
                             + str(failure_types))


class TestFailure(object):
    """Abstract base class that defines the failure interface."""

    @staticmethod
    def loads(s):
        """Creates a TestFailure object from the specified string."""
        return cPickle.loads(s)

    def message(self):
        """Returns a string describing the failure in more detail."""
        raise NotImplementedError

    def __eq__(self, other):
        return self.__class__.__name__ == other.__class__.__name__

    def __ne__(self, other):
        return self.__class__.__name__ != other.__class__.__name__

    def __hash__(self):
        return hash(self.__class__.__name__)

    def dumps(self):
        """Returns the string/JSON representation of a TestFailure."""
        return cPickle.dumps(self)

    def driver_needs_restart(self):
        """Returns True if we should kill the driver before the next test."""
        return False


class FailureTimeout(TestFailure):

    def __init__(self, is_reftest=False):
        super(FailureTimeout, self).__init__()
        self.is_reftest = is_reftest

    def message(self):
        return 'test timed out'

    def driver_needs_restart(self):
        return True


class FailureCrash(TestFailure):

    def __init__(self, is_reftest=False, process_name='content_shell', pid=None, has_log=False):
        super(FailureCrash, self).__init__()
        self.process_name = process_name
        self.pid = pid
        self.is_reftest = is_reftest
        self.has_log = has_log

    def message(self):
        if self.pid:
            return '%s crashed [pid=%d]' % (self.process_name, self.pid)
        return self.process_name + ' crashed'

    def driver_needs_restart(self):
        return True


class FailureLeak(TestFailure):

    def __init__(self, is_reftest=False, log=''):
        super(FailureLeak, self).__init__()
        self.is_reftest = is_reftest
        self.log = log

    def message(self):
        return 'leak detected: %s' % (self.log)


class FailureMissingResult(TestFailure):

    def message(self):
        return '-expected.txt was missing'


class FailureTestHarnessAssertion(TestFailure):

    def message(self):
        return 'asserts failed'


class FailureTextMismatch(TestFailure):

    def message(self):
        return 'text diff'

    def text_mismatch_category(self):
        return 'general text mismatch'


class FailureSpacesAndTabsTextMismatch(FailureTextMismatch):

    def message(self):
        return 'text diff by spaces and tabs only'

    def text_mismatch_category(self):
        return 'spaces and tabs only'


class FailureLineBreaksTextMismatch(FailureTextMismatch):

    def message(self):
        return 'text diff by newlines only'

    def text_mismatch_category(self):
        return 'newlines only'


class FailureSpaceTabLineBreakTextMismatch(FailureTextMismatch):

    def message(self):
        return 'text diff by spaces, tabs and newlines only'

    def text_mismatch_category(self):
        return 'spaces, tabs and newlines only'


class FailureMissingImageHash(TestFailure):

    def message(self):
        return '-expected.png was missing an embedded checksum'


class FailureMissingImage(TestFailure):

    def message(self):
        return '-expected.png was missing'


class FailureImageHashMismatch(TestFailure):

    def message(self):
        return 'image diff'


class FailureImageHashIncorrect(TestFailure):

    def message(self):
        return '-expected.png embedded checksum is incorrect'


class FailureReftestMismatch(TestFailure):

    def __init__(self, reference_filename=None):
        super(FailureReftestMismatch, self).__init__()
        self.reference_filename = reference_filename

    def message(self):
        return 'reference mismatch'


class FailureReftestMismatchDidNotOccur(TestFailure):

    def __init__(self, reference_filename=None):
        super(FailureReftestMismatchDidNotOccur, self).__init__()
        self.reference_filename = reference_filename

    def message(self):
        return "reference mismatch didn't happen"


class FailureReftestNoImageGenerated(TestFailure):

    def __init__(self, reference_filename=None):
        super(FailureReftestNoImageGenerated, self).__init__()
        self.reference_filename = reference_filename

    def message(self):
        return "reference test didn't generate pixel results"


class FailureReftestNoReferenceImageGenerated(TestFailure):

    def __init__(self, reference_filename=None):
        super(FailureReftestNoReferenceImageGenerated, self).__init__()
        self.reference_filename = reference_filename

    def message(self):
        return "-expected.html didn't generate pixel results"


class FailureMissingAudio(TestFailure):

    def message(self):
        return 'expected audio result was missing'


class FailureAudioMismatch(TestFailure):

    def message(self):
        return 'audio mismatch'


class FailureEarlyExit(TestFailure):

    def message(self):
        return 'skipped due to early exit'


# Convenient collection of all failure classes for anything that might
# need to enumerate over them all.
ALL_FAILURE_CLASSES = (FailureTimeout, FailureCrash, FailureMissingResult,
                       FailureTestHarnessAssertion,
                       FailureTextMismatch, FailureSpacesAndTabsTextMismatch,
                       FailureLineBreaksTextMismatch, FailureSpaceTabLineBreakTextMismatch,
                       FailureMissingImageHash,
                       FailureMissingImage, FailureImageHashMismatch,
                       FailureImageHashIncorrect, FailureReftestMismatch,
                       FailureReftestMismatchDidNotOccur,
                       FailureReftestNoImageGenerated,
                       FailureReftestNoReferenceImageGenerated,
                       FailureMissingAudio, FailureAudioMismatch,
                       FailureEarlyExit)
