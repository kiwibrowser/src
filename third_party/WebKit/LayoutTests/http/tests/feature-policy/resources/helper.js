function loadIframes(srcs) {
  var iframes = document.getElementsByTagName('iframe');
  for (var src of srcs) {
    for (var iframe of iframes) {
      loadFrame(iframe, src);
    }
  }
}
