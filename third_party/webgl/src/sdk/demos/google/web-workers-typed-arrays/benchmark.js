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

/**
 * @constructor
 * Run for several seconds with various combinations of settings and record the
 * resulting framerate. Keep track of all the results and display them
 * afterward.
 */
function benchmark() {
    this.tests = [{
        multithreaded: false,
        useTransferables: false,
        numWorkers: null,
        framerate: null
    }, {
        multithreaded: true,
        useTransferables: false,
        numWorkers: 1,
        framerate: null
    }, {
        multithreaded: true,
        useTransferables: false,
        numWorkers: 2,
        framerate: null
    }, {
        multithreaded: true,
        useTransferables: false,
        numWorkers: 4,
        framerate: null
    }, {
        multithreaded: true,
        useTransferables: false,
        numWorkers: 6,
        framerate: null
    },{
        multithreaded: true,
        useTransferables: true,
        numWorkers: 1,
        framerate: null
    }, {
        multithreaded: true,
        useTransferables: true,
        numWorkers: 2,
        framerate: null
    }, {
        multithreaded: true,
        useTransferables: true,
        numWorkers: 4,
        framerate: null
    }, {
        multithreaded: true,
        useTransferables: true,
        numWorkers: 6,
        framerate: null
    }];
}

/**
 * Start a benchmarking run.
 */
benchmark.prototype.start = function() {
    this.currentTest_ = 0;
    if (this.tests)
        this.runTest_(this.tests[this.currentTest_]);
};

/**
 * Index of the current test
 */
benchmark.prototype.currentTest_;

/**
 * Array of test objects, initialized in constructor.
 */
benchmark.prototype.tests;

/**
 * How many update cycles to wait until measuring frame rate
 */
benchmark.prototype.UPDATE_COUNT = 3;

/**
 * @private
 * Runs the given test
 *
 * @param {Object} test the test to run (taken from the tests array).
 */
benchmark.prototype.runTest_ = function(test) {
    console.log('running benchmark');
    document.querySelector('#benchresults').innerHTML = "Running test " + this.currentTest_;
    // run the test
    resetSettings(test);
};

/**
 * @private
 * Output the results of all tests to the #benchresults div.
 */
benchmark.prototype.printResults_ = function() {
    var output = [];
    output.push('<h4>Results</h4>');
    for (t in this.tests) {
        var finishedTest = this.tests[t];
        output.push('<h5>Run ' + t + '</h5>');
        output.push('Multithreaded: ' + finishedTest.multithreaded + '<br />');
        output.push('Use transferables: ' + finishedTest.useTransferables +
                '<br />');
        output.push('Worker count: ' + finishedTest.numWorkers + '<br />');
        output.push('Framerate: ' + finishedTest.framerate + '<br />');
    }
    document.querySelector('#benchresults').innerHTML = output.join('');
};

/**
 * Takes note of the current framerate, stops the current test, and starts the
 * next one.
 */
benchmark.prototype.update_benchmark = function() {
    // if there aren't any pending tests throw an error
    if (!this.tests)
        console.error('no current test in update_benchmark');

    // take the results of the previous test and store
    var currentTest = this.tests[this.currentTest_];
    currentTest.framerate = g_fpsCounter.getFPS();

    // run the next test
    if (this.currentTest_ < this.tests.length - 1) {
        // peek at the beginning of the test queue
        this.currentTest_++;
        var nextTest = this.tests[this.currentTest_];
        this.runTest_(nextTest);
    } else {
        this.printResults_();
    }
};
