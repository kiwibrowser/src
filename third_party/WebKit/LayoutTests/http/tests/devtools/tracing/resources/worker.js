var message_id = 0;
onmessage = function(event)
{
    postMessage("Ack #" + message_id++);
    // Two setTimeout's ensure the JSFrame for onmessage is created between two instant event.
    setTimeout(function() {}, 0);
    setTimeout(function() {}, 0);
};
