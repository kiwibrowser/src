/*
 * Copyright (c) 2009 The Chromium Authors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Creates a frames-per-second counter which displays its output as
 * text in a given element on the web page.
 *
 * @param {!Element} outputElement The HTML element on the web page in
 *   which to write the current frames per second.
 * @param {!number} opt_numSamples The number of samples to take
 *   between each update of the frames-per-second.
 */
function FPSCounter(outputElement, opt_numSamples) {
    this.outputElement_ = outputElement;
    this.startTime_ = new Date();
    if (opt_numSamples) {
        this.numSamples_ = opt_numSamples;
    } else {
        this.numSamples_ = 200;
    }
    this.curSample_ = 0;
    this.curFPS_ = 0;
}

/**
 * Updates this FPSCounter.
 * @return {boolean} whether this FPS counter actually updated this tick.
 */
FPSCounter.prototype.update = function() {
    if (++this.curSample_ >= this.numSamples_) {
        var curTime = new Date();
        var startTime = this.startTime_;
        var diff = curTime.getTime() - startTime.getTime();
        this.curFPS_ = (1000.0 * this.numSamples_ / diff);
        var str = "" + this.curFPS_.toFixed(2) + " frames per second";
        this.outputElement_.innerHTML = str;
        this.curSample_ = 0;
        this.startTime_ = curTime;
        return true;
    }
    return false;
};

/**
 * Gets the most recent FPS measurement.
 * @return {number} the most recent FPS measurement.
 */
FPSCounter.prototype.getFPS = function() {
    return this.curFPS_;
};

FPSCounter.prototype.reset = function() {
    this.curSample_ = 0;
    this.curFPS_ = 0;
}
