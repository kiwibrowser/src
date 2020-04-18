# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import types

from recipe_engine.config import config_item_context, ConfigGroup, BadConf
from recipe_engine.config import ConfigList, Dict, Single, Static, Set, List

from . import api as gclient_api


def BaseConfig(USE_MIRROR=True, CACHE_DIR=None,
               PATCH_PROJECT=None, BUILDSPEC_VERSION=None,
               **_kwargs):
  cache_dir = str(CACHE_DIR) if CACHE_DIR else None
  return ConfigGroup(
    solutions = ConfigList(
      lambda: ConfigGroup(
        name = Single(basestring),
        url = Single((basestring, type(None)), empty_val=''),
        deps_file = Single(basestring, empty_val='.DEPS.git', required=False,
                           hidden=False),
        managed = Single(bool, empty_val=True, required=False, hidden=False),
        custom_deps = Dict(value_type=(basestring, types.NoneType)),
        custom_vars = Dict(value_type=(basestring, types.BooleanType)),
        safesync_url = Single(basestring, required=False),

        revision = Single(
            (basestring, gclient_api.RevisionResolver),
            required=False, hidden=True),
      )
    ),
    deps_os = Dict(value_type=basestring),
    hooks = List(basestring),
    target_os = Set(basestring),
    target_os_only = Single(bool, empty_val=False, required=False),
    target_cpu = Set(basestring),
    target_cpu_only = Single(bool, empty_val=False, required=False),
    cache_dir = Static(cache_dir, hidden=False),

    # If supplied, use this as the source root (instead of the first solution's
    # checkout).
    src_root = Single(basestring, required=False, hidden=True),

    # Maps 'solution' -> build_property
    # TODO(machenbach): Deprecate this in favor of the one below.
    # http://crbug.com/713356
    got_revision_mapping = Dict(hidden=True),

    # Maps build_property -> 'solution'
    got_revision_reverse_mapping = Dict(hidden=True),

    # Addition revisions we want to pass in.  For now theres a duplication
    # of code here of setting custom vars AND passing in --revision. We hope
    # to remove custom vars later.
    revisions = Dict(
        value_type=(basestring, gclient_api.RevisionResolver),
        hidden=True),

    # TODO(iannucci): HACK! The use of None here to indicate that we apply this
    #   to the solution.revision field is really terrible. I mostly blame
    #   gclient.
    # Maps 'parent_build_property' -> 'custom_var_name'
    # Maps 'parent_build_property' -> None
    # If value is None, the property value will be applied to
    # solutions[0].revision. Otherwise, it will be applied to
    # solutions[0].custom_vars['custom_var_name']
    parent_got_revision_mapping = Dict(hidden=True),
    delete_unversioned_trees = Single(bool, empty_val=True, required=False),

    # Maps patch_project to (solution/path, revision).
    #  - solution/path is then used to apply patches as patch root in
    #    bot_update.
    #  - if revision is given, it's passed verbatim to bot_update for
    #    corresponding dependency. Otherwise (ie None), the patch will be
    #    applied on top of version pinned in DEPS.
    # This is essentially a whitelist of which projects inside a solution
    # can be patched automatically by bot_update based on PATCH_PROJECT
    # property.
    # For example, bare chromium solution has this entry in patch_projects
    #     'angle/angle': ('src/third_party/angle', 'HEAD')
    # then a patch to Angle project can be applied to a chromium src's
    # checkout after first updating Angle's repo to its master's HEAD.
    patch_projects = Dict(value_type=tuple, hidden=True),
    # Same as the above, except the keys are full repo URLs.
    repo_path_map = Dict(value_type=tuple, hidden=True),

    # Check out refs/branch-heads.
    # TODO (machenbach): Only implemented for bot_update atm.
    with_branch_heads = Single(
        bool,
        empty_val=False,
        required=False,
        hidden=True),

    # Check out refs/tags.
    with_tags = Single(
        bool,
        empty_val=False,
        required=False,
        hidden=True),

    disable_syntax_validation = Single(bool, empty_val=False, required=False),

    USE_MIRROR = Static(bool(USE_MIRROR)),
    # TODO(tandrii): remove PATCH_PROJECT field.
    # DON'T USE THIS. WILL BE REMOVED.
    PATCH_PROJECT = Static(str(PATCH_PROJECT), hidden=True),
    BUILDSPEC_VERSION= Static(BUILDSPEC_VERSION, hidden=True),
  )

config_ctx = config_item_context(BaseConfig)

def ChromiumGitURL(_c, *pieces):
  return '/'.join(('https://chromium.googlesource.com',) + pieces)

# TODO(phajdan.jr): Move to proper repo and add coverage.
def ChromeInternalGitURL(_c, *pieces):  # pragma: no cover
  return '/'.join(('https://chrome-internal.googlesource.com',) + pieces)

@config_ctx()
def disable_syntax_validation(c):
  c.disable_syntax_validation = True

@config_ctx()
def android(c):
  c.target_os.add('android')

