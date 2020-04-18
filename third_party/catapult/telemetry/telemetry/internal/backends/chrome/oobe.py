# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from telemetry.core import exceptions
from telemetry.internal.browser import web_contents

import py_utils


class Oobe(web_contents.WebContents):
  def __init__(self, inspector_backend):
    super(Oobe, self).__init__(inspector_backend)

  def _GaiaWebviewContext(self):
    webview_contexts = self.GetWebviewContexts()
    for webview in webview_contexts:
      try:
        # GAIA webview has base.href accounts.google.com.
        if webview.EvaluateJavaScript(
            """
            bases = document.getElementsByTagName('base');
            bases.length > 0 ?
                bases[0].href.indexOf('https://accounts.google.com/') == 0 : false;
            """):
          py_utils.WaitFor(webview.HasReachedQuiescence, 20)
          return webview
      except exceptions.DevtoolsTargetCrashException:
        pass
    return None

  def _ExecuteOobeApi(self, api, *args):
    logging.info('Invoking %s', api)
    self.WaitForJavaScriptCondition(
        "typeof Oobe == 'function' && Oobe.readyForTesting", timeout=120)

    if self.EvaluateJavaScript(
        "typeof {{ @api }} == 'undefined'", api=api):
      raise exceptions.LoginException('%s js api missing' % api)

    # Example values:
    #   |api|:    'doLogin'
    #   |args|:   ['username', 'pass', True]
    #   Executes: 'doLogin("username", "pass", true)'
    self.ExecuteJavaScript('{{ @f }}({{ *args }})', f=api, args=args)

  def _WaitForEnterpriseWebview(self, username):
    """Waits for enterprise webview to be visible. We look for a span with the
    title set to the domain, for example <span title="managedchrome.com">."""
    _, domain = username.split('@')
    def _EnterpriseWebviewVisible():
      try:
        webview = self._GaiaWebviewContext()
        return webview and webview.EvaluateJavaScript(
            "document.querySelectorAll('span[title= {{ domain }}]').length;",
            domain=domain)
      except exceptions.DevtoolsTargetCrashException:
        return False
    py_utils.WaitFor(_EnterpriseWebviewVisible, 60)

  def NavigateGuestLogin(self):
    """Logs in as guest."""
    self._ExecuteOobeApi('Oobe.guestLoginForTesting')

  def NavigateFakeLogin(self, username, password, gaia_id,
                        enterprise_enroll=False):
    """Fake user login."""
    self._ExecuteOobeApi('Oobe.loginForTesting', username, password, gaia_id,
                         enterprise_enroll)
    if enterprise_enroll:
      self._WaitForEnterpriseWebview(username)

  def NavigateGaiaLogin(self, username, password,
                        enterprise_enroll=False,
                        for_user_triggered_enrollment=False):
    """Logs in using the GAIA webview. |enterprise_enroll| allows for enterprise
    enrollment. |for_user_triggered_enrollment| should be False for remora
    enrollment."""
    # TODO(achuith): Get rid of this call. crbug.com/804216.
    self._ExecuteOobeApi('Oobe.skipToLoginForTesting')
    if for_user_triggered_enrollment:
      self._ExecuteOobeApi('Oobe.switchToEnterpriseEnrollmentForTesting')

    py_utils.WaitFor(self._GaiaWebviewContext, 20)
    self._NavigateWebviewLogin(username, password,
                               wait_for_close=not enterprise_enroll)

    if enterprise_enroll:
      self.WaitForJavaScriptCondition(
          'Oobe.isEnrollmentSuccessfulForTest()', timeout=30)
      self._ExecuteOobeApi('Oobe.enterpriseEnrollmentDone')

  def _NavigateWebviewLogin(self, username, password, wait_for_close):
    """Logs into the webview-based GAIA screen."""
    self._NavigateWebviewEntry('identifierId', username, 'identifierNext')
    self._NavigateWebviewEntry('password', password, 'passwordNext')
    if wait_for_close:
      py_utils.WaitFor(lambda: not self._GaiaWebviewContext(), 60)

  def _NavigateWebviewEntry(self, field, value, next_field):
    """Navigate a username/password GAIA screen."""
    self._WaitForField(field)
    self._WaitForField(next_field)
    # This code supports both ChromeOS Gaia v1 and v2.
    # In v2 'password' id is assigned to <DIV> element encapsulating
    # unnamed <INPUT>. So this code will select the first <INPUT> element
    # below the given field id in the DOM tree if field id is not attached
    # to <INPUT>.
    self._GaiaWebviewContext().ExecuteJavaScript(
        """
        var field = document.getElementById({{ field }});
        if (field.tagName != 'INPUT')
          field = field.getElementsByTagName('INPUT')[0];

        field.value= {{ value }};
        document.getElementById({{ next_field }}).click();""",
        field=field,
        value=value,
        next_field=next_field)

  def _WaitForField(self, field):
    """Wait for username/password field to become available."""
    self._GaiaWebviewContext().WaitForJavaScriptCondition(
        "document.getElementById({{ field }}) != null",
        field=field, timeout=20)
