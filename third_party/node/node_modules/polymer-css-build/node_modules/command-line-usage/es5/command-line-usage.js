'use strict';

var OptionList = require('./option-list');
var ContentSection = require('./content-section');
var arrayify = require('array-back');

module.exports = commandLineUsage;

function commandLineUsage(sections) {
  sections = arrayify(sections);
  if (sections.length) {
    var output = sections.map(function (section) {
      if (section.optionList) {
        return new OptionList(section);
      } else {
        return new ContentSection(section);
      }
    });
    return '\n' + output.join('\n');
  }
}