// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This module implements the shared functionality for different guestview
// containers, such as web_view, app_view, etc.

var DocumentNatives = requireNative('document_natives');
var GuestView = require('guestView').GuestView;
var GuestViewInternalNatives = requireNative('guest_view_internal');
var IdGenerator = requireNative('id_generator');
var MessagingNatives = requireNative('messaging_natives');

function GuestViewContainer(element, viewType) {
  privates(element).internal = this;
  this.attributes = {};
  this.element = element;
  this.elementAttached = false;
  this.viewInstanceId = IdGenerator.GetNextId();
  this.viewType = viewType;

  this.setupGuestProperty();
  this.guest = new GuestView(viewType);
  this.setupAttributes();

  privates(this).internalElement = this.createInternalElement$();
  this.setupFocusPropagation();
  var shadowRoot = this.element.createShadowRoot();
  shadowRoot.appendChild(privates(this).internalElement);

  GuestViewInternalNatives.RegisterView(this.viewInstanceId, this, viewType);
}

// Prevent GuestViewContainer inadvertently inheriting code from the global
// Object, allowing a pathway for executing unintended user code execution.
// TODO(wjmaclean): Use utils.expose() here instead? Track down other issues
// of Object inheritance. https://crbug.com/701034
GuestViewContainer.prototype.__proto__ = null;

// Forward public API methods from |proto| to their internal implementations.
GuestViewContainer.forwardApiMethods = function(proto, apiMethods) {
  var createProtoHandler = function(m) {
    return function(var_args) {
      var internal = privates(this).internal;
      return $Function.apply(internal[m], internal, arguments);
    };
  };
  for (var i = 0; apiMethods[i]; ++i) {
    proto[apiMethods[i]] = createProtoHandler(apiMethods[i]);
  }
};

// Registers the browserplugin and guestview as custom elements once the
// document has loaded.
GuestViewContainer.registerElement = function(guestViewContainerType) {
  var useCapture = true;
  window.addEventListener('readystatechange', function listener(event) {
    if (document.readyState == 'loading')
      return;

    registerInternalElement(
        $String.toLowerCase(guestViewContainerType.VIEW_TYPE));
    registerGuestViewElement(guestViewContainerType);
    window.removeEventListener(event.type, listener, useCapture);
  }, useCapture);
};

// Create the 'guest' property to track new GuestViews and always listen for
// their resizes.
GuestViewContainer.prototype.setupGuestProperty = function() {
  $Object.defineProperty(this, 'guest', {
    get: $Function.bind(function() {
      return privates(this).guest;
    }, this),
    set: $Function.bind(function(value) {
      privates(this).guest = value;
      if (!value) {
        return;
      }
      privates(this).guest.onresize = $Function.bind(function(e) {
        // Dispatch the 'contentresize' event.
        var contentResizeEvent = new Event('contentresize', { bubbles: true });
        contentResizeEvent.oldWidth = e.oldWidth;
        contentResizeEvent.oldHeight = e.oldHeight;
        contentResizeEvent.newWidth = e.newWidth;
        contentResizeEvent.newHeight = e.newHeight;
        this.dispatchEvent(contentResizeEvent);
      }, this);
    }, this),
    enumerable: true
  });
};

GuestViewContainer.prototype.createInternalElement$ = function() {
  // We create BrowserPlugin as a custom element in order to observe changes
  // to attributes synchronously.
  var browserPluginElement =
      new GuestViewContainer[this.viewType + 'BrowserPlugin']();
  privates(browserPluginElement).internal = this;
  return browserPluginElement;
};

GuestViewContainer.prototype.prepareForReattach_ = function() {};

GuestViewContainer.prototype.setupFocusPropagation = function() {
  if (!this.element.hasAttribute('tabIndex')) {
    // GuestViewContainer needs a tabIndex in order to be focusable.
    // TODO(fsamuel): It would be nice to avoid exposing a tabIndex attribute
    // to allow GuestViewContainer to be focusable.
    // See http://crbug.com/231664.
    this.element.setAttribute('tabIndex', -1);
  }
};

GuestViewContainer.prototype.focus = function() {
  // Focus the internal element when focus() is called on the GuestView element.
  privates(this).internalElement.focus();
}

GuestViewContainer.prototype.attachWindow$ = function() {
  if (!this.internalInstanceId) {
    return true;
  }

  this.guest.attach(this.internalInstanceId,
                    this.viewInstanceId,
                    this.buildParams());
  return true;
};

GuestViewContainer.prototype.makeGCOwnContainer = function(internalInstanceId) {
  MessagingNatives.BindToGC(this, function() {
    GuestViewInternalNatives.DestroyContainer(internalInstanceId);
  }, -1);
};

GuestViewContainer.prototype.onInternalInstanceId = function(
    internalInstanceId) {
  this.internalInstanceId = internalInstanceId;
  this.makeGCOwnContainer(this.internalInstanceId);

  // Track when the element resizes using the element resize callback.
  GuestViewInternalNatives.RegisterElementResizeCallback(
      this.internalInstanceId, this.weakWrapper(this.onElementResize));

  if (!this.guest.getId()) {
    return;
  }
  this.guest.attach(this.internalInstanceId,
                    this.viewInstanceId,
                    this.buildParams());
};

