# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test framework for code that interacts with gerrit.

class GerritTestCase
--------------------------------------------------------------------------------
This class initializes and runs an a gerrit instance on localhost.  To use the
framework, define a class that extends GerritTestCase, and then do standard
python unittest development as described here:

http://docs.python.org/2.7/library/unittest.html#basic-example

When your test code runs, the framework will:

- Download the latest stable(-ish) binary release of the gerrit code.
- Start up a live gerrit instance running in a temp directory on the localhost.
- Set up a single gerrit user account with admin priveleges.
- Supply credential helpers for interacting with the gerrit instance via http
  or ssh.

Refer to depot_tools/testing_support/gerrit-init.sh for details about how the
gerrit instance is set up, and refer to helper methods defined below
(createProject, cloneProject, uploadChange, etc.) for ways to interact with the
gerrit instance from your test methods.


class RepoTestCase
--------------------------------------------------------------------------------
This class extends GerritTestCase, and creates a set of project repositories
and a manifest repository that can be used in conjunction with the 'repo' tool.

Each test method will initialize and sync a brand-new repo working directory.
The 'repo' command may be invoked in a subprocess as part of your tests.

One gotcha: 'repo upload' will always attempt to use the ssh interface to talk
to gerrit.
"""

import collections
import errno
import netrc
import os
import re
import shutil
import signal
import socket
import stat
import subprocess
import sys
import tempfile
import unittest
import urllib

import gerrit_util


DEPOT_TOOLS_DIR = os.path.normpath(os.path.join(
    os.path.realpath(__file__), '..', '..'))


# When debugging test code, it's sometimes helpful to leave the test gerrit
# instance intact and running after the test code exits.  Setting TEARDOWN
# to False will do that.
TEARDOWN = True

class GerritTestCase(unittest.TestCase):
  """Test class for tests that interact with a gerrit server.

  The class setup creates and launches a stand-alone gerrit instance running on
  localhost, for test methods to interact with.  Class teardown stops and
  deletes the gerrit instance.

  Note that there is a single gerrit instance for ALL test methods in a
  GerritTestCase sub-class.
  """

  COMMIT_RE = re.compile(r'^commit ([0-9a-fA-F]{40})$')
  CHANGEID_RE = re.compile('^\s+Change-Id:\s*(\S+)$')
  DEVNULL = open(os.devnull, 'w')
  TEST_USERNAME = 'test-username'
  TEST_EMAIL = 'test-username@test.org'

  GerritInstance = collections.namedtuple('GerritInstance', [
      'credential_file',
      'gerrit_dir',
      'gerrit_exe',
      'gerrit_host',
      'gerrit_pid',
      'gerrit_url',
      'git_dir',
      'git_host',
      'git_url',
      'http_port',
      'netrc_file',
      'ssh_ident',
      'ssh_port',
  ])

  @classmethod
  def check_call(cls, *args, **kwargs):
    kwargs.setdefault('stdout', cls.DEVNULL)
    kwargs.setdefault('stderr', cls.DEVNULL)
    subprocess.check_call(*args, **kwargs)

  @classmethod
  def check_output(cls, *args, **kwargs):
    kwargs.setdefault('stderr', cls.DEVNULL)
    return subprocess.check_output(*args, **kwargs)

  @classmethod
  def _create_gerrit_instance(cls, gerrit_dir):
    gerrit_init_script = os.path.join(
        DEPOT_TOOLS_DIR, 'testing_support', 'gerrit-init.sh')
    http_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    http_sock.bind(('', 0))
    http_port = str(http_sock.getsockname()[1])
    ssh_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ssh_sock.bind(('', 0))
    ssh_port = str(ssh_sock.getsockname()[1])

    # NOTE: this is not completely safe.  These port numbers could be
    # re-assigned by the OS between the calls to socket.close() and gerrit
    # starting up.  The only safe way to do this would be to pass file
    # descriptors down to the gerrit process, which is not even remotely
    # supported.  Alas.
    http_sock.close()
    ssh_sock.close()
    cls.check_call(['bash', gerrit_init_script, '--http-port', http_port,
                    '--ssh-port', ssh_port, gerrit_dir])

    gerrit_exe = os.path.join(gerrit_dir, 'bin', 'gerrit.sh')
    cls.check_call(['bash', gerrit_exe, 'start'])
    with open(os.path.join(gerrit_dir, 'logs', 'gerrit.pid')) as fh:
      gerrit_pid = int(fh.read().rstrip())

    return cls.GerritInstance(
        credential_file=os.path.join(gerrit_dir, 'tmp', '.git-credentials'),
        gerrit_dir=gerrit_dir,
        gerrit_exe=gerrit_exe,
        gerrit_host='localhost:%s' % http_port,
        gerrit_pid=gerrit_pid,
        gerrit_url='http://localhost:%s' % http_port,
        git_dir=os.path.join(gerrit_dir, 'git'),
        git_host='%s/git' % gerrit_dir,
        git_url='file://%s/git' % gerrit_dir,
        http_port=http_port,
        netrc_file=os.path.join(gerrit_dir, 'tmp', '.netrc'),
        ssh_ident=os.path.join(gerrit_dir, 'tmp', 'id_rsa'),
        ssh_port=ssh_port,)

  @classmethod
  def setUpClass(cls):
    """Sets up the gerrit instances in a class-specific temp dir."""
    # Create gerrit instance.
    gerrit_dir = tempfile.mkdtemp()
    os.chmod(gerrit_dir, 0o700)
    gi = cls.gerrit_instance = cls._create_gerrit_instance(gerrit_dir)

    # Set netrc file for http authentication.
    cls.gerrit_util_netrc_orig = gerrit_util.NETRC
    gerrit_util.NETRC = netrc.netrc(gi.netrc_file)

    # gerrit_util.py defaults to using https, but for testing, it's much
    # simpler to use http connections.
    cls.gerrit_util_protocol_orig = gerrit_util.GERRIT_PROTOCOL
    gerrit_util.GERRIT_PROTOCOL = 'http'

    # Because we communicate with the test server via http, rather than https,
    # libcurl won't add authentication headers to raw git requests unless the
    # gerrit server returns 401.  That works for pushes, but for read operations
    # (like git-ls-remote), gerrit will simply omit any ref that requires
    # authentication.  By default gerrit doesn't permit anonymous read access to
    # refs/meta/config.  Override that behavior so tests can access
    # refs/meta/config if necessary.
    clone_path = os.path.join(gi.gerrit_dir, 'tmp', 'All-Projects')
    cls._CloneProject('All-Projects', clone_path)
    project_config = os.path.join(clone_path, 'project.config')
    cls.check_call(['git', 'config', '--file', project_config, '--add',
                    'access.refs/meta/config.read', 'group Anonymous Users'])
    cls.check_call(['git', 'add', project_config], cwd=clone_path)
    cls.check_call(
        ['git', 'commit', '-m', 'Anonyous read for refs/meta/config'],
        cwd=clone_path)
    cls.check_call(['git', 'push', 'origin', 'HEAD:refs/meta/config'],
                   cwd=clone_path)

  def setUp(self):
    self.tempdir = tempfile.mkdtemp()
    os.chmod(self.tempdir, 0o700)

  def tearDown(self):
    if TEARDOWN:
      shutil.rmtree(self.tempdir)

  @classmethod
  def createProject(cls, name, description='Test project', owners=None,
                    submit_type='CHERRY_PICK'):
    """Create a project on the test gerrit server."""
    if owners is None:
      owners = ['Administrators']
    body = {
        'description': description,
        'submit_type': submit_type,
        'owners': owners,
    }
    path = 'projects/%s' % urllib.quote(name, '')
    conn = gerrit_util.CreateHttpConn(
        cls.gerrit_instance.gerrit_host, path, reqtype='PUT', body=body)
    jmsg = gerrit_util.ReadHttpJsonResponse(conn, accept_statuses=[200, 201])
    assert jmsg['name'] == name

  @classmethod
  def _post_clone_bookkeeping(cls, clone_path):
    config_path = os.path.join(clone_path, '.git', 'config')
    cls.check_call(
        ['git', 'config', '--file', config_path, 'user.email', cls.TEST_EMAIL])
    cls.check_call(
        ['git', 'config', '--file', config_path, 'credential.helper',
         'store --file=%s' % cls.gerrit_instance.credential_file])

  @classmethod
  def _CloneProject(cls, name, path):
    """Clone a project from the test gerrit server."""
    gi = cls.gerrit_instance
    parent_dir = os.path.dirname(path)
    if not os.path.exists(parent_dir):
      os.makedirs(parent_dir)
    url = '/'.join((gi.gerrit_url, name))
    cls.check_call(['git', 'clone', url, path])
    cls._post_clone_bookkeeping(path)
    # Install commit-msg hook to add Change-Id lines.
    hook_path = os.path.join(path, '.git', 'hooks', 'commit-msg')
    cls.check_call(['curl', '-o', hook_path,
                    '/'.join((gi.gerrit_url, 'tools/hooks/commit-msg'))])
    os.chmod(hook_path, stat.S_IRWXU)
    return path

  def cloneProject(self, name, path=None):
    """Clone a project from the test gerrit server."""
    if path is None:
      path = os.path.basename(name)
      if path.endswith('.git'):
        path = path[:-4]
    path = os.path.join(self.tempdir, path)
    return self._CloneProject(name, path)

  @classmethod
  def _CreateCommit(cls, clone_path, fn=None, msg=None, text=None):
    """Create a commit in the given git checkout."""
    if not fn:
      fn = 'test-file.txt'
    if not msg:
      msg = 'Test Message'
    if not text:
      text = 'Another day, another dollar.'
    fpath = os.path.join(clone_path, fn)
    with open(fpath, 'a') as fh:
      fh.write('%s\n' % text)
    cls.check_call(['git', 'add', fn], cwd=clone_path)
    cls.check_call(['git', 'commit', '-m', msg], cwd=clone_path)
    return cls._GetCommit(clone_path)

  def createCommit(self, clone_path, fn=None, msg=None, text=None):
    """Create a commit in the given git checkout."""
    clone_path = os.path.join(self.tempdir, clone_path)
    return self._CreateCommit(clone_path, fn, msg, text)

  @classmethod
  def _GetCommit(cls, clone_path, ref='HEAD'):
    """Get the sha1 and change-id for a ref in the git checkout."""
    log_proc = cls.check_output(['git', 'log', '-n', '1', ref], cwd=clone_path)
    sha1 = None
    change_id = None
    for line in log_proc.splitlines():
      match = cls.COMMIT_RE.match(line)
      if match:
        sha1 = match.group(1)
        continue
      match = cls.CHANGEID_RE.match(line)
      if match:
        change_id = match.group(1)
        continue
    assert sha1
    assert change_id
    return (sha1, change_id)

  def getCommit(self, clone_path, ref='HEAD'):
    """Get the sha1 and change-id for a ref in the git checkout."""
    clone_path = os.path.join(self.tempdir, clone_path)
    return self._GetCommit(clone_path, ref)

  @classmethod
  def _UploadChange(cls, clone_path, branch='master', remote='origin'):
    """Create a gerrit CL from the HEAD of a git checkout."""
    cls.check_call(
        ['git', 'push', remote, 'HEAD:refs/for/%s' % branch], cwd=clone_path)

  def uploadChange(self, clone_path, branch='master', remote='origin'):
    """Create a gerrit CL from the HEAD of a git checkout."""
    clone_path = os.path.join(self.tempdir, clone_path)
    self._UploadChange(clone_path, branch, remote)

  @classmethod
  def _PushBranch(cls, clone_path, branch='master'):
    """Push a branch directly to gerrit, bypassing code review."""
    cls.check_call(
        ['git', 'push', 'origin', 'HEAD:refs/heads/%s' % branch],
        cwd=clone_path)

  def pushBranch(self, clone_path, branch='master'):
    """Push a branch directly to gerrit, bypassing code review."""
    clone_path = os.path.join(self.tempdir, clone_path)
    self._PushBranch(clone_path, branch)

  @classmethod
  def createAccount(cls, name='Test User', email='test-user@test.org',
                    password=None, groups=None):
    """Create a new user account on gerrit."""
    username = email.partition('@')[0]
    gerrit_cmd = 'gerrit create-account %s --full-name "%s" --email %s' % (
        username, name, email)
    if password:
      gerrit_cmd += ' --http-password "%s"' % password
    if groups:
      gerrit_cmd += ' '.join(['--group %s' % x for x in groups])
    ssh_cmd = ['ssh', '-p', cls.gerrit_instance.ssh_port,
           '-i', cls.gerrit_instance.ssh_ident,
           '-o', 'NoHostAuthenticationForLocalhost=yes',
           '-o', 'StrictHostKeyChecking=no',
           '%s@localhost' % cls.TEST_USERNAME, gerrit_cmd]
    cls.check_call(ssh_cmd)

  @classmethod
  def _stop_gerrit(cls, gerrit_instance):
    """Stops the running gerrit instance and deletes it."""
    try:
      # This should terminate the gerrit process.
      cls.check_call(['bash', gerrit_instance.gerrit_exe, 'stop'])
    finally:
      try:
        # cls.gerrit_pid should have already terminated.  If it did, then
        # os.waitpid will raise OSError.
        os.waitpid(gerrit_instance.gerrit_pid, os.WNOHANG)
      except OSError as e:
        if e.errno == errno.ECHILD:
          # If gerrit shut down cleanly, os.waitpid will land here.
          # pylint: disable=lost-exception
          return

      # If we get here, the gerrit process is still alive.  Send the process
      # SIGKILL for good measure.
      try:
        os.kill(gerrit_instance.gerrit_pid, signal.SIGKILL)
      except OSError:
        if e.errno == errno.ESRCH:
          # os.kill raised an error because the process doesn't exist.  Maybe
          # gerrit shut down cleanly after all.
          # pylint: disable=lost-exception
          return

      # Announce that gerrit didn't shut down cleanly.
      msg = 'Test gerrit server (pid=%d) did not shut down cleanly.' % (
          gerrit_instance.gerrit_pid)
      print >> sys.stderr, msg

  @classmethod
  def tearDownClass(cls):
    gerrit_util.NETRC = cls.gerrit_util_netrc_orig
    gerrit_util.GERRIT_PROTOCOL = cls.gerrit_util_protocol_orig
    if TEARDOWN:
      cls._stop_gerrit(cls.gerrit_instance)
      shutil.rmtree(cls.gerrit_instance.gerrit_dir)


class RepoTestCase(GerritTestCase):
  """Test class which runs in a repo checkout."""

  REPO_URL = 'https://chromium.googlesource.com/external/repo'
  MANIFEST_PROJECT = 'remotepath/manifest'
  MANIFEST_TEMPLATE = """<?xml version="1.0" encoding="UTF-8"?>
