(async function() {
  TestRunner.addResult('Test ListModel API.');

  var model = new UI.ListModel();
  model.addEventListener(UI.ListModel.Events.ItemsReplaced, event => {
    var data = event.data;
    var inserted = model.slice(data.index, data.index + data.inserted);
    TestRunner.addResult(`Replaced [${data.removed.join(', ')}] at index ${data.index} with [${inserted.join(', ')}]`);
    TestRunner.addResult(`Resulting list: [${model.join(', ')}]`);
    TestRunner.addResult('');
  });

  TestRunner.addResult('Adding 0, 1, 2');
  model.replaceAll([0, 1, 2]);

  TestRunner.addResult('Replacing 0 with 5, 6, 7');
  model.replaceRange(0, 1, [5, 6, 7]);

  TestRunner.addResult('Pushing 10');
  model.insert(model.length, 10);

  TestRunner.addResult('Popping 10');
  model.remove(model.length - 1);

  TestRunner.addResult('Removing 2');
  model.remove(4);

  TestRunner.addResult('Inserting 8');
  model.insert(1, 8);

  TestRunner.addResult('Replacing with 0...20');
  model.replaceAll([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]);

  TestRunner.addResult('Replacing 7 with 27');
  model.replaceRange(7, 8, [27]);

  TestRunner.addResult('Replacing 18, 19 with 28, 29');
  model.replaceRange(18, 20, [28, 29]);

  TestRunner.addResult('Replacing 1, 2, 3 with [31-43]');
  model.replaceRange(1, 4, [31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43]);

  TestRunner.addResult('Replacing all but 29 with []');
  model.replaceRange(0, 29, []);

  TestRunner.completeTest();
})();
