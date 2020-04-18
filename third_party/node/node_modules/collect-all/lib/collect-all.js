'use strict'
var Transform = require('stream').Transform
var util = require('util')
var t = require('typical')
var connect = require('stream-connect')
var through = require('stream-via')

/**
 * Returns a stream which fires a callback and becomes readable once all input is received. Intended for buffer/string streams.
 *
 * @module collect-all
 */
module.exports = collect

function CollectTransform (options) {
  if (!(this instanceof CollectTransform)) return new CollectTransform(options)
  Transform.call(this, options)
  this.buf = new Buffer(0)
}
util.inherits(CollectTransform, Transform)

CollectTransform.prototype._transform = function (chunk, enc, done) {
  if (chunk) {
    this.buf = Buffer.concat([ this.buf, chunk ])
  }
  done()
}

CollectTransform.prototype._flush = function () {
  this.push(this.buf)
  this.push(null)
}

/**
 * @param [callback] {function} - called with the collected json data, once available. The value returned by the callback will be passed downstream.
 * @param [options] {object} - passed to through stream constructor created to house the above callback function.. If the callback function returns a non-string/buffer value, set `objectMode: true`.
 * @return {external:Duplex}
 * @alias module:collect-all
 * @example
 * An example command-line client script - JSON received at stdin is stamped with `received` then written to  stdout.
 * ```js
 * var collectAll = require("collect-all")
 *
 * process.stdin
 *     .pipe(collectAll(function(input){
 *         input += 'received'
 *         return input
 *     }))
 *     .on("error", function(err){
 *         // input from stdin failed to parse
 *     })
 *     .pipe(process.stdout)
 * ```
 */
function collect (callback, options) {
  if (callback && typeof callback === 'function') {
    return connect(CollectTransform(), through(callback, options))
  } else {
    return new CollectTransform()
  }
}

/**
 * @external Duplex
 * @see https://nodejs.org/api/stream.html#stream_class_stream_duplex
 */
