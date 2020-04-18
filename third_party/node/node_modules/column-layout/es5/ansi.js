'use strict';

var ansiEscapeSequence = /\u001b.*?m/g;

exports.remove = remove;
exports.has = has;
exports.regexp = ansiEscapeSequence;

function remove(input) {
  return input.replace(ansiEscapeSequence, '');
}

function has(input) {
  return ansiEscapeSequence.test(input);
}