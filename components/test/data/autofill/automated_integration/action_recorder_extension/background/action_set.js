// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// global IndentedTextFactory, Type, SetContext, Open, ValidateFields
'use strict';

class ActionSet {
  constructor(startingUrl) {
    this._startingUrl = this._stripUrl(startingUrl);
    this._name = this._getTestName(this._startingUrl);
    /**
     * A record of the actions taken by a user on a given test (website).
     *
     * When the user is done with a website, the extension saves the actions to
     * a python script that can be used to re-run the action sequence as a test
     * suite.
     *
     * @type {Array}
     */
    this._steps = [];

    this.addAction(new Open(this._startingUrl));
  }

  addAction(action) { this._steps.push(action); }

  toString() {
    const textFactory = new IndentedTextFactory();

    // Task class setup
    textFactory.addLine(`class ${this._name}(AutofillTask):`);

    textFactory.increase();
    textFactory.addLine('def _create_script(self):');
    textFactory.increase();
    textFactory.addLine('self.script = [');
    textFactory.increase(2);

    for (let i = 0; i < this._steps.length; i++) {
      let action = this._steps[i];
      if (action instanceof ValidateFields) {
        // Trim last character, which is a new line
        const actionText = `${action.toString().slice(0, -1)},`;
        textFactory.addLines(actionText);
      } else {
        const actionText = `${action},`;
        textFactory.addLine(actionText);
      }
    }

    // Close script array
    textFactory.decrease(2);
    textFactory.addLine(']');

    return textFactory.toString();
  }

  /**
   * Return the name of the test based on the form's url |url|
   * (e.g. http://login.example.com/path => test_login_example_com).
   *
   * @param  {String} url The form's url
   * @return {String}     The test name
   */
  _getTestName(url) {
    const a = document.createElement('a');
    a.href = url;
    let splitHostname = a.hostname.split(/[.-]+/);

    let hostname = '';

    for (var i = 0; i < splitHostname.length; i++) {
      let segment = splitHostname[i];

      if (i === 0 && segment === 'www') {
        continue;
      }

      hostname += segment.charAt(0).toUpperCase() + segment.slice(1);
    }

    return `Test${hostname}`;
  }

  /**
   * Removes query and anchor from |url|
   * (e.g. https://example.com/path?query=1#anchor => https://example.com/path).
   *
   * @param  {String} url The full url to be processed
   * @return {String}     The url w/o parameters and anchors
   */
  _stripUrl(url) {
    const a = document.createElement('a');
    a.href = url;
    return a.origin + a.pathname;
  }

  /**
   * Remove the specified set of |indiciesToRemove| from the internal action
   * array.
   *
   * The method does the removal in linear time and in place.
   *
   * Will truncate the internal array by the length of |indiciesToRemove|.
   * @param {Array} indiciesToRemove An array of indicies to remove from _steps
   */
  _removeIndicies(indiciesToRemove) {
    if (!indiciesToRemove || indiciesToRemove.length === 0) {
      return;
    }

    indiciesToRemove.sort((a, b) => a - b);

    let removalIndex = 0;

    // Jump to first removal
    for (var i = indiciesToRemove[0]; i < this._steps.length; i++) {
      if (removalIndex < indiciesToRemove.length &&
          i === indiciesToRemove[removalIndex]) {
        // Undesired element so skip copying it to it's "new" place
        removalIndex++;
      } else {
        this._steps[i - removalIndex] = this._steps[i];
      }
    }

    // Truncate array
    this._steps.length -= removalIndex;
  }

  /**
   * Eliminate redundant actions.
   *
   * Current optimizations:
   *  - Multi-pass removal of redundant context switching
   *  - Remove adjacent typing events for same element
   *
   * Warning: This removes events from the internal action set.
   */
  optimize() {
    this._optimizeContextSwitching();
    this._optimizeTyping();
  }

  /**
   * Remove adjacent typing events for same element.
   *
   * Example: The following set of actions:
   *  Type(ByXPath('//*[@id="tbPhone"]'), ''),
   *  Type(ByXPath('//*[@id="tbPhone"]'), '324'),
   *  Type(ByXPath('//*[@id="tbPhone"]'), '5603928181'),
   *
   * will be reduced to the following:
   *  Type(ByXPath('//*[@id="tbPhone"]'), '5603928181'),
   *
   * Warning: This removes events from the internal action set.
   */
  _optimizeTyping() {
    const indiciesToRemove = [];

    for (let i = 0; i < this._steps.length - 1; i++) {
      const currentAction = this._steps[i];
      if (!(currentAction instanceof Type)) {
        continue;
      }

      if (currentAction.isEqual(this._steps[i + 1])) {
        // Mark this index for removal
        indiciesToRemove.push(i);

        console.log(`Removed redundant typing action ${this._steps[i]}`);
      }
    }

    // Remove duplicate indicies
    this._removeIndicies(indiciesToRemove);
  }

  /**
   * Multi-pass removal of redundant context switching.
   *
   * Note: A ContextSwitch action called with None changes to the parent context
   *
   * Example: The following set of actions:
   *  Click(ByID('register')),
   *  SetContext(ByID('overlayRegFrame')),
   *  Click(ByCssSelector('.regTaEmail')),
   *  SetContext(None),
   *  SetContext(ByID('overlayRegFrame')),
   *  Click(ByCssSelector('div.ui_button.regSubmitBtn')),
   *  SetContext(None),
   *  Click(ByCssSelector('.greeting.link')),
   *
   * will be reduced to the following:
   *  Click(ByID('register')),
   *  SetContext(ByID('overlayRegFrame')),
   *  Click(ByCssSelector('.regTaEmail')),
   *  Click(ByCssSelector('div.ui_button.regSubmitBtn')),
   *  SetContext(None),
   *  Click(ByCssSelector('.greeting.link')),
   *
   * Warning: This removes events from the internal action set.
   */
  _optimizeContextSwitching() {
    let hasChanged = true;

    while (hasChanged) {
      hasChanged = false;
      const indiciesToRemove = [];

      for (let i = 0; i < this._steps.length - 1; i++) {
        const currentAction = this._steps[i];
        const nextAction = this._steps[i + 1];
        if (!(currentAction instanceof SetContext)) {
          console.log(`Skipping ${currentAction}`);
          continue;
        }

        if (currentAction.isEqual(nextAction)) {
          console.log(
              'Removed redundantly inverse context switching actions' +
              `${currentAction} and ${nextAction}`);

          // Mark both indicies for removal
          indiciesToRemove.push(i++);
          indiciesToRemove.push(i);
          hasChanged = true;
        } else {
          console.log(`Not-equal objects ${currentAction} and ${nextAction}`);
        }
      }

      // Remove duplicate indicies
      this._removeIndicies(indiciesToRemove);
    }
  }
}
