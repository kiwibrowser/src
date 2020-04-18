var message_id = 1;
onmessage = function(event) {
  debugger;
  doWork();
};

function doWork() {
  postMessage("Message #" + message_id++);
}
