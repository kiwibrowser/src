[![Build status](https://travis-ci.org/PolymerElements/paper-toast.svg?branch=master)](https://travis-ci.org/PolymerElements/paper-toast)
[![Published on webcomponents.org](https://img.shields.io/badge/webcomponents.org-published-blue.svg)](https://www.webcomponents.org/element/PolymerElements/paper-toast)

## &lt;paper-toast&gt;

Material design: [Snackbars & toasts](https://www.google.com/design/spec/components/snackbars-toasts.html)

`paper-toast` provides a subtle notification toast. Only one `paper-toast` will
be visible on screen.

Use `opened` to show the toast:

Example:

```html
<paper-toast text="Hello world!" opened></paper-toast>
```

Also `open()` or `show()` can be used to show the toast:

Example:

```html
<paper-button on-click="openToast">Open Toast</paper-button>
<paper-toast id="toast" text="Hello world!"></paper-toast>

...

openToast: function() {
  this.$.toast.open();
}
```

Set `duration` to 0, a negative number or Infinity to persist the toast on screen:

Example:

```html
<paper-toast text="Terms and conditions" opened duration="0">
  <a href="#">Show more</a>
</paper-toast>
```

`<paper-toast>` is affected by the [stacking context](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Positioning/Understanding_z_index/The_stacking_context) of its container. Adding `<paper-toast>` inside elements that create a new stacking context - e.g. `<app-drawer>`, `<app-layout>` or `<iron-list>` - might result in toasts partially obstructed or clipped. Add `<paper-toast>` to the top level (`<body>`) element, outside the structure, e.g.:

```html
  <!-- ... -->
  </app-drawer-layout>
  <paper-toast id="toast"></paper-toast>
</template>
```

You can then use custom events to communicate with it from within child components, using `addEventListener` and `dispatchEvent`.

### Styling

The following custom properties and mixins are available for styling:

| Custom property | Description | Default |
| --- | --- | --- |
| `--paper-toast-background-color` | The paper-toast background-color | `#323232` |
| `--paper-toast-color` | The paper-toast color | `#f1f1f1` |

This element applies the mixin `--paper-font-common-base` but does not import `paper-styles/typography.html`.
In order to apply the `Roboto` font to this element, make sure you've imported `paper-styles/typography.html`.


