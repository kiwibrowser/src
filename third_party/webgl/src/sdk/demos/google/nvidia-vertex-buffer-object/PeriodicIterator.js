/*
 * Copyright (c) 2012 The Chromium Authors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer. * Redistributions in binary
 * form must reproduce the above copyright notice, this list of conditions and
 * the following disclaimer in the documentation and/or other materials provided
 * with the distribution. * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


//----------------------------------------------------------------------
// PeriodicIterator
//

function PeriodicIterator(arraySize, period, initialOffset, delta) {
    if (arraySize != undefined) {
        // floating-point steps-per-increment
        var arrayDelta = arraySize * (delta / period);
        // fraction bits == 16
        // fixed-point steps-per-increment
        this.increment = Math.floor(arrayDelta * (1 << 16)) | 0;

        // floating-point initial index
        var offset = arraySize * (initialOffset / period);
        // fixed-point initial index
        this.initOffset = Math.floor(offset * (1 << 16)) | 0;

        var i = 20; // array should be reasonably sized...
        while ((arraySize & (1 << i)) == 0) {
            i--;
        }
        this.arraySizeMask = (1 << i) - 1;
        this.index = this.initOffset;
    }
}

PeriodicIterator.prototype.copy = function() {
    var res = new PeriodicIterator();
    res.increment = this.increment;
    res.initOffset = this.initOffset;
    res.arraySizeMask = this.arraySizeMask;
    res.index = this.index;
    return res;
};

PeriodicIterator.prototype.getIndex = function() {
    return (this.index >> 16) & this.arraySizeMask;
};

PeriodicIterator.prototype.incr = function() {
    this.index += this.increment;
};

PeriodicIterator.prototype.decr = function() {
    this.index -= this.increment;
};

PeriodicIterator.prototype.reset = function() {
    this.index = this.initOffset;
};
