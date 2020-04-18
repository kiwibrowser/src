// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.Sha1');

const BLOCK_SIZE = 512 / 8;

/**
 * SHA-1 cryptographic hash constructor.
 * @final
 */
class Sha1 {
  constructor() {
    /**
     * Holds the previous values of accumulated variables a-e in the compress_
     * function.
     * @private @const {!Array<number>}
     */
    this.chain_ = [];

    /**
     * A buffer holding the partially computed hash result.
     * @private @const {!Array<number>}
     */
    this.buf_ = [];

    /**
     * An array of 80 bytes, each a part of the message to be hashed.  Referred
     * to as the message schedule in the docs.
     * @private @const {!Array<number>}
     */
    this.W_ = [];

    /**
     * Contains data needed to pad messages less than 64 bytes.
     * @private @const {!Array<number>}
     */
    this.pad_ = [];

    this.pad_[0] = 128;
    for (let i = 1; i < BLOCK_SIZE; ++i) {
      this.pad_[i] = 0;
    }

    /**
     * @private {number}
     */
    this.inbuf_ = 0;

    /**
     * @private {number}
     */
    this.total_ = 0;

    this.reset();
  }

  /**
   * Resets the internal accumulator.
   */
  reset() {
    this.chain_[0] = 0x67452301;
    this.chain_[1] = 0xefcdab89;
    this.chain_[2] = 0x98badcfe;
    this.chain_[3] = 0x10325476;
    this.chain_[4] = 0xc3d2e1f0;

    this.inbuf_ = 0;
    this.total_ = 0;
  }

  /**
   * Internal compress helper function.
   * @param {!Array<number>|string} buf Block to compress.
   * @param {number=} offset Offset of the block in the buffer.
   * @private
   */
  compress_(buf, offset = 0) {
    let W = this.W_;

    // get 16 big endian words
    if (typeof buf === 'string') {
      for (let i = 0; i < 16; i++) {
        W[i] = (buf.charCodeAt(offset) << 24) |
            (buf.charCodeAt(offset + 1) << 16) |
            (buf.charCodeAt(offset + 2) << 8) | (buf.charCodeAt(offset + 3));
        offset += 4;
      }
    } else {
      for (let i = 0; i < 16; i++) {
        W[i] = (buf[offset] << 24) | (buf[offset + 1] << 16) |
            (buf[offset + 2] << 8) | (buf[offset + 3]);
        offset += 4;
      }
    }

    // expand to 80 words
    for (let i = 16; i < 80; i++) {
      let t = W[i - 3] ^ W[i - 8] ^ W[i - 14] ^ W[i - 16];
      W[i] = ((t << 1) | (t >>> 31)) & 0xffffffff;
    }

    let a = this.chain_[0];
    let b = this.chain_[1];
    let c = this.chain_[2];
    let d = this.chain_[3];
    let e = this.chain_[4];
    let f, k;

    for (let i = 0; i < 80; i++) {
      if (i < 40) {
        if (i < 20) {
          f = d ^ (b & (c ^ d));
          k = 0x5a827999;
        } else {
          f = b ^ c ^ d;
          k = 0x6ed9eba1;
        }
      } else {
        if (i < 60) {
          f = (b & c) | (d & (b | c));
          k = 0x8f1bbcdc;
        } else {
          f = b ^ c ^ d;
          k = 0xca62c1d6;
        }
      }

      let t = (((a << 5) | (a >>> 27)) + f + e + k + W[i]) & 0xffffffff;
      e = d;
      d = c;
      c = ((b << 30) | (b >>> 2)) & 0xffffffff;
      b = a;
      a = t;
    }

    this.chain_[0] = (this.chain_[0] + a) & 0xffffffff;
    this.chain_[1] = (this.chain_[1] + b) & 0xffffffff;
    this.chain_[2] = (this.chain_[2] + c) & 0xffffffff;
    this.chain_[3] = (this.chain_[3] + d) & 0xffffffff;
    this.chain_[4] = (this.chain_[4] + e) & 0xffffffff;
  }

  /**
   * Adds a string (must only contain 8-bit, i.e., Latin1 characters)
   * to the internal accumulator.
   *
   * @param {string|!Array<number>} bytes Data used for the update.
   * @param {number=} length
   */
  update(bytes, length = bytes.length) {
    let lengthMinusBlock = length - BLOCK_SIZE;
    let n = 0;
    // Using local instead of member variables gives ~5% speedup on Firefox 16.
    let buf = this.buf_;
    let inbuf = this.inbuf_;

    // The outer while loop should execute at most twice.
    while (n < length) {
      // When we have no data in the block to top up, we can directly process
      // the input buffer (assuming it contains sufficient data). This gives
      // ~25% speedup on Chrome 23 and ~15% speedup on Firefox 16, but requires
      // that the data is provided in large chunks (or in multiples of 64
      // bytes).
      if (inbuf == 0) {
        while (n <= lengthMinusBlock) {
          this.compress_(bytes, n);
          n += BLOCK_SIZE;
        }
      }

      if (typeof bytes === 'string') {
        while (n < length) {
          buf[inbuf] = bytes.charCodeAt(n);
          ++inbuf;
          ++n;
          if (inbuf == BLOCK_SIZE) {
            this.compress_(buf);
            inbuf = 0;
            // Jump to the outer loop so we use the full-block optimization.
            break;
          }
        }
      } else {
        while (n < length) {
          buf[inbuf] = bytes[n];
          ++inbuf;
          ++n;
          if (inbuf == BLOCK_SIZE) {
            this.compress_(buf);
            inbuf = 0;
            // Jump to the outer loop so we use the full-block optimization.
            break;
          }
        }
      }
    }

    this.inbuf_ = inbuf;
    this.total_ += length;
  }

  /**
   * @return {!Array<number>} The finalized hash computed
   *     from the internal accumulator.
   */
  digest() {
    let digest = [];
    let totalBits = this.total_ * 8;

    // Add pad 0x80 0x00*.
    if (this.inbuf_ < 56) {
      this.update(this.pad_, 56 - this.inbuf_);
    } else {
      this.update(this.pad_, BLOCK_SIZE - (this.inbuf_ - 56));
    }

    // Add # bits.
    for (let i = BLOCK_SIZE - 1; i >= 56; i--) {
      this.buf_[i] = totalBits & 255;
      totalBits /= 256;  // Don't use bit-shifting here!
    }

    this.compress_(this.buf_);

    let n = 0;
    for (let i = 0; i < 5; i++) {
      for (let j = 24; j >= 0; j -= 8) {
        digest[n] = (this.chain_[i] >> j) & 255;
        ++n;
      }
    }

    return digest;
  }
}

exports = Sha1;
