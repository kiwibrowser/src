'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var os = require('os');
var t = require('typical');

var re = {
  chunk: /[^\s-]+?-\b|\S+|\s+|\r\n?|\n/g,
  ansiEscapeSequence: /\u001b.*?m/g
};

var WordWrap = function () {
  function WordWrap(text, options) {
    _classCallCheck(this, WordWrap);

    options = options || {};
    if (!t.isDefined(text)) text = '';

    this._lines = String(text).split(/\r\n|\n/g);
    this.options = options;
    this.options.width = options.width === undefined ? 30 : options.width;
  }

  _createClass(WordWrap, [{
    key: 'lines',
    value: function lines() {
      var _this = this;

      var flatten = require('reduce-flatten');

      return this._lines.map(trimLine.bind(this)).map(function (line) {
        return line.match(re.chunk) || ['~~empty~~'];
      }).map(function (lineWords) {
        if (_this.options.break) {
          return lineWords.map(breakWord.bind(_this));
        } else {
          return lineWords;
        }
      }).map(function (lineWords) {
        return lineWords.reduce(flatten, []);
      }).map(function (lineWords) {
        return lineWords.reduce(function (lines, word) {
          var currentLine = lines[lines.length - 1];
          if (replaceAnsi(word).length + replaceAnsi(currentLine).length > _this.options.width) {
            lines.push(word);
          } else {
            lines[lines.length - 1] += word;
          }
          return lines;
        }, ['']);
      }).reduce(flatten, []).map(trimLine.bind(this)).filter(function (line) {
        return line.trim();
      }).map(function (line) {
        return line.replace('~~empty~~', '');
      });
    }
  }, {
    key: 'wrap',
    value: function wrap() {
      return this.lines().join(os.EOL);
    }
  }, {
    key: 'toString',
    value: function toString() {
      return this.wrap();
    }
  }], [{
    key: 'wrap',
    value: function wrap(text, options) {
      var block = new this(text, options);
      return block.wrap();
    }
  }, {
    key: 'lines',
    value: function lines(text, options) {
      var block = new this(text, options);
      return block.lines();
    }
  }, {
    key: 'isWrappable',
    value: function isWrappable(text) {
      if (t.isDefined(text)) {
        text = String(text);
        var matches = text.match(re.chunk);
        return matches ? matches.length > 1 : false;
      }
    }
  }, {
    key: 'getChunks',
    value: function getChunks(text) {
      return text.match(re.chunk) || [];
    }
  }]);

  return WordWrap;
}();

function trimLine(line) {
  return this.options.noTrim ? line : line.trim();
}

function replaceAnsi(string) {
  return string.replace(re.ansiEscapeSequence, '');
}

function breakWord(word) {
  if (replaceAnsi(word).length > this.options.width) {
    var letters = word.split('');
    var piece = void 0;
    var pieces = [];
    while ((piece = letters.splice(0, this.options.width)).length) {
      pieces.push(piece.join(''));
    }
    return pieces;
  } else {
    return word;
  }
}

module.exports = WordWrap;