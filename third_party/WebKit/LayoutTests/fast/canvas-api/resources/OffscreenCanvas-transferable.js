self.onmessage = function(e) {
    postMessage({data: e.data.data}, [e.data.data]);
};
