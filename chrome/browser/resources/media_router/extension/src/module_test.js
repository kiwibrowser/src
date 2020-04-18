// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly('module_test');

goog.require('mr.Module');
goog.require('mr.PromiseUtils');



describe('Tests modules', function() {
  let mockModule;

  beforeEach(function() {
    mockModule = {'id': 1};
  });

  afterEach(function() {
    mr.Module.clearForTest();
  });

  it('gets a module before and after it is loaded', function() {
    let module = mr.Module.get('SomeModule');
    expect(module).toBeNull();

    const mockModule2 = {'id': 2};
    mr.Module.onModuleLoaded('AnotherModule', mockModule2);

    module = mr.Module.get('SomeModule');
    expect(module).toBeNull();

    mr.Module.onModuleLoaded('SomeModule', mockModule);
    module = mr.Module.get('SomeModule');
    expect(module).toBe(mockModule);

    module = mr.Module.get('AnotherModule');
    expect(module).toBe(mockModule2);
  });

  it('loads a module which loads a bundle', function(done) {
    spyOn(mr.Module, 'getBundle_').and.returnValue('SomeBundle');
    spyOn(mr.Module, 'doLoadBundle_').and.returnValue(Promise.resolve());

    const promise = mr.Module.load('SomeModule');
    const promise2 = mr.Module.load('SomeModule');

    expect(mr.Module.getBundle_).toHaveBeenCalledWith('SomeModule');
    expect(mr.Module.doLoadBundle_).toHaveBeenCalledWith('SomeBundle');
    expect(mr.Module.doLoadBundle_.calls.count()).toBe(1);

    mr.Module.onModuleLoaded('SomeModule', mockModule);

    const promise3 = mr.Module.load('SomeModule');
    Promise.all([promise, promise2, promise3]).then(modules => {
      for (let module of modules) {
        expect(module).toBe(mockModule);
      }
      done();
    });
  });

  it('load rejects if failed to load a bundle', function(done) {
    spyOn(mr.Module, 'getBundle_').and.returnValue('SomeBundle');
    spyOn(mr.Module, 'doLoadBundle_')
        .and.returnValue(Promise.reject(new Error('failed to load bundle')));

    const promise = mr.Module.load('SomeModule');
    const promise2 = mr.Module.load('SomeModule');

    expect(mr.Module.getBundle_).toHaveBeenCalledWith('SomeModule');
    expect(mr.Module.doLoadBundle_).toHaveBeenCalledWith('SomeBundle');
    expect(mr.Module.doLoadBundle_.calls.count()).toBe(1);

    const promise3 = mr.Module.load('SomeModule');
    mr.PromiseUtils.allSettled([promise, promise2, promise3]).then(results => {
      for (let result of results) {
        expect(result.fulfilled).toBe(false);
      }
      done();
    });
  });

  it('each module is mapped to a bundle', () => {
    for (const key in mr.ModuleId) {
      expect(mr.Module.getBundle_(mr.ModuleId[key]))
          .not.toBeNull(`${mr.ModuleId[key]} does not map to a bundle`);
    }
  });
});
