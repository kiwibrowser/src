var detect = require('feature-detect-es6')

/* required on all node versions */
require('core-js/es7/string')

if (!detect.newArrayFeatures()) {
  require('core-js/es6/array')
}
if (!detect.collections()) {
  require('core-js/es6/weak-map')
  require('core-js/es6/map')
}

module.exports = detect.all('class', 'arrowFunction', 'templateStrings')
  ? require('./src/lib/table-layout')
  : require('./es5/lib/table-layout')
