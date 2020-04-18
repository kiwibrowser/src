# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

def GetNumSuffixes(start_state):
  """Compute number of minimal suffixes automaton accepts from each state.

  For each state reachable from given state, compute number of paths in the
  automaton that start from that state, end in the accepting state and do not
  pass through accepting states in between.

  It is assumed that there are no cyclic paths going entirely through
  non-accepting states.

  Args:
    start_state: start state (as defined in dfa_parser.py).

  Returns:
    Dictionary from reachable states to numbers of suffixes.
  """
  num_suffixes = {}

  def ComputeNumSuffixes(state):
    if state in num_suffixes:
      return

    if state.is_accepting:
      num_suffixes[state] = 1

      # Even though the state itself is accepting, there may be more reachable
      # states behind it.
      for t in state.forward_transitions.values():
        ComputeNumSuffixes(t.to_state)

      return

    if state.any_byte:
      next_state = state.forward_transitions[0].to_state
      ComputeNumSuffixes(next_state)
      num_suffixes[state] = num_suffixes[next_state]
      return

    count = 0
    for t in state.forward_transitions.values():
      ComputeNumSuffixes(t.to_state)
      count += num_suffixes[t.to_state]
    num_suffixes[state] = count

  ComputeNumSuffixes(start_state)
  return num_suffixes


def TraverseTree(state, final_callback, prefix, anyfield=0x01):
  if state.is_accepting:
    final_callback(prefix)
    return

  if state.any_byte:
    assert anyfield < 256
    TraverseTree(
        state.forward_transitions[0].to_state,
        final_callback,
        prefix + [anyfield],
        anyfield=0 if anyfield == 0 else anyfield + 0x11)
    # We add 0x11 each time to get sequence 01 12 23 etc.

    # TODO(shcherbina): change it to much nicer 0x22 once
    # http://code.google.com/p/nativeclient/issues/detail?id=3164 is fixed
    # (right now we want to keep immediates small to avoid problem with sign
    # extension).
  else:
    for byte, t in state.forward_transitions.iteritems():
      TraverseTree(
          t.to_state,
          final_callback,
          prefix + [byte],
          anyfield)


def CreateTraversalTasks(states, initial_state):
  """Create list of DFA traversal subtasks (handy for parallelization).

  Note that unlike TraverseTree function, which stops on accepting states,
  here we go past initial state even when it's accepted.

  Args:
    initial_state: initial state of the automaton.

  Returns:
    List of (prefix, state_index) pairs, where prefix is list of bytes.
  """
  assert not initial_state.any_byte
  num_suffixes = GetNumSuffixes(initial_state)

  # Split by first three bytes.
  tasks = []
  for byte1, t1 in sorted(initial_state.forward_transitions.items()):
    state1 = t1.to_state
    if state1.any_byte or state1.is_accepting or num_suffixes[state1] < 10**4:
      tasks.append(([byte1], states.index(state1)))
      continue
    for byte2, t2 in sorted(state1.forward_transitions.items()):
      state2 = t2.to_state
      if state2.any_byte or state2.is_accepting or num_suffixes[state2] < 10**4:
        tasks.append(([byte1, byte2], states.index(state2)))
        continue
      for byte3, t3 in sorted(state2.forward_transitions.items()):
        state3 = t3.to_state
        tasks.append(([byte1, byte2, byte3], states.index(state3)))

  return tasks
