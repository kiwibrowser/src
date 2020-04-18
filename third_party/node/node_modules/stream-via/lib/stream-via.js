'use strict'
var Transform = require('stream').Transform
var util = require('util')

/**
Process each chunk of a stream via the supplied function.

@module stream-via
*/
module.exports = via
via.async = viaAsync

function SimpleTransform (options) {
  if (!(this instanceof SimpleTransform)) return new SimpleTransform(options)
  Transform.call(this, options)
  this.buf = new Buffer(0)
}
util.inherits(SimpleTransform, Transform)

/**
@param {function}
@param [options] {object} - passed to Transform constructor
@return {external:Duplex}
@alias module:stream-via
*/
function via (transformFunction, options) {
  var stream = SimpleTransform(options)
  stream._transform = function (chunk, enc, done) {
    if (chunk) {
      try {
        this.push(transformFunction(chunk, enc))
      } catch (err) {
        stream.emit('error', err)
      }
    }
    done()
  }
  return stream
}

function viaAsync (transformFunction, options) {
  var stream = SimpleTransform(options)
  stream._transform = function (chunk, enc, done) {
    if (chunk) {
      try {
        this.push(transformFunction(chunk, enc, done))
      } catch (err) {
        stream.emit('error', err)
        done()
      }
    }
  }
  return stream
}

/**
@external Duplex
@see https://nodejs.org/api/stream.html#stream_class_stream_duplex
*/
