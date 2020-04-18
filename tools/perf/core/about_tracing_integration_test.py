# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry import decorators
from telemetry.testing import tab_test_case


class AboutTracingIntegrationTest(tab_test_case.TabTestCase):

  @decorators.Disabled('android',
                       'win',       # https://crbug.com/632871
                       'chromeos')  # https://crbug.com/654044
  def testBasicTraceRecording(self):
    action_runner = self._tab.action_runner
    action_runner.Navigate('chrome://tracing')
    record_button_js = (
        "document.querySelector('tr-ui-timeline-view').shadowRoot."
        "querySelector('#record-button')")

    # Click 'record' to trigger record selection diaglog.
    action_runner.WaitForElement(element_function=record_button_js)
    action_runner.ClickElement(element_function=record_button_js)

    # Wait for record selection diaglog to pop up, then click record.
    action_runner.WaitForElement(selector='.overlay')
    action_runner.ClickElement(
        element_function=("document.querySelector('.overlay').shadowRoot."
                          "querySelectorAll('button')[0]"))

    # Stop recording after 1 seconds.
    action_runner.Wait(1)
    action_runner.ClickElement(
        element_function=
        "document.querySelector('.overlay').querySelector('button')")

    # Make sure that we can see the browser track.
    action_runner.WaitForElement(selector='div[title~="Browser"]')
