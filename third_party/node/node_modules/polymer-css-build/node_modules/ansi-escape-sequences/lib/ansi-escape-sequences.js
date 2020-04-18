'use strict'
var arrayify = require('array-back')

/* Control Sequence Initiator */
var csi = '\x1b['

/**
 * @exports ansi-escape-sequences
 * @typicalname ansi
 * @example
 * var ansi = require('ansi-escape-sequences')
 */
var ansi = {}

/**
 * Various formatting styles (aka Select Graphic Rendition codes).
 * @enum {string}
 * @example
 * console.log(ansi.style.red + 'this is red' + ansi.style.reset)
 */
ansi.style = {
  reset: '\x1b[0m',
  bold: '\x1b[1m',
  italic: '\x1b[3m',
  underline: '\x1b[4m',
  fontDefault: '\x1b[10m',
  font2: '\x1b[11m',
  font3: '\x1b[12m',
  font4: '\x1b[13m',
  font5: '\x1b[14m',
  font6: '\x1b[15m',
  imageNegative: '\x1b[7m',
  imagePositive: '\x1b[27m',
  black: '\x1b[30m',
  red: '\x1b[31m',
  green: '\x1b[32m',
  yellow: '\x1b[33m',
  blue: '\x1b[34m',
  magenta: '\x1b[35m',
  cyan: '\x1b[36m',
  white: '\x1b[37m',
  'bg-black': '\x1b[40m',
  'bg-red': '\x1b[41m',
  'bg-green': '\x1b[42m',
  'bg-yellow': '\x1b[43m',
  'bg-blue': '\x1b[44m',
  'bg-magenta': '\x1b[45m',
  'bg-cyan': '\x1b[46m',
  'bg-white': '\x1b[47m'
}

/**
 * style enum - used by `ansi.styles()`.
 * @enum {number}
 * @private
 */
var eStyles = {
  reset: 0,
  bold: 1,
  italic: 3,
  underline: 4,
  imageNegative: 7,
  fontDefault: 10,
  font2: 11,
  font3: 12,
  font4: 13,
  font5: 14,
  font6: 15,
  imagePositive: 27,
  black: 30,
  red: 31,
  green: 32,
  yellow: 33,
  blue: 34,
  magenta: 35,
  cyan: 36,
  white: 37,
  'bg-black': 40,
  'bg-red': 41,
  'bg-green': 42,
  'bg-yellow': 43,
  'bg-blue': 44,
  'bg-magenta': 45,
  'bg-cyan': 46,
  'bg-white': 47
}

/**
 * Returns an ansi sequence setting one or more effects
 * @param {string | string[]} - a style, or list or styles
 * @returns {string}
 * @example
 * > ansi.styles('green')
 * '\u001b[32m'
 *
 * > ansi.styles([ 'green', 'underline' ])
 * '\u001b[32;4m'
 */
ansi.styles = function (effectArray) {
  effectArray = arrayify(effectArray)
  return csi + effectArray.map(function (effect) { return eStyles[effect] }).join(';') + 'm'
}

/**
 * A convenience function, applying the provided styles to the input string and then resetting.
 *
 * Inline styling can be applied using the syntax `[style-list]{text to format}`, where `style-list` is a space-separated list of styles from {@link module:ansi-escape-sequences.style ansi.style}. For example `[bold white bg-red]{bold white text on a red background}`.
 *
 * @param {string} - the string to format
 * @param [styleArray] {string[]} - a list of styles to add to the input string
 * @returns {string}
 * @example
 * > ansi.format('what?', 'green')
 * '\u001b[32mwhat?\u001b[0m'
 *
 * > ansi.format('what?', ['green', 'bold'])
 * '\u001b[32;1mwhat?\u001b[0m'
 *
 * > ansi.format('[green bold]{what?}')
 * '\u001b[32;1mwhat?\u001b[0m'
 */
ansi.format = function (str, styleArray) {
  var re = /\[([\w\s-]+)\]{([^]*?)}/
  var matches
  if (!str) return ''

  while (matches = str.match(re)) {
    var inlineStyles = matches[1].split(/\s+/)
    var inlineString = matches[2]
    str = str.replace(matches[0], ansi.format(inlineString, inlineStyles))
  }

  return (styleArray && styleArray.length)
    ? ansi.styles(styleArray) + str + ansi.style.reset
    : str
}

/**
 * cursor-related sequences
 */
ansi.cursor = {
  /**
   * Moves the cursor `lines` cells up. If the cursor is already at the edge of the screen, this has no effect
   * @param [lines=1] {number}
   * @return {string}
   */
  up: function (lines) { return csi + (lines || 1) + 'A' },

  /**
   * Moves the cursor `lines` cells down. If the cursor is already at the edge of the screen, this has no effect
   * @param [lines=1] {number}
   * @return {string}
   */
  down: function (lines) { return csi + (lines || 1) + 'B' },

  /**
   * Moves the cursor `lines` cells forward. If the cursor is already at the edge of the screen, this has no effect
   * @param [lines=1] {number}
   * @return {string}
   */
  forward: function (lines) { return csi + (lines || 1) + 'C' },

  /**
   * Moves the cursor `lines` cells back. If the cursor is already at the edge of the screen, this has no effect
   * @param [lines=1] {number}
   * @return {string}
   */
  back: function (lines) { return csi + (lines || 1) + 'D' },

  /**
   * Moves cursor to beginning of the line n lines down.
   * @param [lines=1] {number}
   * @return {string}
   */
  nextLine: function (lines) { return csi + (lines || 1) + 'E' },

  /**
   * Moves cursor to beginning of the line n lines up.
   * @param [lines=1] {number}
   * @return {string}
   */
  previousLine: function (lines) { return csi + (lines || 1) + 'F' },

  /**
   * Moves the cursor to column n.
   * @param n {number} - column number
   * @return {string}
   */
  horizontalAbsolute: function (n) { return csi + n + 'G' },

  /**
   * Moves the cursor to row n, column m. The values are 1-based, and default to 1 (top left corner) if omitted.
   * @param n {number} - row number
   * @param m {number} - column number
   * @return {string}
   */
  position: function (n, m) { return csi + (n || 1) + ';' + (m || 1) + 'H' },

  /**
   * Hides the cursor
   */
  hide: csi + '?25l',

  /**
   * Shows the cursor
   */
  show: csi + '?25h'
}

/**
 * erase sequences
 */
ansi.erase = {
  /**
   * Clears part of the screen. If n is 0 (or missing), clear from cursor to end of screen. If n is 1, clear from cursor to beginning of the screen. If n is 2, clear entire screen.
   * @param n {number}
   * @return {string}
   */
  display: function (n) { return csi + (n || 0) + 'J' },

  /**
   * Erases part of the line. If n is zero (or missing), clear from cursor to the end of the line. If n is one, clear from cursor to beginning of the line. If n is two, clear entire line. Cursor position does not change.
   * @param n {number}
   * @return {string}
   */
  inLine: function (n) { return csi + (n || 0) + 'K' }
}

module.exports = ansi
