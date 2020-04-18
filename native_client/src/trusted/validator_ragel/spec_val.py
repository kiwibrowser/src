# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import spec


class Validator(object):

  BITNESS = None
  MAX_SUPERINSTRUCTION_LENGTH = None

  def ValidateSuperinstruction(self, superinstruction):
    raise NotImplementedError()

  def FitsWithinBundle(self, insns):
    offset = insns[0].address
    last_byte_offset = offset + sum(len(insn.bytes) for insn in insns) - 1
    return offset // spec.BUNDLE_SIZE == last_byte_offset // spec.BUNDLE_SIZE

  def CheckConditions(
      self, insns, precondition, postcondition):
    raise NotImplementedError()

  def CheckFinalCondition(self, end_offset):
    raise NotImplementedError()

  def Validate(self, insns):
    self.messages = []
    self.valid_jump_targets = set()
    self.jumps = {}
    self.condition = spec.Condition()

    i = 0

    while i < len(insns):
      offset = insns[i].address
      self.valid_jump_targets.add(offset)
      try:
        # Greedy: try to match longest superinstructions first.
        for n in range(self.MAX_SUPERINSTRUCTION_LENGTH, 1, -1):
          if i + n > len(insns):
            continue
          try:
            self.ValidateSuperinstruction(insns[i:i+n])
            if not self.FitsWithinBundle(insns[i:i+n]):
              self.messages.append(
                  (offset, 'superinstruction crosses bundle boundary'))
            self.CheckConditions(
                insns[i:i+n],
                precondition=spec.Condition(),
                postcondition=spec.Condition())
            i += n
            break
          except spec.DoNotMatchError:
            continue
        else:
          try:
            jump_destination, precondition, postcondition = (
                spec.ValidateDirectJumpOrRegularInstruction(
                    insns[i],
                    self.BITNESS))
            if not self.FitsWithinBundle(insns[i:i+1]):
              self.messages.append(
                  (offset, 'instruction crosses bundle boundary'))
            self.CheckConditions(
                insns[i:i+1], precondition, postcondition)
            if jump_destination is not None:
              self.jumps[insns[i].address] = jump_destination
            i += 1
          except spec.DoNotMatchError:
            self.messages.append(
                (offset, 'unrecognized instruction %r' % insns[i].disasm))
            i += 1

      except spec.SandboxingError as e:
        self.messages.append((offset, str(e)))
        i += 1
        self.condition = spec.Condition()

    assert i == len(insns)

    end_offset = insns[-1].address + len(insns[-1].bytes)
    self.valid_jump_targets.add(end_offset)
    self.CheckFinalCondition(end_offset)

    for source, destination in sorted(self.jumps.items()):
      if (destination % spec.BUNDLE_SIZE != 0 and
          destination not in self.valid_jump_targets):
        self.messages.append(
            (source, 'jump into a middle of instruction (0x%x)' % destination))

    return self.messages


class Validator32(Validator):

  BITNESS = 32
  MAX_SUPERINSTRUCTION_LENGTH = 2

  def ValidateSuperinstruction(self, superinstruction):
    spec.ValidateSuperinstruction32(superinstruction)

  def CheckConditions(
      self, insns, precondition, postcondition):
    assert precondition == postcondition == spec.Condition()

  def CheckFinalCondition(self, end_offset):
    assert self.condition == spec.Condition()


class Validator64(Validator):

  BITNESS = 64
  MAX_SUPERINSTRUCTION_LENGTH = 5

  def ValidateSuperinstruction(self, superinstruction):
    spec.ValidateSuperinstruction64(superinstruction)

  def CheckConditions(
      self, insns, precondition, postcondition):
    offset = insns[0].address
    if not self.condition.Implies(precondition):
      self.messages.append((offset, self.condition.WhyNotImplies(precondition)))

    if not spec.Condition().Implies(precondition):
      self.valid_jump_targets.remove(offset)

    self.condition = postcondition

    end_offset = offset + sum(len(insn.bytes) for insn in insns)
    if end_offset % spec.BUNDLE_SIZE == 0:
      # At the end of bundle we reset condition to default value
      # (because anybody can jump to this point), so we have to check
      # that it's safe to do so.
      if not self.condition.Implies(spec.Condition()):
        self.messages.append((
            end_offset,
            '%s at the end of bundle'
            % self.condition.WhyNotImplies(spec.Condition())))
      self.condition = spec.Condition()

  def CheckFinalCondition(self, end_offset):
    # If chunk ends mid-bundle, we have to check final condition separately.
    if end_offset % spec.BUNDLE_SIZE != 0:
      if not self.condition.Implies(spec.Condition()):
        self.messages.append((
            end_offset,
            '%s at the end of chunk'
            % self.condition.WhyNotImplies(spec.Condition())))
