// Returns a Promise for future conversion into WebDriver-backed API.
function keyDown(key, modifiers) {
  return new Promise(function(resolve, reject) {
    if (window.eventSender) {
      eventSender.keyDown(key, modifiers);
      resolve();
    } else {
      reject();
    }
  });
}

function selectTarget(selector) {
  const target = document.querySelector(selector);
  if (target.select) {
    target.select();
  } else {
    const selection = window.getSelection();
    selection.collapse(target, 0);
    selection.extend(target, 1);
  }
}

// Combined convenient methods.

function focusAndKeyDown(selector, key, modifiers) {
  document.querySelector(selector).focus();
  return keyDown(key, modifiers);
}

function collapseEndAndKeyDown(selector, key, modifiers) {
  const target = document.querySelector(selector);
  window.getSelection().collapse(target.lastElementChild || target, 1);
  return keyDown(key, modifiers);
}

function selectAndKeyDown(selector, key, modifiers) {
  selectTarget(selector);
  return keyDown(key, modifiers);
}

function selectAndExecCommand(selector, command) {
  assert_not_equals(window.testRunner, undefined, 'This test requires testRunner.');
  selectTarget(selector);
  testRunner.execCommand(command);
}

{
  const inputevent_automation = async_test("InputEvent Automation");
  // Defined in every test and should return a promise that gets resolved when input is finished.
  inject_input().then(inputevent_automation.step_func_done());
}
