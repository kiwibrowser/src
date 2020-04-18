// Returns a promise while will let queued asynchronous tasks run before
// resolving.
function runAsyncTasks() {
    return new Promise((resolve, reject) => {
        setTimeout(() => resolve(), 0);
    });
}
