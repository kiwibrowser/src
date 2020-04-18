self.onmessage = function(e) {
  // Worker does two things:
  // 1. call createImageBitmap() from the ImageBitmap that is transferred
  // from the main thread, which verifies that createImageBitmap(ImageBitmap)
  // works on the worker thread.
  // 2. send the created ImageBitmap back to the main thread, the
  // main thread exam the property of this ImageBitmap to make sure
  // the transfer between main and worker thread didn't lose data
  createImageBitmap(e.data.data).then(imageBitmap => {
    postMessage({data: imageBitmap}, [imageBitmap]);
  });
};
