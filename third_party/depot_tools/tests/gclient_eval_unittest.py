#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import itertools
import logging
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from third_party import schema

import gclient
import gclient_eval


class GClientEvalTest(unittest.TestCase):
  def test_str(self):
    self.assertEqual('foo', gclient_eval._gclient_eval('"foo"'))

  def test_tuple(self):
    self.assertEqual(('a', 'b'), gclient_eval._gclient_eval('("a", "b")'))

  def test_list(self):
    self.assertEqual(['a', 'b'], gclient_eval._gclient_eval('["a", "b"]'))

  def test_dict(self):
    self.assertEqual({'a': 'b'}, gclient_eval._gclient_eval('{"a": "b"}'))

  def test_name_safe(self):
    self.assertEqual(True, gclient_eval._gclient_eval('True'))

  def test_name_unsafe(self):
    with self.assertRaises(ValueError) as cm:
      gclient_eval._gclient_eval('UnsafeName')
    self.assertIn('invalid name \'UnsafeName\'', str(cm.exception))

  def test_invalid_call(self):
    with self.assertRaises(ValueError) as cm:
      gclient_eval._gclient_eval('Foo("bar")')
    self.assertIn('Var is the only allowed function', str(cm.exception))

  def test_call(self):
    self.assertEqual('{bar}', gclient_eval._gclient_eval('Var("bar")'))

  def test_expands_vars(self):
    self.assertEqual(
        'foo',
        gclient_eval._gclient_eval('Var("bar")', {'bar': 'foo'}, True))

  def test_expands_vars_with_braces(self):
    self.assertEqual(
        'foo',
        gclient_eval._gclient_eval('"{bar}"', {'bar': 'foo'}, True))

  def test_invalid_var(self):
    with self.assertRaises(ValueError) as cm:
      gclient_eval._gclient_eval('"{bar}"', {}, True)
    self.assertIn('bar was used as a variable, but was not declared',
                  str(cm.exception))

  def test_plus(self):
    self.assertEqual('foo', gclient_eval._gclient_eval('"f" + "o" + "o"'))

  def test_format(self):
    self.assertEqual('foo', gclient_eval._gclient_eval('"%s" % "foo"'))

  def test_not_expression(self):
    with self.assertRaises(SyntaxError) as cm:
      gclient_eval._gclient_eval('def foo():\n  pass')
    self.assertIn('invalid syntax', str(cm.exception))

  def test_not_whitelisted(self):
    with self.assertRaises(ValueError) as cm:
      gclient_eval._gclient_eval('[x for x in [1, 2, 3]]')
    self.assertIn(
        'unexpected AST node: <_ast.ListComp object', str(cm.exception))

  def test_dict_ordered(self):
    for test_case in itertools.permutations(range(4)):
      input_data = ['{'] + ['"%s": "%s",' % (n, n) for n in test_case] + ['}']
      expected = [(str(n), str(n)) for n in test_case]
      result = gclient_eval._gclient_eval(''.join(input_data))
      self.assertEqual(expected, result.items())


