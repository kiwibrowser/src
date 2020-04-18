#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""%prog [option...] xmlfile

Simple checker for the validator's DFA."""

from __future__ import print_function

import optparse

import dfa_parser


def CheckForProperCleanup(states):
  """Check action lists for proper cleanup.

  All transitions must trigger "end_of_instruction_cleanup" as the last action
  if they transition to the accepting state.

  Triggers an assert error if this rule is violated.

  Args:
      states: list of State objects.
  Returns:
      None
  """

  for state in states:
    for transition in state.forward_transitions.values():
      if transition.to_state.is_accepting:
        assert transition.actions[-1].name == 'end_of_instruction_cleanup'


def CheckDFAActions(start_state, states):
  """Check action lists for accepting non-start states and identify all
  "dangerous" paths in the automaton.

  All paths in the DFA graph between two accepting states should fall into two
  classes:
    1. Path identical to the path from the start_state (including action lists).
       Such a path corresponds to "normal" instruction and are checked by
       separate test.
    2. Path which is not identical to any path from start_state ("dangerous"
       path).
       Such path is only valid if the final state is start_state and in this
       case one or more regular instructions must come before (situation is
       known as "superinstruction").  We list all these superinstructions here.

  For each "dangerous" path CollectSandboxing function is called. As a result,
  it returns list of superinstructions found and, more importantly, generates an
  assert error if DFA does not satisfy the requirements listed above.

  Note that situation when one register is restricted and then used by the next
  instruction to access memory is *not* considered a superinstruction and is
  *not* handled by the DFA.  Instead these cases are handled in actions via
  restricted_register variable (it's set if instruction restricts any register
  and if next command uses it then it's not a valid jump target).

  Args:
      start_state: start state of the DFA
      states: list of State objects.
  Returns:
      List of collected dangerous paths (each one is represented by a list of
      transitions)
  """

  collected_dangerous_paths = []
  for state in states:
    if state.is_accepting and state is not start_state:
      CheckPathActions(start_state, state, start_state, [], [],
                       collected_dangerous_paths)
  return collected_dangerous_paths


def CheckPathActions(start_state,
                     state, main_state,
                     transitions, main_transitions,
                     collected_dangerous_paths):
  """Identify all "dangerous" paths that start with given prefix and check
  actions along them.

  "Dangerous" paths correspond to instructions that are not safe by themselves
  and should always be prefixed with some sandboxing instructions.

  In this traversal we rely on the fact that all such sandboxing instructions
  are "regular" (that is, are safe by themselves).

  This functions adds all the dangerous paths to the collected_dangerous_paths
  list.

  Args:
      start_state: start state of the DFA
      state: state to consider
      main_state: parallel state in path which starts from start_state
      transitions: list of transitions (path prefix collected so far)
      main_transitions: list of transitions (parallel path prefix starting from
                        start_state collected so far)
      collected_dangerous_paths: list to which we add dangerous instructions
                                 (each one is represented by a list of
                                 transitions)
  Returns:
      None
  """

  if state.is_accepting and transitions != []:
    # If we reach accepting state, this path is not 'dangerous'.
    assert main_state.is_accepting
    return
  if not state.is_accepting:
    # If main state reaches accepting state but here we don't reach the
    # accepting state then this means that we process some "safe" instructions
    # as "dangerous" ones.
    assert not main_state.is_accepting
  if state is main_state:
    # Original path merges with parallel path, so it can't be dangerous
    # (whatever is accepted further along original path would be accepted
    # from starting state as well).
    return
  for transition in state.forward_transitions.values():
    main_transition = main_state.forward_transitions.get(transition.byte)
    if (main_transition is not None and
        main_transition.actions == transition.actions):
      transitions.append(transition)
      main_transitions.append(main_transition)
      CheckPathActions(start_state,
                       transition.to_state, main_transition.to_state,
                       transitions, main_transitions,
                       collected_dangerous_paths)
      main_transitions.pop()
      transitions.pop()
    else:
      transitions.append(transition)
      CheckDangerousInstructionPath(start_state, transition.to_state,
                                    transitions, collected_dangerous_paths)
      transitions.pop()


