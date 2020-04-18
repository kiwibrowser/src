'use strict'
const arrayify = require('array-back')
const Definitions = require('./definitions')
const option = require('./option')
const t = require('typical')
const Argv = require('./argv')

/**
 * @module command-line-args
 */
module.exports = commandLineArgs

/**
 * Returns an object containing all options set on the command line. By default it parses the global  [`process.argv`](https://nodejs.org/api/process.html#process_process_argv) array.
 *
 * @param {module:definition[]} - An array of [OptionDefinition](#exp_module_definition--OptionDefinition) objects
 * @param [argv] {string[]} - An array of strings, which if passed will be parsed instead  of `process.argv`.
 * @returns {object}
 * @throws `UNKNOWN_OPTION` if the user sets an option without a definition
 * @throws `NAME_MISSING` if an option definition is missing the required `name` property
 * @throws `INVALID_TYPE` if an option definition has a `type` value that's not a function
 * @throws `INVALID_ALIAS` if an alias is numeric, a hyphen or a length other than 1
 * @throws `DUPLICATE_NAME` if an option definition name was used more than once
 * @throws `DUPLICATE_ALIAS` if an option definition alias was used more than once
 * @throws `DUPLICATE_DEFAULT_OPTION` if more than one option definition has `defaultOption: true`
 * @alias module:command-line-args
 * @example
 * ```js
 * const commandLineArgs = require('command-line-args')
 * const options = commandLineArgs([
 *   { name: 'file' },
 *   { name: 'verbose' },
 *   { name: 'depth'}
 * ])
 * ```
 */
function commandLineArgs (definitions, argv) {
  definitions = new Definitions(definitions)
  argv = new Argv(argv)
  argv.expandOptionEqualsNotation()
  argv.expandGetoptNotation()
  argv.validate(definitions)

  /* create output initialised with default values */
  const output = definitions.createOutput()
  let def

  /* walk argv building the output */
  argv.list.forEach(item => {
    if (option.isOption(item)) {
      def = definitions.get(item)
      if (!t.isDefined(output[def.name])) outputSet(output, def.name, def.getInitialValue())
      if (def.isBoolean()) {
        outputSet(output, def.name, true)
        def = null
      }
    } else {
      /* if the value marker is present at the beginning, strip it */
      const reBeginsWithValueMarker = new RegExp('^' + option.VALUE_MARKER)
      const value = reBeginsWithValueMarker.test(item)
        ? item.replace(reBeginsWithValueMarker, '')
        : item
      if (!def) {
        def = definitions.getDefault()
        if (!def) return
        if (!t.isDefined(output[def.name])) outputSet(output, def.name, def.getInitialValue())
      }

      const outputValue = def.type ? def.type(value) : value
      outputSet(output, def.name, outputValue)

      if (!def.multiple) def = null
    }
  })

  /* clear _initial flags */
  for (let key in output) {
    const value = output[key]
    if (Array.isArray(value) && value._initial) delete value._initial
  }

  /* group the output values */
  if (definitions.isGrouped()) {
    return groupOutput(definitions, output)
  } else {
    return output
  }
}

function outputSet (output, property, value) {
  if (output[property] && output[property]._initial) {
    output[property] = []
    delete output[property]._initial
  }
  if (Array.isArray(output[property])) {
    output[property].push(value)
  } else {
    output[property] = value
  }
}

function groupOutput (definitions, output) {
  const grouped = {
    _all: output
  }

  definitions.whereGrouped().forEach(def => {
    arrayify(def.group).forEach(groupName => {
      grouped[groupName] = grouped[groupName] || {}
      if (t.isDefined(output[def.name])) {
        grouped[groupName][def.name] = output[def.name]
      }
    })
  })

  definitions.whereNotGrouped().forEach(def => {
    if (t.isDefined(output[def.name])) {
      if (!grouped._none) grouped._none = {}
      grouped._none[def.name] = output[def.name]
    }
  })
  return grouped
}
