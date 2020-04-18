var message_id = 1;
onmessage = function(event) {
  message_id++;
  postMessage("WorkerMessageReceived");
  doWork();
};

function doWork() {
  postMessage("Message #" + message_id++);

  var ts = Date.now();
  while (true) {
    try {
      if (Date.now() - ts > 1000) {
          ts = Date.now();
          postMessage("Message #" + message_id++);
      }
    } catch (e) {
       postMessage("Exception " + e);
    }
  }
}

