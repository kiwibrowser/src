(function() {
    var pending_set_permission_resolver = null;
    var shared_worker_port = null;

    set_pending_permission_resolver = function(value) {
        pending_set_permission_resolver = value;
    };

    run_pending_permission_resolver = function() {
        pending_set_permission_resolver();
        pending_set_permission_resolver = null;
    };

    send_from_shared_worker = function(message) {
        wait_for_port = function() {
            if (!shared_worker_port) {
                setTimeout(wait_for_port);
                return;
            }
            shared_worker_port.postMessage(message);
        };
        wait_for_port();
    };

    send_from_service_worker = function(message) {
        wait_for_activation = function() {
            if (!self.registration.active ||
                self.registration.active.state != 'activated') {
                setTimeout(wait_for_activation);
                return;
            }

            self.clients.claim();
            self.clients.matchAll({ includeUncontrolled: true }).then(function(c) {
                c[0].postMessage(message);
            });
        };
        wait_for_activation();
    };

    window_message_handler = function (message) {
        if (message.data.name != 'setPermission')
            return;
        testRunner.setPermission(message.data.permission,
                                 message.data.status,
                                 message.data.origin,
                                 message.data.embedding_origin);

        this.postMessage('setPermission_ACK');
    };

    // Setup message handlers.
    switch (get_current_scope()) {
        case 'Window':
            window.addEventListener('message', function(message) {
                testRunner.setPermission(message.data.permission,
                                         message.data.status,
                                         message.data.origin,
                                         message.data.embedding_origin);

                navigator.serviceWorker.getRegistration(get_script_href()).then(function(registration) {
                    registration.active.postMessage('setPermission_ACK');
                });
            });
            break;
        case 'DedicatedWorker':
            self.addEventListener('message', function(message) {
                if (message.data != 'setPermission_ACK')
                    return;
                if (pending_set_permission_resolver)
                    run_pending_permission_resolver();
            });
            break;
        case 'SharedWorker':
            self.addEventListener('connect', function(e) {
                shared_worker_port = e.ports[0];
                shared_worker_port.addEventListener('message', function(message) {
                    if (message.data != 'setPermission_ACK')
                        return;
                    if (pending_set_permission_resolver)
                        run_pending_permission_resolver();
                });
                shared_worker_port.start();
            });
            break;
        case 'ServiceWorker':
            self.skipWaiting();
            self.addEventListener('message', function(message) {
                if (message.data != 'setPermission_ACK')
                    return;
                if (pending_set_permission_resolver)
                    run_pending_permission_resolver();
            });
            break;
    }
})();

function setPermission(permission, status, origin, embedding_origin) {
    var promise = new Promise(function(resolver) {
        set_pending_permission_resolver(resolver);
    });

    switch (get_current_scope()) {
        case 'Window':
            testRunner.setPermission(permission, status, origin, embedding_origin);
            run_pending_permission_resolver();
            return promise;
        case 'DedicatedWorker':
            self.postMessage({ name: 'setPermission',
                               permission: permission,
                               status: status,
                               origin: origin,
                               embedding_origin: embedding_origin });
            return promise;
        case 'SharedWorker':
            send_from_shared_worker({ name: 'setPermission',
                                      permission: permission,
                                      status: status,
                                      origin: origin,
                                      embedding_origin: embedding_origin });
            return promise;
        case 'ServiceWorker':
            send_from_service_worker({ name: 'setPermission',
                                       permission: permission,
                                       status: status,
                                       origin: origin,
                                       embedding_origin: embedding_origin });
            return promise;
    }

    throw new Error('unknown scope');
}
