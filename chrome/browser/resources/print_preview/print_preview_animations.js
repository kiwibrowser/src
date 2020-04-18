// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Counter used to give animations unique names.
let animationCounter = 0;

const animationEventTracker = new EventTracker();

function addAnimation(code) {
  const name = 'anim' + animationCounter;
  animationCounter++;
  const rules =
      document.createTextNode('@keyframes ' + name + ' {' + code + '}');
  const el = document.createElement('style');
  el.type = 'text/css';
  el.appendChild(rules);
  el.setAttribute('id', name);
  document.body.appendChild(el);

  return name;
}

/**
 * Generates css code for fading in an element by animating the height.
 * @param {number} targetHeight The desired height in pixels after the animation
 *     ends.
 * @return {string} The css code for the fade in animation.
 */
function getFadeInAnimationCode(targetHeight) {
  return '0% { opacity: 0; height: 0; } ' +
      '80% { opacity: 0.5; height: ' + (targetHeight + 4) + 'px; }' +
      '100% { opacity: 1; height: ' + targetHeight + 'px; }';
}

/**
 * Fades in an element. Used for both printing options and error messages
 * appearing underneath the textfields.
 * @param {HTMLElement} el The element to be faded in.
 * @param {boolean=} opt_justShow Whether {@code el} should be shown with no
 *     animation.
 */
function fadeInElement(el, opt_justShow) {
  if (el.classList.contains('visible'))
    return;
  el.classList.remove('closing');
  el.hidden = false;
  el.setAttribute('aria-hidden', 'false');
  el.style.height = 'auto';
  const height = el.offsetHeight;
  if (opt_justShow) {
    el.style.height = '';
    el.style.opacity = '';
  } else {
    el.style.height = height + 'px';
    const animName = addAnimation(getFadeInAnimationCode(height));
    animationEventTracker.add(
        el, 'animationend', onFadeInAnimationEnd.bind(el), false);
    el.style.animationName = animName;
  }
  el.classList.add('visible');
}

/**
 * Fades out an element. Used for both printing options and error messages
 * appearing underneath the textfields.
 * @param {HTMLElement} el The element to be faded out.
 */
function fadeOutElement(el) {
  if (!el.classList.contains('visible'))
    return;
  fadeInAnimationCleanup(el);
  el.style.height = 'auto';
  const height = el.offsetHeight;
  el.style.height = height + 'px';
  /** @suppress {suspiciousCode} */
  el.offsetHeight;  // Should force an update of the computed style.
  animationEventTracker.add(
      el, 'transitionend', onFadeOutTransitionEnd.bind(el), false);
  el.classList.add('closing');
  el.classList.remove('visible');
  el.setAttribute('aria-hidden', 'true');
}

/**
 * Executes when a fade out animation ends.
 * @param {Event} event The event that triggered this listener.
 * @this {HTMLElement} The element where the transition occurred.
 */
function onFadeOutTransitionEnd(event) {
  if (event.propertyName != 'height')
    return;
  animationEventTracker.remove(this, 'transitionend');
  this.hidden = true;
}

/**
 * Executes when a fade in animation ends.
 * @param {Event} event The event that triggered this listener.
 * @this {HTMLElement} The element where the transition occurred.
 */
function onFadeInAnimationEnd(event) {
  this.style.height = '';
  fadeInAnimationCleanup(this);
}

/**
 * Removes the <style> element corresponding to |animationName| from the DOM.
 * @param {HTMLElement} element The animated element.
 */
function fadeInAnimationCleanup(element) {
  if (element.style.animationName) {
    const animEl = $(element.style.animationName);
    if (animEl)
      animEl.parentNode.removeChild(animEl);
    element.style.animationName = '';
    animationEventTracker.remove(element, 'animationend');
  }
}

/**
 * Fades in a printing option existing under |el|.
 * @param {HTMLElement} el The element to hide.
 * @param {boolean=} opt_justShow Whether {@code el} should be hidden with no
 *     animation.
 */
function fadeInOption(el, opt_justShow) {
  if (el.classList.contains('visible'))
    return;
  // To make the option visible during the first fade in.
  el.hidden = false;

  const leftColumn =
      assertInstanceof(el.querySelector('.left-column'), HTMLElement);
  wrapContentsInDiv(leftColumn, ['invisible']);
  const rightColumn =
      assertInstanceof(el.querySelector('.right-column'), HTMLElement);
  wrapContentsInDiv(rightColumn, ['invisible']);

  const toAnimate = el.querySelectorAll('.collapsible');
  for (let i = 0; i < toAnimate.length; i++)
    fadeInElement(assertInstanceof(toAnimate[i], HTMLElement), opt_justShow);
  el.classList.add('visible');
}

/**
 * Fades out a printing option existing under |el|.
 * @param {HTMLElement} el The element to hide.
 * @param {boolean=} opt_justHide Whether {@code el} should be hidden with no
 *     animation.
 */
function fadeOutOption(el, opt_justHide) {
  if (!el.classList.contains('visible'))
    return;

  const leftColumn =
      assertInstanceof(el.querySelector('.left-column'), HTMLElement);
  wrapContentsInDiv(leftColumn, ['visible']);
  const rightColumn =
      assertInstanceof(el.querySelector('.right-column'), HTMLElement);
  if (rightColumn)
    wrapContentsInDiv(rightColumn, ['visible']);

  const toAnimate = el.querySelectorAll('.collapsible');
  for (let i = 0; i < toAnimate.length; i++) {
    if (opt_justHide) {
      toAnimate[i].hidden = true;
      toAnimate[i].classList.add('closing');
      toAnimate[i].classList.remove('visible');
    } else {
      fadeOutElement(assertInstanceof(toAnimate[i], HTMLElement));
    }
  }
  el.classList.remove('visible');
}

/**
 * Wraps the contents of |el| in a div element and attaches css classes
 * |classes| in the new div, only if has not been already done. It is necessary
 * for animating the height of table cells.
 * @param {!HTMLElement} el The element to be processed.
 * @param {!Array} classes The css classes to add.
 */
function wrapContentsInDiv(el, classes) {
  let div = el.querySelector('div');
  if (!div || !div.classList.contains('collapsible')) {
    div = document.createElement('div');
    while (el.childNodes.length > 0)
      div.appendChild(el.firstChild);
    el.appendChild(div);
  }

  div.className = '';
  div.classList.add('collapsible');
  for (let i = 0; i < classes.length; i++)
    div.classList.add(classes[i]);
}
