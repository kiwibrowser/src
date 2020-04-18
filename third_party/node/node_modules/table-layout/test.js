'use strict'
var detect = require('feature-detect-es6')
var TestRunner = require('test-runner')

if (detect.all('class', 'arrowFunction', 'let', 'const')) {
  TestRunner.run('src/test/*.js')
} else {
  TestRunner.run('es5/test/*.js')
}
