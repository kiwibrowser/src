# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Simple Request handler using Jinja2 templates."""

import logging
import os

import jinja2
import webapp2

from google.appengine.api import users

from dashboard.common import utils
from dashboard.common import xsrf

_DASHBOARD_PYTHON_DIR = os.path.dirname(os.path.dirname(__file__))

JINJA2_ENVIRONMENT = jinja2.Environment(
    loader=jinja2.FileSystemLoader(
        [os.path.join(_DASHBOARD_PYTHON_DIR, 'templates')]),
    # Security team suggests that autoescaping be enabled.
    autoescape=True,
    extensions=['jinja2.ext.autoescape'])


class RequestHandler(webapp2.RequestHandler):
  """Base class for requests. Does common template and error handling tasks."""

  def RenderHtml(self, template_file, template_values, status=200):
    """Renders HTML given template and values.

    Args:
      template_file: string. File name under templates directory.
      template_values: dict. Mapping of template variables to corresponding.
          values.
      status: int. HTTP status code.
    """
    self.response.set_status(status)
    template = JINJA2_ENVIRONMENT.get_template(template_file)
    self.GetDynamicVariables(template_values)
    self.response.out.write(template.render(template_values))

  def RenderStaticHtml(self, filename):
    filename = os.path.join(_DASHBOARD_PYTHON_DIR, 'static', filename)
    contents = open(filename, 'r')
    self.response.out.write(contents.read())
    contents.close()

  def GetDynamicVariables(self, template_values, request_path=None):
    """Gets the values that vary for every page.

    Args:
      template_values: dict of name/value pairs.
      request_path: path for login urls, None if using the current path.
    """
    user_info = ''
    xsrf_token = ''
    user = users.get_current_user()
    display_username = 'Sign in'
    title = 'Sign in to an account'
    is_admin = False
    if user:
      display_username = user.email()
      title = 'Switch user'
      xsrf_token = xsrf.GenerateToken(user)
      is_admin = users.is_current_user_admin()
    try:
      login_url = users.create_login_url(request_path or self.request.path_qs)
    except users.RedirectTooLongError:
      # On the bug filing pages, the full login URL can be too long. Drop
      # the correct redirect URL, since the user should already be logged in at
      # this point anyway.
      login_url = users.create_login_url('/')
    user_info = '<a href="%s" title="%s">%s</a>' % (
        login_url, title, display_username)
    # Force out of passive login, as it creates multilogin issues.
    login_url = login_url.replace('passive=true', 'passive=false')
    template_values['login_url'] = login_url
    template_values['display_username'] = display_username
    template_values['user_info'] = user_info
    template_values['is_admin'] = is_admin
    template_values['is_internal_user'] = utils.IsInternalUser()
    template_values['xsrf_token'] = xsrf_token
    template_values['xsrf_input'] = (
        '<input type="hidden" name="xsrf_token" value="%s">' % xsrf_token)
    template_values['login_url'] = login_url
    return template_values

  def ReportError(self, error_message, status=500):
    """Reports the given error to the client and logs the error.

    Args:
      error_message: The message to log and send to the client.
      status: The HTTP response code to use.
    """
    logging.error(error_message)
    self.response.set_status(status)
    self.response.out.write('%s\nrequest_id:%s\n' %
                            (error_message, utils.GetRequestId()))

  def ReportWarning(self, warning_message, status=200):
    """Reports a warning to the client and logs the warning.

    Args:
      warning_message: The warning message to log (as an error).
      status: The http response code to use.
    """
    logging.warning(warning_message)
    self.response.set_status(status)
    self.response.out.write('%s\nrequest_id:%s\n' %
                            (warning_message, utils.GetRequestId()))


class InvalidInputError(Exception):
  """An error class for invalid user input query parameter values."""
  pass
