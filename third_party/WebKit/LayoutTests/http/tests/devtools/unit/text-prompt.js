(async function() {
  TestRunner.addResult("This tests if the TextPrompt autocomplete works properly.\n");

  var suggestions = ["heyoo", "hey it's a suggestion", "hey another suggestion"].map(s => ({text: s}));
  var prompt = new UI.TextPrompt();
  prompt.initialize(() => Promise.resolve(suggestions));
  var div = document.createElement("div");
  UI.inspectorView.element.appendChild(div);
  prompt.attachAndStartEditing(div);
  prompt.setText("hey");
  await prompt.complete();
  TestRunner.addResult("Text:" + prompt.text());
  TestRunner.addResult("TextWithCurrentSuggestion:" + prompt.textWithCurrentSuggestion());

  TestRunner.addResult("\nTest with inexact match:");
  prompt.clearAutocomplete();
  prompt.setText("inexactmatch");
  await prompt.complete();
  TestRunner.addResult("Text:" + prompt.text());
  TestRunner.addResult("TextWithCurrentSuggestion:" + prompt.textWithCurrentSuggestion());

  TestRunner.addResult("\nTest with a blank prompt")
  prompt.setText("");
  await prompt.complete();
  TestRunner.addResult("Text:" + prompt.text());
  TestRunner.addResult("TextWithCurrentSuggestion:" + prompt.textWithCurrentSuggestion());

  TestRunner.addResult("\nTest with disableDefaultSuggestionForEmptyInput")
  prompt.disableDefaultSuggestionForEmptyInput();
  prompt.setText("");
  await prompt.complete();
  TestRunner.addResult("Text:" + prompt.text());
  TestRunner.addResult("TextWithCurrentSuggestion:" + prompt.textWithCurrentSuggestion());

  TestRunner.completeTest();
})();
