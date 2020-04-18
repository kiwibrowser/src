# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story
from telemetry.util import js_template


class PolymerPage(page_module.Page):

  def __init__(self, url, page_set, run_no_page_interactions):
    """ Base class for all polymer pages.

    Args:
      run_no_page_interactions: whether the page will run any interactions after
        navigate steps.
    """
    super(PolymerPage, self).__init__(
      url=url,
      shared_page_state_class=shared_page_state.SharedMobilePageState,
      page_set=page_set,
      name=url)
    self.script_to_evaluate_on_commit = '''
      document.addEventListener("polymer-ready", function() {
        window.__polymer_ready = true;
      });
    '''
    self._run_no_page_interactions = run_no_page_interactions

  def RunPageInteractions(self, action_runner):
    # If a polymer page wants to customize its actions, it should
    # override the PerformPageInteractions method instead of this method.
    if self._run_no_page_interactions:
      return
    self.PerformPageInteractions(action_runner)

  def PerformPageInteractions(self, action_runner):
    """ Override this to perform actions after the page has navigated. """
    pass

  def RunNavigateSteps(self, action_runner):
    super(PolymerPage, self).RunNavigateSteps(action_runner)
    action_runner.WaitForJavaScriptCondition(
        'window.__polymer_ready')


class PolymerCalculatorPage(PolymerPage):

  def __init__(self, page_set, run_no_page_interactions):
    super(PolymerCalculatorPage, self).__init__(
      url=('http://www.polymer-project.org/components/paper-calculator/'
          'demo.html'),
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    self.TapButton(action_runner)
    self.SlidePanel(action_runner)

  def TapButton(self, action_runner):
    with action_runner.CreateInteraction('PolymerAnimation', repeatable=True):
      action_runner.TapElement(element_function='''
          document.querySelector(
              'body /deep/ #outerPanels'
          ).querySelector(
              '#standard'
          ).shadowRoot.querySelector(
              'paper-calculator-key[label="5"]'
          )''')
      action_runner.Wait(2)

  def SlidePanel(self, action_runner):
    # only bother with this interaction if the drawer is hidden
    opened = action_runner.EvaluateJavaScript('''
        (function() {
          var outer = document.querySelector("body /deep/ #outerPanels");
          return outer.opened || outer.wideMode;
          }());''')
    if not opened:
      with action_runner.CreateInteraction('PolymerAnimation', repeatable=True):
        action_runner.SwipeElement(
            left_start_ratio=0.1, top_start_ratio=0.2,
            direction='left', distance=300, speed_in_pixels_per_second=5000,
            element_function='''
                document.querySelector(
                  'body /deep/ #outerPanels'
                ).querySelector(
                  '#advanced'
                ).shadowRoot.querySelector(
                  '.handle-bar'
                )''')
        action_runner.WaitForJavaScriptCondition('''
            var outer = document.querySelector("body /deep/ #outerPanels");
            outer.opened || outer.wideMode;''')


class PolymerShadowPage(PolymerPage):

  def __init__(self, page_set, run_no_page_interactions):
    super(PolymerShadowPage, self).__init__(
      url='http://www.polymer-project.org/components/paper-shadow/demo.html',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('ScrollAndShadowAnimation'):
      action_runner.ExecuteJavaScript(
          "document.getElementById('fab').scrollIntoView()")
      action_runner.Wait(5)
      self.AnimateShadow(action_runner, 'card')
      #FIXME(wiltzius) disabling until this issue is fixed:
      # https://github.com/Polymer/paper-shadow/issues/12
      #self.AnimateShadow(action_runner, 'fab')

  def AnimateShadow(self, action_runner, eid):
    for i in range(1, 6):
      action_runner.ExecuteJavaScript(
          'document.getElementById({{ eid }}).z = {{ i }}', eid=eid, i=i)
      action_runner.Wait(1)


class PolymerSampler(PolymerPage):

  def __init__(self, page_set, anchor, run_no_page_interactions,
               scrolling_page=False):
    """Page exercising interactions with a single Paper Sampler subpage.

    Args:
      page_set: Page set to inforporate this page into.
      anchor: string indicating which subpage to load (matches the element
          type that page is displaying)
      scrolling_page: Whether scrolling the content pane is relevant to this
          content page or not.
    """
    super(PolymerSampler, self).__init__(
      url=('http://www.polymer-project.org/components/%s/demo.html' % anchor),
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)
    self.scrolling_page = scrolling_page
    self.iframe_js = 'document'

  def RunNavigateSteps(self, action_runner):
    super(PolymerSampler, self).RunNavigateSteps(action_runner)
    action_runner.ExecuteJavaScript("""
        window.Polymer.whenPolymerReady(function() {
          {{ @iframe }}.contentWindow.Polymer.whenPolymerReady(function() {
            window.__polymer_ready = true;
          })
        });
        """, iframe=self.iframe_js)
    action_runner.WaitForJavaScriptCondition(
        'window.__polymer_ready')

  def PerformPageInteractions(self, action_runner):
    #TODO(wiltzius) Add interactions for input elements and shadow pages
    if self.scrolling_page:
      # Only bother scrolling the page if its been marked as worthwhile
      self.ScrollContentPane(action_runner)
    self.TouchEverything(action_runner)

  def ScrollContentPane(self, action_runner):
    element_function = (self.iframe_js + '.querySelector('
        '"core-scroll-header-panel").$.mainContainer')
    with action_runner.CreateInteraction('Scroll_Page', repeatable=True):
      action_runner.ScrollElement(use_touch=True,
                                  direction='down',
                                  distance='900',
                                  element_function=element_function)
    with action_runner.CreateInteraction('Scroll_Page', repeatable=True):
      action_runner.ScrollElement(use_touch=True,
                                  direction='up',
                                  distance='900',
                                  element_function=element_function)

  def TouchEverything(self, action_runner):
    tappable_types = [
        'paper-button',
        'paper-checkbox',
        'paper-fab',
        'paper-icon-button',
        # crbug.com/394756
        # 'paper-radio-button',
        'paper-tab',
        'paper-toggle-button',
        'x-shadow',
        ]
    for tappable_type in tappable_types:
      self.DoActionOnWidgetType(action_runner, tappable_type, self.TapWidget)
    swipeable_types = ['paper-slider']
    for swipeable_type in swipeable_types:
      self.DoActionOnWidgetType(action_runner, swipeable_type, self.SwipeWidget)

  def DoActionOnWidgetType(self, action_runner, widget_type, action_function):
    # Find all widgets of this type, but skip any that are disabled or are
    # currently active as they typically don't produce animation frames.
    element_list_query = js_template.Render(
        '{{ @iframe }}.querySelectorAll({{ selector }})',
        iframe=self.iframe_js,
        selector='body %s:not([disabled]):not([active])' % widget_type)

    roles_count_query = element_list_query + '.length'
    for i in range(action_runner.EvaluateJavaScript(roles_count_query)):
      element_query = js_template.Render(
        '{{ @query }}[{{ i }}]', query=element_list_query, i=i)
      if action_runner.EvaluateJavaScript(
          element_query + '.offsetParent != null'):
        # Only try to tap on visible elements (offsetParent != null)
        action_runner.ExecuteJavaScript(element_query + '.scrollIntoView()')
        action_runner.Wait(1) # wait for page to settle after scrolling
        action_function(action_runner, element_query)

  def TapWidget(self, action_runner, element_function):
    with action_runner.CreateInteraction('Tap_Widget', repeatable=True):
      action_runner.TapElement(element_function=element_function)
      action_runner.Wait(1) # wait for e.g. animations on the widget

  def SwipeWidget(self, action_runner, element_function):
    with action_runner.CreateInteraction('Swipe_Widget'):
      action_runner.SwipeElement(element_function=element_function,
                                 left_start_ratio=0.75,
                                 speed_in_pixels_per_second=300)


class PolymerPageSet(story.StorySet):

  def __init__(self, run_no_page_interactions=False):
    super(PolymerPageSet, self).__init__(
      archive_data_file='data/polymer.json',
      cloud_storage_bucket=story.PUBLIC_BUCKET)

    self.AddStory(PolymerCalculatorPage(self, run_no_page_interactions))
    self.AddStory(PolymerShadowPage(self, run_no_page_interactions))

    # Polymer Sampler subpages that are interesting to tap / swipe elements on
    TAPPABLE_PAGES = [
        'paper-button',
        'paper-checkbox',
        'paper-fab',
        'paper-icon-button',
        # crbug.com/394756
        # 'paper-radio-button',
        #FIXME(wiltzius) Disabling x-shadow until this issue is fixed:
        # https://github.com/Polymer/paper-shadow/issues/12
        #'paper-shadow',
        'paper-tabs',
        'paper-toggle-button',
        ]
    for p in TAPPABLE_PAGES:
      self.AddStory(PolymerSampler(
          self, p, run_no_page_interactions=run_no_page_interactions))

    # Polymer Sampler subpages that are interesting to scroll
    SCROLLABLE_PAGES = [
        'core-scroll-header-panel',
        ]
    for p in SCROLLABLE_PAGES:
      self.AddStory(PolymerSampler(
          self, p, run_no_page_interactions=run_no_page_interactions,
          scrolling_page=True))

    for page in self:
      assert (page.__class__.RunPageInteractions ==
              PolymerPage.RunPageInteractions), (
              'Pages in this page set must not override PolymerPage\' '
              'RunPageInteractions method.')
