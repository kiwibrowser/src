'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var Table = require('table-layout');
var ansi = require('ansi-escape-sequences');
var t = require('typical');

var Content = function () {
  function Content(content) {
    _classCallCheck(this, Content);

    this._content = content;
  }

  _createClass(Content, [{
    key: 'lines',
    value: function lines() {
      var content = this._content;
      var defaultPadding = { left: '  ', right: ' ' };

      if (content) {
        if (t.isString(content)) {
          var table = new Table({ column: ansi.format(content) }, {
            padding: defaultPadding,
            maxWidth: 80
          });
          return table.renderLines();
        } else if (Array.isArray(content) && content.every(t.isString)) {
          var rows = content.map(function (string) {
            return { column: ansi.format(string) };
          });
          var _table = new Table(rows, {
            padding: defaultPadding,
            maxWidth: 80
          });
          return _table.renderLines();
        } else if (Array.isArray(content) && content.every(t.isPlainObject)) {
          var _table2 = new Table(content.map(function (row) {
            return ansiFormatRow(row);
          }), {
            padding: defaultPadding
          });
          return _table2.renderLines();
        } else if (t.isPlainObject(content)) {
          if (!content.options || !content.data) {
            throw new Error('must have an "options" or "data" property\n' + JSON.stringify(content));
          }
          var options = Object.assign({ padding: defaultPadding }, content.options);

          if (options.columns) {
            options.columns = options.columns.map(function (column) {
              if (column.nowrap) {
                column.noWrap = column.nowrap;
                delete column.nowrap;
              }
              return column;
            });
          }

          var _table3 = new Table(content.data.map(function (row) {
            return ansiFormatRow(row);
          }), options);
          return _table3.renderLines();
        } else {
          var message = 'invalid input - \'content\' must be a string, array of strings, or array of plain objects:\n\n' + JSON.stringify(content);
          throw new Error(message);
        }
      }
    }
  }]);

  return Content;
}();

function ansiFormatRow(row) {
  for (var key in row) {
    row[key] = ansi.format(row[key]);
  }
  return row;
}

module.exports = Content;