function isUseCounted(frame) {
  var ServiceWorkerControlledPage = 990;  // From UseCounter.h
  return frame.contentWindow.internals.isUseCounted(
      frame.contentDocument, ServiceWorkerControlledPage);
}
