// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
goog.provide('ink.ProtoSerializer');


goog.require('goog.crypt.base64');
goog.require('goog.proto2.ObjectSerializer');  // for debugging
goog.require('net.proto2.contrib.WireSerializer');



/**
 * A proto serializer / deserializer to and from base-64 encoded wire format.
 * @constructor
 * @struct
 */
ink.ProtoSerializer = function() {
  /** @private {!net.proto2.contrib.WireSerializer} */
  this.wireSerializer_ = new net.proto2.contrib.WireSerializer();
};


/**
 * @param {!goog.proto2.Message} e proto to serialize
 * @return {string} The serialized proto as a base 64 encoded string.
 */
ink.ProtoSerializer.prototype.serializeToBase64 = function(e) {
  var buf = this.wireSerializer_.serialize(e);
  return goog.crypt.base64.encodeByteArray(buf);
};


/**
 * Deserializes the given opaque serialized object to a jspb object.
 *
 * @param {string} item serialized object as base64 text
 * @param {!goog.proto2.Message} proto Proto to deserialize into
 * @return {!goog.proto2.Message}
 */
ink.ProtoSerializer.prototype.safeDeserialize = function(item, proto) {
  var buf = goog.crypt.base64.decodeStringToByteArray(item);
  this.wireSerializer_.deserializeTo(proto, new Uint8Array(buf));
  return proto;
};


/**
 * @param {!sketchology.proto.Element} p
 * @return {boolean} Whether the provided Element appears valid.
 * @private
 */
ink.ProtoSerializer.prototype.isValid_ = function(p) {
  if (p == null) {
    return false;
  }

  if (!p.hasStroke()) {
    return false;
  }

  return true;
};


/**
 * Returns a human-readable representation of a proto.
 *
 * @param {!goog.proto2.Message} p The proto to debug.
 * @return {string} A nice string to ponder.
 * @private
 */
ink.ProtoSerializer.prototype.debugProto_ = function(p) {
  var obj = new goog.proto2.ObjectSerializer(
      goog.proto2.ObjectSerializer.KeyOption.NAME).serialize(p);
  return JSON.stringify(obj, null, '  ');
};
