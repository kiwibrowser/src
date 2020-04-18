# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry import story


class ToughAnimationCasesPage(page_module.Page):

  def __init__(self, url, page_set, need_measurement_ready):
    name = url.split('/')[-1]
    if not name:
      name = url.split('/')[-2]
    super(ToughAnimationCasesPage, self).__init__(
        url=url, page_set=page_set, name=name)
    self._need_measurement_ready = need_measurement_ready

  def RunNavigateSteps(self, action_runner):
    super(ToughAnimationCasesPage, self).RunNavigateSteps(action_runner)
    if self._need_measurement_ready:
      action_runner.WaitForJavaScriptCondition('window.measurementReady')

  def RunPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('ToughAnimation'):
      action_runner.Wait(10)

class ToughAnimationCasesPageSet(story.StorySet):

  """
  Description: A collection of animation performance tests
  """

  def __init__(self):
    super(ToughAnimationCasesPageSet, self).__init__(
      archive_data_file='data/tough_animation_cases.json',
      cloud_storage_bucket=story.PARTNER_BUCKET)

    urls_list_one = [
      # Why: Tests the balls animation implemented with SVG animations.
      'file://tough_animation_cases/balls_svg_animations.html',
      # Why: Tests the balls animation implemented with Javascript and canvas.
      'file://tough_animation_cases/balls_javascript_canvas.html',
      # Why: Tests the balls animation implemented with Javascript and CSS.
      'file://tough_animation_cases/balls_javascript_css.html',
      # Why: Tests the balls animation implemented with CSS keyframe animations.
      'file://tough_animation_cases/balls_css_keyframe_animations.html',
      # Why: Tests the balls animation implemented with transforms and CSS
      # keyframe animations to be run on the compositor thread.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/balls_css_keyframe_animations_composited_transform.html',
      # Why: Tests the balls animation implemented with CSS transitions on 2
      # properties.
      'file://tough_animation_cases/balls_css_transition_2_properties.html',
      # Why: Tests the balls animation implemented with CSS transitions on 40
      # properties.
      'file://tough_animation_cases/balls_css_transition_40_properties.html',
      # Why: Tests the balls animation implemented with CSS transitions on all
      # animatable properties.
      'file://tough_animation_cases/balls_css_transition_all_properties.html',
      # pylint: disable=line-too-long
      'file://tough_animation_cases/overlay_background_color_css_transitions.html',

      # Why: Tests many CSS Transitions all starting at the same time triggered
      # by inserting new elements.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_transitions_simultaneous_by_inserting_new_element.html?N=0316',
      # Why: Tests many CSS Transitions all starting at the same time triggered
      # by inserting style sheets.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_transitions_simultaneous_by_inserting_style_element.html?N=0316',
      # Why: Tests many CSS Transitions all starting at the same time triggered
      # by updating class.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_transitions_simultaneous_by_updating_class.html?N=0316',
      # Why: Tests many CSS Transitions all starting at the same time triggered
      # by updating inline style.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_transitions_simultaneous_by_updating_inline_style.html?N=0316',
      # Why: Tests many CSS Transitions chained together using events at
      # different times triggered by inserting new elements.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_transitions_staggered_chaining_by_inserting_new_element.html?N=0316',
      # Why: Tests many CSS Transitions chained together using events at
      # different times triggered by inserting style sheets.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_transitions_staggered_chaining_by_inserting_style_element.html?N=0316',
      # Why: Tests many CSS Transitions chained together using events at
      # different times triggered by updating class.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_transitions_staggered_chaining_by_updating_class.html?N=0316',
      # Why: Tests many CSS Transitions chained together using events at
      # different times triggered by updating inline style.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_transitions_staggered_chaining_by_updating_inline_style.html?N=0316',
      # Why: Tests many CSS Transitions starting at different times triggered by
      # inserting new elements.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_transitions_staggered_triggering_by_inserting_new_element.html?N=0316',
      # Why: Tests many CSS Transitions starting at different times triggered by
      # inserting style sheets.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_transitions_staggered_triggering_by_inserting_style_element.html?N=0316',
      # Why: Tests many CSS Transitions starting at different times triggered by
      # updating class.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_transitions_staggered_triggering_by_updating_class.html?N=0316',
      # Why: Tests many CSS Transitions starting at different times triggered by
      # updating inline style.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_transitions_staggered_triggering_by_updating_inline_style.html?N=0316',

      # Why: Tests many CSS Animations all starting at the same time with 500
      # keyframes each.
      'file://tough_animation_cases/css_animations_many_keyframes.html?N=0316',
      # Why: Tests many CSS Animations all starting at the same time triggered
      # by inserting new elements.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_animations_simultaneous_by_inserting_new_element.html?N=0316',
      # Why: Tests many CSS Animations all starting at the same time triggered
      # by inserting style sheets.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_animations_simultaneous_by_inserting_style_element.html?N=0316',
      # Why: Tests many CSS Animations all starting at the same time triggered
      # by updating class.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_animations_simultaneous_by_updating_class.html?N=0316',
      # Why: Tests many CSS Animations all starting at the same time triggered
      # by updating inline style.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_animations_simultaneous_by_updating_inline_style.html?N=0316',
      # Why: Tests many CSS Animations chained together using events at
      # different times triggered by inserting new elements.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_animations_staggered_chaining_by_inserting_new_element.html?N=0316',
      # Why: Tests many CSS Animations chained together using events at
      # different times triggered by inserting style sheets.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_animations_staggered_chaining_by_inserting_style_element.html?N=0316',
      # Why: Tests many CSS Animations chained together using events at
      # different times triggered by updating class.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_animations_staggered_chaining_by_updating_class.html?N=0316',
      # Why: Tests many CSS Animations chained together using events at
      # different times triggered by updating inline style.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_animations_staggered_chaining_by_updating_inline_style.html?N=0316',
      # Why: Tests many CSS Animations starting at different times triggered by
      # inserting new elements.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_animations_staggered_triggering_by_inserting_new_element.html?N=0316',
      # Why: Tests many CSS Animations all starting at the same time with
      # staggered animation offsets.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_animations_staggered_infinite_iterations.html?N=0316',
      # Why: Tests many CSS Animations starting at different times triggered by
      # inserting style sheets.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_animations_staggered_triggering_by_inserting_style_element.html?N=0316',
      # Why: Tests many CSS Animations starting at different times triggered by
      # updating class.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_animations_staggered_triggering_by_updating_class.html?N=0316',
      # Why: Tests many CSS Animations starting at different times triggered by
      # updating inline style.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_animations_staggered_triggering_by_updating_inline_style.html?N=0316',

      # Why: Tests many Web Animations all starting at the same time with 500
      # keyframes each.
      'file://tough_animation_cases/web_animations_many_keyframes.html?N=0316',
      # Why: Tests many paused Web Animations having their currentTimes updated
      # in every requestAnimationFrame.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/web_animations_set_current_time_in_raf.html?N=0316',
      # Why: Tests many Web Animations all starting at the same time.
      'file://tough_animation_cases/web_animations_simultaneous.html?N=0316',
      # Why: Tests many Web Animations all starting at different times then
      # chained together using events.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/web_animations_staggered_chaining.html?N=0316',
      # Why: Tests many Web Animations all starting at different times with
      # infinite iterations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/web_animations_staggered_infinite_iterations.html?N=0316',
      # Why: Tests many Web Animations all starting at different times.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/web_animations_staggered_triggering.html?N=0316',

      # Why: Tests color animations using CSS Animations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_value_type_color.html?api=css_animations&N=0316',
      # Why: Tests filter animations using CSS Animations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_value_type_filter.html?api=css_animations&N=0316',
      # Why: Tests length 3D animations using CSS Animations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_value_type_length_3d.html?api=css_animations&N=0316',
      # Why: Tests complex length animations using CSS Animations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_value_type_length_complex.html?api=css_animations&N=0316',
      # Why: Tests simple length animations using CSS Animations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_value_type_length_simple.html?api=css_animations&N=0316',
      # Why: Tests path animations using CSS Animations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_value_type_path.html?api=css_animations&N=0316',
      # Why: Tests shadow animations using CSS Animations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_value_type_shadow.html?api=css_animations&N=0316',
      # Why: Tests complex transform animations using CSS Animations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_value_type_transform_complex.html?api=css_animations&N=0316',
      # Why: Tests simple transform animations using CSS Animations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_value_type_transform_simple.html?api=css_animations&N=0316',

      # Why: Tests color animations using Web Animations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_value_type_color.html?api=web_animations&N=0316',
      # Why: Tests length 3D animations using Web Animations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_value_type_length_3d.html?api=web_animations&N=0316',
      # Why: Tests complex length animations using Web Animations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_value_type_length_complex.html?api=web_animations&N=0316',
      # Why: Tests simple length animations using Web Animations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_value_type_length_simple.html?api=web_animations&N=0316',
      # Why: Tests path animations using Web Animations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_value_type_path.html?api=web_animations&N=0316',
      # Why: Tests shadow animations using Web Animations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_value_type_shadow.html?api=web_animations&N=0316',
      # Why: Tests complex transform animations using Web Animations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_value_type_transform_complex.html?api=web_animations&N=0316',
      # Why: Tests simple transform animations using Web Animations.
      # pylint: disable=line-too-long
      'file://tough_animation_cases/css_value_type_transform_simple.html?api=web_animations&N=0316',

      # Why: Test to update and then draw many times a large set of textures
      # to compare one-copy and zero-copy.
      'file://tough_animation_cases/compositor_heavy_animation.html?N=0200',
    ]

    for url in urls_list_one:
      self.AddStory(ToughAnimationCasesPage(url, self,
                                           need_measurement_ready=True))

    urls_list_two = [
      # Why: Tests various keyframed animations.
      'file://tough_animation_cases/keyframed_animations.html',
      # Why: Tests various transitions.
      'file://tough_animation_cases/transform_transitions.html',
      # Why: JS execution blocks CSS transition unless initial transform is set.
      'file://tough_animation_cases/transform_transition_js_block.html',
      # Why: Tests animating elements having mix-blend-mode: difference (a
      # separable popular blend mode).
      'file://tough_animation_cases/mix_blend_mode_animation_difference.html',
      # Why: Tests animating elements having mix-blend-mode: hue (a
      # non-separable blend mode).
      'file://tough_animation_cases/mix_blend_mode_animation_hue.html',
      # Why: Tests animating elements having mix-blend-mode: screen.
      'file://tough_animation_cases/mix_blend_mode_animation_screen.html',
      # Why: Tests software-animating a deep DOM subtree having one blending
      # leaf.
      'file://tough_animation_cases/mix_blend_mode_propagating_isolation.html',

      # Why: Login page is slow because of ineffecient transform operations.
      'http://ie.microsoft.com/testdrive/performance/robohornetpro/',
    ]

    for url in urls_list_two:
      self.AddStory(ToughAnimationCasesPage(url, self,
                                            need_measurement_ready=False))
