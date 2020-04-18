// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.XhrManagerTest');
goog.setTestOnly('mr.XhrManagerTest');

const XhrManager = goog.require('mr.XhrManager');

describe('XhrManager Tests', function() {
  const MAX_REQUESTS = 5;
  const DEFAULT_TIMEOUT = 10;
  const DEFAULT_ATTEMPTS = 3;
  const SEND_URL = 'https://www.google.com';
  const BODY = 'body';

  let mockXhr;
  let xhrManager;

  beforeEach(() => {
    mockXhr = jasmine.createSpyObj('Xhr', ['open', 'send', 'setRequestHeader']);
    xhrManager =
        new XhrManager(MAX_REQUESTS, DEFAULT_TIMEOUT, DEFAULT_ATTEMPTS);
  });

  it('Resolves on first try with defaults', done => {
    spyOn(window, 'XMLHttpRequest').and.returnValue(mockXhr);
    mockXhr.open.and.callFake((method, url, async) => {
      expect(mockXhr.onreadystatechange).toBeDefined();
      expect(mockXhr.timeout).toEqual(DEFAULT_TIMEOUT);
      expect(mockXhr.ontimeout).toBeDefined();
      expect(method).toEqual('POST');
      expect(url).toEqual(SEND_URL);
      expect(async).toBe(true);
    });
    const headersThatWereSet = [];
    mockXhr.setRequestHeader.and.callFake((key, value) => {
      headersThatWereSet.push([key, value]);
    });
    mockXhr.send.and.callFake(body => {
      expect(mockXhr.open).toHaveBeenCalled();
      expect(mockXhr.setRequestHeader).toHaveBeenCalled();
      expect(headersThatWereSet).toEqual([
        ['Content-Type', 'application/x-www-form-urlencoded;charset=utf-8']
      ]);
      expect(mockXhr.responseType).toBe('');
      expect(body).toEqual(BODY);

      setTimeout(() => {
        mockXhr.readyState = XMLHttpRequest.DONE;
        mockXhr.onreadystatechange();
      }, 0);
    });

    xhrManager.send(SEND_URL, 'POST', BODY).then(response => {
      expect(mockXhr.send).toHaveBeenCalled();
      done();
    });
  });

  it('Resolves on first try with overrides', done => {
    const overrides = {
      timeoutMillis: 1234,
      headers: [['Hello', 'World'], ['Eat', 'Cheese']],
      responseType: 'arraybuffer'
    };

    spyOn(window, 'XMLHttpRequest').and.returnValue(mockXhr);
    mockXhr.open.and.callFake((method, url, async) => {
      expect(mockXhr.onreadystatechange).toBeDefined();
      expect(mockXhr.timeout).toEqual(overrides.timeoutMillis);
      expect(mockXhr.ontimeout).toBeDefined();
      expect(method).toEqual('GET');
      expect(url).toEqual(SEND_URL);
      expect(async).toBe(true);
    });
    const headersThatWereSet = [];
    mockXhr.setRequestHeader.and.callFake((key, value) => {
      headersThatWereSet.push([key, value]);
    });
    mockXhr.send.and.callFake(body => {
      expect(mockXhr.open).toHaveBeenCalled();
      expect(mockXhr.setRequestHeader).toHaveBeenCalled();
      expect(headersThatWereSet).toEqual(overrides.headers);
      expect(mockXhr.responseType).toBe(overrides.responseType);
      expect(body).toEqual(BODY);

      setTimeout(() => {
        mockXhr.readyState = XMLHttpRequest.DONE;
        mockXhr.onreadystatechange();
      }, 0);
    });

    xhrManager.send(SEND_URL, 'GET', BODY, overrides).then(response => {
      expect(mockXhr.send).toHaveBeenCalled();
      done();
    });
  });

  it('Resolves on retry', done => {
    spyOn(window, 'XMLHttpRequest').and.returnValue(mockXhr);
    let numAttempts = 0;
    mockXhr.send.and.callFake(() => {
      expect(mockXhr.timeout).toBe(DEFAULT_TIMEOUT);
      ++numAttempts;
      if (numAttempts <= 1) {
        setTimeout(() => mockXhr.ontimeout(), DEFAULT_TIMEOUT);
      } else {
        setTimeout(() => {
          mockXhr.readyState = XMLHttpRequest.DONE;
          mockXhr.onreadystatechange();
        }, 0);
      }
    });

    xhrManager.send(SEND_URL, 'GET').then(() => {
      expect(numAttempts).toBe(2);
      done();
    });
  });

  it('Queues requests', done => {
    let xhrs = [];
    spyOn(window, 'XMLHttpRequest').and.callFake(() => {
      const mockXhr =
          jasmine.createSpyObj('Xhr', ['open', 'send', 'setRequestHeader']);
      mockXhr.send.and.callFake(() => {
        expect(xhrs.length).toBeLessThan(MAX_REQUESTS);
        xhrs.push(mockXhr);
      });
      return mockXhr;
    });

    let promises1 = [];
    let promises2 = [];
    for (let i = 0; i < MAX_REQUESTS; i++) {
      promises1.push(xhrManager.send(SEND_URL, 'POST', BODY));
    }
    for (let i = 0; i < MAX_REQUESTS; i++) {
      promises2.push(xhrManager.send(SEND_URL, 'POST', BODY));
    }

    // Resolve the first 5 requests.
    expect(xhrs.length).toBe(MAX_REQUESTS);
    while (xhrs.length > 0) {
      let xhr = xhrs.shift();
      setTimeout(() => {
        xhr.readyState = XMLHttpRequest.DONE;
        xhr.onreadystatechange();
      }, 0);
    }

    Promise.all(promises1).then(() => {
      // Resolve the next 5 requests.
      expect(xhrs.length).toBe(MAX_REQUESTS);
      while (xhrs.length > 0) {
        let xhr = xhrs.shift();
        setTimeout(() => {
          xhr.readyState = XMLHttpRequest.DONE;
          xhr.onreadystatechange();
        }, 0);
      }
      Promise.all(promises2).then(done);
    });
  });

  it('Rejects if all retries failed', done => {
    spyOn(window, 'XMLHttpRequest').and.returnValue(mockXhr);
    let numAttempts = 0;
    mockXhr.send.and.callFake((path, init) => {
      ++numAttempts;
      setTimeout(() => mockXhr.ontimeout(), DEFAULT_TIMEOUT);
    });

    xhrManager.send(SEND_URL, 'GET')
        .then(
            _ => {
              fail('send() unexpectedly succeeded.');
            },
            () => {
              expect(numAttempts).toBe(DEFAULT_ATTEMPTS);
              done();
            });
  });
});
