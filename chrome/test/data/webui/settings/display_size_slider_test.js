// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for display-size-slider. */
suite('DisplaySizeSlider', function() {
  /** @type {!CrSliderElement} */
  let slider;

  /** @type {!SliderTicks} */
  let ticks = [];

  setup(function() {
    PolymerTest.clearBody();
    const tickValues = [2, 4, 8, 16, 32, 64, 128];
    ticks = [];
    for (let i = 0; i < tickValues.length; i++) {
      ticks.push({value: tickValues[i], label: tickValues[i].toString()});
    }

    slider = document.createElement('display-size-slider');
    slider.ticks = ticks;
    slider.pref = {
      type: chrome.settingsPrivate.PrefType.NUMBER,
      value: ticks[3].value,
    };
    document.body.appendChild(slider);
  });

  test('initialization', function() {
    const selectedIndex = 3;

    assertFalse(slider.disabled);
    assertFalse(slider.dragging);
    assertFalse(slider.expand);
    expectEquals(selectedIndex, slider.index);
    expectEquals(ticks.length - 1, slider.markers.length);
    expectEquals(ticks[selectedIndex].value, slider.pref.value);
  });

  test('check knob position changes', function() {
    let expectedIndex = 3;
    expectEquals(expectedIndex, slider.index);
    expectEquals(ticks[expectedIndex].value, slider.pref.value);

    let expectedLeftPercentage = (expectedIndex * 100) / (ticks.length - 1);
    expectEquals(expectedLeftPercentage + '%', slider.$.sliderKnob.style.left);

    // Reset the index to the first tick.
    slider.resetToMinIndex_();
    expectedIndex = 0;
    expectEquals(expectedIndex, slider.index);
    expectEquals('0%', slider.$.sliderKnob.style.left);
    expectEquals(ticks[expectedIndex].value, slider.pref.value);

    // Decrementing the slider below the lowest index should have no effect.
    slider.decrement_();
    expectEquals(expectedIndex, slider.index);
    expectEquals('0%', slider.$.sliderKnob.style.left);
    expectEquals(ticks[expectedIndex].value, slider.pref.value);

    // Reset the index to the last tick.
    slider.resetToMaxIndex_();
    expectedIndex = ticks.length - 1;
    expectEquals(expectedIndex, slider.index);
    expectEquals('100%', slider.$.sliderKnob.style.left);
    expectEquals(ticks[expectedIndex].value, slider.pref.value);

    // Incrementing the slider when it is already at max should have no effect.
    slider.increment_();
    expectEquals(expectedIndex, slider.index);
    expectEquals('100%', slider.$.sliderKnob.style.left);
    expectEquals(ticks[expectedIndex].value, slider.pref.value);

    slider.decrement_();
    expectedIndex -= 1;
    expectEquals(expectedIndex, slider.index);
    expectedLeftPercentage = (expectedIndex * 100) / (ticks.length - 1);
    expectEquals(
        Math.round(expectedLeftPercentage),
        Math.round(parseFloat(slider.$.sliderKnob.style.left)));
    expectEquals(ticks[expectedIndex].value, slider.pref.value);

  });

  test('check keyboard events', function() {
    let expectedIndex = 3;
    expectEquals(expectedIndex, slider.index);

    // Right keypress should increment the slider by 1.
    MockInteractions.pressAndReleaseKeyOn(slider, 39 /* right */);
    expectedIndex += 1;
    expectEquals(expectedIndex, slider.index);

    // Left keypress should decrement the slider by 1.
    MockInteractions.pressAndReleaseKeyOn(slider, 37 /* left */);
    expectedIndex -= 1;
    expectEquals(expectedIndex, slider.index);

    // Page down should take the slider to the first value.
    MockInteractions.pressAndReleaseKeyOn(slider, 34 /* page down */);
    expectedIndex = 0;
    expectEquals(expectedIndex, slider.index);

    // Page up should take the slider to the last value.
    MockInteractions.pressAndReleaseKeyOn(slider, 33 /* page up */);
    expectedIndex = ticks.length - 1;
    expectEquals(expectedIndex, slider.index);

    // Down keypress should decrement the slider index by 1.
    MockInteractions.pressAndReleaseKeyOn(slider, 40 /* down */);
    expectedIndex -= 1;
    expectEquals(expectedIndex, slider.index);

    // Up keypress should increment the slider index by 1.
    MockInteractions.pressAndReleaseKeyOn(slider, 38 /* up */);
    expectedIndex += 1;
    expectEquals(expectedIndex, slider.index);

    // Home button should take the slider to the first value.
    MockInteractions.pressAndReleaseKeyOn(slider, 36 /* home */);
    expectedIndex = 0;
    expectEquals(expectedIndex, slider.index);

    // End button should take the slider to the last value.
    MockInteractions.pressAndReleaseKeyOn(slider, 35 /* up */);
    expectedIndex = ticks.length - 1;
    expectEquals(expectedIndex, slider.index);
  });

  test('set pref updates slider', function() {
    let expectedIndex = 3;
    expectEquals(expectedIndex, slider.index);
    expectEquals(ticks[expectedIndex].value, slider.pref.value);

    let newIndex = 4;
    slider.set('pref.value', ticks[newIndex].value);
    expectEquals(newIndex, slider.index);

    let expectedLeftPercentage = (newIndex * 100) / (ticks.length - 1);
    expectEquals(
        Math.round(expectedLeftPercentage),
        Math.round(parseFloat(slider.$.sliderKnob.style.left)));
  });

  test('check label values', function() {
    expectEquals('none', getComputedStyle(slider.$.labelContainer).display);
    let hoverClassName = 'hover';
    slider.$.sliderContainer.classList.add(hoverClassName);
    expectEquals('block', getComputedStyle(slider.$.labelContainer).display);

    expectEquals(ticks[slider.index].label, slider.$.labelText.innerText);

    slider.increment_();
    expectEquals(ticks[slider.index].label, slider.$.labelText.innerText);

    slider.resetToMaxIndex_();
    expectEquals(ticks[slider.index].label, slider.$.labelText.innerText);

    slider.resetToMinIndex_();
    expectEquals(ticks[slider.index].label, slider.$.labelText.innerText);
  });

  test('check knob expansion', function() {
    assertFalse(slider.expand);
    let oldIndex = slider.index;

    MockInteractions.down(slider.$.sliderKnob);
    assertTrue(slider.expand);
    expectEquals(oldIndex, slider.index);
    expectEquals(ticks[oldIndex].value, slider.pref.value);


    MockInteractions.up(slider.$.sliderKnob);
    assertFalse(slider.expand);
    expectEquals(oldIndex, slider.index);
    expectEquals(ticks[oldIndex].value, slider.pref.value);
  });

  test('mouse interactions with the slider knobs', function() {
    let dragEventReceived = false;
    let valueInEvent = null;
    slider.addEventListener('immediate-value-changed', function(e) {
      dragEventReceived = true;
      valueInEvent = e.detail.value;
    });

    let oldIndex = slider.index;
    const sliderKnob = slider.$.sliderKnob;
    // Width of each tick.
    const tickWidth = slider.$.sliderBar.offsetWidth / (ticks.length - 1);
    assertFalse(dragEventReceived);
    MockInteractions.down(sliderKnob);
    assertFalse(dragEventReceived);

    let currentPos = MockInteractions.middleOfNode(sliderKnob);
    let nextPos = {x: currentPos.x + tickWidth, y: currentPos.y};
    MockInteractions.move(sliderKnob, currentPos, nextPos);

    // Mouse is still down. So the slider should still be expanded.
    assertTrue(slider.expand);

    // The label should be visible.
    expectEquals('block', getComputedStyle(slider.$.labelContainer).display);

    // We moved by 1 tick width, so the slider index must have increased.
    expectEquals(oldIndex + 1, slider.index);

    // The mouse is still down, so the pref should not be updated.
    expectEquals(ticks[oldIndex].value, slider.pref.value);

    // The event should be fired while dragging the knob.
    assertTrue(dragEventReceived);
    dragEventReceived = false;

    // The value sent by the event must be the current value of the slider.
    expectEquals(ticks[slider.index].value, valueInEvent);

    MockInteractions.up(sliderKnob);

    // There should be no event fired when the knob is not being dragged.
    assertFalse(dragEventReceived);

    // Now that the mouse is down, the pref value should be updated.
    expectEquals(ticks[oldIndex + 1].value, slider.pref.value);

    oldIndex = slider.index;
    MockInteractions.track(sliderKnob, -3 * tickWidth, 0);
    expectEquals(oldIndex - 3, slider.index);
    expectEquals(ticks[oldIndex - 3].value, slider.pref.value);

    // Moving by a very large amount should clamp the value.
    oldIndex = slider.index;
    MockInteractions.track(sliderKnob, 100 * tickWidth, 0);
    expectEquals(ticks.length - 1, slider.index);
    expectEquals(ticks[ticks.length - 1].value, slider.pref.value);

    // Similarly for the other side.
    oldIndex = slider.index;
    MockInteractions.track(sliderKnob, -100 * tickWidth, 0);
    expectEquals(0, slider.index);
    expectEquals(ticks[0].value, slider.pref.value);
  });

  test('mouse interaction with the bar', function() {
    let dragEventReceived = false;
    let valueInEvent = null;
    slider.addEventListener('immediate-value-changed', function(e) {
      dragEventReceived = true;
      valueInEvent = e.detail.value;
    });

    const sliderBar = slider.$.sliderBar;
    const sliderBarOrigin = {
      x: sliderBar.getBoundingClientRect().x,
      y: sliderBar.getBoundingClientRect().y
    };

    let oldIndex = slider.index;
    MockInteractions.down(sliderBar, sliderBarOrigin);

    // Mouse down on the left end of the slider bar should move the knob there.
    expectEquals(0, slider.index);
    expectEquals(0, Math.round(parseFloat(slider.$.sliderKnob.style.left)));

    // Mouse down must trigger a drag event since we are essentially dragging
    // the knob.
    assertTrue(dragEventReceived);
    expectEquals(ticks[slider.index].value, valueInEvent);
    dragEventReceived = false;

    // However the pref value should not update until the mouse is released.
    expectEquals(ticks[oldIndex].value, slider.pref.value);

    // Release mouse to update pref value.
    MockInteractions.up(sliderBar);
    expectEquals(ticks[slider.index].value, slider.pref.value);

    const tickWidth =
        sliderBar.getBoundingClientRect().width / (ticks.length - 1);
    let expectedIndex = 3;
    // The knob position for the 3rd index.
    let sliderBarPos = {
      x: sliderBarOrigin.x + tickWidth * expectedIndex,
      y: sliderBarOrigin.y
    };

    // The slider has not yet started dragging.
    assertFalse(slider.dragging);

    oldIndex = slider.index;
    // Clicking at the 3rd index position on the slider bar should update the
    // knob.
    MockInteractions.down(sliderBar, sliderBarPos);
    let expectedLeftPercentage =
        (tickWidth * expectedIndex * 100) / sliderBar.offsetWidth;
    expectEquals(
        Math.round(expectedLeftPercentage),
        Math.round(parseFloat(slider.$.sliderKnob.style.left)));
    expectEquals(expectedIndex, slider.index);
    expectEquals(ticks[oldIndex].value, slider.pref.value);


    expectedIndex = 5;
    const nextSliderBarPos = {
      x: sliderBarPos.x + tickWidth * (expectedIndex - slider.index),
      y: sliderBarPos.y
    };
    MockInteractions.move(sliderBar, sliderBarPos, nextSliderBarPos);
    expectEquals(expectedIndex, slider.index);
    expectedLeftPercentage =
        (tickWidth * expectedIndex * 100) / sliderBar.offsetWidth;
    expectEquals(
        Math.round(expectedLeftPercentage),
        Math.round(parseFloat(slider.$.sliderKnob.style.left)));

    expectEquals(ticks[oldIndex].value, slider.pref.value);

    assertTrue(dragEventReceived);
    expectEquals(ticks[slider.index].value, valueInEvent);

    MockInteractions.up(sliderBar);
    expectEquals(ticks[expectedIndex].value, slider.pref.value);
  });
});
