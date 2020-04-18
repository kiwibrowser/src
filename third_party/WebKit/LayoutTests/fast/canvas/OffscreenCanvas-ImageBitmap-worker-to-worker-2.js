self.onmessage = function(msg) {
  var canvas = new OffscreenCanvas(10, 10);
  var ctx = canvas.getContext('2d');
  ctx.drawImage(msg.data, 0, 0);
  var image = canvas.transferToImageBitmap();
  self.postMessage(image, [image]);
};