class ExecTest(unittest.TestCase):
  def test_multiple_assignment(self):
    with self.assertRaises(ValueError) as cm:
      gclient_eval.Exec('a, b, c = "a", "b", "c"')
    self.assertIn(
        'invalid assignment: target should be a name', str(cm.exception))

  def test_override(self):
    with self.assertRaises(ValueError) as cm:
      gclient_eval.Exec('a = "a"\na = "x"')
    self.assertIn(
        'invalid assignment: overrides var \'a\'', str(cm.exception))

  def test_schema_wrong_type(self):
    with self.assertRaises(schema.SchemaError):
      gclient_eval.Exec('include_rules = {}')

  def test_recursedeps_list(self):
    local_scope = gclient_eval.Exec(
        'recursedeps = [["src/third_party/angle", "DEPS.chromium"]]')
    self.assertEqual(
        {'recursedeps': [['src/third_party/angle', 'DEPS.chromium']]},
        local_scope)

  def test_var(self):
    local_scope = gclient_eval.Exec('\n'.join([
        'vars = {',
        '  "foo": "bar",',
        '}',
        'deps = {',
        '  "a_dep": "a" + Var("foo") + "b",',
        '}',
    ]))
    self.assertEqual({
        'vars': collections.OrderedDict([('foo', 'bar')]),
        'deps': collections.OrderedDict([('a_dep', 'abarb')]),
    }, local_scope)

  def test_braces_var(self):
    local_scope = gclient_eval.Exec('\n'.join([
        'vars = {',
        '  "foo": "bar",',
        '}',
        'deps = {',
        '  "a_dep": "a{foo}b",',
        '}',
    ]))
    self.assertEqual({
        'vars': collections.OrderedDict([('foo', 'bar')]),
        'deps': collections.OrderedDict([('a_dep', 'abarb')]),
    }, local_scope)

  def test_var_unexpanded(self):
    local_scope = gclient_eval.Exec('\n'.join([
        'vars = {',
        '  "foo": "bar",',
        '}',
        'deps = {',
        '  "a_dep": "a" + Var("foo") + "b",',
        '}',
    ]), False)
    self.assertEqual({
        'vars': collections.OrderedDict([('foo', 'bar')]),
        'deps': collections.OrderedDict([('a_dep', 'a{foo}b')]),
    }, local_scope)

  def test_empty_deps(self):
    local_scope = gclient_eval.Exec('deps = {}')
    self.assertEqual({'deps': {}}, local_scope)

  def test_overrides_vars(self):
    local_scope = gclient_eval.Exec('\n'.join([
        'vars = {',
        '  "foo": "bar",',
        '}',
        'deps = {',
        '  "a_dep": "a{foo}b",',
        '}',
    ]), True, vars_override={'foo': 'baz'})
    self.assertEqual({
        'vars': collections.OrderedDict([('foo', 'bar')]),
        'deps': collections.OrderedDict([('a_dep', 'abazb')]),
    }, local_scope)

  def test_doesnt_override_undeclared_vars(self):
    with self.assertRaises(ValueError) as cm:
      gclient_eval.Exec('\n'.join([
          'vars = {',
          '  "foo": "bar",',
          '}',
          'deps = {',
          '  "a_dep": "a{baz}b",',
          '}',
      ]), True, vars_override={'baz': 'lalala'})
    self.assertIn('baz was used as a variable, but was not declared',
                  str(cm.exception))


class UpdateConditionTest(unittest.TestCase):
  def test_both_present(self):
    info = {'condition': 'foo'}
    gclient_eval.UpdateCondition(info, 'and', 'bar')
    self.assertEqual(info, {'condition': '(foo) and (bar)'})

    info = {'condition': 'foo'}
    gclient_eval.UpdateCondition(info, 'or', 'bar')
    self.assertEqual(info, {'condition': '(foo) or (bar)'})

  def test_one_present_and(self):
    # If one of info's condition or new_condition is present, and |op| == 'and'
    # then the the result must be the present condition.
    info = {'condition': 'foo'}
    gclient_eval.UpdateCondition(info, 'and', None)
    self.assertEqual(info, {'condition': 'foo'})

    info = {}
    gclient_eval.UpdateCondition(info, 'and', 'bar')
    self.assertEqual(info, {'condition': 'bar'})

  def test_both_absent_and(self):
    # Nothing happens
    info = {}
    gclient_eval.UpdateCondition(info, 'and', None)
    self.assertEqual(info, {})

  def test_or(self):
    # If one of info's condition and new_condition is not present, then there
    # shouldn't be a condition. An absent value is treated as implicitly True.
    info = {'condition': 'foo'}
    gclient_eval.UpdateCondition(info, 'or', None)
    self.assertEqual(info, {})

    info = {}
    gclient_eval.UpdateCondition(info, 'or', 'bar')
    self.assertEqual(info, {})

    info = {}
    gclient_eval.UpdateCondition(info, 'or', None)
    self.assertEqual(info, {})


