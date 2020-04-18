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
  optEquals: new Arg(/^(--\S+)=(.*)/)
}

module.exports = option
