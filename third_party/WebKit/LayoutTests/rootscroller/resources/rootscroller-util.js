// Helper async function to block execution for n number of rAFs.
async function nFrames(n) {
  return new Promise(resolve => {
    let remainingFrames = n;
    let func = function() {
      --remainingFrames;
      if (remainingFrames === 0)
        resolve();
      else {
        requestAnimationFrame(func);
      }
    };

    if (n === 0) {
      resolve();
    } else {
      requestAnimationFrame(() => {
        func(resolve);
      });
    }
  });
}
