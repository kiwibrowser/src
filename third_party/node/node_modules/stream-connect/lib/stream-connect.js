'use strict'
var Duplex = require('stream').Duplex
var arrayify = require('array-back')
var assert = require('assert')

/**
 * @module stream-connect
 * @example
 * const connect = require('stream-connect')
 */
module.exports = connect

/**
 * Connect streams.
 *
 * @param streams {...external:Duplex} - One or more streams to connect.
 * @return {external:Duplex}
 * @alias module:stream-connect
 */
function connect () {
  var streams = arrayify(arguments)
  assert.ok(streams.length >= 2, 'Must supply at least two input stream.')

  var first = streams[0]
  var last = streams[streams.length - 1]
  var connected = new Duplex({ objectMode: true })

  streams.forEach(function (stream) {
    stream.on('error', function (err) {
      connected.emit('error', err)
    })
  })

  streams.reduce(function (prev, curr) {
    prev.pipe(curr)
    return curr
  })

  connected._write = function (chunk, enc, done) {
    first.write(chunk)
    done()
  }
  connected._read = function () {}
  connected
    .on('finish', function () {
      first.end()
    })
    .on('pipe', function (src) {
      first.emit('pipe', src)
    })

  /* use flowing rather than paused mode, for node 0.10 compatibility. */
  last
    .on('data', function (chunk) {
      connected.push(chunk)
    })
    .on('end', function () {
      connected.push(null)
    })

  return connected
}

/**
 * @external Duplex
 * @see https://nodejs.org/api/stream.html#stream_class_stream_duplex
 */
