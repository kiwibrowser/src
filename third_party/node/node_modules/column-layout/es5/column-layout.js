'use strict';

var Table = require('./table');
var Columns = require('./columns');
var Rows = require('./rows');

module.exports = columnLayout;

function columnLayout(data, options) {
  var table = new Table(data, options);
  return table.render();
}

columnLayout.lines = function (data, options) {
  var table = new Table(data, options);
  return table.renderLines();
};

columnLayout.table = function (data, options) {
  return new Table(data, options);
};

columnLayout.Columns = Columns;
columnLayout.Rows = Rows;