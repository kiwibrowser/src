[![Build status](https://travis-ci.org/PolymerElements/iron-overlay-behavior.svg?branch=master)](https://travis-ci.org/PolymerElements/iron-overlay-behavior)
[![Published on webcomponents.org](https://img.shields.io/badge/webcomponents.org-published-blue.svg)](https://www.webcomponents.org/element/PolymerElements/iron-overlay-behavior)

_[Demo and API docs](https://elements.polymer-project.org/elements/iron-overlay-behavior)_


## Polymer.IronOverlayBehavior

Use `Polymer.IronOverlayBehavior` to implement an element that can be hidden or shown, and displays
on top of other content. It includes an optional backdrop, and can be used to implement a variety
of UI controls including dialogs and drop downs. Multiple overlays may be displayed at once.

See the [demo source code](https://github.com/PolymerElements/iron-overlay-behavior/blob/master/demo/simple-overlay.html)
for an example.

### Changes in 2.0
- Focus wrapping is not guaranteed to work for elements with `tabindex > 0`, see more details [here](https://github.com/PolymerElements/iron-overlay-behavior/pull/241).
Consider overriding [`_focusableNodes`](http://jsbin.com/siwutox/1/edit) or using the [Blocking Elements polyfill](https://github.com/PolymerLabs/blockingElements).

### Closing and canceling

An overlay may be hidden by closing or canceling. The difference between close and cancel is user
intent. Closing generally implies that the user acknowledged the content on the overlay. By default,
it will cancel whenever the user taps outside it or presses the escape key. This behavior is
configurable with the `no-cancel-on-esc-key` and the `no-cancel-on-outside-click` properties.
`close()` should be called explicitly by the implementer when the user interacts with a control
in the overlay element. When the dialog is canceled, the overlay fires an 'iron-overlay-canceled'
event. Call `preventDefault` on this event to prevent the overlay from closing.

### Positioning

By default the element is sized and positioned to fit and centered inside the window. You can
position and size it manually using CSS. See `Polymer.IronFitBehavior`.

### Backdrop

Set the `with-backdrop` attribute to display a backdrop behind the overlay. The backdrop is
appended to `<body>` and is of type `<iron-overlay-backdrop>`. See its doc page for styling
options.

In addition, `with-backdrop` will wrap the focus within the content in the light DOM.
Override the [`_focusableNodes` getter](#Polymer.IronOverlayBehavior:property-_focusableNodes)
to achieve a different behavior.

### Limitations

The element is styled to appear on top of other content by setting its `z-index` property. You
must ensure no element has a stacking context with a higher `z-index` than its parent stacking
context. You should place this element as a child of `<body>` whenever possible.

## &lt;iron-overlay-backdrop&gt;

`iron-overlay-backdrop` is a backdrop used by `Polymer.IronOverlayBehavior`. It should be a
singleton.

### Styling

The following custom properties and mixins are available for styling.

| Custom property | Description | Default |
| --- | --- | --- |
| `--iron-overlay-backdrop-background-color` | Backdrop background color | #000 |
| `--iron-overlay-backdrop-opacity` | Backdrop opacity | 0.6 |
| `--iron-overlay-backdrop` | Mixin applied to `iron-overlay-backdrop`. | {} |
| `--iron-overlay-backdrop-opened` | Mixin applied to `iron-overlay-backdrop` when it is displayed | {} |


