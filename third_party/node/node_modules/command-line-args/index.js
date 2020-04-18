var detect = require('feature-detect-es6')

if (detect.all('class', 'arrowFunction', 'newArrayFeatures')) {
  module.exports = require('./lib/command-line-args')
} else {
  require('core-js/es6/array')
  module.exports = require('./es5/command-line-args')
}
