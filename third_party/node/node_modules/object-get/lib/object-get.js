'use strict'

/**
 * Access nested property values at any depth with a simple expression.
 *
 * @module object-get
 * @typicalname objectGet
 * @example
 * ```js
 * const objectGet = require('object-get')
 *
 * const colour = objectGet(mammal, 'fur.appearance.colour')
 * const text = objectGet(el, 'children[2].children[1].children[1].textContent')
 * ```
 *
 * Helps avoid long logical expressions like:
 *
 * ```js
 * const colour = mammal && mammal.fur && mammal.fur.appearance && mammal.fur.appearance.colour
 * ```
 */
module.exports = objectGet

/**
 * Returns the value at the given property.
 *
 * @param {object} - the input object
 * @param {string} - the property accessor expression. 
 * @returns {*}
 * @alias module:object-get
 * @example
 * > objectGet({ animal: 'cow' }, 'animal')
 * 'cow'
 *
 * > objectGet({ animal: { mood: 'lazy' } }, 'animal')
 * { mood: 'lazy' }
 *
 * > objectGet({ animal: { mood: 'lazy' } }, 'animal.mood')
 * 'lazy'
 *
 * > objectGet({ animal: { mood: 'lazy' } }, 'animal.email')
 * undefined
 */
function objectGet (object, expression) {
  if (!(object && expression)) throw new Error('both object and expression args are required')
  return expression.trim().split('.').reduce(function (prev, curr) {
    var arr = curr.match(/(.*?)\[(.*?)\]/)
    if (arr) {
      return prev && prev[arr[1]][arr[2]]
    } else {
      return prev && prev[curr]
    }
  }, object)
}
