chrome.test.runTests([function tab() {
  // Check that the localstorage stuff we stored is still there.
  chrome.test.assertTrue(localStorage.foo == "bar");

  // Check that the database stuff we stored is still there.
  try {
    console.log("Opening database...");
    var db = window.openDatabase("mydb2", "1.0", "database test", 2048);
    console.log("DONE opening database");
  } catch (err) {
    console.log("Exception");
    console.log(err.message);
  }
  if (!db)
    chrome.test.fail("failed to open database");

  console.log("Performing transaction...");
  db.transaction(function(tx) {
    tx.executeSql("select body from note", [], function(tx, results) {
      chrome.test.assertTrue(results.rows.length == 1);
      chrome.test.assertTrue(results.rows.item(0).body == "hotdog");
      chrome.test.succeed();
    }, function(tx, error) {
      chrome.test.fail(error.message);
    });
  }, function(error) {
    chrome.test.fail(error.message);
  });
}]);
