// Copyright 2006 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.
/**
 * @pageoverview Some logic for the jstemplates test pages.
 *
 * @author Steffen Meschkat (mesch@google.com)
 */


function elem(id) {
  return document.getElementById(id);
}

/**
 * Are we actively profiling JstProcessor?
 * @type boolean
 */
var profiling = false;

logHtml = function(html) {
  elem('log').innerHTML += html + '<br/>';
};


/**
 * Initializer for interactive test: copies the value from the actual
 * template HTML into a text area to make the HTML source visible.
 */
function jsinit() {
  elem('template').value = elem('tc').innerHTML;
}


/**
 * Interactive test.
 *
 * @param {boolean} reprocess If true, reprocesses the output of the
 * previous invocation.
 */
function jstest(reprocess) {
  var jstext = elem('js').value;
  var input = jsEval(jstext);
  var t;
  if (reprocess) {
    t = elem('out');
  } else {
    elem('tc').innerHTML = elem('template').value;
    t = jstGetTemplate('t');
    elem('out').innerHTML = '';
    elem('out').appendChild(t);
  }
  if (profiling) Profiler.reset();
  jstProcess(new JsEvalContext(input), t);
  if (profiling) Profiler.dump();
  elem('html').value = elem('out').innerHTML;
}


/**
 * Performance test: jst initial processing.
 *
 * @param {Object} data Test data to apply the template to.
 */
function perf1(data) {
  elem('out').innerHTML = '';
  var t = jstGetTemplate('t1');
  elem('out').appendChild(t);
  if (profiling) Profiler.reset();
  jstProcess(new JsEvalContext(data), t);
  if (profiling) Profiler.dump();
}


/**
 * Performance test: jst reprocessing or previous processing output.
 *
 * @param {Object} data Test data to apply the template to.
 */
function perf1a(data) {
  if (profiling) Profiler.reset();
  jstProcess(new JsEvalContext(data), elemOrDie('out'));
  if (profiling) Profiler.dump();
}


/**
 * Performance test: jst initial processing, with display:none during
 * processing.
 *
 * @param {Object} data Test data to apply the template to.
 */
function perf1b(data) {
  var o = elem('out');
  o.innerHTML = '';
  var t = jstGetTemplate('t1');
  o.appendChild(t);
  displayNone(o);
  if (profiling) Profiler.reset();
  jstProcess(new JsEvalContext(data), t);
  if (profiling) Profiler.dump();
  displayDefault(o);
}


/**
 * Performance test: create output procedurally as string and assign
 * to innerHTML.
 *
 * @param {Object} data Test data to apply the template to.
 */
function perf2(data) {
  var t = [];
  t.push("<table><tr><th>item</th><th>label</th><th>address</th></tr>");
  for (var i = 0; i < data.entries.length; ++i) {
    var e = data.entries[i];
    t.push("<tr><td>" + i + "</td><td>" + e.label + "</td><td>" +
           e.address + "</td></tr>");
  }
  t.push("</table>");
  elem("out").innerHTML = t.join('');
}


/**
 * A test runner for a test. Does the timing. @constructor
 *
 * @param {number} times number of iterations the test is executed.
 * @param {Function} test The test to execute.
 * @param {Function} result Function will be called with the execution
 * time as argument.
 */
function TestRun(times, test, result) {
  this.count_ = 0;
  this.times_ = times;
  this.test_ = test;
  this.result_ = result;
  this.start_ = (new Date).valueOf();
  this.run_();
}


/**
 * Executes the test run.
 */
TestRun.prototype.run_ = function() {
  if (this.count_ < this.times_) {
    this.test_(this.count_);
    this.count_ += 1;
    objectSetTimeout(this, this.run_, 0);
  } else {
    this.stop_ = (new Date).valueOf();
    this.result_(this.stop_ - this.start_);
  }
};


/**
 * Creates a testrun function for test count invocations of function
 * f, whose runtime will be output to the element with is given in
 * result.
 *
 * @param {Object} data The test data object.
 * @param {Function} f
 * @param {number} count
 * @param {string} result
*/
function createTestRun(count, test) {
  var data = {
    entries: []
  };
  for (var i = 0; i < count; ++i) {
    data.entries.push({ label: "label" + i, address: "address" + i });
  }
  // This function is passed to the TimeoutSequence, and receives the
  // TimeoutSequence as argument on invocation.
  return function(s) {
    new TestRun(1, function(i) {
      window[test](data);
    }, function(time) {
      elem(test + '-' + count).innerHTML = time + 'ms';
      s.run();
    });
  };
}

/**
 * Runs all tests the given number of times. Invoked from the HTML page.
 *
 * @param {number} count
 */
function jsperf(count) {
  elemOrDie('log').innerHTML = '';
  profiling = !!elemOrDie('profile').checked;
  if (profiling && !JstProcessor.profiling_) {
    JstProcessor.profiling_ = true;
    Profiler.monitorAll(proto(JstProcessor), false);
  }

  var s = new TimeoutSequence(null, null, true);

  s.add(createTestRun(count, "perf1"));
  s.add(createTestRun(count, "perf1b"));
  s.add(createTestRun(count, "perf1a"));
  s.add(createTestRun(count, "perf2"));

  s.run();
}

function run(test, count) {
  var data = {
    entries: []
  };
  for (var i = 0; i < count; ++i) {
    data.entries.push({ label: "label" + i, address: "address" + i });
  }
  new TestRun(1, function() {
    window[test](data);
  }, function(time) {
    elem(test + '-' + count).innerHTML = time + 'ms';
  });
}
