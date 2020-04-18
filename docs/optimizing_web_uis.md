# Optimizing Chrome Web UIs

## How do I do it?

In order to build with a fast configuration, try setting these options in your
GN args:

```
optimize_webui = true
is_debug = false
```

## How is the code optimized?

### Resource combination

[HTML imports](https://www.html5rocks.com/en/tutorials/webcomponents/imports/)
are a swell technology, but can be used is slow ways.  Each import may also
contain additional imports, which must be satisfied before certain things can
continue (i.e. script execution may be paused).

```html
<!-- If a.html contains more imports... -->
<link rel="import" href="a.html">
<!-- This script is blocked until done. -->
<script> startThePageUp(); </script>
```

To reduce this latency, Chrome uses a tool created by the Polymer project named
[polymer-bundler](https://github.com/Polymer/polymer-bundler).  It processes
a page starting from a URL entry point and inlines resources the first time
they're encountered.  This greatly decreases latency due to HTML imports.

```html
<!-- Contents of a.html and all its dependencies. -->
<script> startThePageUp(); </script>
```

### CSS @apply to --var transformation

We also use
[polymer-css-build](https://github.com/PolymerLabs/polymer-css-build) to
transform CSS @apply mixins (which are not yet natively supported) into faster
--css-variables.  This turns something like this:

```css
:host {
  --mixin-name: {
    color: red;
    display: block;
  };
}
/* In a different place */
.red-thing {
  @apply(--mixin-name);
}
```

into the more performant:

```css
:host {
  --mixin-name_-_color: red;
  --mixin-name_-_display: block;
}
/* In a different place */
.red-thing {
  color: var(--mixin-name_-_color);
  display: var(--mixin-name_-_display);
}
```

### JavaScript Minification

In order to minimize disk size, we run
[uglifyjs](https://github.com/mishoo/UglifyJS2) on all combined JavaScript. This
reduces installer and the size of resources required to load to show a UI.

Code like this:

```js
function fizzBuzz() {
  for (var i = 1; i <= 100; i++) {
    var fizz = i % 3 == 0 ? 'fizz' : '';
    var buzz = i % 5 == 0 ? 'buzz' : '';
    console.log(fizz + buzz || i);
  }
}
fizzBuzz();
```

would be minified to:

```js
function fizzBuzz(){for(var z=1;100>=z;z++){var f=z%3==0?"fizz":"",o=z%5==0?"buzz":"";console.log(f+o||z)}}fizzBuzz();
```

If you'd like to more easily debug minified code, click the "{}" prettify button
in Chrome's developer tools, which will beautify the code and allow setting
breakpoints on the un-minified version.

### Gzip compression of web resources

In certain cases, it might be preferable to leave web resources compressed on
disk and inflate them when needed (i.e. when a user wants to see a page).

In this case, you can run `gzip --rsyncable` on a resource before it's put into
a .pak file via GRIT with this syntax:

```xml
<include name="IDR_MY_PAGE" file="my/page.html" type="BINDATA" compress="gzip" />
```

Gzip is currently set up to apply to a whole WebUI's data source, though it's
possible to exclude specific paths for things like dynamically generated content
(i.e. many pages load translations dynamically from a path named "strings.js").

To mark a WebUI's resources compressed, you'll need to do something like:

```c++
WebUIDataSource* data_source = WebUIDataSource::Create(...);
data_source->SetDefaultResource(IDR_MY_PAGE);
data_source->UseGzip({"strings.js", ...});  // Omit arg to compress everything
```
