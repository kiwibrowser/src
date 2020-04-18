'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var t = require('typical');
var Padding = require('./padding');

var _padding = new WeakMap();

var Column = function () {
  function Column(column) {
    _classCallCheck(this, Column);

    if (t.isDefined(column.name)) this.name = column.name;

    if (t.isDefined(column.width)) this.width = column.width;
    if (t.isDefined(column.maxWidth)) this.maxWidth = column.maxWidth;
    if (t.isDefined(column.minWidth)) this.minWidth = column.minWidth;
    if (t.isDefined(column.noWrap)) this.noWrap = column.noWrap;
    if (t.isDefined(column.break)) this.break = column.break;
    if (t.isDefined(column.contentWrappable)) this.contentWrappable = column.contentWrappable;
    if (t.isDefined(column.contentWidth)) this.contentWidth = column.contentWidth;
    if (t.isDefined(column.minContentWidth)) this.minContentWidth = column.minContentWidth;
    this.padding = column.padding || { left: ' ', right: ' ' };
    this.generatedWidth = null;
  }

  _createClass(Column, [{
    key: 'isResizable',
    value: function isResizable() {
      return !this.isFixed();
    }
  }, {
    key: 'isFixed',
    value: function isFixed() {
      return t.isDefined(this.width) || this.noWrap || !this.contentWrappable;
    }
  }, {
    key: 'generateWidth',
    value: function generateWidth() {
      this.generatedWidth = this.width || this.contentWidth + this.padding.length();
    }
  }, {
    key: 'generateMinWidth',
    value: function generateMinWidth() {
      this.minWidth = this.minContentWidth + this.padding.length();
    }
  }, {
    key: 'padding',
    set: function set(padding) {
      _padding.set(this, new Padding(padding));
    },
    get: function get() {
      return _padding.get(this);
    }
  }, {
    key: 'wrappedContentWidth',
    get: function get() {
      return Math.max(this.generatedWidth - this.padding.length(), 0);
    }
  }]);

  return Column;
}();

module.exports = Column;