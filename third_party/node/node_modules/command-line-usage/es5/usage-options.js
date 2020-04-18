'use strict';

var arrayify = require('array-back');

module.exports = UsageOptions;

function UsageOptions(options) {
  options = options || {};

  this.header = options.header;

  this.title = options.title;

  this.description = options.description;

  this.synopsis = options.synopsis || options.usage && options.usage.forms || options.forms;

  this.groups = options.groups;

  this.examples = options.examples;

  this.footer = options.footer;

  this.hide = arrayify(options.hide);
}