GuestViewContainer.prototype.handleInternalElementAttributeMutation =
    function(name, oldValue, newValue) {
  if (name == 'internalinstanceid' && !oldValue && !!newValue) {
    privates(this).internalElement.removeAttribute('internalinstanceid');
    this.onInternalInstanceId(parseInt(newValue));
  }
};

GuestViewContainer.prototype.onElementResize = function(newWidth, newHeight) {
  if (!this.guest.getId())
    return;
  this.guest.setSize({normal: {width: newWidth, height: newHeight}});
};

GuestViewContainer.prototype.buildParams = function() {
  var params = this.buildContainerParams();
  params['instanceId'] = this.viewInstanceId;
  // When the GuestViewContainer is not participating in layout (display:none)
  // then getBoundingClientRect() would report a width and height of 0.
  // However, in the case where the GuestViewContainer has a fixed size we can
  // use that value to initially size the guest so as to avoid a relayout of the
  // on display:block.
  var css = window.getComputedStyle(this.element, null);
  var elementRect = this.element.getBoundingClientRect();
  params['elementWidth'] = parseInt(elementRect.width) ||
      parseInt(css.getPropertyValue('width'));
  params['elementHeight'] = parseInt(elementRect.height) ||
      parseInt(css.getPropertyValue('height'));
  return params;
};

GuestViewContainer.prototype.dispatchEvent = function(event) {
  return this.element.dispatchEvent(event);
}

// Returns a wrapper function for |func| with a weak reference to |this|.
GuestViewContainer.prototype.weakWrapper = function(func) {
  var viewInstanceId = this.viewInstanceId;
  return function() {
    var view = GuestViewInternalNatives.GetViewFromID(viewInstanceId);
    if (view) {
      return $Function.apply(func, view, $Array.slice(arguments));
    }
  };
};

// Implemented by the specific view type, if needed.
GuestViewContainer.prototype.buildContainerParams = function() { return {}; };
GuestViewContainer.prototype.willAttachElement = function() {};
GuestViewContainer.prototype.onElementAttached = function() {};
GuestViewContainer.prototype.onElementDetached = function() {};
GuestViewContainer.prototype.setupAttributes = function() {};

// Registers the browser plugin <object> custom element. |viewType| is the
// name of the specific guestview container (e.g. 'webview').
function registerInternalElement(viewType) {
  var proto = $Object.create(HTMLElement.prototype);

  proto.createdCallback = function() {
    this.setAttribute('type', 'application/browser-plugin');
    this.setAttribute('id', 'browser-plugin-' + IdGenerator.GetNextId());
    this.style.width = '100%';
    this.style.height = '100%';
  };

  proto.attachedCallback = function() {
    // Load the plugin immediately.
    var unused = this.nonExistentAttribute;
  };

  proto.attributeChangedCallback = function(name, oldValue, newValue) {
    var internal = privates(this).internal;
    if (!internal) {
      return;
    }
    internal.handleInternalElementAttributeMutation(name, oldValue, newValue);
  };

  GuestViewContainer[viewType + 'BrowserPlugin'] =
      DocumentNatives.RegisterElement(viewType + 'browserplugin',
                                      {extends: 'object', prototype: proto});

  delete proto.createdCallback;
  delete proto.attachedCallback;
  delete proto.detachedCallback;
  delete proto.attributeChangedCallback;
};

// Registers the guestview container as a custom element.
// |guestViewContainerType| is the type of guestview container
// (e.g. WebViewImpl).
function registerGuestViewElement(guestViewContainerType) {
  var proto = $Object.create(HTMLElement.prototype);

  proto.createdCallback = function() {
    new guestViewContainerType(this);
  };

  proto.attachedCallback = function() {
    var internal = privates(this).internal;
    if (!internal) {
      return;
    }
    internal.elementAttached = true;
    internal.willAttachElement();
    internal.onElementAttached();
  };

  proto.attributeChangedCallback = function(name, oldValue, newValue) {
    var internal = privates(this).internal;
    if (!internal || !internal.attributes[name]) {
      return;
    }

    // Let the changed attribute handle its own mutation.
    internal.attributes[name].maybeHandleMutation(oldValue, newValue);
  };

  proto.detachedCallback = function() {
    var internal = privates(this).internal;
    if (!internal) {
      return;
    }
    internal.elementAttached = false;
    internal.internalInstanceId = 0;
    internal.guest.destroy();
    internal.onElementDetached();
  };

  // Override |focus| to let |internal| handle it.
  proto.focus = function() {
    var internal = privates(this).internal;
    if (!internal) {
      return;
    }
    internal.focus();
  };

  // Let the specific view type add extra functionality to its custom element
  // through |proto|.
  if (guestViewContainerType.setupElement) {
    guestViewContainerType.setupElement(proto);
  }

  window[guestViewContainerType.VIEW_TYPE] = DocumentNatives.RegisterElement(
      $String.toLowerCase(guestViewContainerType.VIEW_TYPE),
      {prototype: proto});

  // Delete the callbacks so developers cannot call them and produce unexpected
  // behavior.
  delete proto.createdCallback;
  delete proto.attachedCallback;
  delete proto.detachedCallback;
  delete proto.attributeChangedCallback;
}

// Exports.
exports.$set('GuestViewContainer', GuestViewContainer);
