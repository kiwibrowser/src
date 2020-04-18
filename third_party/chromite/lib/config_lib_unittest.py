# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for config."""

from __future__ import print_function

import copy
import cPickle
import json

from chromite.lib.const import waterfall
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_test_lib

# pylint: disable=protected-access


def MockBuildConfig():
  """Create a BuildConfig object for convenient testing pleasure."""
  site_config = MockSiteConfig()
  return site_config['amd64-generic-paladin']


def MockSiteConfig():
  """Create a SiteConfig object for convenient testing pleasure.

  Shared amoung a number of unittest files, so be careful if changing it.
  """
  result = config_lib.SiteConfig()

  # Add a single, simple build config.
  result.Add(
      'amd64-generic-paladin',
      active_waterfall='chromiumos',
      boards=['amd64-generic'],
      display_label='MockLabel',
      build_type='paladin',
      chrome_sdk=True,
      chrome_sdk_build_chrome=False,
      description='Commit Queue',
      doc='http://mock_url/',
      image_test=True,
      images=['base', 'test'],
      important=True,
      manifest_version=True,
      prebuilts='public',
      upload_standalone_images=False,
      vm_tests=[config_lib.VMTestConfig('smoke_suite')],
  )

  return result


def AssertSiteIndependentParameters(site_config):
  """Helper function to test that SiteConfigs contain site-independent values.

  Args:
    site_config: A SiteConfig object.

  Returns:
    A boolean. True if the config contained all site-independent values.
    False otherwise.
  """
  # Enumerate the necessary site independent parameter keys.
  # All keys must be documented.
  # TODO (msartori): Fill in this list.
  site_independent_params = [
  ]

  site_params = site_config.params
  return all([x in site_params for x in site_independent_params])


class _CustomObject(object):
  """Simple object. For testing deepcopy."""

  def __init__(self, x):
    self.x = x

  def __eq__(self, other):
    return self.x == other.x


class _CustomObjectWithSlots(object):
  """Simple object with slots. For testing deepcopy."""

  __slots__ = ['x']

  def __init__(self, x):
    self.x = x

  def __eq__(self, other):
    return self.x == other.x


