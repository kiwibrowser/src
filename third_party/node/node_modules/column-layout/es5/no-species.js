'use strict';

var t = require('typical');

function markNoSpecies(constructor) {
  if (t.isDefined(global['Symbol']) && Symbol.species) {
    Object.defineProperty(constructor, Symbol.species, {
      get: function get() {
        return undefined;
      },
      configurable: true
    });
  }
  return constructor;
}

module.exports = markNoSpecies;