@config_ctx()
def nacl(c):
  s = c.solutions.add()
  s.name = 'native_client'
  s.url = ChromiumGitURL(c, 'native_client', 'src', 'native_client.git')
  m = c.got_revision_mapping
  m['native_client'] = 'got_revision'

@config_ctx()
def webports(c):
  s = c.solutions.add()
  s.name = 'src'
  s.url = ChromiumGitURL(c, 'webports.git')
  m = c.got_revision_mapping
  m['src'] = 'got_revision'

@config_ctx()
def wasm_llvm(c):
  s = c.solutions.add()
  s.name = 'src'
  s.url = ChromiumGitURL(
      c, 'external', 'github.com', 'WebAssembly', 'waterfall.git')
  m = c.got_revision_mapping
  m['src'] = 'got_waterfall_revision'
  c.revisions['src'] = 'origin/master'

@config_ctx()
def gyp(c):
  s = c.solutions.add()
  s.name = 'gyp'
  s.url = ChromiumGitURL(c, 'external', 'gyp.git')
  m = c.got_revision_mapping
  m['gyp'] = 'got_revision'

@config_ctx()
def build(c):
  s = c.solutions.add()
  s.name = 'build'
  s.url = ChromiumGitURL(c, 'chromium', 'tools', 'build.git')
  m = c.got_revision_mapping
  m['build'] = 'got_revision'

@config_ctx()
def depot_tools(c):  # pragma: no cover
  s = c.solutions.add()
  s.name = 'depot_tools'
  s.url = ChromiumGitURL(c, 'chromium', 'tools', 'depot_tools.git')
  m = c.got_revision_mapping
  m['depot_tools'] = 'got_revision'

@config_ctx()
def skia(c):  # pragma: no cover
  s = c.solutions.add()
  s.name = 'skia'
  s.url = 'https://skia.googlesource.com/skia.git'
  m = c.got_revision_mapping
  m['skia'] = 'got_revision'

@config_ctx()
def skia_buildbot(c):  # pragma: no cover
  s = c.solutions.add()
  s.name = 'skia_buildbot'
  s.url = 'https://skia.googlesource.com/buildbot.git'
  m = c.got_revision_mapping
  m['skia_buildbot'] = 'got_revision'

@config_ctx()
def chrome_golo(c):  # pragma: no cover
  s = c.solutions.add()
  s.name = 'chrome_golo'
  s.url = 'https://chrome-internal.googlesource.com/chrome-golo/chrome-golo.git'
  c.got_revision_mapping['chrome_golo'] = 'got_revision'

@config_ctx()
def build_internal(c):
  s = c.solutions.add()
  s.name = 'build_internal'
  s.url = 'https://chrome-internal.googlesource.com/chrome/tools/build.git'
  c.got_revision_mapping['build_internal'] = 'got_revision'
  # We do not use 'includes' here, because we want build_internal to be the
  # first solution in the list as run_presubmit computes upstream revision
  # from the first solution.
  build(c)
  c.got_revision_mapping['build'] = 'got_build_revision'

@config_ctx()
def build_internal_scripts_slave(c):
  s = c.solutions.add()
  s.name = 'build_internal/scripts/slave'
  s.url = ('https://chrome-internal.googlesource.com/'
           'chrome/tools/build_limited/scripts/slave.git')
  c.got_revision_mapping['build_internal/scripts/slave'] = 'got_revision'
  # We do not use 'includes' here, because we want build_internal to be the
  # first solution in the list as run_presubmit computes upstream revision
  # from the first solution.
  build(c)
  c.got_revision_mapping['build'] = 'got_build_revision'

@config_ctx()
def master_deps(c):
  s = c.solutions.add()
  s.name = 'master.DEPS'
  s.url = ('https://chrome-internal.googlesource.com/'
           'chrome/tools/build/master.DEPS.git')
  c.got_revision_mapping['master.DEPS'] = 'got_revision'

@config_ctx()
def slave_deps(c):
  s = c.solutions.add()
  s.name = 'slave.DEPS'
  s.url = ('https://chrome-internal.googlesource.com/'
           'chrome/tools/build/slave.DEPS.git')
  c.got_revision_mapping['slave.DEPS'] = 'got_revision'

@config_ctx()
def internal_deps(c):
  s = c.solutions.add()
  s.name = 'internal.DEPS'
  s.url = ('https://chrome-internal.googlesource.com/'
           'chrome/tools/build/internal.DEPS.git')
  c.got_revision_mapping['internal.DEPS'] = 'got_revision'

@config_ctx()
def pdfium(c):
  soln = c.solutions.add()
  soln.name = 'pdfium'
  soln.url = 'https://pdfium.googlesource.com/pdfium.git'
  m = c.got_revision_mapping
  m['pdfium'] = 'got_revision'

@config_ctx()
def mojo(c):
  soln = c.solutions.add()
  soln.name = 'src'
  soln.url = 'https://chromium.googlesource.com/external/mojo.git'

@config_ctx()
def crashpad(c):
  soln = c.solutions.add()
  soln.name = 'crashpad'
  soln.url = 'https://chromium.googlesource.com/crashpad/crashpad.git'

