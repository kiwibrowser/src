# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story


class KeySilkCasesPage(page_module.Page):

  def __init__(self, url, page_set, run_no_page_interactions):
    """ Base class for all key silk cases pages.

    Args:
      run_no_page_interactions: whether the page will run any interactions after
        navigate steps.
    """
    name = url
    if not name.startswith('http'):
      name = url.split('/')[-1]
    super(KeySilkCasesPage, self).__init__(
        url=url, page_set=page_set,
        shared_page_state_class=shared_page_state.SharedMobilePageState,
        name=name)
    self._run_no_page_interactions = run_no_page_interactions

  def RunNavigateSteps(self, action_runner):
    super(KeySilkCasesPage, self).RunNavigateSteps(action_runner)
    action_runner.Wait(2)

  def RunPageInteractions(self, action_runner):
    # If a key silk case page wants to customize it actions, it should
    # overrides the PerformPageInteractions method instead of this method.
    if self._run_no_page_interactions:
      return
    self.PerformPageInteractions(action_runner)

  def PerformPageInteractions(self, action_runner):
    """ Perform interactions on page after navigate steps.
    Override this to define custom actions to be run after navigate steps.
    """
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollPage()


class Page1(KeySilkCasesPage):

  """ Why: Infinite scroll. Brings out all of our perf issues. """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page1, self).__init__(
      url='http://groupcloned.com/test/plain/list-recycle-transform.html',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollElement(selector='#scrollable')


class Page2(KeySilkCasesPage):

  """ Why: Brings out layer management bottlenecks. """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page2, self).__init__(
      url='file://key_silk_cases/list_animation_simple.html',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('SimpleAnimation'):
      action_runner.Wait(2)


class Page3(KeySilkCasesPage):

  """
  Why: Best-known method for fake sticky. Janks sometimes. Interacts badly with
  compositor scrolls.
  """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page3, self).__init__(
      # pylint: disable=line-too-long
      url='http://groupcloned.com/test/plain/sticky-using-webkit-backface-visibility.html',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollElement(selector='#container')


class Page4(KeySilkCasesPage):

  """
  Why: Card expansion: only the card should repaint, but in reality lots of
  storms happen.
  """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page4, self).__init__(
      url='http://jsfiddle.net/3yDKh/15/show/',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('CardExpansionAnimation'):
      action_runner.Wait(3)


class Page5(KeySilkCasesPage):

  """
  Why: Card expansion with animated contents, using will-change on the card
  """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page5, self).__init__(
      url='http://jsfiddle.net/jx5De/14/show/',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

    self.gpu_raster = True

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('CardExpansionAnimation'):
      action_runner.Wait(4)


class Page6(KeySilkCasesPage):

  """
  Why: Card fly-in: It should be fast to animate in a bunch of cards using
  margin-top and letting layout do the rest.
  """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page6, self).__init__(
      url='http://jsfiddle.net/3yDKh/16/show/',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('CardFlyingAnimation'):
      action_runner.Wait(3)


class Page7(KeySilkCasesPage):

  """
  Why: Image search expands a spacer div when you click an image to accomplish
  a zoomin effect. Each image has a layer. Even so, this triggers a lot of
  unnecessary repainting.
  """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page7, self).__init__(
      url='http://jsfiddle.net/R8DX9/4/show/',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('ZoominAnimation'):
      action_runner.Wait(3)


class Page8(KeySilkCasesPage):

  """
  Why: Swipe to dismiss of an element that has a fixed-position child that is
  its pseudo-sticky header. Brings out issues with layer creation and
  repainting.
  """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page8, self).__init__(
      url='http://jsfiddle.net/rF9Gh/7/show/',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('SwipeToDismissAnimation'):
      action_runner.Wait(3)


class Page9(KeySilkCasesPage):

  """
  Why: Horizontal and vertical expansion of a card that is cheap to layout but
  costly to rasterize.
  """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page9, self).__init__(
      url='http://jsfiddle.net/TLXLu/3/show/',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

    self.gpu_raster = True

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('CardExpansionAnimation'):
      action_runner.Wait(4)


class Page10(KeySilkCasesPage):

  """
  Why: Vertical Expansion of a card that is cheap to layout but costly to
  rasterize.
  """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page10, self).__init__(
      url='http://jsfiddle.net/cKB9D/7/show/',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

    self.gpu_raster = True

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('CardExpansionAnimation'):
      action_runner.Wait(4)


class Page11(KeySilkCasesPage):

  """
  Why: Parallax effect is common on photo-viewer-like applications, overloading
  software rasterization
  """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page11, self).__init__(
      url='http://jsfiddle.net/vBQHH/11/show/',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

    self.gpu_raster = True

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('ParallaxAnimation'):
      action_runner.Wait(4)


class Page12(KeySilkCasesPage):

  """ Why: Addressing paint storms during coordinated animations. """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page12, self).__init__(
      url='http://jsfiddle.net/ugkd4/10/show/',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('CoordinatedAnimation'):
      action_runner.Wait(5)


class Page13(KeySilkCasesPage):

  """ Why: Mask transitions are common mobile use cases. """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page13, self).__init__(
      url='http://jsfiddle.net/xLuvC/1/show/',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

    self.gpu_raster = True

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('MaskTransitionAnimation'):
      action_runner.Wait(4)


class Page14(KeySilkCasesPage):

  """ Why: Card expansions with images and text are pretty and common. """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page14, self).__init__(
      url='http://jsfiddle.net/bNp2h/3/show/',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

    self.gpu_raster = True

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('CardExpansionAnimation'):
      action_runner.Wait(4)


class Page15(KeySilkCasesPage):

  """ Why: Coordinated animations for expanding elements. """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page15, self).__init__(
      url='file://key_silk_cases/font_wipe.html',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('CoordinatedAnimation'):
      action_runner.Wait(5)


class Page16(KeySilkCasesPage):

  def __init__(self, page_set, run_no_page_interactions):
    super(Page16, self).__init__(
      url='file://key_silk_cases/inbox_app.html?swipe_to_dismiss',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def SwipeToDismiss(self, action_runner):
    with action_runner.CreateGestureInteraction('SwipeAction'):
      action_runner.SwipeElement(
          left_start_ratio=0.8, top_start_ratio=0.2,
          direction='left', distance=400, speed_in_pixels_per_second=5000,
          element_function='document.getElementsByClassName("message")[2]')

  def PerformPageInteractions(self, action_runner):
    self.SwipeToDismiss(action_runner)


class Page17(KeySilkCasesPage):

  def __init__(self, page_set, run_no_page_interactions):
    super(Page17, self).__init__(
      url='file://key_silk_cases/inbox_app.html?stress_hidey_bars',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    self.StressHideyBars(action_runner)

  def StressHideyBars(self, action_runner):
    with action_runner.CreateGestureInteraction(
        'ScrollAction', repeatable=True):
      action_runner.WaitForElement(selector='#messages')
      action_runner.ScrollElement(
        selector='#messages', direction='down', speed_in_pixels_per_second=200)
    with action_runner.CreateGestureInteraction(
        'ScrollAction', repeatable=True):
      action_runner.WaitForElement(selector='#messages')
      action_runner.ScrollElement(
          selector='#messages', direction='up', speed_in_pixels_per_second=200)
    with action_runner.CreateGestureInteraction(
        'ScrollAction', repeatable=True):
      action_runner.WaitForElement(selector='#messages')
      action_runner.ScrollElement(
          selector='#messages', direction='down',
          speed_in_pixels_per_second=200)


class Page18(KeySilkCasesPage):

  def __init__(self, page_set, run_no_page_interactions):
    super(Page18, self).__init__(
      url='file://key_silk_cases/inbox_app.html?toggle_drawer',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    for _ in xrange(6):
      self.ToggleDrawer(action_runner)

  def ToggleDrawer(self, action_runner):
    with action_runner.CreateInteraction('Action_TapAction', repeatable=True):
      action_runner.TapElement('#menu-button')
      action_runner.Wait(1)


class Page19(KeySilkCasesPage):

  def __init__(self, page_set, run_no_page_interactions):
    super(Page19, self).__init__(
      url='file://key_silk_cases/inbox_app.html?slide_drawer',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def ToggleDrawer(self, action_runner):
    with action_runner.CreateGestureInteraction('TapAction'):
      action_runner.TapElement('#menu-button')

    with action_runner.CreateInteraction('Wait'):
      action_runner.WaitForJavaScriptCondition('''
          document.getElementById("nav-drawer").active &&
          document.getElementById("nav-drawer").children[0]
              .getBoundingClientRect().left == 0''')

  def RunNavigateSteps(self, action_runner):
    super(Page19, self).RunNavigateSteps(action_runner)
    action_runner.Wait(2)
    self.ToggleDrawer(action_runner)

  def PerformPageInteractions(self, action_runner):
    self.SlideDrawer(action_runner)

  def SlideDrawer(self, action_runner):
    with action_runner.CreateInteraction('Action_SwipeAction'):
      action_runner.SwipeElement(
          left_start_ratio=0.8, top_start_ratio=0.2,
          direction='left', distance=200,
          element_function='document.getElementById("nav-drawer").children[0]')
      action_runner.WaitForJavaScriptCondition(
          '!document.getElementById("nav-drawer").active')


class Page20(KeySilkCasesPage):

  """ Why: Shadow DOM infinite scrolling. """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page20, self).__init__(
      url='file://key_silk_cases/infinite_scrolling.html',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollElement(
          selector='#container', speed_in_pixels_per_second=5000)


class GwsExpansionPage(KeySilkCasesPage):
  """Abstract base class for pages that expand Google knowledge panels."""

  def NavigateWait(self, action_runner):
    super(GwsExpansionPage, self).RunNavigateSteps(action_runner)
    action_runner.Wait(3)

  def ExpandKnowledgeCard(self, action_runner):
    # expand card
    with action_runner.CreateInteraction('Action_TapAction'):
      action_runner.TapElement(
          element_function='document.getElementsByClassName("vk_arc")[0]')
      action_runner.Wait(2)

  def ScrollKnowledgeCardToTop(self, action_runner, card_id):
    # scroll until the knowledge card is at the top
    action_runner.ExecuteJavaScript(
        "document.getElementById({{ card_id }}).scrollIntoView()",
        card_id=card_id)

  def PerformPageInteractions(self, action_runner):
    self.ExpandKnowledgeCard(action_runner)


class GwsGoogleExpansion(GwsExpansionPage):

  """ Why: Animating height of a complex content card is common. """

  def __init__(self, page_set, run_no_page_interactions):
    super(GwsGoogleExpansion, self).__init__(
      url='http://www.google.com/#q=google',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def RunNavigateSteps(self, action_runner):
    self.NavigateWait(action_runner)
    self.ScrollKnowledgeCardToTop(action_runner, 'kno-result')


class GwsBoogieExpansion(GwsExpansionPage):

  """ Why: Same case as Google expansion but text-heavy rather than image. """

  def __init__(self, page_set, run_no_page_interactions):
    super(GwsBoogieExpansion, self).__init__(
      url='https://www.google.com/search?hl=en&q=define%3Aboogie',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def RunNavigateSteps(self, action_runner):
    self.NavigateWait(action_runner)
    self.ScrollKnowledgeCardToTop(action_runner, 'rso')


class Page22(KeySilkCasesPage):

  def __init__(self, page_set, run_no_page_interactions):
    super(Page22, self).__init__(
      url='http://plus.google.com/app/basic/stream',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def RunNavigateSteps(self, action_runner):
    super(Page22, self).RunNavigateSteps(action_runner)
    action_runner.WaitForJavaScriptCondition(
        'document.getElementsByClassName("fHa").length > 0')
    action_runner.Wait(2)

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollElement(selector='#mainContent')


class Page23(KeySilkCasesPage):

  """
  Why: Physical simulation demo that does a lot of element.style mutation
  triggering JS and recalc slowness
  """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page23, self).__init__(
      url='http://jsbin.com/UVIgUTa/38/quiet',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateGestureInteraction('ScrollAction',
                                                repeatable=True):
      action_runner.ScrollPage(
          distance_expr='window.innerHeight / 2',
          direction='down',
          use_touch=True)
    with action_runner.CreateGestureInteraction('ScrollAction',
                                                repeatable=True):
      action_runner.Wait(1)


class Page24(KeySilkCasesPage):

  """
  Why: Google News: this iOS version is slower than accelerated scrolling
  """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page24, self).__init__(
      url='http://mobile-news.sandbox.google.com/news/pt0?scroll',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def RunNavigateSteps(self, action_runner):
    super(Page24, self).RunNavigateSteps(action_runner)
    action_runner.WaitForJavaScriptCondition(
        'document.getElementById(":h") != null')
    action_runner.Wait(1)

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollElement(
          element_function='document.getElementById(":5")',
          distance=2500,
          use_touch=True)


class Page25(KeySilkCasesPage):

  def __init__(self, page_set, run_no_page_interactions):
    super(Page25, self).__init__(
      url='http://mobile-news.sandbox.google.com/news/pt0?swipe',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def RunNavigateSteps(self, action_runner):
    super(Page25, self).RunNavigateSteps(action_runner)
    action_runner.WaitForJavaScriptCondition(
        'document.getElementById(":h") != null')
    action_runner.Wait(1)

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateGestureInteraction('SwipeAction', repeatable=True):
      action_runner.SwipeElement(
          direction='left', distance=100,
          element_function='document.getElementById(":f")')
    with action_runner.CreateGestureInteraction('SwipeAction', repeatable=True):
      action_runner.Wait(1)


class Page26(KeySilkCasesPage):

  """ Why: famo.us twitter demo """

  def __init__(self, page_set, run_no_page_interactions):
    super(Page26, self).__init__(
      url='http://s.codepen.io/befamous/fullpage/pFsqb?scroll',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def RunNavigateSteps(self, action_runner):
    super(Page26, self).RunNavigateSteps(action_runner)
    action_runner.WaitForJavaScriptCondition(
        'document.getElementsByClassName("tweet").length > 0')
    action_runner.Wait(1)

  def PerformPageInteractions(self, action_runner):
    # Add a touch-action: none because this page prevent defaults all
    # touch moves.
    action_runner.ExecuteJavaScript('''
        var style = document.createElement("style");
        document.head.appendChild(style);
        style.sheet.insertRule("body { touch-action: none }", 0);
        ''')
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollPage(distance=5000)


class SVGIconRaster(KeySilkCasesPage):

  """ Why: Mutating SVG icons; these paint storm and paint slowly. """

  def __init__(self, page_set, run_no_page_interactions):
    super(SVGIconRaster, self).__init__(
      url='http://wiltzius.github.io/shape-shifter/',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def RunNavigateSteps(self, action_runner):
    super(SVGIconRaster, self).RunNavigateSteps(action_runner)
    action_runner.WaitForJavaScriptCondition(
        'loaded = true')
    action_runner.Wait(1)

  def PerformPageInteractions(self, action_runner):
    for i in xrange(9):
      button_func = ('document.getElementById("demo").$.'
                     'buttons.children[%d]') % i
      with action_runner.CreateInteraction('Action_TapAction', repeatable=True):
        action_runner.TapElement(element_function=button_func)
        action_runner.Wait(1)


class UpdateHistoryState(KeySilkCasesPage):

  """ Why: Modern apps often update history state, which currently is janky."""

  def __init__(self, page_set, run_no_page_interactions):
    super(UpdateHistoryState, self).__init__(
      url='file://key_silk_cases/pushState.html',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def RunNavigateSteps(self, action_runner):
    super(UpdateHistoryState, self).RunNavigateSteps(action_runner)
    action_runner.ExecuteJavaScript('''
        window.requestAnimationFrame(function() {
            window.__history_state_loaded = true;
          });
        ''')
    action_runner.WaitForJavaScriptCondition(
        'window.__history_state_loaded == true;')

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('animation_interaction'):
      action_runner.Wait(5) # JS runs the animation continuously on the page


class SilkFinance(KeySilkCasesPage):

  """ Why: Some effects repaint the page, possibly including plenty of text. """

  def __init__(self, page_set, run_no_page_interactions):
    super(SilkFinance, self).__init__(
      url='file://key_silk_cases/silk_finance.html',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('animation_interaction'):
      action_runner.Wait(10) # animation runs automatically


class PolymerTopeka(KeySilkCasesPage):

  """ Why: Sample Polymer app. """

  def __init__(self, page_set, run_no_page_interactions):
    super(PolymerTopeka, self).__init__(
      url='https://polymer-topeka.appspot.com/',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    profile = 'html /deep/ topeka-profile /deep/ '
    first_name = profile + 'paper-input#first /deep/ input'
    action_runner.WaitForElement(selector=first_name)
    # Input First Name:
    action_runner.ExecuteJavaScript('''
        var fn = document.querySelector({{ first_name }});
        fn.value = 'Chrome';
        fn.fire('input');''',
        first_name=first_name)
    # Input Last Initial:
    action_runner.ExecuteJavaScript('''
        var li = document.querySelector({{ selector }});
        li.value = 'E';
        li.fire('input');''',
        selector='%s paper-input#last /deep/ input' % profile)
    with action_runner.CreateInteraction('animation_interaction'):
      # Click the check-mark to login:
      action_runner.ExecuteJavaScript('''
          window.topeka_page_transitions = 0;
          [].forEach.call(document.querySelectorAll(
              'html /deep/ core-animated-pages'), function(p){
                  p.addEventListener(
                      'core-animated-pages-transition-end', function(e) {
                          window.topeka_page_transitions++;
                      });
              });
          document.querySelector({{ selector }}).fire('tap')''',
          selector='%s paper-fab' % profile)
      # Wait for category list to animate in:
      action_runner.WaitForJavaScriptCondition('''
          window.topeka_page_transitions === 1''')
      # Click a category to start a quiz:
      action_runner.ExecuteJavaScript('''
          document.querySelector('\
              html /deep/ core-selector.category-list').fire(
              'tap',1,document.querySelector('html /deep/ \
                      div.category-item.red-theme'));''')
      # Wait for the category splash to animate in:
      action_runner.WaitForJavaScriptCondition('''
          window.topeka_page_transitions === 2''')
      # Click to start the quiz:
      action_runner.ExecuteJavaScript('''
          document.querySelector('html /deep/ topeka-category-front-page /deep/\
              paper-fab').fire('tap');''')
      action_runner.WaitForJavaScriptCondition('''
          window.topeka_page_transitions === 4''')
      # Input a mostly correct answer:
      action_runner.ExecuteJavaScript('''
          document.querySelector('html /deep/ topeka-quiz-fill-blank /deep/\
              input').value = 'arkinsaw';
          document.querySelector('html /deep/ topeka-quiz-fill-blank /deep/\
              input').fire('input');
          document.querySelector('html /deep/ topeka-quizzes /deep/ \
              paper-fab').fire('tap');''')
      action_runner.WaitForJavaScriptCondition('''
          window.topeka_page_transitions === 6''')

class Masonry(KeySilkCasesPage):

  """ Why: Popular layout hack. """

  def __init__(self, page_set, run_no_page_interactions):
    super(Masonry, self).__init__(
      url='file://key_silk_cases/masonry.html',
      page_set=page_set, run_no_page_interactions=run_no_page_interactions)

  def PerformPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('animation_interaction'):
      action_runner.ExecuteJavaScript('window.brick()')
      action_runner.WaitForJavaScriptCondition('window.done')


class KeySilkCasesPageSet(story.StorySet):

  """ Pages hand-picked for project Silk. """

  def __init__(self, run_no_page_interactions=False):
    super(KeySilkCasesPageSet, self).__init__(
      archive_data_file='data/key_silk_cases.json',
      cloud_storage_bucket=story.PARTNER_BUCKET)

    self.AddStory(Page1(self, run_no_page_interactions))
    self.AddStory(Page2(self, run_no_page_interactions))
    self.AddStory(Page3(self, run_no_page_interactions))
    self.AddStory(Page4(self, run_no_page_interactions))
    self.AddStory(Page5(self, run_no_page_interactions))
    self.AddStory(Page6(self, run_no_page_interactions))
    self.AddStory(Page7(self, run_no_page_interactions))
    self.AddStory(Page8(self, run_no_page_interactions))
    self.AddStory(Page9(self, run_no_page_interactions))
    self.AddStory(Page10(self, run_no_page_interactions))
    self.AddStory(Page11(self, run_no_page_interactions))
    self.AddStory(Page12(self, run_no_page_interactions))
    self.AddStory(Page13(self, run_no_page_interactions))
    self.AddStory(Page14(self, run_no_page_interactions))
    self.AddStory(Page15(self, run_no_page_interactions))
    self.AddStory(Page16(self, run_no_page_interactions))
    self.AddStory(Page17(self, run_no_page_interactions))
    self.AddStory(Page18(self, run_no_page_interactions))
    # Missing frames during tap interaction; crbug.com/446332
    self.AddStory(Page19(self, run_no_page_interactions))
    self.AddStory(Page20(self, run_no_page_interactions))
    self.AddStory(GwsGoogleExpansion(self, run_no_page_interactions))
    self.AddStory(GwsBoogieExpansion(self, run_no_page_interactions))
    # Times out on Windows; crbug.com/338838
    self.AddStory(Page22(self, run_no_page_interactions))
    self.AddStory(Page23(self, run_no_page_interactions))
    self.AddStory(Page24(self, run_no_page_interactions))
    self.AddStory(Page25(self, run_no_page_interactions))
    self.AddStory(Page26(self, run_no_page_interactions))
    self.AddStory(SVGIconRaster(self, run_no_page_interactions))
    self.AddStory(UpdateHistoryState(self, run_no_page_interactions))
    self.AddStory(SilkFinance(self, run_no_page_interactions))
    # Flaky interaction steps on Android; crbug.com/507865
    self.AddStory(PolymerTopeka(self, run_no_page_interactions))
    self.AddStory(Masonry(self, run_no_page_interactions))

    for page in self:
      assert (page.__class__.RunPageInteractions ==
              KeySilkCasesPage.RunPageInteractions), (
              'Pages in this page set must not override KeySilkCasesPage\' '
              'RunPageInteractions method.')
