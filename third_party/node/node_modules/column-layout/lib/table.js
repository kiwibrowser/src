'use strict'
const wrap = require('wordwrapjs')
const os = require('os')
const Rows = require('./rows')
const ansi = require('./ansi')
const extend = require('deep-extend')

const _options = new WeakMap()
/**
 * @class
 * @classdesc Table containing the data
 * @param
 */
class Table {
  constructor (data, options) {
    let ttyWidth = (process && (process.stdout.columns || process.stderr.columns)) || 0

    /* Windows quirk workaround  */
    if (ttyWidth && os.platform() === 'win32') ttyWidth--

    let defaults = {
      padding: {
        left: ' ',
        right: ' '
      },
      viewWidth: ttyWidth || 80,
      columns: []
    }

    _options.set(this, extend(defaults, options))
    this.load(data)
  }

  /**
   * @chainable
   */
  load (data) {
    let options = _options.get(this)

    /* remove empty columns */
    if (options.ignoreEmptyColumns) {
      data = Rows.removeEmptyColumns(data)
    }

    this.columns = Rows.getColumns(data)
    this.rows = new Rows(data, this.columns)

    /* load default column properties from options */
    this.columns.viewWidth = options.viewWidth
    this.columns.list.forEach(column => {
      if (options.padding) column.padding = options.padding
      if (options.nowrap) column.nowrap = options.nowrap
      if (options.break) {
        column.break = options.break
        column.contentWrappable = true
      }
    })

    /* load column properties from options.columns */
    options.columns.forEach(optionColumn => {
      let column = this.columns.get(optionColumn.name)
      if (column) {
        if (optionColumn.padding) {
          column.padding.left = optionColumn.padding.left
          column.padding.right = optionColumn.padding.right
        }
        if (optionColumn.width) column.width = optionColumn.width
        if (optionColumn.maxWidth) column.maxWidth = optionColumn.maxWidth
        if (optionColumn.minWidth) column.minWidth = optionColumn.minWidth
        if (optionColumn.nowrap) column.nowrap = optionColumn.nowrap
        if (optionColumn.break) {
          column.break = optionColumn.break
          column.contentWrappable = true
        }
      }
    })

    this.columns.autoSize()
    return this
  }

  getWrapped () {
    this.columns.autoSize()
    return this.rows.list.map(row => {
      let line = []
      row.forEach((cell, column) => {
        if (column.nowrap) {
          line.push(cell.value.split(/\r\n?|\n/))
        } else {
          line.push(wrap.lines(cell.value, {
            width: column.wrappedContentWidth,
            ignore: ansi.regexp,
            break: column.break
          }))
        }
      })
      return line
    })
  }

  getLines () {
    var wrappedLines = this.getWrapped()
    var lines = []
    wrappedLines.forEach(wrapped => {
      let mostLines = getLongestArray(wrapped)
      for (let i = 0; i < mostLines; i++) {
        let line = []
        wrapped.forEach(cell => {
          line.push(cell[i] || '')
        })
        lines.push(line)
      }
    })
    return lines
  }

  renderLines () {
    var lines = this.getLines()
    return lines.map(line => {
      return line.reduce((prev, cell, index) => {
        let column = this.columns.list[index]
        return prev + padCell(cell, column.padding, column.generatedWidth)
      }, '')
    })
  }

  render () {
    return this.renderLines().join(os.EOL) + os.EOL
  }
}

/**
 * Array of arrays in.. Returns the length of the longest one
 * @returns {number}
 */
function getLongestArray (arrays) {
  var lengths = arrays.map(array => array.length)
  return Math.max.apply(null, lengths)
}

function padCell (cellValue, padding, width) {
  var ansiLength = cellValue.length - ansi.remove(cellValue).length
  cellValue = cellValue || ''
  return (padding.left || '') +
  cellValue.padEnd(width - padding.length() + ansiLength) +
  (padding.right || '')
}

/**
@module table
*/
module.exports = Table
