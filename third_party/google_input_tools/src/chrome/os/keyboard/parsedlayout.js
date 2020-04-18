// Copyright 2013 The ChromeOS IME Authors. All Rights Reserved.
// limitations under the License.
// See the License for the specific language governing permissions and
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// distributed under the License is distributed on an "AS-IS" BASIS,
// Unless required by applicable law or agreed to in writing, software
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// You may obtain a copy of the License at
// you may not use this file except in compliance with the License.
// Licensed under the Apache License, Version 2.0 (the "License");
//

/**
 * @fileoverview Defines the parsed layout object which will do layout parsing
 *     and expose the keymappings and the transforms to Model.
 */

goog.provide('i18n.input.chrome.vk.ParsedLayout');

goog.require('goog.object');
goog.require('i18n.input.chrome.vk.KeyCode');



/**
 * Creates the parsed layout object per the raw layout info.
 *
 * @param {!Object} layout The raw layout object defined in the
 *     xxx_layout.js.
 * @constructor
 */
i18n.input.chrome.vk.ParsedLayout = function(layout) {
  /**
   * The layout code (a.k.a. id).
   *
   * @type {string}
   */
  this.id = layout['id'];

  /**
   * The view object needed by UI rendering, including the key
   * mappings. Some extra keys are not appear in following, which are
   * '', 's', 'l', 'sl', 'cl', 'sc', 'scl'. They define the key mappings
   * for each keyboard mode:
   *   '' means normal;
   *   's' means SHIFT;
   *   'l' means CAPSLOCK;
   *   'c' means CTRL+ALT.
   * Those modes will be filled when parsing the raw layout.
   * If certain modes are not defined by the raw layout, this.view.<mode>
   * won't be filled in.
   * The mode format is: {
   *   '<keyChar>': ['<disp type(S|P)>', '<disp chars>', '<commit chars>']
   * }.
   *
   * @type {!Object}
   */
  this.view = {
    'id': layout['id'],
    'title': layout['title'],
    'isRTL': layout['direction'] == 'rtl',
    'is102': !!layout['is102Keyboard'],
    'mappings': goog.object.create([
      '', null,
      's', null,
      'c', null,
      'l', null,
      'sc', null,
      'cl', null,
      'sl', null,
      'scl', null
    ])
  };

  /**
   * The parsed layout transforms. There are only 3 elements of this array.
   * !st is the long exgexp to match, 2nd is the map of:
   * <match location>: [<regexp>, <replacement>].
   * 3rd/4th are the regexp for prefix matches.
   *
   * @type {Array.<!Object>}
   */
  this.transforms = null;

  /**
   * The parsed layout ambiguous chars.
   *
   * @type {Object}
   * @private
   */
  this.ambiRegex_ = null;

  // Parses the key mapping & transforms of the layout.
  this.parseKeyMappings_(layout);
  this.parseTransforms_(layout);
};


/**
 * Parses the key mappings of the given layout.
 *
 * @param {!Object} layout The raw layout object. It's format is:
 *     id: <layout id> in {string}
 *     title: <layout title> in {string}
 *     direction: 'rtl' or 'ltr'
 *     is102Keyboard: True if vk is 102, False/undefined for 101
 *     mappings: key map in {Object.<string,string>}
 *       '': keycodes (each char's charCode represents keycode) in normal state
 *       s: keycodes in SHIFT state
 *       c: keycodes in ALTGR state
 *       l: keycodes in CAPSLOCK state
 *       <the states could be combined, e.g. ',s,sc,sl,scl'>
 *     transform: in {Object.<string,string>}
 *       <regexp>: <replacement>
 *     historyPruneRegex: <regexp string to represent the ambiguities>.
 * @private
 */