def CheckDangerousInstructionPath(start_state, state, transitions,
                                  collected_dangerous_paths):
  """Enumerate all "dangerous" paths starting from given prefix and check them.

  We assume that we already diverged from the parallel path, so there is no need
  to track it anymore.

  A "dangerous" path is a path between two accepting states consuming a
  sequences of bytes S (e.g. the encoding of "jmp *%rax") such that these
  sequences S is not accepted when starting from start_start.

  Note that "dangerous" path does not necessarily split from "safe" paths on the
  first byte (for example "dangerous" instruction "jmp *%r8" is "0x41 0xff 0xe0"
  and "safe" instruction "inc %r8d" is "0x41 0xff 0xc0" share first two bytes
  and thus the first two transitions will be identical in the DFA).

  In validator's DFA it must finish at start_state and it needs some sandboxing.
  That is: it requires some fixed set of bytes present first for the DFA to
  recognize it as valid.  There are endless number of such possible sandboxing
  byte sequences but we only list the minimal ones (see the CollectSandboxing
  for the exact definition of what "minimal" means).  If it finishes in some
  other state (not in the start_state) then this function will stop at assert
  and if DFA is correct then it adds all the possible sandboxing sequences
  to the collected_superinstructions set.

  This functions adds all the dangerous paths to the collected_dangerous_paths
  list.

  Args:
      start_state: start state of the DFA
      state: state to consider
      transitions: transitions collected so far
      collected_dangerous_paths: list to which we add dangerous instructions
                                 (each one is represented by a list of
                                 transitions)
  Returns:
      None
  """

  if state.is_accepting:
    assert state is start_state
    collected_dangerous_paths.append(list(transitions))
  else:
    for transition in state.forward_transitions.values():
      transitions.append(transition)
      CheckDangerousInstructionPath(start_state, transition.to_state,
                                    transitions, collected_dangerous_paths)
      transitions.pop()


def CollectSandboxing(start_state, transitions, collected_superinstructions):
  """Collect sandboxing for the given "dangerous" instruction.

  Definition: sandboxing sequence for a given set of transitions is a sequence
  of bytes which are accepted by the DFA starting from the start_state which
  finishes with a given sequence of transitions.

  Many sandboxing sequences are suffixes of other sandboxing sequences.  For
  example sequence of bytes accept correspond to the sequence of instructions
  "and $~0x1f,%eax; and $~0x1f,%ebx; add %r15,%rbx; jmp *%rbx" is one such
  sequence (for the transitions which accept "jmp *%rbx"), but it includes a
  shorter one: "and $~0x1f,%ebx; add %r15,%rbx; jmp *%rbx".

  If any sandboxing sequence is suffix of some other sandboxing sequence and it
  triggers different actions then this function should assert, otherwise it
  adds all the minimal sandboxing sequences (sandboxings which are not
  suffixes of any other sandboxing sequences) to the collected_superinstructions
  set.

  Args:
      start_state: start state of the DFA
      transitions: transitions collected so far
                   (note: we go back in graph here, not forward)
      collected_superinstructions: list to which we add superinstructions (each
                                   one is represented by a list of transitions)
  Returns:
      None
  """

  if transitions[0].from_state.is_accepting:
    # We've added one additional instruction.  Let's see if this sequence is
    # recognizable from the beginning.
    main_state = start_state
    for transition in transitions:
      main_transition = main_state.forward_transitions.get(transition.byte)
      # No suitable transitions.  We need to go deeper.
      if main_transition is None:
        break
      assert main_transition.actions == transition.actions
      assert (main_transition.to_state.is_accepting ==
              transition.to_state.is_accepting)
      main_state = main_transition.to_state
    else:
      # Transition is recognized from the initial path
      collected_superinstructions.append(list(transitions))
      return
  assert len(transitions[0].from_state.back_transitions) > 0
  for transition in transitions[0].from_state.back_transitions:
    transitions.insert(0, transition)
    CollectSandboxing(start_state, transitions, collected_superinstructions)
    transitions.pop(0)


def main():
  parser = optparse.OptionParser(__doc__)
  parser.add_option('-o', '--out',
                    help='Output file name (instead of sys.stdout')

  (options, args) = parser.parse_args()

  if len(args) != 1:
    parser.error('need exactly one ragel XML file')

  xml_file = args[0]
  dfa = dfa_parser.ParseXml(xml_file)

  CheckForProperCleanup(dfa.states)

  collected_dangerous_paths = CheckDFAActions(dfa.initial_state, dfa.states)

  all_superinstructions = set()
  for dangerous_paths in collected_dangerous_paths:
    superinstructions = []
    CollectSandboxing(dfa.initial_state, dangerous_paths, superinstructions)
    assert len(superinstructions) > 0
    for superinstruction in superinstructions:
      all_superinstructions.add(
          '.byte ' + ', '.join(['0x{0:02x}'.format(transition.byte)
                                for transition in superinstruction]))

  if options.out is not None:
    with open(options.out, "w") as out_file:
      for superinstruction in sorted(all_superinstructions):
        print(superinstruction, file=out_file)
  else:
    for superinstruction in sorted(all_superinstructions):
      print(superinstruction)


if __name__ == '__main__':
  main()
