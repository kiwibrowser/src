
onmessage = e => {
  fetch('./blank.html').then(() => {
    e.source.postMessage({});
  });
};
