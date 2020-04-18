self.addEventListener('message', function(e) {
  createImageBitmap(e.data).then(imageBitmap => {
    postMessage({data: imageBitmap}, [imageBitmap]);
  });
});
