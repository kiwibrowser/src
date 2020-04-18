'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var t = require('typical');
var Padding = require('./padding');
var arrayify = require('array-back');

var _viewWidth = new WeakMap();

var Columns = function () {
  function Columns(columns) {
    _classCallCheck(this, Columns);

    this.list = [];
    arrayify(columns).forEach(this.add.bind(this));
  }

  _createClass(Columns, [{
    key: 'totalWidth',
    value: function totalWidth() {
      return this.list.length ? this.list.map(function (col) {
        return col.generatedWidth;
      }).reduce(function (a, b) {
        return a + b;
      }) : 0;
    }
  }, {
    key: 'totalFixedWidth',
    value: function totalFixedWidth() {
      return this.getFixed().map(function (col) {
        return col.generatedWidth;
      }).reduce(function (a, b) {
        return a + b;
      }, 0);
    }
  }, {
    key: 'get',
    value: function get(columnName) {
      return this.list.find(function (column) {
        return column.name === columnName;
      });
    }
  }, {
    key: 'getResizable',
    value: function getResizable() {
      return this.list.filter(function (column) {
        return column.isResizable();
      });
    }
  }, {
    key: 'getFixed',
    value: function getFixed() {
      return this.list.filter(function (column) {
        return column.isFixed();
      });
    }
  }, {
    key: 'add',
    value: function add(column) {
      var col = column instanceof Column ? column : new Column(column);
      this.list.push(col);
      return col;
    }
  }, {
    key: 'autoSize',
    value: function autoSize() {
      var _this = this;

      var viewWidth = _viewWidth.get(this);

      this.list.forEach(function (column) {
        column.generateWidth();
        column.generateMinWidth();
      });

      this.list.forEach(function (column) {
        if (t.isDefined(column.maxWidth) && column.generatedWidth > column.maxWidth) {
          column.generatedWidth = column.maxWidth;
        }

        if (t.isDefined(column.minWidth) && column.generatedWidth < column.minWidth) {
          column.generatedWidth = column.minWidth;
        }
      });

      var width = {
        total: this.totalWidth(),
        view: viewWidth,
        diff: this.totalWidth() - viewWidth,
        totalFixed: this.totalFixedWidth(),
        totalResizable: Math.max(viewWidth - this.totalFixedWidth(), 0)
      };

      if (width.diff > 0) {
        var grownColumns;
        var shrunkenColumns;
        var salvagedSpace;

        (function () {
          var resizableColumns = _this.getResizable();
          resizableColumns.forEach(function (column) {
            column.generatedWidth = Math.floor(width.totalResizable / resizableColumns.length);
          });

          grownColumns = _this.list.filter(function (column) {
            return column.generatedWidth > column.contentWidth;
          });
          shrunkenColumns = _this.list.filter(function (column) {
            return column.generatedWidth < column.contentWidth;
          });
          salvagedSpace = 0;

          grownColumns.forEach(function (column) {
            var currentGeneratedWidth = column.generatedWidth;
            column.generateWidth();
            salvagedSpace += currentGeneratedWidth - column.generatedWidth;
          });
          shrunkenColumns.forEach(function (column) {
            column.generatedWidth += Math.floor(salvagedSpace / shrunkenColumns.length);
          });
        })();
      }

      return this;
    }
  }, {
    key: 'viewWidth',
    set: function set(val) {
      _viewWidth.set(this, val);
    }
  }]);

  return Columns;
}();

var _padding = new WeakMap();

var Column = function () {
  function Column(column) {
    _classCallCheck(this, Column);

    if (t.isDefined(column.name)) this.name = column.name;

    if (t.isDefined(column.width)) this.width = column.width;
    if (t.isDefined(column.maxWidth)) this.maxWidth = column.maxWidth;
    if (t.isDefined(column.minWidth)) this.minWidth = column.minWidth;
    if (t.isDefined(column.nowrap)) this.nowrap = column.nowrap;
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
      return t.isDefined(this.width) || this.nowrap || !this.contentWrappable;
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

module.exports = require('./no-species')(Columns);