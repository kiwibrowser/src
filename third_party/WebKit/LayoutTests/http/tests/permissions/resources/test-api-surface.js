if (self.importScripts) {
    importScripts('helpers.js');

    if (get_current_scope() == 'ServiceWorker')
        importScripts('../../serviceworker/resources/worker-testharness.js');
    else
        importScripts('../../resources/testharness.js');
}

test(function() {
    assert_own_property(self, 'Permissions');
    assert_idl_attribute(navigator, 'permissions', 'navigator.permissions exists');

    assert_inherits(navigator.permissions, 'query');

    assert_own_property(self, 'PermissionStatus');
}, 'Check the Permissions API surface in the ' + get_current_scope() + ' scope.');

done();