class BuildConfigClassTest(cros_test_lib.TestCase):
  """BuildConfig tests."""

  def setUp(self):
    self.fooConfig = config_lib.BuildConfig(name='foo', foo=1)
    self.barConfig = config_lib.BuildConfig(name='bar', bar=2)
    self.deepConfig = config_lib.BuildConfig(
        name='deep', nested=[1, 2, 3], deep=3,
        child_configs=[self.fooConfig, self.barConfig])

  def testMockSiteConfig(self):
    """Make sure Mock generator fucntion doesn't crash."""
    site_config = MockSiteConfig()
    self.assertIsNotNone(site_config)

    build_config = MockBuildConfig()
    self.assertIsNotNone(build_config)

  def testValueAccess(self):
    self.assertEqual(self.fooConfig.name, 'foo')
    self.assertEqual(self.fooConfig.name, self.fooConfig['name'])

    self.assertRaises(AttributeError, getattr, self.fooConfig, 'foobar')

  def testApplyEmpty(self):
    orig = self.fooConfig.deepcopy()

    # Do nothing.
    self.fooConfig.apply()
    self.assertEqual(self.fooConfig, orig)

  def testApplyValues(self):
    # Apply simple values..
    self.fooConfig.apply(a=1, b=2)
    self.assertEqual(self.fooConfig, dict(name='foo', foo=1, a=1, b=2))

  def testApplyBuildConfig(self):
    # Apply a BuildConfig.
    self.fooConfig.apply(self.barConfig)
    self.assertEqual(self.fooConfig, dict(name='bar', foo=1, bar=2))

  def testApplyMixed(self):
    # Apply simple values..
    config = config_lib.BuildConfig()
    config.apply(self.fooConfig, self.barConfig, a=1, b=2, bar=3)
    self.assertEqual(config, dict(name='bar', foo=1, bar=3, a=1, b=2))

  def testDeriveMixed(self):
    config = config_lib.BuildConfig()
    result = config.derive(self.fooConfig, self.barConfig, a=1, b=2, bar=3)

    self.assertIsNot(config, result)
    self.assertEqual(config, dict())
    self.assertEqual(result, dict(name='bar', foo=1, bar=3, a=1, b=2))

  def testApplyCallable(self):
    # Callable that adds a configurable amount.
    def append(x):
      return lambda base: base + ' ' + x

    site_config = config_lib.SiteConfig()

    site_config.AddTemplate('add1', foo=append('one'))
    site_config.AddTemplate('add2', foo=append('two'))
    site_config.AddTemplate('fixed', foo='fixed')

    site_config.AddTemplate('derived',
                            site_config.templates.add1)

    site_config.AddTemplate('stacked',
                            site_config.templates.add1,
                            site_config.templates.add2)

    site_config.AddTemplate('stackedDeep',
                            site_config.templates.fixed,
                            site_config.templates.add1,
                            site_config.templates.add1,
                            site_config.templates.add1,
                            foo=append('deep'))

    site_config.AddTemplate('stackedDeeper',
                            site_config.templates.stacked,
                            site_config.templates.stackedDeep,
                            foo=append('deeper'))

    base = config_lib.BuildConfig(foo='base')

    # Basic apply.
    result = base.derive(site_config.templates.add1)
    self.assertEqual(result.foo, 'base one')

    # Callable template + local callable.
    result = base.derive(site_config.templates.add1, foo=append('local'))
    self.assertEqual(result.foo, 'base one local')

    # Callable template + local fixed.
    result = base.derive(site_config.templates.add1, foo='local')
    self.assertEqual(result.foo, 'local')

    # Derived template.
    result = base.derive(site_config.templates.derived)
    self.assertEqual(result.foo, 'base one')

    # Template with fixed override after stacking template (all callable magic
    # should disappear).
    result = base.derive(site_config.templates.fixed)
    self.assertEqual(result.foo, 'fixed')

    # Template with stacked.
    result = base.derive(site_config.templates.stacked)
    self.assertEqual(result.foo, 'base one two')

    # Callables on top of fixed from template.
    result = base.derive(site_config.templates.stackedDeep)
    self.assertEqual(result.foo, 'fixed one one one deep')

    # Just get crazy with it.
    result = base.derive(site_config.templates.stackedDeeper)
    self.assertEqual(result.foo, 'fixed one one one deep deeper')

    # Ensure objects derived from weren't modified.
    self.assertEqual(base.foo, 'base')
    self.assertEqual(site_config.templates.fixed.foo, 'fixed')

  def AssertDeepCopy(self, obj1, obj2, obj3):
    """Assert that |obj3| is a deep copy of |obj1|.

    Args:
      obj1: Object that was copied.
      obj2: A true deep copy of obj1 (produced using copy.deepcopy).
      obj3: The purported deep copy of obj1.
    """
    # Check whether the item was copied by deepcopy. If so, then it
    # must have been copied by our algorithm as well.
    if obj1 is not obj2:
      self.assertIsNot(obj1, obj3)

    # Assert the three items are all equal.
    self.assertEqual(obj1, obj2)
    self.assertEqual(obj1, obj3)

    if isinstance(obj1, (tuple, list)):
      # Copy tuples and lists item by item.
      for i in range(len(obj1)):
        self.AssertDeepCopy(obj1[i], obj2[i], obj3[i])
    elif isinstance(obj1, set):
      # Compare sorted versions of the set.
      self.AssertDeepCopy(list(sorted(obj1)), list(sorted(obj2)),
                          list(sorted(obj3)))
    elif isinstance(obj1, dict):
      # Copy dicts item by item.
      for k in obj1:
        self.AssertDeepCopy(obj1[k], obj2[k], obj3[k])
    elif hasattr(obj1, '__dict__'):
      # Make sure the dicts are copied.
      self.AssertDeepCopy(obj1.__dict__, obj2.__dict__, obj3.__dict__)
    elif hasattr(obj1, '__slots__'):
      # Make sure the slots are copied.
      for attr in obj1.__slots__:
        self.AssertDeepCopy(getattr(obj1, attr), getattr(obj2, attr),
                            getattr(obj3, attr))
    else:
      # This should be an object that copy.deepcopy didn't copy (probably an
      # immutable object.) If not, the test needs to be updated to handle this
      # kind of object.
      self.assertIs(obj1, obj2)

  def testDeepCopy(self):
    """Test that we deep copy correctly."""
    for cfg in [self.fooConfig, self.barConfig, self.deepConfig]:
      self.AssertDeepCopy(cfg, copy.deepcopy(cfg), cfg.deepcopy())

  def testAssertDeepCopy(self):
    """Test that we test deep copy correctly."""
    test1 = ['foo', 'bar', ['hey']]
    tests = [test1,
             set([tuple(x) for x in test1]),
             dict(zip([tuple(x) for x in test1], test1)),
             _CustomObject(test1),
             _CustomObjectWithSlots(test1)]

    for x in tests + [[tests]]:
      copy_x = copy.deepcopy(x)
      self.AssertDeepCopy(x, copy_x, copy.deepcopy(x))
      self.AssertDeepCopy(x, copy_x, cPickle.loads(cPickle.dumps(x, -1)))
      self.assertRaises(AssertionError, self.AssertDeepCopy, x,
                        copy_x, x)
      if not isinstance(x, set):
        self.assertRaises(AssertionError, self.AssertDeepCopy, x,
                          copy_x, copy.copy(x))


