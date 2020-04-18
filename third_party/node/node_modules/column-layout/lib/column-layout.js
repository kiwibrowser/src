'use strict'
var Table = require('./table')
var Columns = require('./columns')
var Rows = require('./rows')

/**
@module column-layout
*/
module.exports = columnLayout

/**
 * Returns JSON data formatted in columns.
 *
 * @param {object[]} - input data
 * @param [options] {object} - optional settings
 * @param [options.viewWidth] {number} - maximum width of layout
 * @param [options.nowrap] {boolean} - disable wrapping on all columns
 * @param [options.break] {boolean} - enable word-breaking on all columns
 * @param [options.columns] {module:column-layout~columnOption} - array of column options
 * @param [options.ignoreEmptyColumns] {boolean}
 * @param [options.padding] {object} - Padding values to set on each column. Per-column overrides can be set in the `options.columns` array.
 * @param [options.padding.left] {string}
 * @param [options.padding.right] {string}
 * @returns {string}
 * @alias module:column-layout
 * @example
 * > columnFormat = require("column-format")
 * > jsonData = [{
 *     col1: "Some text you wish to read in column layout",
 *     col2: "And some more text in column two. "
 * }]
 * > console.log(columnFormat(jsonData, { viewWidth: 30 }))
 *  Some text you  And some more
 *  wish to read   text in
 *  in column      column two.
 *  layout
 */
function columnLayout (data, options) {
  var table = new Table(data, options)
  return table.render()
}

/**
 * Identical to {@link module:column-layout} with the exception of the rendered result being returned as an array of lines, rather that a single string.
 * @param {object[]} - input data
 * @param [options] {object} - optional settings
 * @returns {Array}
 * @example
 * > columnFormat = require("column-format")
 * > jsonData = [{
     col1: "Some text you wish to read in column layout",
     col2: "And some more text in column two. "
 * }]
 * > columnFormat.lines(jsonData, { viewWidth: 30 })
 * [ ' Some text you  And some more ',
 * ' wish to read   text in       ',
 * ' in column      column two.   ',
 * ' layout                       ' ]
 */
columnLayout.lines = function (data, options) {
  var table = new Table(data, options)
  return table.renderLines()
}

/**
 * @param {object[]} - input data
 * @param [options] {object} - optional settings
 * @returns {Table}
 */
columnLayout.table = function (data, options) {
  return new Table(data, options)
}

/**
 * @typedef module:column-layout~columnOption
 * @property name {string} - column name, must match a property name in the input
 * @property [width] {number} - column width
 * @property [minWidth] {number} - column min width
 * @property [maxWidth] {number} - column max width
 * @property [nowrap] {boolean} - disable wrapping for this column
 * @property [break] {boolean} - enable word-breaking for this columns
 * @property [padding] {object} - padding options
 * @property [padding.left] {string} - a string to pad the left of each cell (default: `" "`)
 * @property [padding.right] {string} - a string to pad the right of each cell (default: `" "`)
 */

columnLayout.Columns = Columns
columnLayout.Rows = Rows
