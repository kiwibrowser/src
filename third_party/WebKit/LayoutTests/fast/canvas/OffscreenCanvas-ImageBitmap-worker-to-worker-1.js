self.onmessage = function(msg) {
  var canvas = new OffscreenCanvas(10, 10);
  var ctx = canvas.getContext('2d');
  ctx.fillStyle = '#0f0';
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  var image = canvas.transferToImageBitmap();
  self.postMessage(image, [image]);
};
