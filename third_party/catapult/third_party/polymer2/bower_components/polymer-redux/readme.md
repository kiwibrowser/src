# Polymer Redux

[![Build Status](https://travis-ci.org/tur-nr/polymer-redux.svg?branch=master)](https://travis-ci.org/tur-nr/polymer-redux)
[![Coverage Status](https://coveralls.io/repos/github/tur-nr/polymer-redux/badge.svg?branch=master)](https://coveralls.io/github/tur-nr/polymer-redux?branch=master)

Polymer bindings for Redux. Bind store state to properties and dispatch
actions from within Polymer Elements.

Polymer is a modern library for creating Web Components within an application.
Redux is a state container for managing predictable data. Binding the two
libraries together allows developers to create powerful and complex
applications faster and simpler. This approach allows the components you build
with Polymer to be more focused on functionality than the applications state.

## Installation

```bash
bower install --save polymer-redux
```

## Example

```javascript
// Create a Redux store
const store = Redux.createStore((state = {}, action) => state)

// Create the PolymerRedux mixin
const ReduxMixin = PolymerRedux(store)

// Bind Elements using the mixin
class MyElement extends ReduxMixin(Polymer.Element) {
    static get is() {
        return 'my-element'
    }

    connectedCallback() {
        super.connectedCallback();
        const state = this.getState();
    }
}

// Define your Element
customElements.define(MyElement.is, MyElement)
```

Now `MyElement` has a connection to the Redux store and can bind properties to
it's state and dispatch actions.

## Documentation

See the documentation page: [https://tur-nr.github.io/polymer-redux/docs](https://tur-nr.github.io/polymer-redux/docs).

## API

#### PolymerRedux

##### `new PolymerRedux(<store>)`

* `store` Object, Redux store.

Returns a `ReduxMixin` function.

#### Redux Mixin

These methods are available on the instance of the component, the element.

##### `#getState()`

Returns current store's state.

##### `#dispatch(<name>, [args, ...])`

* `name` String, action name in the actions list.
* `arg...` *, Arguments to pass to action function.

Returns the action object.


##### `#dispatch(<fn>)`

* `fn` Function, Redux middleware dispatch function.

Returns the action object.


##### `#dispatch(<action>)`

* `action` Object, the action object.

Returns the action object.


#### Events

##### `state-changed`

Fires when the store's state has changed.

## License

[MIT](LICENSE)

Copyright (c) 2017 [Christopher Turner](https://github.com/tur-nr)
