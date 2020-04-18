'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var ansi = require('ansi-escape-sequences');
var os = require('os');
var arrayify = require('array-back');

var Section = function () {
  function Section() {
    _classCallCheck(this, Section);

    this.list = [];
  }

  _createClass(Section, [{
    key: 'add',
    value: function add(content) {
      var _this = this;

      arrayify(content).forEach(function (line) {
        return _this.list.push(ansi.format(line));
      });
    }
  }, {
    key: 'emptyLine',
    value: function emptyLine() {
      this.list.push('');
    }
  }, {
    key: 'header',
    value: function header(text) {
      if (text) {
        this.add(ansi.format(text, ['underline', 'bold']));
        this.emptyLine();
      }
    }
  }, {
    key: 'toString',
    value: function toString() {
      return this.list.join(os.EOL);
    }
  }]);

  return Section;
}();

module.exports = Section;