class SiteParametersClassTest(cros_test_lib.TestCase):
  """SiteParameters tests."""

  def testAttributeAccess(self):
    """Test that SiteParameters dot-accessor works correctly."""
    site_params = config_lib.SiteParameters()

    # Ensure our test key is not in site_params.
    self.assertNotIn('foo', site_params)

    # Test that we raise when accessing a non-existent value.
    # pylint: disable=pointless-statement
    with self.assertRaises(AttributeError):
      site_params.foo

    # Test the dot-accessor.
    site_params.update({'foo': 'bar'})
    self.assertEquals('bar', site_params.foo)


class SiteConfigTest(cros_test_lib.TestCase):
  """Config tests."""

  @staticmethod
  def _callable(x):
    return x + ' extended'

  def setUp(self):
    self.complex_defaults = {
        'value': 'default',
    }

    self.complex_site_params = {
        'site_foo': True,
        'site_bar': False,
        'nested': {'sub1': 1, 'sub2': 2},
    }

    # Construct our test config.
    site_config = config_lib.SiteConfig(
        defaults=self.complex_defaults, site_params=self.complex_site_params)

    site_config.AddTemplate('match', value='default')
    site_config.AddTemplate('template', value='template')
    site_config.AddTemplate('mixin', value='mixin')
    site_config.AddTemplate('unused', value='unused')
    site_config.AddTemplate('callable', value=self._callable)

    default = site_config.Add('default')

    default_with_override = site_config.Add(
        'default_with_override',
        value='override')

    site_config.AddWithoutTemplate(
        'default_with_mixin',
        site_config.templates.mixin)

    site_config.AddWithoutTemplate(
        'mixin_with_override',
        site_config.templates.mixin,
        value='override')

    site_config.Add(
        'default_with_template',
        site_config.templates.template)

    site_config.Add(
        'template_with_override',
        site_config.templates.template,
        value='override')

    site_config.Add(
        'template_with_mixin',
        site_config.templates.template,
        site_config.templates.mixin)

    site_config.Add(
        'template_with_mixin_override',
        site_config.templates.template,
        site_config.templates.mixin,
        value='override')

    site_config.Add(
        'match',
        value='default')

    site_config.Add(
        'template_back_to_default',
        site_config.templates.template,
        value='default')

    site_config.Add(
        'calling',
        site_config.templates.callable)

    site_config.Add(
        'vm_tests',
        vm_tests=[config_lib.VMTestConfig('vm_suite')],
        vm_tests_override=[config_lib.VMTestConfig('vm_override')])

    site_config.Add(
        'hw_tests',
        hw_tests=[config_lib.HWTestConfig('hw_suite')],
        hw_tests_override=[config_lib.HWTestConfig('hw_override')])

    site_config.Add(
        'tast_vm_tests',
        tast_vm_tests=[config_lib.TastVMTestConfig('tast_vm_suite',
                                                   ['(bvt)'])])

    site_config.AddGroup(
        'parent',
        default,
        default_with_override)

    self.site_config = site_config

  def testAddedContents(self):
    """Verify that our complex config looks like we expect, before saving."""
    expected = {
        'default': {
            '_template': None,
            'name': 'default',
            'value': 'default',
        },
        'default_with_override': {
            '_template': None,
            'name': 'default_with_override',
            'value': 'override',
        },
        'default_with_mixin': {
            '_template': None,
            'name': 'default_with_mixin',
            'value': 'mixin',
        },
        'mixin_with_override': {
            '_template': None,
            'name': 'mixin_with_override',
            'value': 'override',
        },
        'default_with_template': {
            '_template': 'template',
            'name': 'default_with_template',
            'value': 'template',
        },
        'template_with_override': {
            '_template': 'template',
            'name': 'template_with_override',
            'value': 'override'
        },
        'template_with_mixin': {
            '_template': 'template',
            'name': 'template_with_mixin',
            'value': 'mixin',
        },
        'template_with_mixin_override': {
            '_template': 'template',
            'name': 'template_with_mixin_override',
            'value': 'override'
        },
        'calling': {
            '_template': 'callable',
            'name': 'calling',
            'value': 'default extended',
        },
        'match': {
            '_template': None,
            'name': 'match',
            'value': 'default',
        },
        'template_back_to_default': {
            '_template': 'template',
            'name': 'template_back_to_default',
            'value': 'default',
        },
        'vm_tests': {
            '_template': None,
            'name': 'vm_tests',
            'vm_tests': [config_lib.VMTestConfig('vm_suite')],
            'vm_tests_override': [config_lib.VMTestConfig('vm_override')],
        },
        'hw_tests': {
            '_template': None,
            'name': 'hw_tests',
            'hw_tests': [config_lib.HWTestConfig('hw_suite')],
            'hw_tests_override': [config_lib.HWTestConfig('hw_override')],
        },
        'tast_vm_tests': {
            '_template': None,
            'name': 'tast_vm_tests',
            'tast_vm_tests': [
                config_lib.TastVMTestConfig('tast_vm_suite', ['(bvt)'])],
        },
        'parent': {
            '_template': None,
            'name': 'parent',
            'value': 'default',
        },
    }

    # Make sure our expected build configs exist.
    self.assertItemsEqual(self.site_config.keys(), expected.keys())

    # Make sure each one contains
    self.longMessage = True
    for name in expected.keys():
      self.assertDictContainsSubset(expected[name],
                                    self.site_config[name],
                                    name)

    # Special handling for child configs.

    children = self.site_config['parent'].child_configs
    self.assertEqual(len(children), 2)
    self.assertDictContainsSubset(
        {
            '_template': None,
            'name': 'default',
            'value': 'default',
            'grouped': True,
        },
        children[0])

    self.assertDictContainsSubset(
        {
            '_template': None,
            'name': 'default_with_override',
            'value': 'override',
            'grouped': True,
        },
        children[1])

  def testAddErrors(self):
    """Test the SiteConfig.Add behavior."""
    self.site_config.Add('foo')

    # Test we can't add the 'foo' config again.
    with self.assertRaises(AssertionError):
      self.site_config.Add('foo')

    # Create a template without using AddTemplate so the site config doesn't
    # know about it.
    fake_template = config_lib.BuildConfig(
        name='fake_template', _template='fake_template')

    with self.assertRaises(AssertionError):
      self.site_config.Add('bar', fake_template)

  def testTemplateAttr(self):
    """Test the SiteConfig.templates.name behavior."""
    template1 = self.site_config.AddTemplate('template1', value='template')
    template2 = self.site_config.AddTemplate('template2', value='template')

    self.assertIs(template1, self.site_config.templates.template1)
    self.assertIs(template2, self.site_config.templates.template2)

    # Try to fetch a non-existent template.
    with self.assertRaises(AttributeError):
      # pylint: disable=pointless-statement
      self.site_config.templates.no_such_template
      # pylint: enable=pointless-statement

  def testAddForBoards(self):
    per_board = {
        'foo': config_lib.BuildConfig(value='foo'),
        'bar': config_lib.BuildConfig(value='bar'),
        'multiboard': config_lib.BuildConfig(boards=['foo', 'bar']),
    }

    # Test the minimal invocation.
    self.site_config.AddForBoards(
        'minimal',
        ['foo', 'bar'],
    )

    self.assertIn('foo-minimal', self.site_config)
    self.assertEqual(self.site_config['foo-minimal'].boards, ['foo'])
    self.assertEqual(self.site_config['bar-minimal'].boards, ['bar'])

    # Test a partial set of per-board values specified.
    self.site_config.AddForBoards(
        'partial_per_board',
        ['foo', 'no_per'],
        per_board,
    )

    self.assertIn('foo-partial_per_board', self.site_config)
    self.assertIn('no_per-partial_per_board', self.site_config)
    self.assertEqual(self.site_config['foo-partial_per_board'].value, 'foo')

    # Test all boards with per_board values specified, and test we can
    # override board listing in per_board values.
    self.site_config.AddForBoards(
        'per_board',
        ['foo', 'bar', 'multiboard'],
        per_board,
    )

    self.assertEqual(self.site_config['foo-per_board'].value, 'foo')
    self.assertEqual(self.site_config['bar-per_board'].value, 'bar')
    self.assertEqual(self.site_config['multiboard-per_board'].boards,
                     ['foo', 'bar'])

    # Test using a template
    self.site_config.AddForBoards(
        'template',
        ['foo', 'bar'],
        None,
        self.site_config.templates.template,
    )

    self.assertEqual(self.site_config['foo-template'].value, 'template')

    # Test a template, and a mixin.
    self.site_config.AddForBoards(
        'mixin',
        ['foo', 'bar'],
        None,
        self.site_config.templates.template,
        self.site_config.templates.mixin,
    )

    self.assertEqual(self.site_config['foo-mixin'].value, 'mixin')

    # Test a template, and a mixin, and a per-board.
    self.site_config.AddForBoards(
        'mixin_per_board',
        ['foo', 'bar'],
        per_board,
        self.site_config.templates.template,
        self.site_config.templates.mixin,
    )

    self.assertEqual(self.site_config['foo-mixin_per_board'].value, 'foo')

    # Test a template, and a mixin, and a per-board, and an override.
    self.site_config.AddForBoards(
        'override',
        ['foo', 'bar'],
        per_board,
        self.site_config.templates.template,
        self.site_config.templates.mixin,
        value='override',
    )

    self.assertEqual(self.site_config['foo-override'].value, 'override')

  def _verifyLoadSave(self, site_config):
    """Make sure that we can save and re-load a site."""
    config_str = site_config.SaveConfigToString()
    loaded = config_lib.LoadConfigFromString(config_str)

    #
    # BUG ALERT ON TEST FAILURE
    #
    # assertDictEqual can correctly compare these structs for equivalence, but
    # has a bug when displaying differences on failure. The embedded
    # HWTestConfig values are correctly compared, but ALWAYS display as
    # different, if something else triggers a failure.
    #

    # This for loop is to make differences easier to find/read.
    self.longMessage = True
    for name in site_config.iterkeys():
      self.assertDictEqual(loaded[name], site_config[name], name)

    # This includes templates and the default build config.
    self.assertEqual(site_config, loaded)

    loaded_str = loaded.SaveConfigToString()

    self.assertEqual(config_str, loaded_str)

    # Cycle through save load again, just for completeness.
    loaded2 = config_lib.LoadConfigFromString(loaded_str)
    loaded2_str = loaded2.SaveConfigToString()
    self.assertEqual(loaded_str, loaded2_str)

    # Make sure we can dump long content without crashing.
    self.assertNotEqual(site_config.DumpExpandedConfigToString(), '')
    self.assertNotEqual(loaded.DumpExpandedConfigToString(), '')

    return loaded

  def testSaveLoadEmpty(self):
    """Create, save, and reload an empty config."""
    site_config = config_lib.SiteConfig()

    loaded = self._verifyLoadSave(site_config)

    self.assertEqual(loaded.keys(), [])
    self.assertEqual(loaded._templates.keys(), [])
    self.assertDictEqual(
        loaded.GetDefault(), config_lib.DefaultSettings())
    self.assertDictEqual(
        loaded.params, config_lib.DefaultSiteParameters())

  def testSaveLoadComplex(self):
    """Create, save, and reload an complex config."""
    # Verify it.
    loaded = self._verifyLoadSave(self.site_config)

    # Verify default build config
    expected_defaults = config_lib.DefaultSettings()
    expected_defaults.update(self.complex_defaults)
    self.assertDictEqual(loaded.GetDefault(), expected_defaults)

    # Verify Site Params.
    expected_site_params = config_lib.DefaultSiteParameters()
    expected_site_params.update(self.complex_site_params)
    self.assertDictEqual(loaded.params, expected_site_params)

    # Ensure that expected templates are present.
    self.assertItemsEqual(loaded.templates.keys(), ['template', 'callable'])

  def testTemplatesToSave(self):
    def _invert(x):
      return not x

    config = config_lib.SiteConfig()
    config.AddTemplate('base', foo=True, important=False)
    config.AddTemplate('callable', important=_invert)
    config.AddTemplate('unused', bar=True)
    config.Add('build1', config.templates.base, var=1)
    config.Add('build2', var=2)
    config.Add('build3', config.templates.base, var=3)
    config.Add('build4', config.templates.callable, var=4)

    self.assertItemsEqual(
        config.templates,
        {
            'base': {'_template': 'base', 'foo': True, 'important': False},
            'callable': {'_template': 'callable', 'important': _invert},
            'unused': {'_template': 'unused', 'bar': True},
        }
    )

    results = config._MarshalTemplates()

    self.assertItemsEqual(
        results,
        {
            'base': {'_template': 'base', 'foo': True},
            'callable': {'_template': 'callable', 'important': True},
        }
    )


