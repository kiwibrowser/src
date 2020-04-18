if (self.importScripts) {
    importScripts('../../resources/helpers.js');
    importScripts('testrunner-helpers.js');

    if (get_current_scope() == 'ServiceWorker')
        importScripts('../../../serviceworker/resources/worker-testharness.js');
    else
        importScripts('../../../resources/testharness.js');
}

var DEFAULT_PERMISSION_STATE = 'denied';
var tests = [
{
    test: async_test('Test PermissionDescription WebIDL rules in ' + get_current_scope() + ' scope.'),
    fn: function(callback) {
        // Revoking a random permission name should fail.
        navigator.permissions.revoke({name:'foobar'}).then(function(result) {
            assert_unreached('revoking a random permission should fail');
            callback();
        }, function(error) {
            assert_equals(error.name, 'TypeError');

            // Revoking a permission without a name should fail.
            return navigator.permissions.revoke({});
        }).then(function(result) {
            assert_unreached('revoking a permission without a name should fail');
            callback();
        }, function(error) {
            assert_equals(error.name, 'TypeError');
            callback();
        });
    }
},
{
    test: async_test('Test geolocation permission in ' + get_current_scope() + ' scope.'),
    fn: function(callback) {
      setPermission('geolocation', 'granted', location.origin, location.origin).then(function() {
          return navigator.permissions.revoke({name:'geolocation'});
      }).then(function(result) {
          assert_true(result instanceof PermissionStatus);
          assert_equals(result.state, DEFAULT_PERMISSION_STATE);
          callback();
      }).catch(function() {
          assert_unreached('revoking geolocation permission should not fail.');
          callback();
      });
    }
},
{
    test: async_test('Test midi permission in ' + get_current_scope() + ' scope.'),
    fn: function(callback) {
        setPermission('midi-sysex', 'granted', location.origin, location.origin).then(function() {
            return navigator.permissions.revoke({name:'midi'});
        }).then(function(result) {
            assert_true(result instanceof PermissionStatus);
            assert_equals(result.state, DEFAULT_PERMISSION_STATE);
            return navigator.permissions.revoke({name:'midi', sysex:false});
        }).then(function(result) {
            assert_true(result instanceof PermissionStatus);
            assert_equals(result.state, DEFAULT_PERMISSION_STATE);
            return navigator.permissions.revoke({name:'midi', sysex:true});
        }).then(function(result) {
            assert_true(result instanceof PermissionStatus);
            assert_equals(result.state, DEFAULT_PERMISSION_STATE);
            callback();
        }).catch(function() {
            assert_unreached('revoking midi permission should not fail.')
            callback();
        });
    }
},
{
    test: async_test('Test push permission in ' + get_current_scope() + ' scope.'),
    fn: function(callback) {
        setPermission('push-messaging', 'granted', location.origin, location.origin).then(function() {
            return navigator.permissions.revoke({name:'push'});
        }).catch(function(e) {
            // By default, the permission revocation is rejected if "userVisibleOnly" option
            // isn't set or set to true.
            assert_equals(e.name, "NotSupportedError");

            // Test for userVisibleOnly=false.
            return navigator.permissions.revoke({name:'push', userVisibleOnly: false});
        }).catch(function(e) {
            // By default, the permission revocation is rejected if "userVisibleOnly" option
            // isn't set or set to true.
            assert_equals(e.name, "NotSupportedError");

            // Test for userVisibleOnly=true.
            return navigator.permissions.revoke({name:'push', userVisibleOnly: true});
        }).then(function(result) {
            assert_true(result instanceof PermissionStatus);
            assert_equals(result.state, DEFAULT_PERMISSION_STATE);
            callback();
        }).catch(function() {
            assert_unreached('revoking push permission should not fail.')
            callback();
        });
    }
},
{
    test: async_test('Test notifications permission in ' + get_current_scope() + ' scope.'),
    fn: function(callback) {
        setPermission('notifications', 'granted', location.origin, location.origin).then(function() {
            return navigator.permissions.revoke({name:'notifications'});
        }).then(function(result) {
            assert_true(result instanceof PermissionStatus);
            assert_equals(result.state, DEFAULT_PERMISSION_STATE);
            callback();
        }).catch(function() {
            assert_unreached('revoking notifications permission should not fail.')
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
