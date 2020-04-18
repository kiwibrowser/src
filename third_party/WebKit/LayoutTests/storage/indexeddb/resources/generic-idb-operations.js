if (this.importScripts) {
    importScripts('../../../resources/testharness.js');
}

function assertChangesEqual(actual, expected) {
  assert_equals(actual.database.name, expected.dbName, 'The change record database should be the same as the database being acted on');
  var stores = Object.keys(expected.records);
  assert_equals(actual.records.size, stores.length, 'Incorrect number of objectStores recorded by observer');

  var storeNamesString = Array.from(actual.records.keys()).join(", ");
  for (var i in stores) {
    var key = stores[i];
    assert_true(actual.records.has(key), "Store '" + key + "' not found in changes. Stores: " + storeNamesString);
    var actualObsv = actual.records.get(key);
    var expectedObsv = expected.records[key];
    assert_equals(actualObsv.length, expectedObsv.length, 'Number of observations recorded for objectStore '+ key + ' should match observed operations');
    for (var j in expectedObsv)
      assertObservationsEqual(actualObsv[j], expectedObsv[j]);
  }
}

function assertObservationsEqual(actual, expected) {
  assert_equals(actual.type, expected.type);
  if (actual.type === 'clear') {
    assert_equals(actual.key, undefined, 'clear operation has no key');
    assert_equals(actual.value, null, 'clear operation has no value');
    return;
  }
  if (actual.type === 'delete') {
    assert_equals(actual.key.lower, expected.key.lower, 'Observed operation key lower bound should match operation performed');
    assert_equals(actual.key.upper, expected.key.upper, 'Observed operation key upper bound should match operation performed');
    assert_equals(actual.key.lower_open, expected.key.lower_open, 'Observed operation key lower open should match operation performed');
    assert_equals(actual.key.upper_open, expected.key.upper_open, 'Observed operation key upper open should match operation performed');
    // TODO(dmurph): Value needs to be updated, once returned correctly. Issue crbug.com/609934.
    assert_equals(actual.value, null, 'Delete operation has no value');
    return;
  }
  assert_equals(actual.key.lower, expected.key, 'Observed operation key lower bound should match operation performed');
  assert_equals(actual.key.upper, expected.key, 'Observed operation key upper bound should match operation performed');
  if (expected.value != undefined) {
    assert_equals(actual.value, expected.value, 'Put/Add operation value does not match');
  } else {
    assert_equals(actual.value, null, 'Put/Add operation has unexpected value');
  }
}

function countCallbacks(actual, expected) {
  assert_equals(actual, expected, 'Number of callbacks fired for observer should match number of transactions it observed')
}