class SiteConfigFindTests(cros_test_lib.TestCase):
  """Tests related to Find helpers on SiteConfig."""

  def testGetBoardsMockConfig(self):
    site_config = MockSiteConfig()
    self.assertEqual(
        site_config.GetBoards(),
        set(['amd64-generic']))

  def testGetBoardsComplexConfig(self):
    site_config = MockSiteConfig()
    site_config.Add('build_a', boards=['foo_board'])
    site_config.Add('build_b', boards=['bar_board'])
    site_config.Add('build_c', boards=['foo_board', 'car_board'])

    self.assertEqual(
        site_config.GetBoards(),
        set(['amd64-generic', 'foo_board', 'bar_board', 'car_board']))

  def testGetSlaveConfigMapForMasterAll(self):
    """Test GetSlaveConfigMapForMaster, GetSlavesForMaster all slaves."""

    site_config = MockSiteConfig()
    master = site_config.Add('master', master=True, manifest_version=True,
                             slave_configs=['slave_a', 'slave_b'])
    slave_a = site_config.Add('slave_a', important=True)
    slave_b = site_config.Add('slave_b', important=False)
    site_config.Add('other')

    results_map = site_config.GetSlaveConfigMapForMaster(master,
                                                         important_only=False)
    results_slaves = site_config.GetSlavesForMaster(master,
                                                    important_only=False)

    self.assertEqual(results_map, {'slave_a': slave_a, 'slave_b': slave_b})
    self.assertItemsEqual(results_slaves, [slave_a, slave_b])

  def testGetSlaveConfigMapForMasterImportant(self):
    """Test GetSlaveConfigMapForMaster, GetSlavesForMaster important only."""

    site_config = MockSiteConfig()
    master = site_config.Add('master', master=True, manifest_version=True,
                             slave_configs=['slave_a', 'slave_b'])
    slave_a = site_config.Add('slave_a', important=True)
    site_config.Add('slave_b', important=False)
    site_config.Add('other')

    results_map = site_config.GetSlaveConfigMapForMaster(master)
    results_slaves = site_config.GetSlavesForMaster(master)

    self.assertEqual(results_map, {'slave_a': slave_a})
    self.assertItemsEqual(results_slaves, [slave_a])


