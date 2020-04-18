'use strict'
var detect = require('feature-detect-es6')

if (detect.all('class', 'arrowFunction', 'let', 'const')) {
  module.exports = require('./src/lib/wordwrapjs')
} else {
  module.exports = require('./es5/lib/wordwrapjs')
}
