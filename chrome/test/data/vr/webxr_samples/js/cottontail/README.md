Cottontail
==========================================
A simple WebGL renderering framework optimised for demonstrating WebXR concepts.

Cottontail does two things well and not much else:

1) Loading and Rendering GLTF 2.0 files.
2) Optimising for WebXR-style rendering.

However it explicitly goes out of it's way to NOT wrap much, if any, WebXR
functionality. This is because arguably it's sole purpose in life is to enable
the WebXR samples, who's sole purpose in life is to provide easy-to-follow code
snippets demonstrating the API use and thus anything this code did to hide the
WebXR API's direct use is counter productive.

Using Cottontail for your own projects is very much not recommended, as you will
almost certainly be better served by one of the other more popular frameworks
out there.

**About the name:** I wanted something that evoked XR imagery without being too
heavy handed about it. The "White Rabbit" is prominent symbol in multiple peices
of media about virtual worlds, such as the movie _The Matrix_ and the novel
_Rainbows End_, which has a particularly heavy focus on AR/MR. So I liked the
idea of riffing off of that. It's also nice (though unintended) bonus that the
Daydream logo looks a bit like a stylized rabbit tail. Finally, it's based on
some code that I started on Easter day, 2017.

The real world may rest on the back of a giant tortise, but I propose
that the virtual one is bunnies all the way down. ;)

Building
--------
Cottontail can be used directly by any browser with good JavaScript modules
support, but to ensure widest compatibility (and shorter download times) using
a version compiled into a single file is recommended. To build first install the
[node.js](https://nodejs.org/en/) dependencies with:

  `npm install`

Then build with:

  `npm run build-all`

For faster development iteration you can also have the project rebuild with any
code changes automatically by running:

  `npm run watch`
