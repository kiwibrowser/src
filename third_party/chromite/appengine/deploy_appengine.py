# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helper script to deploy the App Engine application."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import contextlib
import os

import jinja2

from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import osutils

# The templates' names.
_APP_TEMPLATE_YAML = 'app_template.yaml'
_APP_YAML = 'app.yaml'
_OPENAPI_TEMPLATE_YAML = 'openapi-appengine_template.yaml'
_OPENAPI_YAML = 'openapi-appengine.yaml'


@contextlib.contextmanager
def _PrepareAppFolder(options):
  """Copies this folder and its symlink'd dependencies into a temporary dir.

  Returns:
    A contextmanager that yields a temporary directory and cleans up afterward.
  """
  with osutils.TempDir() as tempdir:
    # This is rsync in 'archive' mode, but symlinks are followed to copy actual
    # files/directories.
    rsync_cmd = ['rsync', '-qrLgotD',
                 '--exclude', '*.pyc',
                 '--exclude', '__pycache__',
                 '--exclude', '*.git']
    for path in options.skip_paths:
      rsync_cmd.extend(['--exclude', path])

    cros_build_lib.RunCommand(rsync_cmd + ['.', tempdir],
                              cwd=options.project_path)
    yield tempdir


def _GenerateAppYaml(tempdir, app_template, injected_keys):
  """Generates app.yaml by filling a template.

  Args:
    tempdir: The tempdir where the project was copied to.
    app_template: A string file name, indicating the template to fill in.
    injected_keys: A dict, indicating the keys to be injected.
  """
  env = jinja2.Environment(loader=jinja2.FileSystemLoader(tempdir))
  contents = env.get_template(app_template).stream(injected_keys)
  contents.dump(os.path.join(tempdir, _APP_YAML))


def _GenerateOpenAPIYaml(tempdir, openapi_template, injected_keys):
  """Generates openapi-appengine.yaml by filling a template.

  Args:
    tempdir: The tempdir where the project was copied to.
    openapi_template: A string file name, indicating he template to fill in.
    injected_keys: A dict, indicating the keys to be injected.
  """
  env = jinja2.Environment(loader=jinja2.FileSystemLoader(tempdir))
  contents = env.get_template(openapi_template).stream(injected_keys)
  contents.dump(os.path.join(tempdir, _OPENAPI_YAML))


def _DeployEndpoint(tempdir, options):
  """Deploy Endpoint Service.

  Args:
    tempdir: The tempdir where the project was copied to.
    options: The parsed options to deploy.
  """
  logging.info('Deploying Endpoint:')
  injected_keys = {'endpoints_service': options.endpoints_service}
  _GenerateOpenAPIYaml(tempdir, options.openapi_template, injected_keys)
  cros_build_lib.RunCommand(
      ['gcloud', 'endpoints', 'services', 'deploy', _OPENAPI_YAML,
       '--project=%s' % options.project_id],
      cwd=tempdir,
      mute_output=False)


def _GetNewestEndpoint(tempdir, endpoints_service):
  """Get the newest deployed Endpoint id.

  Args:
    tempdir: The tempdir where the project was copied to.
    endpoints_service: The deployed endpoint service.

  Returns:
    A string, indicating the newest endpoint config or None if no endpoints
    service is required.
  """
  if not endpoints_service:
    return None

  result = cros_build_lib.RunCommand(
      ['gcloud', 'endpoints', 'configs', 'list',
       '--service=%s' % endpoints_service, '--limit=1'], cwd=tempdir,
      capture_output=True)
  logging.info('Endpoints configs list:\n%s', result.output)
  config = result.output.splitlines()[1]
  return config.split(' ')[0].strip()


def _GenerateRequirements(tempdir):
  """Generates a requirements.txt file.

  Appengine uses the requirements.txt file to build the docker image.

  Args:
    tempdir: The tempdir where the project was copied to.
  """
  path = os.path.join(tempdir, 'requirements.txt')
  cros_build_lib.RunCommand(['pipenv', 'lock', '-r'],
                            log_stdout_to_file=path, cwd=tempdir)


def _DeployApp(tempdir, config_id, options):
  """Deploy GAE App Service.

  Args:
    tempdir: The tempdir where the project was copied to.
    config_id: the most recent endpoint id for this app. If it's None, it means
      no endpoint service is needed for this app.
    options: The parsed options to deploy.
  """
  logging.info('Deploying GAE app:')
  injected_keys = {}
  if config_id:
    injected_keys = {'endpoints_service': options.endpoints_service,
                     'endpoints_service_config_id': config_id}

  _GenerateAppYaml(tempdir, options.app_template, injected_keys)
  _GenerateRequirements(tempdir)
  cros_build_lib.RunCommand(
      ['gcloud', 'app', 'deploy', _APP_YAML,
       '--project=%s' % options.project_id],
      cwd=tempdir, mute_output=False)


def _MakeParser():
  """Return parser for deploy_app."""
  parser = commandline.ArgumentParser()
  parser.add_argument(
      '--project_path', help='The location of the project to deploy',
      required=True)
  parser.add_argument(
      '--project_id', help='The project to deploy', required=True)

  parser.add_argument(
      '--app_template', help='The app template to fill in',
      default=_APP_TEMPLATE_YAML)
  parser.add_argument(
      '--skip_paths', action='append', help='The paths to skip in deployment')

  parser.add_argument(
      '--endpoints_service', help='The endpoint service to deploy')
  parser.add_argument(
      '--openapi_template', help='The openapi template to fill in',
      default=_OPENAPI_TEMPLATE_YAML)

  parser.add_argument(
      '--skip_endpoint', help='Skip endpoint deployment.', action='store_true')
  parser.add_argument(
      '--skip_app', help='Skip app deployment.', action='store_true')
  return parser


def _VerifyOptions(options):
  """Verify the passed-in options.

  Args:
    options: The parsed options to verify.

  Returns:
    Boolean, True if verification passes, False otherwise.
  """
  if options.endpoints_service and not options.openapi_template:
    logging.error('Please specify openAPI template with --openapi_template '
                  'in deploying endpoints.')
    return False

  if options.openapi_template and not options.endpoints_service:
    logging.error('Please specify endpoints service with --endpoints_service '
                  'in deploying endpoints.')
    return False

  if (options.endpoints_service and
      options.project_id not in options.endpoints_service):
    logging.error('The project "%s" is not matched to the endpoints service '
                  '"%s".', options.project_id, options.endpoints_service)
    return False

  return True


def main(argv):
  """Deploys the endpoints & app."""
  deploy_parser = _MakeParser()
  options = deploy_parser.parse_args(argv)

  if not _VerifyOptions(options):
    deploy_parser.print_help()
    return

  with _PrepareAppFolder(options) as tempdir:
    logging.info('Deployment directory: %s', tempdir)
    if options.endpoints_service and not options.skip_endpoint:
      _DeployEndpoint(tempdir, options)

    if not options.skip_app:
      endpoint_config_id = _GetNewestEndpoint(tempdir,
                                              options.endpoints_service)
      _DeployApp(tempdir, endpoint_config_id, options)
