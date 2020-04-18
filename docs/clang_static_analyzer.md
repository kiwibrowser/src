# The Clang Static Analyzer

The Clang C/C++ compiler comes with a static analyzer which can be used to find
bugs using path sensitive analysis. Path sensitive analysis is
a technique that explores all the possible branches in code and
records the codepaths that might lead to bad or undefined behavior,
like an uninitialized reads, use after frees, pointer leaks, and so on.

You can now use these static analysis capabilities to find potential bugs in
Chromium code! Note that this capability is quite new, and as of this writing,
there are still a large number of warnings to be fixed in Chromium and especially
in its third_party dependencies. Some of the warnings might be false positives,
see the section on "Addressing false positives" for more information on
resolving them.

We're still evaluating this tool, please let us know if you find it useful.

See the [official Clang static analyzer page](http://clang-analyzer.llvm.org/)
for more background information.

## Save some time, look at the buildbot logs!

We run static analysis builds continously, all day long on FYI buildbots.
You can save yourself some time by first inspecting their build logs for errors
before running your own analysis builds. You will probably need to Ctrl-F the
logs to find any issues for the specific files you're interested in.

You can find the analysis logs in the `compile stdout` step.
* [Linux buildbot logs](https://ci.chromium.org/buildbot/chromium.fyi/Linux%20Clang%20Analyzer/)

## Enabling static analysis
To get static analysis running for your build, add the following flag to your GN
args.

```
use_clang_static_analyzer = true
```

The next time you run your build, you should see static analysis warnings appear
inline with the usual Clang build warnings and errors. Expect some slowdown on
your build; anywhere from a 10% increase on local builds, to well over 100% under Goma
([crbug](crbug.com/733363)).

## Supported checks
Clang's static analyzer comes with a wide variety of checkers. Some of the checks
aren't useful because they are intended for different languages, platforms, or
coding conventions than the ones used for Chromium development.

The checkers that we are interested in running for Chromium are in the
`analyzer_option_flags` variable in
[clang_static_analyzer_wrapper.py](../build/toolchain/clang_static_analyzer_wrapper.py).

As of this writing, the checker suites we support are
[core](https://clang-analyzer.llvm.org/available_checks.html#core_checkers),
[cplusplus](https://clang-analyzer.llvm.org/available_checks.html#cplusplus_checkers), and
[deadcode](https://clang-analyzer.llvm.org/available_checks.html#deadcode_checkers).

To add or remove checkers, simply modify the `-analyzer-checker=` flags.
Remember that checkers aren't free; additional checkers will add to the
analysis time.

## Addressing false positives

Some of the errors you encounter might be false positives, which occurs when the
static analyzer naively follows codepaths which are practically impossible to hit
at runtime. Fortunately, we have a tool at our disposal for guiding the analyzer
away from impossible codepaths: assertion handlers like DCHECK/CHECK/LOG(FATAL).
The analyzer won't check the codepaths which we assert are unreachable.

An example would be that if the analyzer detected the function argument `*my_ptr`
might be null and dereferencing it would potentially segfault, you would see the
error `warning: Dereference of null pointer (loaded from variable 'my_ptr')`.
If you know for a fact that my_ptr will not be null in practice, then you can
place an assert at the top of the function: `DCHECK(my_ptr)`. The analyzer will
no longer generate the warning.

Be mindful about only specifying assertions which are factually correct! Don't
DCHECK recklessly just to quiet down the analyzer. :)

Other types of false positives and their suppressions:
* Unreachable code paths. To suppress, add the `ANALYZER_SKIP_THIS_PATH();`
  directive to the relevant code block.
* Dead stores. To suppress, use the macro
  `ANALYZER_ALLOW_UNUSED(my_var)`. This also suppresses dead store warnings
  on conventional builds without static analysis enabled!

See the definitions of the ANALYZER_* macros in base/logging.h for more
detailed information about how the annotations are implemented.

## Logging bugs

If you find any issues with the static analyzer, or find Chromium code behaving
badly with the analyzer, please check the `Infra>CodeAnalysis` CrBug component
to look for known issues, or file a bug if it is a new problem.

***

## Technical details
### GN hooks
The platform toolchain .gni/BUILD.gn files check for the
`use_clang_static_analyzer` flag and modify the compiler command line so as to
call the analysis wrapper script rather than call the compiler directly.
The flag has no effect on assembler invocations, linker invocations, or
NaCl toolchain builds.

### Analysis wrapper script
The entry point for running analysis is the Python script
`//build/toolchain/clang_static_analyzer_wrapper.py` which invokes Clang
with the parameters for running static analysis.

**Alternatives considered**
A script-less, GN-based solution is not possible because GN's control flows
are very limited in how they may be extended.

The `scan-build` wrapper script included with Clang does not
work with Goma, so it couldn't be used.

