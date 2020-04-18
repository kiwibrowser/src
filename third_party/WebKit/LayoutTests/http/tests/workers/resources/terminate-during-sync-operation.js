var workersStarted;
var workersClosed;
// More than 20 workers causes OOM crashes (hitting RLIMIT_DATA memory limits)
// on Linux.
var workers = new Array(20);

function startWorkers(operationName)
{
    log('Testing interrupting: ' +  operationName);
    log('Starting workers.');
    workersStarted = 0;
    workersClosed = 0;
    for (var i = 0; i < workers.length; ++i) {
        workers[i] = new Worker('resources/sync-operations.js?arg=' + i)
        workers[i].onmessage = onWorkerStarted.bind(null, operationName);
    }
}

// Do our best to try to interrupt the database open
// call by waiting for the worker to start and then
// telling it to do the open database call (and
// then terminate the worker).
function onWorkerStarted(operationName)
{
    workersStarted++;
    log('Started worker count: ' + workersStarted);
    if (workersStarted < workers.length)
        return;

    log('Running operation.');
    for (var i = 0; i < workers.length; ++i)
        workers[i].postMessage(operationName);

    setTimeout('closeWorker()', 0);
}

function closeWorker()
{
    workers[workersClosed].terminate();
    workersClosed++;
    log('Closed worker count: ' + workersClosed);
    if (workersClosed < workers.length)
        setTimeout('closeWorker()', 3);
    else
        waitUntilWorkerThreadsExit(done)
}

function runTest(operationName)
{
    log('Starting test run.');
    if (window.testRunner) {
        testRunner.dumpAsText();
        testRunner.waitUntilDone();
    }
    startWorkers(operationName);
}
