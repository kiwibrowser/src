Ensure that scripts imported into a Worker from same-origin hosts trigger detailed onerror messages.

On success, you will see a series of "PASS" messages, followed by "TEST COMPLETE".


PASS workerOnerror.message is "Uncaught This is a custom error message."
PASS workerOnerror.filename is "http://127.0.0.1:8000/workers/resources/worker-importScripts-throw.js"
PASS workerOnerror.lineno is 1
PASS workerOnerror.colno is 1
PASS workerOnerror.error is not null
PASS workerOnerror.error is "This is a custom error message."
PASS pageOnerror.message is "Uncaught This is a custom error message."
PASS pageOnerror.filename is "http://127.0.0.1:8000/workers/resources/worker-importScripts-throw.js"
PASS pageOnerror.lineno is 1
PASS pageOnerror.colno is 1
PASS pageOnerror.error is null
PASS successfullyParsed is true

TEST COMPLETE

