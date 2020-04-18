'use strict'

/**
 * A module for testing for and extracting names from options (e.g. `--one`, `-o`)
 *
 * @module option
 * @private
 */

class Arg {
  constructor (re) {
    this.re = re
  }

  name (arg) {
    return arg.match(this.re)[1]
  }
  test (arg) {
    return this.re.test(arg)
  }
}

const option = {
  short: new Arg(/^-([^\d-])$/),
  long: new Arg(/^--(\S+)/),
  combined: new Arg(/^-([^\d-]{2,})$/),
  isOption (arg) { return this.short.test(arg) || this.long.test(arg) },
  optEquals: new Arg(/^(--\S+?)=(.*)/),
  VALUE_MARKER: '552f3a31-14cd-4ced-bd67-656a659e9efb' // must be unique
}

module.exports = option
