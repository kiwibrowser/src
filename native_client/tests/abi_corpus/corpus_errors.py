#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A list of known bad NEXEs and CRXs in the corpus."""

import corpus_utils


# Sha1s of known non-validating NEXEs in the corpus.
BAD_NEXES = set([
    '1c2b051cb60367cf103128c9cd76769ffa1cf356',
    '41c4a2e842da16de1c1be9e89fbfee5e695e2f4b',
    '903d3e5f29b909db162f2679363685144f61e3cc',
    'bb127ff5b58caa8b875b19c05b9ade8c0881a5ed',
    'f4814677d321785f3bcd65f16bafa728b5dbc883',
    'fac308d79f5c2d3a7f1f27112cfe19bf8431acfa',
])


NEXES_TO_SKIP = set([
    # TODO(shcherbina): remove when
    # http://code.google.com/p/nativeclient/issues/detail?id=3149 is fixed.
    'fac308d79f5c2d3a7f1f27112cfe19bf8431acfa',
    'bb127ff5b58caa8b875b19c05b9ade8c0881a5ed',
    '903d3e5f29b909db162f2679363685144f61e3cc',
    '41c4a2e842da16de1c1be9e89fbfee5e695e2f4b',
    'f4814677d321785f3bcd65f16bafa728b5dbc883',
])


class CrxResult(object):
  def __init__(self, id, custom=None, precidence=1):
    self.id = id
    self.custom = custom
    self.precidence = precidence

  def __str__(self):
    return "corpus_errors.CrxResult('%s', custom='%s')" % (
        self.id, self.custom)

  def Matches(self, other):
    return (self.id == other.id and
            self.custom == other.custom and
            self.precidence == other.precidence)

  def Merge(self, other):
    if other.precidence > self.precidence:
      return other
    else:
      return self


# Common result types.
GOOD = CrxResult('GOOD', precidence=0)
MODULE_DIDNT_START = CrxResult('nacl module not started', precidence=2)
MODULE_CRASHED = CrxResult('nacl module crashed', precidence=3)
BAD_MANIFEST = CrxResult('BAD MANIFEST!', precidence=4)
COULD_NOT_LOAD = CrxResult('could not load', precidence=5)

# Mapping of sha1 to expected result for known bad CRXs.
BAD_CRXS = {
  # Bad manifest
  '0937b653af5553856532454ec340d0e0075bc0b4': BAD_MANIFEST,
  '09ffe3793113fe564b71800a5844189c00bd8210': BAD_MANIFEST,
  '14f389a8c406d60e0fc05a1ec0189a652a1f006e': BAD_MANIFEST,
  '2f97cec9f13b0f774d1f49490f26f32213e4e0a5': BAD_MANIFEST,
  '3d6832749c8c1346c65b30f4b191930dec5f04a3': BAD_MANIFEST,
  '612a5aaa821b4b636168025f027e721c0f046e7c': BAD_MANIFEST,
  '81a4a3de69dd4ad169b1d4a7268b44c78ea5ffa8': BAD_MANIFEST,
  '8249dc3bafb983b82a588df97b2f92621a3556c1': BAD_MANIFEST,
  '9afa2fc3dcfef799e6371013a245d083f1a66101': BAD_MANIFEST,
  'a8aa42d699dbef3e1403e4fdc49325e89a91f653': BAD_MANIFEST,
  'c6d40d4f3c8dccc710d8c09bfd074b2d20a504d2': BAD_MANIFEST,
  'ced1fea90b71b0a8da08c1a1e6cb35975cc84f52': BAD_MANIFEST,
  # No nacl module
  '12e167d935a1f339a37b223b545d6f7ed7b474f7': MODULE_DIDNT_START,
  '23eaab8e1ac0fad4f943c1e3c1664a392a316a34': MODULE_DIDNT_START,
  '48ac71edfeac0c79d5e2b651e082abfa76da25f9': MODULE_DIDNT_START,
  '57be161e5ff7011d2283e507a70f9005c448002b': MODULE_DIDNT_START,
  '5b9f0f7f7401cb666015090908d97346f5b6bccb': MODULE_DIDNT_START,
  '6f912a0c7d6d762c2df2ac42893328a2fb357c42': MODULE_DIDNT_START,
  '76dd4cb33ebdeb26e80a5e1105e9c306ec17efc9': MODULE_DIDNT_START,
  '78bbafcec8832252b0e694cc0c9387a68fac5299': MODULE_DIDNT_START,
  '8f1dcad3539281ea66857f36059b8085d3dfc37e': MODULE_DIDNT_START,
  'c188ce67e32c8a8ca9543875a35078b36f2b02a5': MODULE_DIDNT_START,
  'cfd62adf6790eed0520da2deb2246fc02e70c57e': MODULE_DIDNT_START,
  'd1f33ad38f9f6e78150d30c2103b5c77a9f0adcd': MODULE_DIDNT_START,
  'd51802b12a503f7fdfbec60d96e27a5ffa09d002': MODULE_DIDNT_START,
  # NaCl module crash
  'ab0692192976dc6693212582364c6f23b6551d1a': MODULE_CRASHED,
  'b458cd57c8b4e6c313b18f370fad59779f573afc': MODULE_CRASHED,
  # Could not load extension.
  '1f861c0d8c173b64df3e70cfa1a5cd710ba59430': COULD_NOT_LOAD,
  '4beecff67651f13e013c12a5bf3661041ded323c': COULD_NOT_LOAD,
  '8de65668cc7280ffb70ffd2fa5b2a22112156966': COULD_NOT_LOAD,
  }


def NexeShouldValidate(path):
  """Checks a blacklist to decide if a nexe should validate.

  Args:
    path: path to the nexe.
  Returns:
    Boolean indicating if the nexe should validate.
  """
  return corpus_utils.Sha1FromFilename(path) not in BAD_NEXES


def NexeShouldBeSkipped(path):
  """Checks if nexe is in the skip list.

  Args:
    path: path to the nexe.
  Returns:
    Boolean indicating if the nexe should be skipped.
  """
  return corpus_utils.Sha1FromFilename(path) in NEXES_TO_SKIP


def ExpectedCrxResult(path):
  """Checks what the expected result for this app is.

  Args:
    path: path to the crx.
  Returns:
    Reg-ex that matches the expected status.
  """
  return BAD_CRXS.get(corpus_utils.Sha1FromFilename(path), GOOD)
