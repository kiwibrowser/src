# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module defines the core structure of the classification rules.

This module does NOT specify how the rules filter the data: this responsibility
is of to the concrete classifiers, which have to override the Rule class herein
defined and know how to do the math.

This module, instead, defines the format of the rules and the way they are
encoded and loaded (in a python-style dictionary file).
Rules are organized in a tree, where the root is always represented by a 'Total'
node, and the leaves are arbitrarily defined by the user, according to the
following principles:
- Order of siblings rules matter: what is caught by a rule will not be caught
  by the next ones, but it is propagated to its children rules if any.
- Every non-leaf node X gets an implicit extra-children named X-other. This
  catch-all child catches everything (within the parent rule scope) that is
  not caught by the other siblings. This is to guarantee that, when doing the
  math (the aggregation), at any level, the sum of the values in the leaves
  match the value of their parent.

The format of a rule dictionary is the following:
[
{
  'name':       'Name of the rule',
  'filter-X':   'The embedder will know how to interpret this value and will use
                 it to filter the data'
  'filter-Y':   'Idem'
  children: [
    {
      'name':   'Name of the sub-rule 1'
      ... and so on recursively ,
    },
  ]
},
]

And a typical resulting rule tree looks like this:
                          +----------------------+
                          |        Total         |
                          |----------------------|
       +------------------+      Match all.      +--------------------+
       |                  +----------+-----------+                    |
       |                             |                                |
 +-----v-----+                 +-----v-----+                   +------v----+
 |    Foo    |                 |    Bar    |                   |Total-other|
 |-----------|                 |-----------|                   |-----------|
 |File: foo* |             +---+File: bar* +-----+             | Match all |
 +-----------+             |   +-----------+     |             +-----------+
                           |                     |
                    +------v------+       +------v----+
                    | Bar::Coffee |       | Bar-other |
                    |-------------|       |-----------|
                    |File: bar*cof|       | Match all |
                    +-------------+       +-----------+
"""

import ast


def Load(content, rule_builder):
  """Construct a rule tree from a python-style dict representation.

  Args:
    content: a string containing the dict (i.e. content of the rule file).
    rule_builder: a method which takes two arguments (rule_name, filters_dict)
        and returns a subclass of |Rule|. |filters_dict| is a dict of the keys
        (filter-foo, filter-bar in the example above) for the rule node.
  """
  rules_dict = ast.literal_eval(content)
  root = Rule('Total')
  _MakeRuleNodeFromDictNode(root, rules_dict, rule_builder)
  return root


class Rule(object):
  """ An abstract class representing a rule node in the rules tree.

  Embedders must override the Match method when deriving this class.
  """

  def __init__(self, name):
    self.name = name
    self.children = []

  def Match(self, _):  # pylint: disable=R0201
    """ The rationale of this default implementation is modeling the root
    ('Total') and the catch-all (*-other) rules that every |RuleTree| must have,
    regardless of the embedder-specific children rules. This is to guarantee
    that the totals match at any level of the tree.
    """
    return True

  def AppendChild(self, child_rule):
    assert(isinstance(child_rule, Rule))
    duplicates = filter(lambda x: x.name == child_rule.name, self.children)
    assert(not duplicates), 'Duplicate rule ' + child_rule.name
    self.children.append(child_rule)


def _MakeRuleNodeFromDictNode(rule_node, dict_nodes, rule_builder):
  """Recursive rule tree builder for traversing the rule dict."""
  for dict_node in dict_nodes:
    assert('name' in dict_node)
    # Extract the filter keys (e.g., mmap-file, mmap-prot) that will be passed
    # to the |rule_builder|
    filter_keys = set(dict_node.keys()) - set(('name', 'children'))
    filters = dict((k, dict_node[k]) for k in filter_keys)
    child_rule = rule_builder(dict_node['name'], filters)
    rule_node.AppendChild(child_rule)
    dict_children = dict_node.get('children', {})
    _MakeRuleNodeFromDictNode(child_rule, dict_children, rule_builder)

    # If the rule_node isn't a leaf, add the 'name-other' catch-all sibling to
    # catch all the entries that matched this node but none of its children.
  if len(rule_node.children):
    rule_node.AppendChild(Rule(rule_node.name + '-other'))