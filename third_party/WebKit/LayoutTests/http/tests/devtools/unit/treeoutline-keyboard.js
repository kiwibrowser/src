(async function() {
  TestRunner.addResult("This tests if the treeoutline responds to navigation keys.");

  var treeOutline = new UI.TreeOutlineInShadow();
  UI.inspectorView.element.appendChild(treeOutline.element);

  for (var i = 0; i < 10; i++) {
    var treeElement = new UI.TreeElement(String(i), true);
    treeElement.appendChild(new UI.TreeElement(String(i) + ' child'));
    treeOutline.appendChild(treeElement)
  }

  treeOutline.addEventListener(UI.TreeOutline.Events.ElementSelected, event => {
    TestRunner.addResult('Selected: ' + event.data.title);
  });

  treeOutline.firstChild().select(false, true);

  var distance = 25;

  TestRunner.addResult("\nTraveling down");
  for (var i = 0; i < distance; i++)
    sendKey("ArrowDown");

  TestRunner.addResult("\nTraveling up");
  for (var i = 0; i < distance; i++)
    sendKey("ArrowUp");

  TestRunner.addResult("\nEnd");
  sendKey("End");

  TestRunner.addResult("\nHome");
  sendKey("Home");

  TestRunner.addResult("\nTraveling right");
  for (var i = 0; i < distance; i++)
    sendKey("ArrowRight");

  TestRunner.addResult("\nTraveling left");
  for (var i = 0; i < distance; i++)
    sendKey("ArrowLeft");

  TestRunner.completeTest();

  function sendKey(key) {
    document.deepActiveElement().dispatchEvent(TestRunner.createKeyEvent(key));
  }
})();
