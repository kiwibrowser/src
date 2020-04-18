# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry import story

from page_sets import polymer

class PaperCalculatorHitTest(polymer.PolymerPage):

  def __init__(self, page_set):
    super(PaperCalculatorHitTest, self).__init__(
      # Generated from https://github.com/zqureshi/paper-calculator
      # vulcanize --inline --strip paper-calculator/demo.html
      url='file://key_hit_test_cases/paper-calculator-no-rendering.html',
      page_set=page_set, run_no_page_interactions=False)


  def PerformPageInteractions(self, action_runner):
    # pay cost of selecting tap target only once
    action_runner.ExecuteJavaScript('''
        window.__tapTarget = document.querySelector(
            'body /deep/ #outerPanels'
        ).querySelector(
            '#standard'
        ).shadowRoot.querySelector(
            'paper-calculator-key[label="5"]'
        )''')
    action_runner.WaitForJavaScriptCondition(
        'window.__tapTarget != null')

    for _ in xrange(100):
        self.TapButton(action_runner)

  def TapButton(self, action_runner):
    with action_runner.CreateInteraction('Action_TapAction'):
      action_runner.TapElement(element_function='''window.__tapTarget''')


class KeyHitTestCasesPageSet(story.StorySet):

  def __init__(self):
    super(KeyHitTestCasesPageSet, self).__init__()

    self.AddStory(PaperCalculatorHitTest(self))