i18n.input.chrome.vk.ParsedLayout.prototype.parseKeyMappings_ = function(
    layout) {
  var codes = this.view['is102'] ? i18n.input.chrome.vk.KeyCode.CODES102 :
      i18n.input.chrome.vk.KeyCode.CODES101;

  var mappings = layout['mappings'];
  for (var m in mappings) {
    var map = mappings[m];
    var modes = m.split(/,/);
    if (modes.join(',') != m) {
      modes.push(''); // IE splits 'a,b,' into ['a','b']
    }
    var parsed = {};
    // Example for map is like:
    //   1) {'': '\u00c0123456...', ...}
    //   2) {'QWERT': 'QWERT', ...}
    //   3) {'A': 'aa', ...}
    //   4) {'BCD': '{{bb}}cd', ...}
    //   5) {'EFG': '{{S||e||ee}}FG', ...}
    //   6) {'HI': '{{P||12||H}}i', ...}
    for (var from in map) {
      // In case #1, from is '', to is '\u00c0123456...'.
      // In case #3, from is 'A', to is 'aa'.
      var to = map[from];
      if (from == '') {
        from = codes;
        // If is 102 keyboard, modify 'to' to be compatible with the old vk.
        if (this.view['is102']) {
          // Moves the 26th char {\} to be the 38th char (after {'}).
          var normalizedTo = to.slice(0, 25);
          normalizedTo += to.slice(26, 37);
          normalizedTo += to.charAt(25);
          normalizedTo += to.slice(37);
          to = normalizedTo;
        }
      }
      // Replaces some chars for backward compatibility to old layout
      // definitions.
      from = from.replace('m', '\u00bd');
      from = from.replace('=', '\u00bb');
      from = from.replace(';', '\u00ba');
      if (from.length == 1) {
        // Case #3: single char map to chars.
        parsed[from] = ['S', to, to];
      } else {
        var j = 0;
        for (var i = 0, c; c = from.charAt(i); ++i) {
          var t = to.charAt(j++);
          if (t == to.charAt(j) && t == '{') {
            // Case #4/5/6: {{}} to define single char map to chars.
            var k = to.indexOf('}}', j);
            if (k < j) break;
            var s = to.slice(j + 1, k);
            var parts = s.split('||');
            if (parts.length == 3) {
              // Case #5/6: button/commit chars seperation.
              parsed[c] = parts;
            } else if (parts.length == 1) {
              // Case #4.
              parsed[c] = ['S', s, s];
            }
            j = k + 2;
          } else {
            // Normal case: single char map to according single char.
            parsed[c] = ['S', t, t];
          }
        }
      }
    }
    for (var i = 0, mode; mode = modes[i], mode != undefined; ++i) {
      this.view['mappings'][mode] = parsed;
    }
  }
};


/**
 * Prefixalizes the regexp string.
 *
 * @param {string} re_str The original regexp string.
 * @return {string} The prefixalized the regexp string.
 * @private
 */
