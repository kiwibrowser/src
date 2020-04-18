# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from page_sets.login_helpers import login_utils


def LoginAccount(action_runner, credential,
                 credentials_path=login_utils.DEFAULT_CREDENTIAL_PATH):
  """Logs in into a Dropbox account.

  This function navigates the tab into Dropbox's login page and logs in a user
  using credentials in |credential| part of the |credentials_path| file.

  Args:
    action_runner: Action runner responsible for running actions on the page.
    credential: The credential to retrieve from the credentials file (string).
    credentials_path: The path to credential file (string).

  Raises:
    exceptions.Error: See ExecuteJavaScript()
    for a detailed list of possible exceptions.
  """
  account_name, password = login_utils.GetAccountNameAndPassword(
      credential, credentials_path=credentials_path)

  action_runner.Navigate('https://www.pandora.com/account/sign-in')
  login_utils.InputWithSelector(
      action_runner, account_name, 'input[id=login_username]')
  login_utils.InputWithSelector(
      action_runner, password, 'input[id=login_password]')

  login_button_function = ('document.getElementsByClassName('
                           '"onboarding__form__button onboarding__b1 '
                           'onboarding__form__button--blue loginButton")[0]')
  action_runner.ClickElement(element_function=login_button_function)
