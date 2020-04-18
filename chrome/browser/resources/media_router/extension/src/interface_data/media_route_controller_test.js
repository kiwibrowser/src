goog.module('mr.MediaRouteControllerTest');
goog.setTestOnly('mr.MediaRouteControllerTest');

const MediaRouteController = goog.require('mr.MediaRouteController');
const UnitTestUtils = goog.require('mr.UnitTestUtils');

describe('MediaRouteController test', () => {
  let binding;
  let observer;

  beforeEach(() => {
    UnitTestUtils.mockMojoApi();
    binding = UnitTestUtils.createMojoBindingSpyObj();
    observer = UnitTestUtils.createMojoMediaStatusObserverSpyObj();
    spyOn(mojo, 'Binding').and.returnValue(binding);
  });

  function createController() {
    const controllerRequest = {};
    const controller =
        new MediaRouteController('TestController', controllerRequest, observer);
    expect(mojo.Binding)
        .toHaveBeenCalledWith(
            mojo.MediaController, controller, controllerRequest);
    expect(binding.setConnectionErrorHandler).toHaveBeenCalled();
    expect(observer.ptr.setConnectionErrorHandler).toHaveBeenCalled();
    return controller;
  }

  it('Sets things up on construction', () => {
    createController();
  });

  it('Cleans up on Mojo connection error', () => {
    const controller = createController();
    spyOn(controller, 'onControllerInvalidated').and.callThrough();
    const onMojoConnectionError =
        binding.setConnectionErrorHandler.calls.argsFor(0)[0];
    onMojoConnectionError();
    expect(binding.close).toHaveBeenCalled();
    expect(observer.ptr.reset).toHaveBeenCalled();
    expect(controller.onControllerInvalidated.calls.count()).toBe(1);
  });

  it('Cleans up on calling dispose()', () => {
    const controller = createController();
    spyOn(controller, 'disposeInternal').and.callThrough();
    controller.dispose();
    expect(binding.close).toHaveBeenCalled();
    expect(observer.ptr.reset).toHaveBeenCalled();
    expect(controller.disposeInternal.calls.count()).toBe(1);
    // Calling dispose twice has no effect.
    controller.dispose();
    expect(controller.disposeInternal.calls.count()).toBe(1);
  });

  it('Notifies observer', () => {
    const controller = createController();
    controller.notifyObserver();
    expect(observer.onMediaStatusUpdated)
        .toHaveBeenCalledWith(controller.currentMediaStatus);
  });

  it('Does not notify observer after cleanup', () => {
    const controller = createController();
    controller.dispose();
    controller.notifyObserver();
    expect(observer.onMediaStatusUpdated).not.toHaveBeenCalled();
  });
});
