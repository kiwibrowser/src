"use strict";

function getArrayBufferFromBigBuffer(bigBuffer) {
  if (bigBuffer.$tag == mojoBase.mojom.BigBuffer.Tags.bytes) {
    return new Uint8Array(bigBuffer.bytes).buffer;
  }
  return bigBuffer.sharedMemory.bufferHandle.mapBuffer(0,
      bigBuffer.sharedMemory.size).buffer;
}
