// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

describe('Decompressor', function() {
  /**
   * @const {string}
   */
  var FILE_SYSTEM_ID = 'fileSystemId';

  /**
   * @const {number}
   */
  var METADATA_REQUEST_ID = 0;

  /**
   * @const {number}
   */
  var OPEN_REQUEST_ID = 1;

  /**
   * @const {number}
   */
  var READ_REQUEST_ID = 2;

  /**
   * @const {number}
   */
  var CLOSE_REQUEST_ID = 3;

  /**
   * @const {string}
   */
  var FILE_PATH = '/dummy';

  /**
   * @const {number}
   */
  var OFFSET = 50;

  /**
   * @const {number}
   */
  var LENGTH = 200;

  /**
   * @const {string}
   */
  var ENCODING = 'CP1250';

  /**
   * @const {!Blob}
   */
  var BLOB =
      new Blob([new Uint8Array(100)], {type: 'application/octet-stream'});

  var naclModule;
  var decompressor;
  var blobContents;
  var onSuccessSpy;
  var onErrorSpy;

  beforeEach(function(done) {
    naclModule = {postMessage: sinon.spy()};
    decompressor = new unpacker.Decompressor(naclModule, FILE_SYSTEM_ID, BLOB);
    onSuccessSpy = sinon.spy();
    onErrorSpy = sinon.spy();

    // Load BLOB contents.
    var reader = new FileReader();
    reader.onload = function(event) {
      blobContents = event.target.result;
      done();
    };
    reader.readAsArrayBuffer(BLOB);
  });

  it('should not have any requests in progress if no method was called',
     function() {
       expect(Object.keys(decompressor.requestsInProgress).length).to.equal(0);
     });

  // Test readMetadata.
  describe('that reads metadata', function() {
    beforeEach(function() {
      decompressor.readMetadata(
          METADATA_REQUEST_ID, ENCODING, onSuccessSpy, onErrorSpy);
    });

    it('should add a new request in progress', function() {
      expect(decompressor.requestsInProgress[METADATA_REQUEST_ID])
          .to.not.be.undefined;
    });

    it('should call naclModule.postMessage once', function() {
      expect(naclModule.postMessage.calledOnce).to.be.true;
    });

    it('should call naclModule.postMessage with read metadata request',
       function() {
         var readMetadataRequest = unpacker.request.createReadMetadataRequest(
             FILE_SYSTEM_ID, METADATA_REQUEST_ID, ENCODING, BLOB.size);
         expect(naclModule.postMessage.calledWith(readMetadataRequest))
             .to.be.true;
       });

    // Test READ_METADATA_DONE.
    describe(
        'and receives a processMessage with READ_METADATA_DONE', function() {
          var data = {};
          beforeEach(function() {
            data[unpacker.request.Key.METADATA] = 'metadata';  // Not important.
            decompressor.processMessage(
                data, unpacker.request.Operation.READ_METADATA_DONE,
                METADATA_REQUEST_ID);
          });

          it('should call onSuccess with the metadata', function() {
            expect(onSuccessSpy.calledWith(data[unpacker.request.Key.METADATA]))
                .to.be.true;
            expect(onSuccessSpy.calledOnce).to.be.true;
          });

          it('should not call onError', function() {
            expect(onErrorSpy.called).to.be.false;
          });

          it('should remove the request in progress', function() {
            expect(decompressor.requestsInProgress[METADATA_REQUEST_ID])
                .to.be.undefined;
          });
        });

    // Test READ_CHUNK.
    describe('and receives a processMessage with READ_CHUNK', function() {
      var data = {};

      describe('that has length < file.size - offset', function() {
        it('should call naclModule.postMessage with READ_CHUNK_DONE response',
           function(done) {
             var expectedResponse =
                 unpacker.request.createReadChunkDoneResponse(
                     FILE_SYSTEM_ID, METADATA_REQUEST_ID, blobContents, 0);
             data[unpacker.request.Key.OFFSET] =
                 '0';  // Received as string from NaCl.
             data[unpacker.request.Key.LENGTH] = BLOB.size / 2;

             naclModule.postMessage = function(response) {
               expect(response).to.deep.equal(expectedResponse);
               done();
             };
             decompressor.processMessage(
                 data, unpacker.request.Operation.READ_CHUNK,
                 METADATA_REQUEST_ID);
           });
      });

      describe('that has length > file.size - offset', function() {
        it('should call naclModule.postMessage with READ_CHUNK_DONE response',
           function(done) {
             var expectedResponse =
                 unpacker.request.createReadChunkDoneResponse(
                     FILE_SYSTEM_ID, METADATA_REQUEST_ID, blobContents, 0);
             data[unpacker.request.Key.OFFSET] =
                 '0';  // Received as string from NaCl.
             data[unpacker.request.Key.LENGTH] = BLOB.size * 2;

             naclModule.postMessage = function(response) {
               expect(response).to.deep.equal(expectedResponse);
               done();
             };
             decompressor.processMessage(
                 data, unpacker.request.Operation.READ_CHUNK,
                 METADATA_REQUEST_ID);
           });
      });
    });

    // Test FILE_SYSTEM_ERROR.
    describe(
        'and receives a processMessage with FILE_SYSTEM_ERROR', function() {
          beforeEach(function() {
            var data = {};
            data[unpacker.request.Key.ERROR] =
                'Expected error at reading metadata.';
            decompressor.processMessage(
                data, unpacker.request.Operation.FILE_SYSTEM_ERROR,
                METADATA_REQUEST_ID);
          });

          it('should not call onSuccess', function() {
            expect(onSuccessSpy.called).to.be.false;
          });

          it('should call onError with FAILED', function() {
            expect(onErrorSpy.calledWith('FAILED')).to.be.true;
          });

          it('should remove the request in progress', function() {
            expect(decompressor.requestsInProgress[METADATA_REQUEST_ID])
                .to.be.undefined;
          });
        });
  });  // Test readMetadata.

  // Test openFile.
  describe('that opens a file', function() {
    beforeEach(function() {
      decompressor.openFile(
          OPEN_REQUEST_ID, FILE_PATH, ENCODING, onSuccessSpy, onErrorSpy);
    });

    it('should add a new open request in progress', function() {
      expect(decompressor.requestsInProgress[OPEN_REQUEST_ID])
          .to.not.be.undefined;
    });

    it('should call naclModule.postMessage once', function() {
      expect(naclModule.postMessage.calledOnce).to.be.true;
    });

    it('should call naclModule.postMessage with open file request', function() {
      var openFileRequest = unpacker.request.createOpenFileRequest(
          FILE_SYSTEM_ID, OPEN_REQUEST_ID, FILE_PATH, ENCODING, BLOB.size);
      expect(naclModule.postMessage.calledWith(openFileRequest)).to.be.true;
    });

    // Test OPEN_FILE_DONE.
    describe('and receives a processMessage with OPEN_FILE_DONE', function() {
      beforeEach(function() {
        decompressor.processMessage(
            {} /* Not important. */, unpacker.request.Operation.OPEN_FILE_DONE,
            OPEN_REQUEST_ID);
      });

      it('should call onSuccess once', function() {
        expect(onSuccessSpy.calledOnce).to.be.true;
      });

      it('should not call onError', function() {
        expect(onErrorSpy.called).to.be.false;
      });

      it('should NOT remove the request in progress', function() {
        expect(decompressor.requestsInProgress[OPEN_REQUEST_ID])
            .to.not.be.undefined;
      });
    });

    // Test closeFile.
    describe('and then closes it', function() {
      beforeEach(function() {
        // Restore spies.
        onSuccessSpy = sinon.spy();
        onErrorSpy = sinon.spy();

        decompressor.closeFile(
            CLOSE_REQUEST_ID, OPEN_REQUEST_ID, onSuccessSpy, onErrorSpy);
      });

      it('should add a new close request in progress', function() {
        expect(decompressor.requestsInProgress[CLOSE_REQUEST_ID])
            .to.not.be.undefined;
      });

      it('should call naclModule.postMessage twice', function() {
        // First time was for openFile.
        expect(naclModule.postMessage.calledTwice).to.be.true;
      });

      it('should call naclModule.postMessage with close file request',
         function() {
           var closeFileRequest = unpacker.request.createCloseFileRequest(
               FILE_SYSTEM_ID, CLOSE_REQUEST_ID, OPEN_REQUEST_ID);
           expect(naclModule.postMessage.calledWith(closeFileRequest))
               .to.be.true;
         });

      describe(
          'and receives a processMessage with CLOSE_FILE_DONE', function() {
            var data = {};
            beforeEach(function() {
              data[unpacker.request.Key.OPEN_REQUEST_ID] = OPEN_REQUEST_ID;
              decompressor.processMessage(
                  data, unpacker.request.Operation.CLOSE_FILE_DONE,
                  CLOSE_REQUEST_ID);
            });

            it('should call onSuccess once', function() {
              expect(onSuccessSpy.calledOnce).to.be.true;
            });

            it('should not call onError', function() {
              expect(onErrorSpy.called).to.be.false;
            });

            it('should remove the request in progress for open operation',
               function() {
                 expect(decompressor.requestsInProgress[OPEN_REQUEST_ID])
                     .to.be.undefined;
               });

            it('should remove the request in progress for close operation',
               function() {
                 expect(decompressor.requestsInProgress[CLOSE_REQUEST_ID])
                     .to.be.undefined;
               });
          });
    });  // Test closeFile.

    // Test readFile.
    describe('and then reads it', function() {
      beforeEach(function() {
        // Restore spies.
        onSuccessSpy = sinon.spy();
        onErrorSpy = sinon.spy();

        decompressor.readFile(
            READ_REQUEST_ID, OPEN_REQUEST_ID, OFFSET, LENGTH, onSuccessSpy,
            onErrorSpy);
      });

      it('should add a new read request in progress', function() {
        expect(decompressor.requestsInProgress[READ_REQUEST_ID])
            .to.not.be.undefined;
      });

      it('should call naclModule.postMessage twice', function() {
        // First time was for openFile.
        expect(naclModule.postMessage.calledTwice).to.be.true;
      });

      it('should call naclModule.postMessage with read file request',
         function() {
           var readFileRequest = unpacker.request.createReadFileRequest(
               FILE_SYSTEM_ID, READ_REQUEST_ID, OPEN_REQUEST_ID, OFFSET,
               LENGTH);
           expect(naclModule.postMessage.calledWith(readFileRequest))
               .to.be.true;
         });

      describe('and receives a processMessage with READ_FILE_DONE', function() {
        describe('that has more data to read', function() {
          var data = {};
          beforeEach(function() {
            data[unpacker.request.Key.READ_FILE_DATA] = 'data';
            data[unpacker.request.Key.HAS_MORE_DATA] = true;
            decompressor.processMessage(
                data, unpacker.request.Operation.READ_FILE_DONE,
                READ_REQUEST_ID);
          });

          it('should call onSuccess with file data and hasMore as true',
             function() {
               expect(onSuccessSpy.calledWith('data', true)).to.be.true;
             });

          it('should not call onError', function() {
            expect(onErrorSpy.called).to.be.false;
          });

          it('should NOT remove the request in progress for open operation',
             function() {
               expect(decompressor.requestsInProgress[OPEN_REQUEST_ID])
                   .to.not.be.undefined;
             });

          it('should NOT remove the request in progress for read operation',
             function() {
               // decompressor will still receive READ_FILE_DONE.
               expect(decompressor.requestsInProgress[READ_REQUEST_ID])
                   .to.not.be.undefined;
             });
        });

        describe('that doesn\'t have any more data to read', function() {
          var data = {};
          beforeEach(function() {
            data[unpacker.request.Key.READ_FILE_DATA] = 'data';
            data[unpacker.request.Key.HAS_MORE_DATA] = false;
            decompressor.processMessage(
                data, unpacker.request.Operation.READ_FILE_DONE,
                READ_REQUEST_ID);
          });

          it('should call onSuccess with file data and hasMore as false',
             function() {
               expect(onSuccessSpy.calledWith('data', false)).to.be.true;
             });

          it('should not call onError', function() {
            expect(onErrorSpy.called).to.be.false;
          });

          it('should NOT remove the request in progress for open operation',
             function() {
               expect(decompressor.requestsInProgress[OPEN_REQUEST_ID])
                   .to.not.be.undefined;
             });

          it('should remove the request in progress for read operation',
             function() {
               // Last call, so it should be removed.
               expect(decompressor.requestsInProgress[READ_REQUEST_ID])
                   .to.be.undefined;
             });
        });
      });  // Test READ_FILE_DONE.
    });    // Test readFile.
  });      // Test openFile.
});
