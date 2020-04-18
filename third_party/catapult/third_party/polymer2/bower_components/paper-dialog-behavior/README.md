[![Build status](https://travis-ci.org/PolymerElements/paper-dialog-behavior.svg?branch=master)](https://travis-ci.org/PolymerElements/paper-dialog-behavior)
[![Published on webcomponents.org](https://img.shields.io/badge/webcomponents.org-published-blue.svg)](https://www.webcomponents.org/element/PolymerElements/paper-dialog-behavior)

## Polymer.PaperDialogBehavior

Use `Polymer.PaperDialogBehavior` and `paper-dialog-shared-styles.html` to implement a Material Design
dialog.

For example, if `<paper-dialog-impl>` implements this behavior:

```html
<paper-dialog-impl>
    <h2>Header</h2>
    <div>Dialog body</div>
    <div class="paper-dialog-buttons">
        <paper-button dialog-dismiss>Cancel</paper-button>
        <paper-button dialog-confirm>Accept</paper-button>
    </div>
</paper-dialog-impl>
```

`paper-dialog-shared-styles.html` provide styles for a header, content area, and an action area for buttons.
Use the `<h2>` tag for the header and the `paper-dialog-buttons` or `buttons` class for the action area. You can use the
`paper-dialog-scrollable` element (in its own repository) if you need a scrolling content area.

Use the `dialog-dismiss` and `dialog-confirm` attributes on interactive controls to close the
dialog. If the user dismisses the dialog with `dialog-confirm`, the `closingReason` will update
to include `confirmed: true`.

### Changes in 2.0
- `paper-dialog-shared-styles` styles direct `h2` and `.buttons` children of the dialog because of how [`::slotted` works](https://developers.google.com/web/fundamentals/primers/shadowdom/?hl=en#stylinglightdom)
(compound selector will select only top level nodes)
- Removed `paper-dialog-common.css` as it's a duplicate of `paper-dialog-shared-styles.html`.
Import the shared styles via `<style include="paper-dialog-shared-styles">` ([see example](demo/simple-dialog.html))

### Accessibility

This element has `role="dialog"` by default. Depending on the context, it may be more appropriate
to override this attribute with `role="alertdialog"`.

If `modal` is set, the element will prevent the focus from exiting the element.
It will also ensure that focus remains in the dialog.


