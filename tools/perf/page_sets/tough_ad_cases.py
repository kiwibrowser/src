# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from page_sets import repeatable_synthesize_scroll_gesture_shared_state

from telemetry.core import util
from telemetry.page import page as page_module
from telemetry import story


class SwiffyPage(page_module.Page):

  def __init__(self, url, page_set):
    super(SwiffyPage, self).__init__(url=url, page_set=page_set,
                                     make_javascript_deterministic=False,
                                     name=url)

  def RunNavigateSteps(self, action_runner):
    super(SwiffyPage, self).RunNavigateSteps(action_runner)
    # Make sure the ad has finished loading.
    util.WaitFor(action_runner.tab.HasReachedQuiescence, 60)
    # Swiffy overwrites toString() to return a constant string, so "undo" that
    # here so that we don't think it has stomped over console.time.
    action_runner.EvaluateJavaScript(
        'Function.prototype.toString = function() { return "[native code]"; }')
    # Make sure we have a reasonable viewport for mobile.
    action_runner.EvaluateJavaScript("""
        var meta = document.createElement("meta");
        meta.name = "viewport";
        meta.content = "width=device-width";
        document.getElementsByTagName("head")[0].appendChild(meta);
        """)

  def RunPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('ToughAd'):
      action_runner.Wait(10)


class AdPage(page_module.Page):

  def __init__(self, url, page_set, make_javascript_deterministic=True,
               y_scroll_distance_multiplier=0.5,
               scroll=False,
               wait_for_interactive_or_better=False):
    name = url
    if not name.startswith('http'):
      name = url.split('/')[-1]
    super(AdPage, self).__init__(
        url=url,
        page_set=page_set,
        make_javascript_deterministic=make_javascript_deterministic,
        shared_page_state_class=(
            repeatable_synthesize_scroll_gesture_shared_state.\
                RepeatableSynthesizeScrollGestureSharedState),
        name=name)
    self._y_scroll_distance_multiplier = y_scroll_distance_multiplier
    self._scroll = scroll
    self._wait_for_interactive_or_better = wait_for_interactive_or_better

  def RunNavigateSteps(self, action_runner):
    # Rewrite file urls to point to the replay server instead.
    if self.is_file:
      url = self.file_path_url_with_scheme
      url = action_runner.tab.browser.platform.http_server.UrlOf(
          url[len('file://'):])
    else:
      url = self._url
    action_runner.tab.Navigate(url)

    # Wait for the page to be scrollable. Simultaneously (to reduce latency due
    # to main thread round trips),  insert a no-op touch handler on the body.
    # Most ads have touch handlers and we want to simulate the worst case of the
    # user trying to scroll the page by grabbing an ad.
    if self._wait_for_interactive_or_better:
        action_runner.WaitForJavaScriptCondition(
            '(document.readyState == "interactive" || '
            'document.readyState == "complete") &&'
            'document.body != null && '
            'document.body.scrollHeight > window.innerHeight && '
            '!document.body.addEventListener("touchstart", function() {})')
    else:
        action_runner.WaitForJavaScriptCondition(
            'document.body != null && '
            'document.body.scrollHeight > window.innerHeight && '
            '!document.body.addEventListener("touchstart", function() {})')

  def RunPageInteractions(self, action_runner):
    if not self._scroll:
      with action_runner.CreateInteraction('ToughAd'):
        action_runner.Wait(30)
    else:
      action_runner.RepeatableBrowserDrivenScroll(
          y_scroll_distance_ratio=self._y_scroll_distance_multiplier,
          repeat_count=9)


class ForbesAdPage(AdPage):

  def __init__(self, url, page_set, scroll=False):
    # forbes.com uses a strange dynamic transform on the body element,
    # which occasionally causes us to try scrolling from outside the
    # screen. Start at the very top of the viewport to avoid this.
    super(ForbesAdPage, self).__init__(
        url=url, page_set=page_set, make_javascript_deterministic=False,
        scroll=scroll,
        wait_for_interactive_or_better=True)

  def RunNavigateSteps(self, action_runner):
    super(ForbesAdPage, self).RunNavigateSteps(action_runner)
    # Wait until the interstitial banner goes away.
    action_runner.WaitForJavaScriptCondition(
        'window.location.pathname.indexOf("welcome") == -1')


