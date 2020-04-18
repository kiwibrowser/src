function log(message)
{
    postMessage(message);
}

onmessage = function(event)
{
    log(navigator.hardwareConcurrency > 0 ? 'PASS' : 'FAIL');
    log('DONE');
}
