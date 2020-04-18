'use strict'
const t = require('typical')

// Mark a constructor not supporting @@species
function markNoSpecies (constructor) {
  if (t.isDefined(global['Symbol']) && Symbol.species) {
    Object.defineProperty(constructor, Symbol.species, {
      get: function () { return undefined; },
      configurable: true,
    })
  }
  return constructor
}

/**
 * @module no-species
 */
module.exports = markNoSpecies
