// Sends back the count of connections with this shared worker.
onconnect = e => {
  if (self.count === undefined)
    self.count = 0;
  self.count++;
  e.ports[0].postMessage({ connection_count: self.count });
};