class EvaluateConditionTest(unittest.TestCase):
  def test_true(self):
    self.assertTrue(gclient_eval.EvaluateCondition('True', {}))

  def test_variable(self):
    self.assertFalse(gclient_eval.EvaluateCondition('foo', {'foo': 'False'}))

  def test_variable_cyclic_reference(self):
    with self.assertRaises(ValueError) as cm:
      self.assertTrue(gclient_eval.EvaluateCondition('bar', {'bar': 'bar'}))
    self.assertIn(
        'invalid cyclic reference to \'bar\' (inside \'bar\')',
        str(cm.exception))

  def test_operators(self):
    self.assertFalse(gclient_eval.EvaluateCondition(
        'a and not (b or c)', {'a': 'True', 'b': 'False', 'c': 'True'}))

  def test_expansion(self):
    self.assertTrue(gclient_eval.EvaluateCondition(
        'a or b', {'a': 'b and c', 'b': 'not c', 'c': 'False'}))

  def test_string_equality(self):
    self.assertTrue(gclient_eval.EvaluateCondition(
        'foo == "baz"', {'foo': '"baz"'}))
    self.assertFalse(gclient_eval.EvaluateCondition(
        'foo == "bar"', {'foo': '"baz"'}))

  def test_string_inequality(self):
    self.assertTrue(gclient_eval.EvaluateCondition(
        'foo != "bar"', {'foo': '"baz"'}))
    self.assertFalse(gclient_eval.EvaluateCondition(
        'foo != "baz"', {'foo': '"baz"'}))

  def test_string_bool(self):
    self.assertFalse(gclient_eval.EvaluateCondition(
        'false_str_var and true_var',
        {'false_str_var': 'False', 'true_var': True}))

  def test_string_bool_typo(self):
    with self.assertRaises(ValueError) as cm:
      gclient_eval.EvaluateCondition(
          'false_var_str and true_var',
          {'false_str_var': 'False', 'true_var': True})
    self.assertIn(
        'invalid "and" operand \'false_var_str\' '
            '(inside \'false_var_str and true_var\')',
        str(cm.exception))