<manifest>
  <remote name="remote1"
          fetch="%(gerrit_url)s"
          review="%(gerrit_host)s" />
  <remote name="remote2"
          fetch="%(gerrit_url)s"
          review="%(gerrit_host)s" />
  <default revision="refs/heads/master" remote="remote1" sync-j="1" />
  <project remote="remote1" path="localpath/testproj1" name="remotepath/testproj1" />
  <project remote="remote1" path="localpath/testproj2" name="remotepath/testproj2" />
  <project remote="remote2" path="localpath/testproj3" name="remotepath/testproj3" />
  <project remote="remote2" path="localpath/testproj4" name="remotepath/testproj4" />
</manifest>
"""

  @classmethod
  def setUpClass(cls):
    GerritTestCase.setUpClass()
    gi = cls.gerrit_instance

    # Create local mirror of repo tool repository.
    repo_mirror_path = os.path.join(gi.git_dir, 'repo.git')
    cls.check_call(
        ['git', 'clone', '--mirror', cls.REPO_URL, repo_mirror_path])

    # Check out the top-level repo script; it will be used for invocation.
    repo_clone_path = os.path.join(gi.gerrit_dir, 'tmp', 'repo')
    cls.check_call(['git', 'clone', '-n', repo_mirror_path, repo_clone_path])
    cls.check_call(
        ['git', 'checkout', 'origin/stable', 'repo'], cwd=repo_clone_path)
    shutil.rmtree(os.path.join(repo_clone_path, '.git'))
    cls.repo_exe = os.path.join(repo_clone_path, 'repo')

    # Create manifest repository.
    cls.createProject(cls.MANIFEST_PROJECT)
    clone_path = os.path.join(gi.gerrit_dir, 'tmp', 'manifest')
    cls._CloneProject(cls.MANIFEST_PROJECT, clone_path)
    manifest_path = os.path.join(clone_path, 'default.xml')
    with open(manifest_path, 'w') as fh:
      fh.write(cls.MANIFEST_TEMPLATE % gi.__dict__)
    cls.check_call(['git', 'add', 'default.xml'], cwd=clone_path)
    cls.check_call(['git', 'commit', '-m', 'Test manifest.'], cwd=clone_path)
    cls._PushBranch(clone_path)

    # Create project repositories.
    for i in xrange(1, 5):
      proj = 'testproj%d' % i
      cls.createProject('remotepath/%s' % proj)
      clone_path = os.path.join(gi.gerrit_dir, 'tmp', proj)
      cls._CloneProject('remotepath/%s' % proj, clone_path)
      cls._CreateCommit(clone_path)
      cls._PushBranch(clone_path, 'master')

  def setUp(self):
    super(RepoTestCase, self).setUp()
    manifest_url = '/'.join((self.gerrit_instance.gerrit_url,
                            self.MANIFEST_PROJECT))
    repo_url = '/'.join((self.gerrit_instance.gerrit_url, 'repo'))
    self.check_call(
        [self.repo_exe, 'init', '-u', manifest_url, '--repo-url',
         repo_url, '--no-repo-verify'], cwd=self.tempdir)
    self.check_call([self.repo_exe, 'sync'], cwd=self.tempdir)
    for i in xrange(1, 5):
      clone_path = os.path.join(self.tempdir, 'localpath', 'testproj%d' % i)
      self._post_clone_bookkeeping(clone_path)
      # Tell 'repo upload' to upload this project without prompting.
      config_path = os.path.join(clone_path, '.git', 'config')
      self.check_call(
          ['git', 'config', '--file', config_path, 'review.%s.upload' %
           self.gerrit_instance.gerrit_host, 'true'])

  @classmethod
  def runRepo(cls, *args, **kwargs):
    # Unfortunately, munging $HOME appears to be the only way to control the
    # netrc file used by repo.
    munged_home = os.path.join(cls.gerrit_instance.gerrit_dir, 'tmp')
    if 'env' not in kwargs:
      env = kwargs['env'] = os.environ.copy()
      env['HOME'] = munged_home
    else:
      env.setdefault('HOME', munged_home)
    args[0].insert(0, cls.repo_exe)
    cls.check_call(*args, **kwargs)

  def uploadChange(self, clone_path, branch='master', remote='origin'):
    review_host = self.check_output(
        ['git', 'config', 'remote.%s.review' % remote],
        cwd=clone_path).strip()
    assert(review_host)
    projectname = self.check_output(
        ['git', 'config', 'remote.%s.projectname' % remote],
        cwd=clone_path).strip()
    assert(projectname)
    GerritTestCase._UploadChange(
        clone_path, branch=branch, remote='%s://%s/%s' % (
            gerrit_util.GERRIT_PROTOCOL, review_host, projectname))
