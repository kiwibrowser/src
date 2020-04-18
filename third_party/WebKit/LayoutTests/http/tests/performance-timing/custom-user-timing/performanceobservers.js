// Compares a list of performance entries to a predefined one.
// perfEntriesToCheck is an array of performance entries from the user agent,
// and expectedEntries is an array of performance entries minted by the test.
function checkEntries(perfEntriesToCheck, expectedEntries) {
  function findMatch(pe) {
    // We match based on entryType, name, detail (if listed) and
    // startTime (if listed).
    for (let i = 0; i < expectedEntries.length; i++) {
      const ex = expectedEntries[i];
      if (ex.entryType === pe.entryType && ex.name === pe.name &&
          (ex.detail === undefined ||
              JSON.stringify(ex.detail) === JSON.stringify(pe.detail)) &&
          (ex.startTime === undefined || ex.startTime === pe.startTime) &&
          (ex.duration === undefined || ex.duration === pe.duration)) {
        return ex;
      }
    }
    return null;
  }

  assert_equals(perfEntriesToCheck.length, expectedEntries.length,
      "performance entries must match");

  perfEntriesToCheck.forEach(function(pe) {
    assert_not_equals(findMatch(pe), null, "Entry matches. Entry:" + JSON.stringify(pe) + ", All:" + JSON.stringify(expectedEntries));
  });
}