class VarTest(unittest.TestCase):
  def assert_adds_var(self, before, after):
    local_scope = gclient_eval.Exec('\n'.join(before))
    gclient_eval.AddVar(local_scope, 'baz', 'lemur')
    results = gclient_eval.RenderDEPSFile(local_scope)
    self.assertEqual(results, '\n'.join(after))

  def test_adds_var(self):
    before = [
        'vars = {',
        '  "foo": "bar",',
        '}',
    ]
    after = [
        'vars = {',
        '  "baz": "lemur",',
        '  "foo": "bar",',
        '}',
    ]
    self.assert_adds_var(before, after)

  def test_adds_var_twice(self):
    local_scope = gclient_eval.Exec('\n'.join([
        'vars = {',
        '  "foo": "bar",',
        '}',
    ]))

    gclient_eval.AddVar(local_scope, 'baz', 'lemur')
    gclient_eval.AddVar(local_scope, 'v8_revision', 'deadbeef')
    result = gclient_eval.RenderDEPSFile(local_scope)

    self.assertEqual(result, '\n'.join([
        'vars = {',
        '  "v8_revision": "deadbeef",',
        '  "baz": "lemur",',
        '  "foo": "bar",',
        '}',
    ]))

  def test_gets_and_sets_var(self):
    local_scope = gclient_eval.Exec('\n'.join([
        'vars = {',
        '  "foo": "bar",',
        '}',
    ]))

    result = gclient_eval.GetVar(local_scope, 'foo')
    self.assertEqual(result, "bar")

    gclient_eval.SetVar(local_scope, 'foo', 'baz')
    result = gclient_eval.RenderDEPSFile(local_scope)

    self.assertEqual(result, '\n'.join([
        'vars = {',
        '  "foo": "baz",',
        '}',
    ]))

  def test_add_preserves_formatting(self):
    before = [
        '# Copyright stuff',
        '# some initial comments',
        '',
        'vars = { ',
        '  # Some comments.',
        '  "foo": "bar",',
        '',
        '  # More comments.',
        '  # Even more comments.',
        '  "v8_revision":   ',
        '       "deadbeef",',
        ' # Someone formatted this wrong',
        '}',
    ]
    after = [
        '# Copyright stuff',
        '# some initial comments',
        '',
        'vars = { ',
        '  "baz": "lemur",',
        '  # Some comments.',
        '  "foo": "bar",',
        '',
        '  # More comments.',
        '  # Even more comments.',
        '  "v8_revision":   ',
        '       "deadbeef",',
        ' # Someone formatted this wrong',
        '}',
    ]
    self.assert_adds_var(before, after)

  def test_set_preserves_formatting(self):
    local_scope = gclient_eval.Exec('\n'.join([
        'vars = {',
        '   # Comment with trailing space ',
        ' "foo": \'bar\',',
        '}',
    ]))

    gclient_eval.SetVar(local_scope, 'foo', 'baz')
    result = gclient_eval.RenderDEPSFile(local_scope)

    self.assertEqual(result, '\n'.join([
        'vars = {',
        '   # Comment with trailing space ',
        ' "foo": \'baz\',',
        '}',
    ]))


class CipdTest(unittest.TestCase):
  def test_gets_and_sets_cipd(self):
    local_scope = gclient_eval.Exec('\n'.join([
        'deps = {',
        '    "src/cipd/package": {',
        '        "packages": [',
        '            {',
        '                "package": "some/cipd/package",',
        '                "version": "version:1234",',
        '            },',
        '            {',
        '                "package": "another/cipd/package",',
        '                "version": "version:5678",',
        '            },',
        '        ],',
        '        "condition": "checkout_android",',
        '        "dep_type": "cipd",',
        '    },',
        '}',
    ]))

    result = gclient_eval.GetCIPD(
        local_scope, 'src/cipd/package', 'another/cipd/package')
    self.assertEqual(result, '5678')

    gclient_eval.SetCIPD(
        local_scope, 'src/cipd/package', 'another/cipd/package', '6.789')
    result = gclient_eval.RenderDEPSFile(local_scope)

    self.assertEqual(result, '\n'.join([
        'deps = {',
        '    "src/cipd/package": {',
        '        "packages": [',
        '            {',
        '                "package": "some/cipd/package",',
        '                "version": "version:1234",',
        '            },',
        '            {',
        '                "package": "another/cipd/package",',
        '                "version": "version:6.789",',
        '            },',
        '        ],',
        '        "condition": "checkout_android",',
        '        "dep_type": "cipd",',
        '    },',
        '}',
    ]))


