# Dawn, a WebGPU implementation

Dawn (formerly NXT) is an open-source and cross-platform implementation of the work-in-progress WebGPU standard.
It exposes a C/C++ API that maps almost one-to-one to the WebGPU IDL and can be managed as part of a larger system such as a Web browser.

Dawn provides several WebGPU building blocks:
 - **WebGPU C/C++ headers** that applications and other building blocks use.
 - **A "native" implementation of WebGPU** using platforms' GPU APIs:
   - **D3D12** on Windows 10
   - **Metal** on OSX (and eventually iOS)
   - **Vulkan** on Windows, Linux (eventually ChromeOS and Android too)
   - OpenGL as best effort where available
 - **A client-server implementation of WebGPU** for applications that are in a sandbox without access to native drivers

## Directory structure

- `dawn.json`: description of the API used to drive code generators.
- `examples`: examples showing how Dawn is used.
- `generator`: code generator for files produces from `dawn.json`
  - `templates`: Jinja2 templates for the generator
- `scripts`: scripts to support things like continuous testing, build files, etc.
- `src`: 
  - `common`: helper code shared between core Dawn libraries and tests/samples
  - `dawn_native`: native implementation of WebGPU, one subfolder per backend
  - `dawn_wire`: client-server implementation of WebGPU
  - `include`: public headers for Dawn
  - `tests`: internal Dawn tests
    - `end2end`: WebGPU tests performing GPU operations
    - `unittests`: unittests and by extension tests not using the GPU
      - `validation`: WebGPU validation tests not using the GPU (frontend tests)
  - `utils`: helper code to use Dawn used by tests and samples
- `third_party`: directory where dependencies live as well as their buildfiles.

## Building Dawn

Dawn uses the Chromium build system and dependency management so you need to [install depot_tools] and add it to the PATH.

[install depot_tools]: http://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up

On Linux you need to have the `pkg-config` command:
```sh
# Install pkg-config on Ubuntu
sudo apt-get install pkg-config
```

Then get the source as follows:

```sh
# Clone the repo as "dawn"
git clone https://dawn.googlesource.com/dawn dawn && cd dawn

# Bootstrap the gclient configuration
cp scripts/standalone.gclient .gclient

# Fetch external dependencies and toolchains with gclient
gclient sync
```

Then generate build files using `gn args out/Debug` or `gn args out/Release`.
A text editor will appear asking build options, the most common option is `is_debug=true/false`; otherwise `gn args out/Release --list` shows all the possible options.

Then use `ninja -C out/Release` to build dawn and for example `./out/Release/dawn_end2end_tests` to run the tests.

## Contributing

Please read and follow [CONTRIBUTING.md](/CONTRIBUTING.md).
Dawn doesn't have a formal coding style yet, except what's defined by our clang format style.
Overall try to use the same style and convention as code around your change.

If you find issues with Dawn, please feel free to report them on the [bug tracker](https://bugs.chromium.org/p/dawn/issues/entry).
For other discussions, please post to [Dawn's mailing list](https://groups.google.com/forum/#!members/dawn-graphics).

## License

Please see [LICENSE](/LICENSE).

## Disclaimer

This is not an officially supported Google product.