i18n.input.chrome.vk.ParsedLayout.prototype.prefixalizeRegexString_ = function(
    re_str) {
  // Makes sure [...\[\]...] won't impact the later replaces.
  re_str = re_str.replace(/\\./g, function(m) {
    if (/^\\\[/.test(m)) {
      return '\u0001';
    }
    if (/^\\\]/.test(m)) {
      return '\u0002';
    }
    return m;
  });
  // Prefixalizes.
  re_str = re_str.replace(/\\.|\[[^\[\]]*\]|\{.*\}|[^\|\\\(\)\[\]\{\}\*\+\?]/g,
      function(m) {
        if (/^\{/.test(m)) {
          return m;
        }
        return '(?:' + m + '|$)';
      });
  // Restores the \[\].
  re_str = re_str.replace(/\u0001/g, '\\[');
  re_str = re_str.replace(/\u0002/g, '\\]');
  return re_str;
};


/**
 * Parses the transforms of the given layout.
 *
 * @param {!Object} layout The raw layout object. It's format is:
 *     id: <layout id> in {string}
 *     title: <layout title> in {string}
 *     direction: 'rtl' or 'ltr'
 *     is102Keyboard: True if vk is 102, False/undefined for 101
 *     mappings: key map in {Object.<string,string>}
 *       '': keycodes (each char's charCode represents keycode) in normal state
 *       s: keycodes in SHIFT state
 *       c: keycodes in ALTGR state
 *       l: keycodes in CAPSLOCK state
 *       <the states could be combined, e.g. ',s,sc,sl,scl'>
 *     transform: in {Object.<string,string>}
 *       <regexp>: <replacement>
 *     historyPruneRegex: <regexp string to represent the ambiguities>.
 * @private
 */
i18n.input.chrome.vk.ParsedLayout.prototype.parseTransforms_ = function(
    layout) {
  var transforms = layout['transform'];
  if (transforms) {
    // regobjs is RegExp objects of the regexp string.
    // regexsalone will be used to get the long regexp which concats all the
    // transform regexp as (...$)|(...$)|...
    // The long regexp is needed because it is ineffecient to match each regexp
    // one by one. Instead, we match the long regexp only once. But we need to
    // know where the match happens and which replacement we need to use.
    // So regobjs will hold the map between the match location and the
    // regexp/replacement.
    var regobjs = [], regexesalone = [], partialRegexs = [];
    // sum_numgrps is the index of current reg group for future matching.
    // Don't care about the whole string in array index 0.
    var sum_numgrps = 1;
    for (var regex in transforms) {
      var regobj = new RegExp(regex + '$');
      var repl = transforms[regex];
      regobjs[sum_numgrps] = [regobj, repl];
      regexesalone.push('(' + regex + '$)');
      partialRegexs.push('^(' + this.prefixalizeRegexString_(regex) + ')');
      // The match should happen to count braces.
      var grpCountRegexp = new RegExp(regex + '|.*');
      // The length attribute would count whole string as well.
      // However, that extra count 1 is compensated by
      // extra braces added.
      var numgrps = grpCountRegexp.exec('').length;
      sum_numgrps += numgrps;
    }
    var longregobj = new RegExp(regexesalone.join('|'));
    // Saves 2 long regexp objects for later prefix matching.
    // The reason to save a regexp with '\u0001' is to make sure the whole
    // string won't match as a prefix for the whole pattern. For example,
    // 'abc' shouldn't match /abc/.
    // In above case, /abc/ is prefixalized as re = /(a|$)(b|$)(c|$)/.
    // 'a', 'ab' & 'abc' can all match re.
    // So make another re2 = /(a|$)(b|$)(c|$)\u0001/, therefore, 'abc' will
    // fail to match. Finally, we can use this checks to make sure the prefix
    // match: "s matches re but it doesn't match re2".
    var prefixregobj = new RegExp(partialRegexs.join('|'));
    // Uses reverse-ordered regexp for prefix matching. Details are explained
    // in predictTransform().
    var prefixregobj2 = new RegExp(partialRegexs.reverse().join('|'));
    this.transforms = [longregobj, regobjs, prefixregobj, prefixregobj2];
  }

  var hisPruReg = layout['historyPruneRegex'];
  if (hisPruReg) {
    this.ambiRegex_ = new RegExp('^(' + hisPruReg + ')$');
  }
};


/**
 * Predicts whether there would be future transforms for the given string.
 *
 * @param {string} text The given string.
 * @return {number} The matched position in the string. Returns -1 for no match.
 */
i18n.input.chrome.vk.ParsedLayout.prototype.predictTransform = function(text) {
  if (!this.transforms || !text) {
    return -1;
  }
  for (var i = 0; i < text.length; i++) {
    var s = text.slice(i - text.length);
    // Uses multiple mathches to make sure the prefix match.
    // Refers to comments in parseTransforms_() method.
    var matches = s.match(this.transforms[2]);
    if (matches && matches[0]) {
      for (var j = 1; j < matches.length && !matches[j]; j++) {}
      var matchedIndex = j;
      // Ties to match the reversed regexp and see whether the matched indexes
      // are pointed to the same rule.
      matches = s.match(this.transforms[3]);
      if (matches && matches[0]) { // This should always match!
        for (var j = 1; j < matches.length && !matches[j]; j++) {}
        if (matchedIndex != matches.length - j) {
          // If the matched and reverse-matched index are not the same, it
          // means the string must be a prefix, because the layout transforms
          // shouldn't have duplicated transforms.
          return i;
        } else {
          // Gets the matched rule regexp, and revise it to add a never-matched
          // char X in the end. And tries to match it with s+X.
          // If matched, it means the s is a full match instead of a prefix
          // match.
          var re = this.transforms[1][matchedIndex][0];
          re = new RegExp(re.toString().match(/\/(.*)\//)[1] + '\u0001');
          if (!(s + '\u0001').match(re)) {
            return i;
          }
        }
      }
    }
  }
  return -1;
};


/**
 * Applies the layout transform and gets the result.
 *
 * @param {string} prevstr The previous text.
 * @param {number} transat The position of previous transform. If it's -1,
 *     it means no transform happened.
 * @param {string} ch The new chars currently added to prevstr.
 * @return {Object} The transform result. It's format is:
 *     {back: <the number of chars to be deleted in the end of the prevstr>,
 *     chars: <the chars to add at the tail after the deletion>}.
 *     If there is no transform applies, return null.
 */
i18n.input.chrome.vk.ParsedLayout.prototype.transform = function(
    prevstr, transat, ch) {
  if (!this.transforms) return null;

  var str;
  if (transat > 0) {
    str = prevstr.slice(0, transat) + '\u001d' +
          prevstr.slice(transat) + ch;
  } else {
    str = prevstr + ch;
  }
  var longr = this.transforms[0];
  var matchArr = longr.exec(str);
  if (matchArr) {
    var rs = this.transforms[1];

    for (var i = 1; i < matchArr.length && !matchArr[i]; i++) {}
    var matchGroup = i;

    var regobj = rs[matchGroup][0];
    var repl = rs[matchGroup][1];
    var m = regobj.exec(str);

    // String visible to user does not have LOOK_BEHIND_SEP_ and chars.
    // So need to discount them in backspace count.
    var rmstr = str.slice(m.index);
    var numseps = rmstr.search('\u001d') > -1 ? 1 : 0;
    var backlen = rmstr.length - numseps - ch.length;

    var newstr = str.replace(regobj, repl);
    var replstr = newstr.slice(m.index);
    replstr = replstr.replace('\u001d', '');

    return {back: backlen, chars: replstr};
  }

  return null;
};


/**
 * Gets whether the given chars is ambiguious chars.
 *
 * @param {string} chars The chars to be judged.
 * @return {boolean} True if given chars is ambiguious chars, false
 *     otherwise.
 */
i18n.input.chrome.vk.ParsedLayout.prototype.isAmbiChars = function(chars) {
  return this.ambiRegex_ ? !!this.ambiRegex_.exec(chars) : false;
};
