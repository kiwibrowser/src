'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var t = require('typical');

var _value = new WeakMap();
var _column = new WeakMap();

var Cell = function () {
  function Cell(value, column) {
    _classCallCheck(this, Cell);

    this.value = value;
    _column.set(this, column);
  }

  _createClass(Cell, [{
    key: 'value',
    set: function set(val) {
      _value.set(this, val);
    },
    get: function get() {
      var cellValue = _value.get(this);
      if (t.isFunction(cellValue)) cellValue = cellValue.call(_column.get(this));
      if (cellValue === undefined) {
        cellValue = '';
      } else {
        cellValue = String(cellValue);
      }
      return cellValue;
    }
  }]);

  return Cell;
}();

module.exports = Cell;