class GetConfigTests(cros_test_lib.TestCase):
  """Tests related to SiteConfig.GetConfig()."""

  def testGetConfigCaching(self):
    """Test that config_lib.GetConfig() caches it's results correctly."""
    config_a = config_lib.GetConfig()
    config_b = config_lib.GetConfig()

    # Ensure that we get a SiteConfig, and that the result is cached.
    self.assertIsInstance(config_a, config_lib.SiteConfig)
    self.assertIs(config_a, config_b)

    # Clear our cache.
    config_lib.ClearConfigCache()
    config_c = config_lib.GetConfig()
    config_d = config_lib.GetConfig()

    # Ensure that this gives us a new instance of the SiteConfig.
    self.assertIsNot(config_a, config_c)

    # But also that it's cached going forward.
    self.assertIsInstance(config_c, config_lib.SiteConfig)
    self.assertIs(config_c, config_d)


class ConfigLibHelperTests(cros_test_lib.TestCase):
  """Tests related to helper methods in config_lib."""

  def testUseBuildbucketScheduler(self):
    """Test UseBuildbucketScheduler."""
    cq_master_config = config_lib.BuildConfig(
        name=constants.CQ_MASTER,
        active_waterfall=waterfall.WATERFALL_INTERNAL)
    self.assertTrue(config_lib.UseBuildbucketScheduler(
        cq_master_config))

    pre_cq_config = config_lib.BuildConfig(
        name=constants.PRE_CQ_LAUNCHER_NAME,
        active_waterfall=waterfall.WATERFALL_INTERNAL)
    self.assertTrue(config_lib.UseBuildbucketScheduler(
        pre_cq_config))

    pfq_master_config = config_lib.BuildConfig(
        name=constants.PFQ_MASTER,
        active_waterfall=waterfall.WATERFALL_INTERNAL)
    self.assertTrue(config_lib.UseBuildbucketScheduler(
        pfq_master_config))

    toolchain_master = config_lib.BuildConfig(
        name=constants.TOOLCHAIN_MASTTER,
        active_waterfall=waterfall.WATERFALL_INTERNAL)
    self.assertTrue(config_lib.UseBuildbucketScheduler(
        toolchain_master))

    pre_cq_config = config_lib.BuildConfig(
        name=constants.BINHOST_PRE_CQ,
        active_waterfall=waterfall.WATERFALL_SWARMING)
    self.assertFalse(config_lib.UseBuildbucketScheduler(
        pre_cq_config))

    release_branch_config = config_lib.BuildConfig(
        name=constants.CANARY_MASTER,
        active_waterfall=waterfall.WATERFALL_RELEASE)
    self.assertTrue(config_lib.UseBuildbucketScheduler(
        release_branch_config))

  def testScheduledByBuildbucket(self):
    """Test ScheduledByBuildbucket."""
    cq_master_config = config_lib.BuildConfig(
        name=constants.CQ_MASTER,
        build_type=constants.PALADIN_TYPE)
    self.assertFalse(config_lib.ScheduledByBuildbucket(
        cq_master_config))

    cq_slave_config = config_lib.BuildConfig(
        name='slave-paladin',
        build_type=constants.PALADIN_TYPE)
    self.assertTrue(config_lib.ScheduledByBuildbucket(
        cq_slave_config))

    pfq_master_config = config_lib.BuildConfig(
        name=constants.PFQ_MASTER,
        build_type=constants.PFQ_TYPE)

    self.assertFalse(config_lib.ScheduledByBuildbucket(
        pfq_master_config))


