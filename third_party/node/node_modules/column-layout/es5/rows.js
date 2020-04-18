'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var Columns = require('./columns');
var ansi = require('./ansi');
var arrayify = require('array-back');
var wrap = require('wordwrapjs');
var Cell = require('./cell');
var t = require('typical');

var Rows = function () {
  function Rows(rows, columns) {
    _classCallCheck(this, Rows);

    this.list = [];
    this.load(rows, columns);
  }

  _createClass(Rows, [{
    key: 'load',
    value: function load(rows, columns) {
      var _this = this;

      arrayify(rows).forEach(function (row) {
        return _this.list.push(new Map(objectToIterable(row, columns)));
      });
    }
  }], [{
    key: 'getColumns',
    value: function getColumns(rows) {
      var columns = new Columns();
      arrayify(rows).forEach(function (row) {
        for (var columnName in row) {
          var column = columns.get(columnName);
          if (!column) {
            column = columns.add({ name: columnName, contentWidth: 0, minContentWidth: 0 });
          }
          var cell = new Cell(row[columnName], column);
          var cellValue = cell.value;
          if (ansi.has(cellValue)) {
            cellValue = ansi.remove(cellValue);
          }

          if (cellValue.length > column.contentWidth) column.contentWidth = cellValue.length;

          var longestWord = getLongestWord(cellValue);
          if (longestWord > column.minContentWidth) {
            column.minContentWidth = longestWord;
          }
          if (!column.contentWrappable) column.contentWrappable = wrap.isWrappable(cellValue);
        }
      });
      return columns;
    }
  }, {
    key: 'removeEmptyColumns',
    value: function removeEmptyColumns(data) {
      var distinctColumnNames = data.reduce(function (columnNames, row) {
        Object.keys(row).forEach(function (key) {
          if (columnNames.indexOf(key) === -1) columnNames.push(key);
        });
        return columnNames;
      }, []);

      var emptyColumns = distinctColumnNames.filter(function (columnName) {
        var hasValue = data.some(function (row) {
          var value = row[columnName];
          return t.isDefined(value) && !t.isString(value) || t.isString(value) && /\S+/.test(value);
        });
        return !hasValue;
      });

      return data.map(function (row) {
        emptyColumns.forEach(function (emptyCol) {
          return delete row[emptyCol];
        });
        return row;
      });
    }
  }]);

  return Rows;
}();

function getLongestWord(line) {
  var words = wrap.getWords(line);
  return words.reduce(function (max, word) {
    return Math.max(word.length, max);
  }, 0);
}

function objectToIterable(row, columns) {
  return columns.list.map(function (column) {
    return [column, new Cell(row[column.name], column)];
  });
}

module.exports = require('./no-species')(Rows);