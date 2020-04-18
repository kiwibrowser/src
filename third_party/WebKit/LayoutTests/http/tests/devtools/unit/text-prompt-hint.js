(async function() {
  TestRunner.addResult("Tests that the hint displays properly on a UI.TextPrompt with autocomplete.");

  var suggestions = [{text:"testTextPrompt"}];
  var waitingForAutocomplete = null;
  var completionsDone = function () {
      console.error("completionsDone called too early!");
      TestRunner.completeTest();
  }
  var prompt = new UI.TextPrompt();
  prompt.initialize(completions);
  var element = createElement("div");
  UI.inspectorView.element.appendChild(element);
  var proxy = prompt.attachAndStartEditing(element);
  prompt.setText("testT");
  waitForAutocomplete().then(step1);
  prompt.complete();
  dumpTextPrompt();

  function step1() {
      dumpTextPrompt();

      typeCharacter("e");
      dumpTextPrompt();

      waitForAutocomplete().then(step2);
  }
  function step2()
  {
      dumpTextPrompt();

      typeCharacter("z");
      waitForAutocomplete().then(step3);
  }

  function step3()
  {
      dumpTextPrompt();
      typeCharacter(null);
      waitForAutocomplete().then(step4);
  }

  function step4()
  {
      dumpTextPrompt();
      typeCharacter(null);
      waitForAutocomplete().then(step5);
  }
  async function step5()
  {
      dumpTextPrompt();
      prompt.setText("something_before test");
      await Promise.all([
          prompt.complete(),
          completionsDone()
      ]);

      dumpTextPrompt();
      typeCharacter("T");
      dumpTextPrompt();
      TestRunner.completeTest();
  }

  function completions(expression, query)
  {
      var callback;
      var promise = new Promise(x => callback = x);
      TestRunner.addResult("Requesting completions");
      completionsDone = () => {
          callback(suggestions.filter(s => s.text.startsWith(query.toString())))
          return Promise.resolve();
      };
      var temp = waitingForAutocomplete;
      waitingForAutocomplete = null;
      if (temp)
          temp();
      return promise;
  }

  function waitForAutocomplete()
  {
      return new Promise(x => waitingForAutocomplete = x).then(() => completionsDone());
  }

  function dumpTextPrompt()
  {
      TestRunner.addResult("Text:" + prompt.text());
      TestRunner.addResult("TextWithCurrentSuggestion:" + prompt.textWithCurrentSuggestion());
      TestRunner.addResult("");
  }

  function typeCharacter(character)
  {
      var keyboardEvent = new KeyboardEvent("keydown", {
          key: character || "Backspace",
          charCode: character ? character.charCodeAt(0) : ""
      });
      element.dispatchEvent(keyboardEvent);

      var selection = element.getComponentSelection();
      var range = selection.getRangeAt(0);
      var textNode = prompt._ghostTextElement.parentNode ? prompt._ghostTextElement.previousSibling : element.childTextNodes()[element.childTextNodes().length - 1];
      if (!character)
          textNode.textContent = textNode.textContent.substring(0,textNode.textContent.length-1);
      else
          textNode.textContent += character;
      range.setStart(range.startContainer, range.startContainer.textContent.length);
      selection.removeAllRanges();
      selection.addRange(range);
      element.dispatchEvent(new Event("input", {bubbles: true, cancelable: false}));
  }
})();
