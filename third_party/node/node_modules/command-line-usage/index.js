var detect = require('feature-detect-es6')

module.exports = detect.class() && detect.arrowFunction() && detect.templateStrings()
  ? require('./lib/command-line-usage')
  : require('./es5/command-line-usage')
