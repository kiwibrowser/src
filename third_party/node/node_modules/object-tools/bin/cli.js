#!/usr/bin/env node
'use strict'
var collectJson = require('collect-json')
var o = require('../')

process.argv.splice(0, 2)
var method = process.argv.shift()
var value = process.argv.shift()

if (!method) {
  console.error('Must specify a method')
  process.exit(1)
}
process.stdin
  .pipe(collectJson(function (json) {
    var result = o[method](json, value)
    return JSON.stringify(result, null, '  ') + '\n'
  }))
  .pipe(process.stdout)
