'use strict'
var connect = require('stream-connect')
var through = require('stream-via')
var collect = require('collect-all')

/**
 * Returns a stream which becomes readable with a single value once all (valid) JSON is received.
 *
 * @module collect-json
 */
module.exports = collectJson
collectJson.async = collectJsonAsync

/**
 * @param [callback] {function} - called with the collected json data, once available. The value returned by the callback will be passed downstream.
 * @return {external:Duplex}
 * @alias module:collect-json
 * @example
 * An example command-line client script - JSON received at stdin is stamped with `received` then written to stdout.
 * ```js
 * var collectJson = require("collect-json")
 *
 * process.stdin
 *     .pipe(collectJson(function(json){
 *         json.received = true
 *         return JSON.stringify(json)
 *     }))
 *     .on("error", function(err){
 *         // input from stdin failed to parse
 *     })
 *     .pipe(process.stdout)
 * ```
 */
function collectJson (throughFunction) {
  var stream = collect(function (data) {
    try {
      var json = JSON.parse(data)
    } catch (err) {
      err.input = data
      err.message = 'collect-json: [' + err.message + '], input:\n' + data
      throw err
    }
    return json
  }, { readableObjectMode: true })
  if (throughFunction) {
    return connect(stream, through(throughFunction, { objectMode: true }))
  } else {
    return stream
  }
}

function collectJsonAsync (throughFunction, options) {
}

/**
 * @external Duplex
 * @see https://nodejs.org/api/stream.html#stream_class_stream_duplex
 */
