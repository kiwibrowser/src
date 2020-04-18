addEventListener("error", function(e) {
  postMessage({ value: e.error });
});

throw "testError";
