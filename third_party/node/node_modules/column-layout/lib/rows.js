'use strict'
const Columns = require('./columns')
const ansi = require('./ansi')
const arrayify = require('array-back')
const wrap = require('wordwrapjs')
const Cell = require('./cell')
const t = require('typical')

/**
 * @class Rows
 */
class Rows {
  constructor (rows, columns) {
    this.list = []
    this.load(rows, columns)
  }

  load (rows, columns) {
    arrayify(rows).forEach(row => this.list.push(new Map(objectToIterable(row, columns))))
  }

  /**
   * returns all distinct columns from input
   * @param  {object[]}
   * @return {module:columns}
   */
  static getColumns (rows) {
    var columns = new Columns()
    arrayify(rows).forEach(row => {
      for (let columnName in row) {
        let column = columns.get(columnName)
        if (!column) {
          column = columns.add({ name: columnName, contentWidth: 0, minContentWidth: 0 })
        }
        let cell = new Cell(row[columnName], column)
        let cellValue = cell.value
        if (ansi.has(cellValue)) {
          cellValue = ansi.remove(cellValue)
        }

        if (cellValue.length > column.contentWidth) column.contentWidth = cellValue.length

        let longestWord = getLongestWord(cellValue)
        if (longestWord > column.minContentWidth) {
          column.minContentWidth = longestWord
        }
        if (!column.contentWrappable) column.contentWrappable = wrap.isWrappable(cellValue)
      }
    })
    return columns
  }

  static removeEmptyColumns (data) {
    const distinctColumnNames = data.reduce((columnNames, row) => {
      Object.keys(row).forEach(key => {
        if (columnNames.indexOf(key) === -1) columnNames.push(key)
      })
      return columnNames
    }, [])

    const emptyColumns = distinctColumnNames.filter(columnName => {
      const hasValue = data.some(row => {
        const value = row[columnName]
        return (t.isDefined(value) && !t.isString(value)) || (t.isString(value) && /\S+/.test(value))
      })
      return !hasValue
    })

    return data.map(row => {
      emptyColumns.forEach(emptyCol => delete row[emptyCol])
      return row
    })
  }
}

function getLongestWord (line) {
  const words = wrap.getWords(line)
  return words.reduce((max, word) => {
    return Math.max(word.length, max)
  }, 0)
}

function objectToIterable (row, columns) {
  return columns.list.map(column => {
    return [ column, new Cell(row[column.name], column) ]
  })
}

/**
 * @module rows
 */
module.exports = require('./no-species')(Rows)
