var p = Promise.reject(new Error("Handled error"));

onmessage = function(event)
{
    p.catch(function() {});
}
