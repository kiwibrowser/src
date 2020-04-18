if (self.importScripts) {
    importScripts('../../resources/helpers.js');
    importScripts('testrunner-helpers.js');

    if (get_current_scope() == 'ServiceWorker')
        importScripts('../../../serviceworker/resources/worker-testharness.js');
    else
        importScripts('../../../resources/testharness.js');
}

async_test(function(t) {
    setPermission('geolocation', 'granted', location.origin, location.origin).then(t.step_func(function() {
        navigator.permissions.query({name:'geolocation'}).then(t.step_func(function(p) {
            assert_equals(p.state, 'granted');

            p.onchange = t.step_func(function() {
                assert_equals(p.state, 'denied');

                p.onchange = t.step_func(function() {
                    assert_unreached('the permission should not change again.');
                });

                setPermission('geolocation', 'prompt', 'https://example.com', 'https://example.com');
                setPermission('geolocation', 'prompt', 'https://example.com', location.origin);
                setPermission('geolocation', 'prompt', location.origin, 'https://example.com');

                navigator.permissions.query({name:'geolocation'}).then(t.step_func(function(p) {
                    assert_equals(p.state, 'denied');
                    t.done();
                }));
            });

            setPermission('geolocation', 'denied', location.origin, location.origin);
        }));
    }));
}, 'Testing that the change event is correctly sent. Scope: ' + get_current_scope());

done();
