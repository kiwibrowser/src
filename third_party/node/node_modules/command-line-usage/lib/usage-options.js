'use strict'
var arrayify = require('array-back')

/**
@module usage-options
*/
module.exports = UsageOptions

/**
 * @class
 * @classDesc The class describes all valid options for the `getUsage` function. Inline formatting can be used within any text string supplied using valid [ansi-escape-sequences formatting syntax](https://github.com/75lb/ansi-escape-sequences#module_ansi-escape-sequences.format).
 * @alias module:usage-options
 * @typicalname options
 */
function UsageOptions (options) {
  options = options || {}

  /**
   * Use this field to display a banner or header above the main body.
   * @type {module:usage-options~textBlock}
   */
  this.header = options.header

  /**
  * The title line at the top of the usage, typically the name of the app. By default it is underlined but this formatting can be overridden by passing a {@link module:usage-options~textObject}.
  *
  * @type {string}
  * @example
  * { title: "my-app" }
  */
  this.title = options.title

  /**
   * A description to go underneath the title. For example, some words about what the app is for.
   * @type {module:usage-options~textBlock}
   */
  this.description = options.description

  /**
  * An array of strings highlighting the main usage forms of the app.
  * @type {module:usage-options~textBlock}
  */
  this.synopsis = options.synopsis || (options.usage && options.usage.forms) || options.forms

  /**
  * Specify which groups to display in the output by supplying an object of key/value pairs, where the key is the name of the group to include and the value is a string or textObject. If the value is a string it is used as the group title. Alternatively supply an object containing a `title` and `description` string.
  * @type {object}
  * @example
  * {
  *     main: {
  *         title: "Main options",
  *         description: "This group contains the most important options."
  *     },
  *     misc: "Miscellaneous"
  * }
  */
  this.groups = options.groups

  /**
  Examples
  @type {module:usage-options~textBlock}
  */
  this.examples = options.examples

  /**
  * Displayed at the foot of the usage output.
  * @type {module:usage-options~textBlock}
  * @example
  * {
  *     footer: "Project home: [underline]{https://github.com/me/my-app}"
  * }
  */
  this.footer = options.footer

  /**
  * If you want to hide certain options from the output, specify their names here. This is sometimes used to hide the `defaultOption`.
  * @type {string|string[]}
  * @example
  * {
  *     hide: "files"
  * }
  */
  this.hide = arrayify(options.hide)
}

/**
 * A text block can be a string:
 *
 * ```js
 * {
 *   description: 'This is a single-line description.'
 * }
 * ```
 * .. or multiple strings:
 * ```js
 * {
 *   description: [
 *     'This is a multi-line description.',
 *     'A new string in the array represents a new line.'
 *   ]
 * }
 * ```
 * .. or an array of objects. In which case, it will be formatted by [column-layout](https://github.com/75lb/column-layout):
 * ```js
 * {
 *   description: {
 *     column1: 'This will go in column 1.',
 *     column2: 'Second column text.'
 *   }
 * }
 * ```
 * If you want set specific column-layout options, pass an object with two properties: `options` and `data`.
 * ```js
 * {
 *   description: {
 *     options: {
 *       columns: [
 *         { name: 'two', width: 40, nowrap: true }
 *       ]
 *     },
 *     data: {
 *       column1: 'This will go in column 1.',
 *       column2: 'Second column text.'
 *     }
 *   }
 * }
 * ```
 * @typedef textBlock
 * @type {string | string[] | object[] | {{ options: string, data: array }} }
 */
