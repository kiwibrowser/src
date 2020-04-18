#!/usr/bin/env node
'use strict'
var ansi = require('../')
var collect = require('collect-all')

process.argv.splice(0, 2)
if (!process.argv.length) {
  console.error(ansi.format('Usage', 'underline'))
  console.error('$ ansi <method> <args>')
  console.error()
  console.error(ansi.format('Example', 'underline'))
  console.error('$ echo yeah | ansi format underline')
  process.exit(1)
}
var method = process.argv.shift()
var args = process.argv

process.stdin
  .pipe(collect(function (input) {
    if (method === 'format') {
      return ansi.format(input.toString(), args)
    } else {
      console.error(ansi.format('invalid method: ' + method, 'red'))
    }
  }))
  .pipe(process.stdout)
