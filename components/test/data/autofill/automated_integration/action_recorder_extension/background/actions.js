// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* global IndentedTextFactory */
'use strict';

function escapeSingleQuotes(str) {
  return str.replace(/\'/g, '\\\'');
}

function compareArrays(array1, array2) {
  if (array1.length !== array2.length) {
    return false;
  }

  for (let i = 0; i < array1.length; i++) {
    if (array1[i] !== array2[i]) {
      return false;
    }
  }

  return true;
}

class ByXPath {
  constructor(xPath) {
    if (xPath) {
      xPath = escapeSingleQuotes(xPath);
    }

    this._xPath = xPath || null;
  }

  /**
   * Defines the method used to represent an XPath selector as executable python
   * code.
   *
   * @return {String} Python code representation of the selector class
   */
  toString() { return `ByXPath('${this._xPath}')`; }

  /**
   * Compares the two actions to check whether they are identical.
   *
   * @param  {Action}  other Action to compare this against
   * @return {Boolean}       Whether the actions are identical
   */
  isEqual(other) {
    return (other instanceof ByXPath) && this._xPath === other._xPath;
  }
}

class Action {
  /**
   * @constructor
   * @param {String}  type     Action type name, also the name of the class
   *                           used in the generated test
   * @param {ByXPath} selector An instance of a valid selector which will be
   *                           used to find the element to perform the action on
   * @param {String}  ...args  Additional arguments specific to the action
   */
  constructor(type, selector, ...args) {
    this._type = type;
    this._selector = selector;
    this._extraArgs = args;
  }

  /**
   * Defines the method used to represent Actions as executable python code.
   *
   * These stringified actions can be combined in an action set to create
   * automated browser integration tests.
   *
   * @return {String} Python code representation of the Action class
   */
  toString() {
    let extraArgString = '';

    for (let i = 0; i < this._extraArgs.length; i++) {
      if (this._extraArgs[i] !== undefined && this._extraArgs[i] !== null) {
        extraArgString += `, ${this._extraArgs[i]}`;
      }
    }

    return `${this._type}(${this._selector}${extraArgString})`;
  }

  /**
   * Compares the two actions to check whether they are identical.
   *
   * @param  {Action}  other Action to compare this against
   * @return {Boolean}       Whether the actions are identical
   */
  isEqual(other) {
    return this._type === other._type &&
        this._selector.isEqual(other._selector) &&
        compareArrays(this._extraArgs, other._extraArgs);
  }
}

class Open extends Action {
  /**
   * @constructor
   * @param  {String} url Absolute url to navigate the browser to
   */
  constructor(url) {
    if (url) {
      url = escapeSingleQuotes(url);
    }

    super('Open', `'${url}'`);
  }
}

class SetContext extends Action {
  /**
   * @constructor
   * @param {ByXPath} selector An instance of a valid selector which will be
   *                           used to find the element to perform the action on
   * @param {Boolean} ignorable Whether the test can proceed if the action fails
   * @param {ByXPath} inverse  If this selector is 'None', then this property is
   *                           the selector for the context that it is returning
   *                           'out' of (used to reduce redundant actions)
   */
  constructor(selector, ignorable, inverse) {
    super('SetContext', selector, ignorable);

    this._inverse = inverse;
  }

  /**
   * Compares the two SetContext actions and evaluates whether they cancel
   * eachother (and thus can be both removed with no net effect).
   *
   * @param  {Action}  other Action to compare this against
   * @return {Boolean}       Whether the two actions can be removed safely
   */
  isEqual(other) {
    if (this._type !== other._type) {
      return false;
    }

    if (this._selector === 'None') {
      return this._inverse.isEqual(other._selector);
    } else if (other._selector === 'None') {
      return this._selector.isEqual(other._inverse);
    }

    return false;
  }
}

class Type extends Action {
  /**
   * @constructor
   * @param {ByXPath} selector An instance of a valid selector which will be
   *                           used to find the element to perform the action on
   * @param {String}  text     Content to fill the input field with
   * @param {Boolean} ignorable Whether the test can proceed if the action fails
   * @param {Boolean*} rawText  Whether the text should be printed as is. This
   *                            is useful for injecting helper functions
   *                            (ex. GenEmail())
   */
  constructor(selector, text, ignorable, rawText) {
    text = rawText ? text : `'${text}'`;
    super('Type', selector, text, ignorable);
  }

  /**
   * Compares the two typing actions.
   *
   * Note: even if the text of the two actions is different, they are
   * considered as equal as long as they target the same object.
   *
   * @param  {Action}  other Action to compare this against
   * @return {Boolean}       Whether the first of the two actions is redundant
   */
  isEqual(other) {
    return this._type === other._type &&
        this._selector.isEqual(other._selector);
  }
}

class Select extends Action {
  /**
   * @constructor
   * @param {ByXPath} selector An instance of a valid selector which will be
   *                           used to find the element to perform the action on
   * @param {String}  value    Value of the option to select
   * @param {Boolean} ignorable Whether the test can proceed if the action fails
   * @param {Boolean*} byLabel  Whether |value| represents the option's label.
   *                            This is useful for improving the reliability of
   *                            an action if the value is less stable than the
   *                            human readable form
   */
  constructor(selector, value, ignorable, byLabel) {
    super('Select', selector, `'${value}'`, ignorable, byLabel);
  }
}

class Click extends Action {
  /**
   * @constructor
   * @param {ByXPath} selector An instance of a valid selector which will be
   *                           used to find the element to perform the action on
   * @param {Boolean} ignorable Whether the test can proceed if the action fails
   */
  constructor(selector, ignorable) { super('Click', selector, ignorable); }
}

class TriggerAutofill extends Action {
  /**
   * @constructor
   * Will attempt to trigger autofill using keystrokes with a form field in
   * focus.
   * @param {ByXPath} selector An instance of a valid selector which will be
   *                           used to find the element to perform the action on
   * @param {Boolean} ignorable Whether the test can proceed if the action fails
   */
  constructor(selector, ignorable) {
    super('TriggerAutofill', selector, ignorable);
  }
}

class TypedField {
  constructor(selector, fieldType) {
    this._selector = selector;
    this._fieldType = fieldType;
  }

  toString() { return `(${this._selector}, '${this._fieldType}')`; }
}

class ValidateFields {
  constructor(fields) { this._fields = fields; }

  toString() {
    const textFactory = new IndentedTextFactory();

    textFactory.addLine('ValidateFields([');
    textFactory.increase(2);

    for (let i = 0; i < this._fields.length; i++) {
      textFactory.addLine(`${this._fields[i]},`);
    }

    textFactory.decrease(2);
    textFactory.addLine('])');

    return textFactory.toString();
  }

  isEqual() { return false; }
}