class RevisionTest(unittest.TestCase):
  def assert_gets_and_sets_revision(self, before, after, rev_before='deadbeef'):
    local_scope = gclient_eval.Exec('\n'.join(before))

    result = gclient_eval.GetRevision(local_scope, 'src/dep')
    self.assertEqual(result, rev_before)

    gclient_eval.SetRevision(local_scope, 'src/dep', 'deadfeed')
    self.assertEqual('\n'.join(after), gclient_eval.RenderDEPSFile(local_scope))

  def test_revision(self):
    before = [
        'deps = {',
        '  "src/dep": "https://example.com/dep.git@deadbeef",',
        '}',
    ]
    after = [
        'deps = {',
        '  "src/dep": "https://example.com/dep.git@deadfeed",',
        '}',
    ]
    self.assert_gets_and_sets_revision(before, after)

  def test_revision_new_line(self):
    before = [
        'deps = {',
        '  "src/dep": "https://example.com/dep.git@"',
        '             + "deadbeef",',
        '}',
    ]
    after = [
        'deps = {',
        '  "src/dep": "https://example.com/dep.git@"',
        '             + "deadfeed",',
        '}',
    ]
    self.assert_gets_and_sets_revision(before, after)

  def test_revision_inside_dict(self):
    before = [
        'deps = {',
        '  "src/dep": {',
        '    "url": "https://example.com/dep.git@deadbeef",',
        '    "condition": "some_condition",',
        '  },',
        '}',
    ]
    after = [
        'deps = {',
        '  "src/dep": {',
        '    "url": "https://example.com/dep.git@deadfeed",',
        '    "condition": "some_condition",',
        '  },',
        '}',
    ]
    self.assert_gets_and_sets_revision(before, after)

  def test_follows_var_braces(self):
    before = [
        'vars = {',
        '  "dep_revision": "deadbeef",',
        '}',
        'deps = {',
        '  "src/dep": "https://example.com/dep.git@{dep_revision}",',
        '}',
    ]
    after = [
        'vars = {',
        '  "dep_revision": "deadfeed",',
        '}',
        'deps = {',
        '  "src/dep": "https://example.com/dep.git@{dep_revision}",',
        '}',
    ]
    self.assert_gets_and_sets_revision(before, after)

  def test_follows_var_braces_newline(self):
    before = [
        'vars = {',
        '  "dep_revision": "deadbeef",',
        '}',
        'deps = {',
        '  "src/dep": "https://example.com/dep.git"',
        '             + "@{dep_revision}",',
        '}',
    ]
    after = [
        'vars = {',
        '  "dep_revision": "deadfeed",',
        '}',
        'deps = {',
        '  "src/dep": "https://example.com/dep.git"',
        '             + "@{dep_revision}",',
        '}',
    ]
    self.assert_gets_and_sets_revision(before, after)

  def test_follows_var_function(self):
    before = [
        'vars = {',
        '  "dep_revision": "deadbeef",',
        '}',
        'deps = {',
        '  "src/dep": "https://example.com/dep.git@" + Var("dep_revision"),',
        '}',
    ]
    after = [
        'vars = {',
        '  "dep_revision": "deadfeed",',
        '}',
        'deps = {',
        '  "src/dep": "https://example.com/dep.git@" + Var("dep_revision"),',
        '}',
    ]
    self.assert_gets_and_sets_revision(before, after)

  def test_pins_revision(self):
    before = [
        'deps = {',
        '  "src/dep": "https://example.com/dep.git",',
        '}',
    ]
    after = [
        'deps = {',
        '  "src/dep": "https://example.com/dep.git@deadfeed",',
        '}',
    ]
    self.assert_gets_and_sets_revision(before, after, rev_before=None)


  def test_preserves_formatting(self):
    before = [
        'vars = {',
        ' # Some coment on deadbeef ',
        '  "dep_revision": "deadbeef",',
        '}',
        'deps = {',
        '  "src/dep": {',
        '    "url": "https://example.com/dep.git@" + Var("dep_revision"),',
        '',
        '    "condition": "some_condition",',
        ' },',
        '}',
    ]
    after = [
        'vars = {',
        ' # Some coment on deadbeef ',
        '  "dep_revision": "deadfeed",',
        '}',
        'deps = {',
        '  "src/dep": {',
        '    "url": "https://example.com/dep.git@" + Var("dep_revision"),',
        '',
        '    "condition": "some_condition",',
        ' },',
        '}',
    ]
    self.assert_gets_and_sets_revision(before, after)


