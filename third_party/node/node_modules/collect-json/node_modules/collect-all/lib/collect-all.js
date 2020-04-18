'use strict'
var Transform = require('stream').Transform
var util = require('util')
var connect = require('stream-connect')
var through = require('stream-via')

/**
 * Returns a stream which fires a callback and becomes readable once all input is received.
 *
 * By default the callback is invoked with a Buffer instance containing all concatenated input. If you set the option `{ objectMode: true }` the callback is invoked with an array containing all objects received.
 *
 * @module collect-all
 */
module.exports = collectAll

function CollectTransform (options) {
  options = options || {}
  if (!(this instanceof CollectTransform)) return new CollectTransform(options)
  Transform.call(this, options)
  this.options = options
  this.buf = options.objectMode || options.writableObjectMode ? [] : new Buffer(0)

  /* node 0.10 polyfil */
  if (!options.objectMode) {
    if (options.readableObjectMode) this._readableState.objectMode = true
    if (options.writableObjectMode) this._writableState.objectMode = true
  }
}
util.inherits(CollectTransform, Transform)

CollectTransform.prototype._transform = function (chunk, enc, done) {
  if (chunk) {
    if (this.options.objectMode) {
      this.buf.push(chunk)
    } else {
      this.buf = Buffer.concat([ this.buf, chunk ])
    }
  }
  done()
}

CollectTransform.prototype._flush = function () {
  this.push(this.buf)
  this.push(null)
}

/**
 * @param [callback] {function} - Called once with the collected input data (by default a `Buffer` instance, or array in `objectMode`.). The value returned by this callback function will be passed downstream.
 * @param [options] {object} - [Stream options](https://nodejs.org/dist/latest-v5.x/docs/api/stream.html#stream_new_stream_readable_options) object, passed to the constructor for the stream returned by `collect-all`. If the callback function supplied returns a non-string/buffer value, set `options.objectMode` to `true`.
 * @return {external:Duplex}
 * @alias module:collect-all
 * @example
 * An example command-line client script - string input received at stdin is stamped with `received` then written to  stdout.
 * ```js
 * var collectAll = require('collect-all')
 * process.stdin
 *   .pipe(collectAll(function (input) {
 *     input = 'received: ' + input
 *     return input
 *   }))
 *   .pipe(process.stdout)
 * ```
 *
 * An object-mode example:
 * ```js
 * var collectAll = require('collect-all')
 *
 * function onAllCollected (collected) {
 *   console.log('Objects collected: ' + collected.length)
 * }
 *
 * var stream = collectAll(onAllCollected, { objectMode: true })
 * stream.write({})
 * stream.write({})
 * stream.end({}) // outputs 'Objects collected: 3'
 * ```
 */
function collectAll (callback, options) {
  var collectTransform = new CollectTransform(options)
  if (callback && typeof callback === 'function') {
    return connect(collectTransform, through(callback, options))
  } else {
    return collectTransform
  }
}

/**
 * @external Duplex
 * @see https://nodejs.org/api/stream.html#stream_class_stream_duplex
 */
