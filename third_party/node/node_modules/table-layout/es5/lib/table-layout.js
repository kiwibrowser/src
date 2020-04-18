'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var os = require('os');
var Rows = require('./rows');
var Columns = require('./columns');
var ansi = require('./ansi');
var extend = require('deep-extend');

var Table = function () {
  function Table(data, options) {
    _classCallCheck(this, Table);

    var ttyWidth = process && (process.stdout.columns || process.stderr.columns) || 0;

    if (ttyWidth && os.platform() === 'win32') ttyWidth--;

    var defaults = {
      padding: {
        left: ' ',
        right: ' '
      },
      maxWidth: ttyWidth || 80,
      columns: []
    };

    this.options = extend(defaults, options);
    this.load(data);
  }

  _createClass(Table, [{
    key: 'load',
    value: function load(data) {
      var _this = this;

      var options = this.options;

      if (options.ignoreEmptyColumns) {
        data = Rows.removeEmptyColumns(data);
      }

      this.columns = Columns.getColumns(data);
      this.rows = new Rows(data, this.columns);

      this.columns.maxWidth = options.maxWidth;
      this.columns.list.forEach(function (column) {
        if (options.padding) column.padding = options.padding;
        if (options.noWrap) column.noWrap = options.noWrap;
        if (options.break) {
          column.break = options.break;
          column.contentWrappable = true;
        }
      });

      options.columns.forEach(function (optionColumn) {
        var column = _this.columns.get(optionColumn.name);
        if (column) {
          if (optionColumn.padding) {
            column.padding.left = optionColumn.padding.left;
            column.padding.right = optionColumn.padding.right;
          }
          if (optionColumn.width) column.width = optionColumn.width;
          if (optionColumn.maxWidth) column.maxWidth = optionColumn.maxWidth;
          if (optionColumn.minWidth) column.minWidth = optionColumn.minWidth;
          if (optionColumn.noWrap) column.noWrap = optionColumn.noWrap;
          if (optionColumn.break) {
            column.break = optionColumn.break;
            column.contentWrappable = true;
          }
        }
      });

      this.columns.autoSize();
      return this;
    }
  }, {
    key: 'getWrapped',
    value: function getWrapped() {
      var _this2 = this;

      var wrap = require('wordwrapjs');

      this.columns.autoSize();
      return this.rows.list.map(function (row) {
        var line = [];
        row.forEach(function (cell, column) {
          if (column.noWrap) {
            line.push(cell.value.split(/\r\n?|\n/));
          } else {
            line.push(wrap.lines(cell.value, {
              width: column.wrappedContentWidth,
              break: column.break,
              noTrim: _this2.options.noTrim
            }));
          }
        });
        return line;
      });
    }
  }, {
    key: 'getLines',
    value: function getLines() {
      var wrappedLines = this.getWrapped();
      var lines = [];
      wrappedLines.forEach(function (wrapped) {
        var mostLines = getLongestArray(wrapped);

        var _loop = function _loop(i) {
          var line = [];
          wrapped.forEach(function (cell) {
            line.push(cell[i] || '');
          });
          lines.push(line);
        };

        for (var i = 0; i < mostLines; i++) {
          _loop(i);
        }
      });
      return lines;
    }
  }, {
    key: 'renderLines',
    value: function renderLines() {
      var _this3 = this;

      var lines = this.getLines();
      return lines.map(function (line) {
        return line.reduce(function (prev, cell, index) {
          var column = _this3.columns.list[index];
          return prev + padCell(cell, column.padding, column.generatedWidth);
        }, '');
      });
    }
  }, {
    key: 'toString',
    value: function toString() {
      return this.renderLines().join(os.EOL) + os.EOL;
    }
  }]);

  return Table;
}();

function getLongestArray(arrays) {
  var lengths = arrays.map(function (array) {
    return array.length;
  });
  return Math.max.apply(null, lengths);
}

function padCell(cellValue, padding, width) {
  var ansiLength = cellValue.length - ansi.remove(cellValue).length;
  cellValue = cellValue || '';
  return (padding.left || '') + cellValue.padEnd(width - padding.length() + ansiLength) + (padding.right || '');
}

module.exports = Table;