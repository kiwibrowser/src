// This file is using testharness.js coding style.

function get_script_href()
{
    var filename = window.location.href.substr(window.location.href.lastIndexOf('/') + 1);
    return 'resources/' + filename.replace('.html', '.js');
}

function get_current_scope()
{
    if ('document' in self) {
        return 'Window';
    }
    if ('DedicatedWorkerGlobalScope' in self &&
        self instanceof DedicatedWorkerGlobalScope) {
        return 'DedicatedWorker';
    }
    if ('SharedWorkerGlobalScope' in self &&
        self instanceof SharedWorkerGlobalScope) {
        return 'SharedWorker';
    }
    if ('ServiceWorkerGlobalScope' in self &&
        self instanceof ServiceWorkerGlobalScope) {
        return 'ServiceWorker';
    }

    throw new Error('unknown scope');
}

function run_test() {
    var script_href = get_script_href();

    // Run the tests on the Window scope.
    var script_element = document.createElement('script');
    script_element.src = script_href;
    document.body.appendChild(script_element);

    // Run the tests on {Dedicated,Shared,Service}Worker.
    fetch_tests_from_worker(new Worker(script_href));
    fetch_tests_from_worker(new SharedWorker(script_href));
    service_worker_test(script_href);
}
