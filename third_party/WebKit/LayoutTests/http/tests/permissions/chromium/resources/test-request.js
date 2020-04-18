if (self.importScripts) {
    importScripts('../../resources/helpers.js');
    importScripts('testrunner-helpers.js');

    if (get_current_scope() == 'ServiceWorker')
        importScripts('../../../serviceworker/resources/worker-testharness.js');
    else
        importScripts('../../../resources/testharness.js');
}

var tests = [
{
    test: async_test('Test PermissionDescription WebIDL rules in ' + get_current_scope() + ' scope.'),
    fn: function(callback) {
        // Requesting a random permission name should fail.
        navigator.permissions.request({name:'foobar'}).then(function(result) {
            throw 'requesting a random permission should fail';
        }, function(error) {
            assert_equals(error.name, 'TypeError');

            // Querying a permission without a name should fail.
            return navigator.permissions.request({});
        }).then(function(result) {
            throw 'requesting a permission without a name should fail';
        }, function(error) {
            assert_equals(error.name, 'TypeError');
            callback();
        }).catch(function(error) {
            assert_unreached(error);
            callback();
        });
    }
},
{
    // request() is expected to show a UI then return the new permission status.
    // In layout tests no UI is shown so it boils down to returning the permission
    // status. The tests only check that behaviour given that trying to simulate
    // user decision would simply test the test infrastructure.
    test: async_test('Test basic request behaviour in ' + get_current_scope() + ' scope.'),
    fn: function(callback) {
      navigator.permissions.request({name:'geolocation'}).then(function(result) {
          assert_true(result instanceof PermissionStatus);
          assert_equals(result.state, 'denied');
          return setPermission('geolocation', 'granted', location.origin, location.origin);
      }).then(function() {
          return navigator.permissions.request({name:'geolocation'});
      }).then(function(result) {
          assert_true(result instanceof PermissionStatus);
          assert_equals(result.state, 'granted');
          navigator.permissions.revoke({name:'geolocation'}).then(callback);
      }).catch(function(error) {
          assert_unreached(error);
          callback();
      });
    }
}];

function runTest(i) {
  tests[i].test.step(function() {
      tests[i].fn(function() {
          tests[i].test.done();
          if (i + 1 < tests.length) {
              runTest(i + 1);
          } else {
              done();
          }
      });
  });
}
runTest(0);
