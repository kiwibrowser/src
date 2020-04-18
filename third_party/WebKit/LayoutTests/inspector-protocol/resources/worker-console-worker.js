self.onmessage = function(event)
{
    if (event.data.indexOf("throw") === 0)
        Promise.reject(event.data);
    else
        console.log(event.data);
    self.postMessage(event.data);
}
self.postMessage("ready");
