#!/usr/bin/env node
'use strict'
var columnLayout = require('../')
var cliArgs = require('command-line-args')
var collectJson = require('collect-json')
var ansi = require('ansi-escape-sequences')
var o = require('object-tools')
var t = require('typical')

var cli = cliArgs([
  { name: 'help', type: Boolean, alias: 'h' },
  { name: 'width', type: String, multiple: true, alias: 'w', typeLabel: '<widths>',
  description: 'specify a list of column widths in the format \'<column>:<width>\', for example:\n$ cat <json data> | column-layout --width "column 1: 10" "column 2: 30"' },
  { name: 'padding-left', type: String, alias: 'l',
  description: "One or more characters to pad the left of each column. Defaults to ' '." },
  { name: 'padding-right', type: String, alias: 'r',
  description: "One or more characters to pad the right of each column. Defaults to ' '." },
  { name: 'lines', type: Boolean, description: 'return an array of lines' }
])
var options = cli.parse()

if (options.help) {
  console.error(cli.getUsage({
    title: 'column-layout',
    description: 'Pretty-print JSON data in columns',
    synopsis: [
      '$ cat [underline]{jsonfile} | column-format [options]'
    ]
  }))
  process.exit(0)
}

var columns = []
if (options.width) {
  options.width.forEach(function (columnWidth) {
    var split = columnWidth.split(':').map(function (item) {
      return item.trim()
    })
    if (split[0] && split[1]) {
      columns.push({ name: split[0], width: Number(split[1]) })
    }
  })
}

process.stdin
  .pipe(collectJson(function (json) {
    var clOptions = {
      viewWidth: process.stdout.columns,
      padding: {}
    }

    if (t.isDefined(options['padding-left'])) clOptions.padding.left = options['padding-left']
    if (t.isDefined(options['padding-right'])) clOptions.padding.right = options['padding-right']

    /* split input into data and options */
    if (!Array.isArray(json)) {
      if (json.options && json.data) {
        clOptions = o.extend(clOptions, json.options)
        json = json.data
      } else {
        throw new Error('Invalid input data')
      }
    }

    if (columns.length) clOptions.columns = columns
    return options.lines
      ? JSON.stringify(columnLayout.lines(json, clOptions), null, '  ') + '\n'
      : columnLayout(json, clOptions)
  }))
  .on('error', function (err) {
    console.error(ansi.format(err.stack, 'red'))
    process.exit(1)
  })
  .pipe(process.stdout)
