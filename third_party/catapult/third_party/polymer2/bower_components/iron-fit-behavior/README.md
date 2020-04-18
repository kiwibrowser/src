[![Build status](https://travis-ci.org/PolymerElements/iron-fit-behavior.svg?branch=master)](https://travis-ci.org/PolymerElements/iron-fit-behavior)

_[Demo and API docs](https://elements.polymer-project.org/elements/iron-fit-behavior)_


## Polymer.IronFitBehavior

`Polymer.IronFitBehavior` fits an element in another element using `max-height` and `max-width`, and
optionally centers it in the window or another element.

The element will only be sized and/or positioned if it has not already been sized and/or positioned
by CSS.

| CSS properties | Action |
| --- | --- |
| `position` set | Element is not centered horizontally or vertically |
| `top` or `bottom` set | Element is not vertically centered |
| `left` or `right` set | Element is not horizontally centered |
| `max-height` set | Element respects `max-height` |
| `max-width` set | Element respects `max-width` |

`Polymer.IronFitBehavior` can position an element into another element using
`verticalAlign` and `horizontalAlign`. This will override the element's css position.

```html
  <div class="container">
    <iron-fit-impl vertical-align="top" horizontal-align="auto">
      Positioned into the container
    </iron-fit-impl>
  </div>
```

Use `noOverlap` to position the element around another element without overlapping it.

```html
  <div class="container">
    <iron-fit-impl no-overlap vertical-align="auto" horizontal-align="auto">
      Positioned around the container
    </iron-fit-impl>
  </div>
```