class ParseTest(unittest.TestCase):
  def callParse(self, expand_vars=True, validate_syntax=True,
                vars_override=None):
    return gclient_eval.Parse('\n'.join([
        'vars = {',
        '  "foo": "bar",',
        '}',
        'deps = {',
        '  "a_dep": "a{foo}b",',
        '}',
    ]), expand_vars, validate_syntax, '<unknown>', vars_override)

  def test_expands_vars(self):
    for validate_syntax in True, False:
      local_scope = self.callParse(validate_syntax=validate_syntax)
      self.assertEqual({
          'vars': {'foo': 'bar'},
          'deps': {'a_dep': {'url': 'abarb',
                             'dep_type': 'git'}},
      }, local_scope)

  def test_no_expands_vars(self):
    for validate_syntax in True, False:
      local_scope = self.callParse(False,
                                   validate_syntax=validate_syntax)
      self.assertEqual({
          'vars': {'foo': 'bar'},
          'deps': {'a_dep': {'url': 'a{foo}b',
                             'dep_type': 'git'}},
      }, local_scope)

  def test_overrides_vars(self):
    for validate_syntax in True, False:
      local_scope = self.callParse(validate_syntax=validate_syntax,
                                   vars_override={'foo': 'baz'})
      self.assertEqual({
          'vars': {'foo': 'bar'},
          'deps': {'a_dep': {'url': 'abazb',
                             'dep_type': 'git'}},
      }, local_scope)

  def test_no_extra_vars(self):
    deps_file = '\n'.join([
        'vars = {',
        '  "foo": "bar",',
        '}',
        'deps = {',
        '  "a_dep": "a{baz}b",',
        '}',
    ])

    with self.assertRaises(ValueError) as cm:
      gclient_eval.Parse(
          deps_file, True, True,
          '<unknown>', {'baz': 'lalala'})
    self.assertIn('baz was used as a variable, but was not declared',
                  str(cm.exception))

    with self.assertRaises(KeyError) as cm:
      gclient_eval.Parse(
          deps_file, True, False,
          '<unknown>', {'baz': 'lalala'})
    self.assertIn('baz', str(cm.exception))

  def test_standardizes_deps_string_dep(self):
    for validate_syntax in True, False:
      local_scope = gclient_eval.Parse('\n'.join([
        'deps = {',
        '  "a_dep": "a_url@a_rev",',
        '}',
      ]), False, validate_syntax, '<unknown>')
      self.assertEqual({
          'deps': {'a_dep': {'url': 'a_url@a_rev',
                             'dep_type': 'git'}},
      }, local_scope)

  def test_standardizes_deps_dict_dep(self):
    for validate_syntax in True, False:
      local_scope = gclient_eval.Parse('\n'.join([
        'deps = {',
        '  "a_dep": {',
        '     "url": "a_url@a_rev",',
        '     "condition": "checkout_android",',
        '  },',
        '}',
      ]), False, validate_syntax, '<unknown>')
      self.assertEqual({
          'deps': {'a_dep': {'url': 'a_url@a_rev',
                             'dep_type': 'git',
                             'condition': 'checkout_android'}},
      }, local_scope)

  def test_ignores_none_in_deps_os(self):
    for validate_syntax in True, False:
      local_scope = gclient_eval.Parse('\n'.join([
        'deps = {',
        '  "a_dep": "a_url@a_rev",',
        '}',
        'deps_os = {',
        '  "mac": {',
        '     "a_dep": None,',
        '  },',
        '}',
      ]), False, validate_syntax, '<unknown>')
      self.assertEqual({
          'deps': {'a_dep': {'url': 'a_url@a_rev',
                             'dep_type': 'git'}},
      }, local_scope)

  def test_merges_deps_os_extra_dep(self):
    for validate_syntax in True, False:
      local_scope = gclient_eval.Parse('\n'.join([
        'deps = {',
        '  "a_dep": "a_url@a_rev",',
        '}',
        'deps_os = {',
        '  "mac": {',
        '     "b_dep": "b_url@b_rev"',
        '  },',
        '}',
      ]), False, validate_syntax, '<unknown>')
      self.assertEqual({
          'deps': {'a_dep': {'url': 'a_url@a_rev',
                             'dep_type': 'git'},
                   'b_dep': {'url': 'b_url@b_rev',
                             'dep_type': 'git',
                             'condition': 'checkout_mac'}},
      }, local_scope)

  def test_merges_deps_os_existing_dep_with_no_condition(self):
    for validate_syntax in True, False:
      local_scope = gclient_eval.Parse('\n'.join([
        'deps = {',
        '  "a_dep": "a_url@a_rev",',
        '}',
        'deps_os = {',
        '  "mac": {',
        '     "a_dep": "a_url@a_rev"',
        '  },',
        '}',
      ]), False, validate_syntax, '<unknown>')
      self.assertEqual({
          'deps': {'a_dep': {'url': 'a_url@a_rev',
                             'dep_type': 'git'}},
      }, local_scope)

  def test_merges_deps_os_existing_dep_with_condition(self):
    for validate_syntax in True, False:
      local_scope = gclient_eval.Parse('\n'.join([
        'deps = {',
        '  "a_dep": {',
        '    "url": "a_url@a_rev",',
        '    "condition": "some_condition",',
        '  },',
        '}',
        'deps_os = {',
        '  "mac": {',
        '     "a_dep": "a_url@a_rev"',
        '  },',
        '}',
      ]), False, validate_syntax, '<unknown>')
      self.assertEqual({
          'deps': {
              'a_dep': {'url': 'a_url@a_rev',
                        'dep_type': 'git',
                        'condition': '(checkout_mac) or (some_condition)'},
          },
      }, local_scope)

  def test_merges_deps_os_multiple_os(self):
    for validate_syntax in True, False:
      local_scope = gclient_eval.Parse('\n'.join([
        'deps_os = {',
        '  "win": {'
        '     "a_dep": "a_url@a_rev"',
        '  },',
        '  "mac": {',
        '     "a_dep": "a_url@a_rev"',
        '  },',
        '}',
      ]), False, validate_syntax, '<unknown>')
      self.assertEqual({
          'deps': {
              'a_dep': {'url': 'a_url@a_rev',
                        'dep_type': 'git',
                        'condition': '(checkout_mac) or (checkout_win)'},
          },
      }, local_scope)

  def test_fails_to_merge_same_dep_with_different_revisions(self):
    for validate_syntax in True, False:
      with self.assertRaises(gclient_eval.gclient_utils.Error) as cm:
        gclient_eval.Parse('\n'.join([
          'deps = {',
          '  "a_dep": {',
          '    "url": "a_url@a_rev",',
          '    "condition": "some_condition",',
          '  },',
          '}',
          'deps_os = {',
          '  "mac": {',
          '     "a_dep": "a_url@b_rev"',
          '  },',
          '}',
        ]), False, validate_syntax, '<unknown>')
      self.assertIn('conflicts with existing deps', str(cm.exception))

  def test_merges_hooks_os(self):
    for validate_syntax in True, False:
      local_scope = gclient_eval.Parse('\n'.join([
        'hooks = [',
        '  {',
        '    "action": ["a", "action"],',
        '  },',
        ']',
        'hooks_os = {',
        '  "mac": [',
        '    {',
        '       "action": ["b", "action"]',
        '    },',
        '  ]',
        '}',
      ]), False, validate_syntax, '<unknown>')
      self.assertEqual({
          "hooks": [{"action": ["a", "action"]},
                    {"action": ["b", "action"], "condition": "checkout_mac"}],
      }, local_scope)



if __name__ == '__main__':
  level = logging.DEBUG if '-v' in sys.argv else logging.FATAL
  logging.basicConfig(
      level=level,
      format='%(asctime).19s %(levelname)s %(filename)s:'
             '%(lineno)s %(message)s')
  unittest.main()