class SyntheticToughAdCasesPageSet(story.StorySet):
  """Pages for measuring rendering performance with advertising content."""

  def __init__(self):
    super(SyntheticToughAdCasesPageSet, self).__init__(
        archive_data_file='data/tough_ad_cases.json',
        cloud_storage_bucket=story.INTERNAL_BUCKET)

    base_url = 'http://localhost:8000'

    # See go/swiffy-chrome-samples for how to add new pages here or how to
    # update the existing ones.
    swiffy_pages = [
        'CICAgICQ15a9NxDIARjIASgBMghBC1XuTk8ezw.swiffy72.html',
        'shapes-CK7ptO3F8bi2KxDQAhiYAigBMgij6QBQtD2gyA.swiffy72.html',
        'CNP2xe_LmqPEKBCsAhj6ASgBMggnyMqth81h8Q.swiffy72.html',
        'clip-paths-CICAgMDO7Ye9-gEQ2AUYWigBMgjZxDii6aoK9w.swiffy72.html',
        'filters-CNLa0t2T47qJ_wEQoAEY2AQoATIIFaIdc7VMBr4.swiffy72.html',
        'shapes-CICAgMDO7cfIzwEQ1AMYPCgBMghqY8tqyRCArQ.swiffy72.html',
        'CICAgIDQ2Pb-MxCsAhj6ASgBMgi5DLoSO0gPbQ.swiffy72.html',
        'CICAgKCN39CopQEQoAEY2AQoATIID59gK5hjjIg.swiffy72.html',
        'CICAgKCNj4HgyAEQeBjYBCgBMgjQpPkOjyWNdw.1.swiffy72.html',
        'clip-paths-CILZhLqO_-27bxB4GNgEKAEyCC46kMLBXnMT.swiffy72.html',
        'CICAgMDOrcnRGRB4GNgEKAEyCP_ZBSfwUFsj.swiffy72.html',
    ]
    for page_name in swiffy_pages:
      url = base_url + '/' + page_name
      self.AddStory(SwiffyPage(url, self))


class SyntheticToughWebglAdCasesPageSet(story.StorySet):
  """Pages for measuring rendering performance with WebGL ad content."""

  def __init__(self):
    super(SyntheticToughWebglAdCasesPageSet, self).__init__(
        archive_data_file='data/tough_ad_cases.json',
        cloud_storage_bucket=story.INTERNAL_BUCKET)

    base_url = 'http://localhost:8000'

    # See go/swiffy-chrome-samples for how to add new pages here or how to
    # update the existing ones.
    swiffy_pages = [
        'CICAgICQ15a9NxDIARjIASgBMghBC1XuTk8ezw.swf.webglbeta.html',
        'shapes-CK7ptO3F8bi2KxDQAhiYAigBMgij6QBQtD2gyA.swf.webglbeta.html',
        'CNP2xe_LmqPEKBCsAhj6ASgBMggnyMqth81h8Q.swf.webglbeta.html',
        'clip-paths-CICAgMDO7Ye9-gEQ2AUYWigBMgjZxDii6aoK9w.swf.webglbeta.html',
        'filters-CNLa0t2T47qJ_wEQoAEY2AQoATIIFaIdc7VMBr4.swf.webglbeta.html',
        'shapes-CICAgMDO7cfIzwEQ1AMYPCgBMghqY8tqyRCArQ.swf.webglbeta.html',
        'CICAgIDQ2Pb-MxCsAhj6ASgBMgi5DLoSO0gPbQ.swf.webglbeta.html',
        'CICAgKCN39CopQEQoAEY2AQoATIID59gK5hjjIg.swf.webglbeta.html',
        'CICAgKCNj4HgyAEQeBjYBCgBMgjQpPkOjyWNdw.1.swf.webglbeta.html',
        'clip-paths-CILZhLqO_-27bxB4GNgEKAEyCC46kMLBXnMT.swf.webglbeta.html',
        'CICAgMDOrcnRGRB4GNgEKAEyCP_ZBSfwUFsj.swf.webglbeta.html',
    ]
    for page_name in swiffy_pages:
      url = base_url + '/' + page_name
      self.AddStory(SwiffyPage(url, self))


class ToughAdCasesPageSet(story.StorySet):
  """Pages for measuring performance with advertising content."""

  def __init__(self, scroll=False):
    super(ToughAdCasesPageSet, self).__init__(
        archive_data_file='data/tough_ad_cases.json',
        cloud_storage_bucket=story.INTERNAL_BUCKET)

    self.AddStory(AdPage('file://tough_ad_cases/'
        'swiffy_collection.html', self, make_javascript_deterministic=False,
        y_scroll_distance_multiplier=0.25, scroll=scroll))
    self.AddStory(AdPage('file://tough_ad_cases/'
        'swiffy_webgl_collection.html',
        self, make_javascript_deterministic=False, scroll=scroll))
    self.AddStory(AdPage('http://www.latimes.com', self, scroll=scroll,
        wait_for_interactive_or_better=True))
    self.AddStory(ForbesAdPage('http://www.forbes.com/sites/parmyolson/'
        '2015/07/29/jana-mobile-data-facebook-internet-org/',
        self, scroll=scroll))
    self.AddStory(AdPage('http://androidcentral.com', self, scroll=scroll,
        wait_for_interactive_or_better=True))
    self.AddStory(AdPage('http://mashable.com', self, scroll=scroll,
        y_scroll_distance_multiplier=0.25))
    self.AddStory(AdPage('http://www.androidauthority.com/'
        'reduce-data-use-turn-on-data-compression-in-chrome-630064/', self,
        scroll=scroll))
    self.AddStory(AdPage(('http://www.cnn.com/2015/01/09/politics/'
                          'nebraska-keystone-pipeline/index.html'),
                         self, scroll=scroll))
    self.AddStory(AdPage('http://time.com/3977891/'
        'donald-trump-debate-republican/', self, scroll=scroll))
    self.AddStory(AdPage('http://www.theguardian.com/uk', self, scroll=scroll))
    self.AddStory(AdPage('http://m.tmz.com', self, scroll=scroll,
        y_scroll_distance_multiplier=0.25))
    self.AddStory(AdPage('http://androidpolice.com', self, scroll=scroll,
        wait_for_interactive_or_better=True))
