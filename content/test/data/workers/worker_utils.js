var shared_worker_count = 0;
function getWorker(worker_url)
{
  // Create either a dedicated or shared worker, depending on flags
  var url = document.location.toString();
  if (url.search("shared") >= 0) {
    // Make a shared worker that looks like a worker
    var worker = new SharedWorker(worker_url, "name" + ++shared_worker_count);
    worker.port.onmessage = function(evt) {
      worker.onmessage(evt);
    };
    worker.postMessage = function(msg, port) {
      worker.port.postMessage(msg, port);
    };
    return worker;
  } else {
    return new Worker(worker_url);
  }
}

function onSuccess()
{
  setTimeout(onFinished, 0, "OK");
}

function onFailure() {
  setTimeout(onFinished, 0, "FAIL");
}

function onFinished(result) {
  var statusPanel = document.getElementById("statusPanel");
  if (statusPanel) {
    statusPanel.innerHTML = result;
  }

  document.title = result;
}
