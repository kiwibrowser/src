Basic benchmarks to measure HTML5 performance and support for game related features

Running the Tests
-----------------

1. Install [Node.js][1] (0.6.0+).
2. Run in shell: `npm install`
3. Run in shell: `node server`
4. Open in browser: `http://localhost:3002/`
5. Configure parameters of the test run, if desired.
6. Click "Run Tests".
7. If you uploaded results, see the `server/uploads/` directory.

[1]: http://nodejs.org/

Interpreting Results
--------------------

### Sprite tests

The `Source type` indicates the data being displayed.

The `Technique` indicates the browser method used to render the data.

The `Test type` indicates what transformations were performed on the data using
the technique.

The `JS time (ms)` result shows how much JavaScript time was spent rendering
the objects in one second.

The `Objects at 30FPS` result shows how many objects were able to be rendered
while rendering at 30 frames per second.

### Audio latency test

The `Cold play latency` indicates how much time it took between `.play()` and
the first subsequent `play` or `timeupdate` event on a new WAV `<audio>`
element.

The `Warm play latency` indicates how much time it took between `.play()` and
the first subsequent `play` or `timeupdate` event on a WAV `<audio>` element
which has already played.

### Canvas text test

The `Score` result shows how many renders could be made per 100 milliseconds.

Code structure
--------------

Test cases are build into a recursive object structure.  Test cases are run
through the test runner in `js/testRunner.js`.

Test cases are displayed based upon the specification in `js/tables.js`.

### Sprite tests

Sprite tests are found under `js/sprites/`.

There are three interleaved components:

#### Sources

Each source represents some asset (e.g. a sprite sheet).  Maps to the `Source
type` result.

#### Transformers

Transformers modify the source by applying affine or other transformations.
Maps to the `Test type` result.

#### Renderers

Renderers displayed transformed sources using different techniques.  Maps to the
`Technique` result.  See `js/sprites/renderers/README.md` for details.