@config_ctx()
def boringssl(c):
  soln = c.solutions.add()
  soln.name = 'boringssl'
  soln.url = 'https://boringssl.googlesource.com/boringssl.git'
  soln.deps_file = 'util/bot/DEPS'

@config_ctx()
def dart(c):
  soln = c.solutions.add()
  soln.name = 'sdk'
  soln.url = ('https://dart.googlesource.com/sdk.git')
  soln.deps_file = 'DEPS'
  soln.managed = False

@config_ctx()
def infra(c):
  soln = c.solutions.add()
  soln.name = 'infra'
  soln.url = 'https://chromium.googlesource.com/infra/infra.git'
  c.got_revision_mapping['infra'] = 'got_revision'

  p = c.patch_projects
  p['infra/luci/luci-py'] = ('infra/luci', 'HEAD')
  # TODO(phajdan.jr): remove recipes-py when it's not used for project name.
  p['infra/luci/recipes-py'] = ('infra/recipes-py', 'HEAD')
  p['recipe_engine'] = ('infra/recipes-py', 'HEAD')

@config_ctx()
def infra_internal(c):  # pragma: no cover
  soln = c.solutions.add()
  soln.name = 'infra_internal'
  soln.url = 'https://chrome-internal.googlesource.com/infra/infra_internal.git'
  c.got_revision_mapping['infra_internal'] = 'got_revision'

@config_ctx(includes=['infra'])
def luci_gae(c):
  # luci/gae is checked out as a part of infra.git solution at HEAD.
  c.revisions['infra'] = 'origin/master'
  # luci/gae is developed together with luci-go, which should be at HEAD.
  c.revisions['infra/go/src/go.chromium.org/luci'] = 'origin/master'
  c.revisions['infra/go/src/go.chromium.org/gae'] = (
      gclient_api.RevisionFallbackChain('origin/master'))
  m = c.got_revision_mapping
  del m['infra']
  m['infra/go/src/go.chromium.org/gae'] = 'got_revision'

@config_ctx(includes=['infra'])
def luci_go(c):
  # luci-go is checked out as a part of infra.git solution at HEAD.
  c.revisions['infra'] = 'origin/master'
  c.revisions['infra/go/src/go.chromium.org/luci'] = (
      gclient_api.RevisionFallbackChain('origin/master'))
  m = c.got_revision_mapping
  del m['infra']
  m['infra/go/src/go.chromium.org/luci'] = 'got_revision'

@config_ctx(includes=['infra'])
def luci_py(c):
  # luci-py is checked out as part of infra just to have appengine
  # pre-installed, as that's what luci-py PRESUBMIT relies on.
  c.revisions['infra'] = 'origin/master'
  # TODO(tandrii): make use of c.patch_projects.
  c.revisions['infra/luci'] = (
      gclient_api.RevisionFallbackChain('origin/master'))
  m = c.got_revision_mapping
  del m['infra']
  m['infra/luci'] = 'got_revision'

@config_ctx(includes=['infra'])
def recipes_py(c):
  c.revisions['infra'] = 'origin/master'
  # TODO(tandrii): make use of c.patch_projects.
  c.revisions['infra/recipes-py'] = (
      gclient_api.RevisionFallbackChain('origin/master'))
  m = c.got_revision_mapping
  del m['infra']
  m['infra/recipes-py'] = 'got_revision'

@config_ctx()
def recipes_py_bare(c):
  soln = c.solutions.add()
  soln.name = 'recipes-py'
  soln.url = 'https://chromium.googlesource.com/infra/luci/recipes-py'
  c.got_revision_mapping['recipes-py'] = 'got_revision'

@config_ctx()
def catapult(c):
  soln = c.solutions.add()
  soln.name = 'catapult'
  soln.url = 'https://chromium.googlesource.com/catapult'
  c.got_revision_mapping['catapult'] = 'got_revision'

@config_ctx(includes=['infra_internal'])
def infradata_master_manager(c):
  soln = c.solutions.add()
  soln.name = 'infra-data-master-manager'
  soln.url = (
      'https://chrome-internal.googlesource.com/infradata/master-manager.git')
  del c.got_revision_mapping['infra_internal']
  c.got_revision_mapping['infra-data-master-manager'] = 'got_revision'

@config_ctx()
def with_branch_heads(c):
  c.with_branch_heads = True

@config_ctx()
def with_tags(c):
  c.with_tags = True

@config_ctx()
def custom_tabs_client(c):
  soln = c.solutions.add()
  soln.name = 'custom_tabs_client'
  # TODO(pasko): test custom-tabs-client within a full chromium checkout.
  soln.url = 'https://chromium.googlesource.com/custom-tabs-client'
  c.got_revision_mapping['custom_tabs_client'] = 'got_revision'

@config_ctx()
def gerrit_test_cq_normal(c):
  soln = c.solutions.add()
  soln.name = 'gerrit-test-cq-normal'
  soln.url = 'https://chromium.googlesource.com/playground/gerrit-cq/normal.git'

@config_ctx()
def angle(c):
  soln = c.solutions.add()
  soln.name = 'angle'
  soln.url = 'https://chromium.googlesource.com/angle/angle.git'
