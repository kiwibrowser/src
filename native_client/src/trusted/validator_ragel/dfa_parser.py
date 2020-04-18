#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
from lxml import etree


DFA = collections.namedtuple(
    'DFA',
    ['states', 'initial_state', 'error_action'])


class State(object):

  __slots__ = [
      # Index in list of all states of the automaton.
      'index',

      # Dictionary {byte: transition}. If byte is not accepted, there is no
      # corresponding entry in the dictionary.
      'forward_transitions',

      # Backward transitions:
      #   List of transitions which can bring us to a given state
      'back_transitions',

      # True if state is an accepting state
      'is_accepting',

      # True if all transitions lead to the same state and marked with special
      # '@any_byte' action. It is used to distinguish immediates, displacements
      # and relative jump targets - stuff that we do not need to enumerate in
      # enumeration tests.
      'any_byte',
  ]


class Transition(object):

  __slots__ = [
      # This transition goes from the
      'from_state',
      # to the
      'to_state',
      # if
      'byte',
      # is encountered and it executes
      'actions'
      # if that happens.
  ]


Action = collections.namedtuple('Action', ['name', 'body'])


def ParseXml(xml_file):
  """Parse XML representation of automaton and return (states, start) pair.

  Args:
    xml_file: name of the XML file to parse.

  Returns:
    DFA object.
  """

  xml_tree = etree.parse(xml_file)

  actions = []
  xml_actions = xml_tree.xpath('//action')

  for expected_action_id, action in enumerate(xml_actions):
    # Ragel always dumps actions in order, but better to check that it's still
    # true.
    action_id = int(action.get('id'))
    assert action_id == expected_action_id
    # If action have no name then use it's text as a name
    action_name = action.get('name')
    action_body = ''
    assert len(action) <= 1
    if len(action) == 1:
      action_body = action[0].text
      assert action_body is not None
    actions.append(Action(action_name, action_body))

  action_tables = []
  xml_action_tables = xml_tree.xpath('//action_table')

  for expected_action_table_id, action_table in enumerate(xml_action_tables):
    # Ragel always dumps action tables in order, but better to check that it's
    # still true.
    action_table_id = int(action_table.get('id'))
    assert action_table_id == expected_action_table_id
    action_list = [actions[int(action_id)]
                   for action_id in action_table.text.split()]
    action_tables.append(action_list)

  # There are only one global error action and it's called "report_fatal_error"
  (error_action,) = action_tables[0]
  assert error_action.name == 'report_fatal_error'

  (error_state,) = xml_tree.xpath('//error_state')
  error_state_id = int(error_state.text)
  assert error_state_id == 0

  xml_states = xml_tree.xpath('//state')

  states = [State() for _ in xml_states]

  for expected_state_id, xml_state in enumerate(xml_states):
    # Ragel always dumps states in order, but better to check that it's still
    # true.
    state_id = int(xml_state.get('id'))
    assert state_id == expected_state_id

    state = states[state_id]

    state.index = state_id
    state.forward_transitions = {}
    state.back_transitions = []
    state.is_accepting = bool(xml_state.get('final'))

    (xml_transitions,) = xml_state.xpath('trans_list')
    # Mark available transitions.
    for t in xml_transitions.getchildren():
      range_begin, range_end, next_state, action_table_id = t.text.split(' ')
      if next_state == 'x':
        # This is error action. We are supposed to have just one of these.
        assert action_table_id == '0'
        continue

      for byte in range(int(range_begin), int(range_end) + 1):
        transition = Transition()
        transition.from_state = states[state_id]
        transition.byte = byte
        transition.to_state = states[int(next_state)]
        if action_table_id == 'x':
          transition.actions = []
        else:
          transition.actions = action_tables[int(action_table_id)]
        state.forward_transitions[byte] = transition

    # State actions are only ever used in validator to detect error conditions
    # in non-accepting states.  Check that it's always so.
    state_actions = xml_state.xpath('state_actions')
    if state.is_accepting or state_id == error_state_id:
      assert len(state_actions) == 0
    else:
      assert len(state_actions) == 1
      (state_actions,) = state_actions
      assert state_actions.text == 'x x 0'

    # We mark the state as 'any_byte' if all transitions are present, lead to
    # the same state, are marked with the same actions, and 'any_byte' action
    # is among them.
    transitions = state.forward_transitions.values()
    if (len(transitions) == 256 and
        len(set(t.to_state for t in transitions)) == 1 and
        len(set(tuple(t.actions) for t in transitions)) == 1 and
        any(action.name == 'any_byte' for action in transitions[0].actions)):
      state.any_byte = True
    else:
      state.any_byte = False

  # Populate backward transitions.
  for state in states:
    for transition in state.forward_transitions.values():
      transition.to_state.back_transitions.append(transition)

  start_states = xml_tree.xpath('//start_state')
  assert len(start_states) == 1, 'Can not find the initial state in the XML'
  start_state = states[int(start_states[0].text)]

  return DFA(states, start_state, error_action)