class GEBuildConfigTests(cros_test_lib.TestCase):
  """Test GE build config related methods."""

  def setUp(self):
    self._fake_ge_build_config_json = '''
{
  "metadata_version": "1.0",
  "reference_board_unified_builds": [
    {
      "name": "reef",
      "reference_board_name": "reef",
      "builder": "RELEASE",
      "experimental": true,
      "arch": "X86_INTERNAL",
      "models" : [
        {
          "board_name": "reef"
        },
        {
          "board_name": "pyro"
        }
      ]
    }
  ],
  "boards": [
    {
      "name": "reef",
      "configs": [
        {
          "builder": "RELEASE",
          "experimental": false,
          "leader_board": true,
          "board_group": "reef",
          "arch": "X86_INTERNAL"
        }
      ]
    }
  ]
}
    '''
    self._fake_ge_build_config = json.loads(self._fake_ge_build_config_json)

  def testGetArchBoardDict(self):
    """Test GetArchBoardDict."""
    ge_build_config = config_lib.LoadGEBuildConfigFromFile()
    arch_board_dict = config_lib.GetArchBoardDict(ge_build_config)
    self.assertIsNotNone(arch_board_dict)

  def testGetArchBoardDictUnifiedBuilds(self):
    """Test GetArchBoardDict."""
    arch_board_dict = config_lib.GetArchBoardDict(self._fake_ge_build_config)
    self.assertIsNotNone(arch_board_dict)
    self.assertIs(1, len(arch_board_dict[config_lib.CONFIG_X86_INTERNAL]))

  def testGetUnifiedBuildConfigAllBuilds(self):
    uni_builds = config_lib.GetUnifiedBuildConfigAllBuilds(
        self._fake_ge_build_config)
    self.assertEquals(1, len(uni_builds))

  def testGetUnifiedBuildConfigAllBuildsWithNoBuilds(self):
    uni_builds = config_lib.GetUnifiedBuildConfigAllBuilds({})
    self.assertEquals(0, len(uni_builds))
