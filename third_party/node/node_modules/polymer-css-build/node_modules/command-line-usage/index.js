var detect = require('feature-detect-es6')

module.exports = detect.all('class', 'arrowFunction', 'templateStrings', 'defaultParamValues')
  ? require('./lib/command-line-usage')
  : require('./es5/command-line-usage')
