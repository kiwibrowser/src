function getVideoURI(dummy) {
  var bool=function(any){return!(any=="no"||!any)};
  return "../../../content/test." + (bool(document.createElement("video").canPlayType('video/ogg; codecs="theora"')) ? "ogv" : "mp4");
}

function getAudioURI(dummy) {
  return "../../../content/test.wav";
}

function testStep(testFunction) {
  try {
    testFunction();
  } catch (e) {
    testFailed('Aborted with exception: ' + e.message);
  }
}

function testDone() {
  // Match the semantics of testharness.js done(), where nothing that
  // happens after that call has any effect on the test result.
  if (!window.wasFinishJSTestCalled) {
    finishJSTest();
    assert_equals = assert_true = assert_false = function() { };
  }
}

function test(testFunction) {
  description(document.title);
  testStep(testFunction);
}

function async_test(title, options) {
  window.jsTestIsAsync = true;
  description(title);
  return {
    step: testStep,
    done: testDone
  }
}

document.write("<p id=description></p><div id=console></div>");
document.write("<script src='../../../../resources/js-test.js'></" + "script>");

assert_equals = function(a, b) { shouldBe('"' + a + '"', '"' + b + '"'); }
assert_true = function(a) { shouldBeTrue("" + a); }
assert_false = function(a) { shouldBeFalse("" + a); }

if (window.testRunner)
  testRunner.dumpAsText();
