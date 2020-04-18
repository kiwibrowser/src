Android Deps Repository Generator
---------------------------------

Tool to generate a gradle-specified repository for Android and Java
dependencies.

### Usage

    fetch_all.sh [--help]

For each of the dependencies specified in `build.gradle`, the above command
will take care of the following tasks:

- Download the library
- Generate a README.chromium file
- Download the LICENSE
- Generate a GN target in BUILD.gn
- Generate .info files for AAR libraries
- Generate CIPD yaml files describing the packages

### Adding a new library
Full steps to add a new third party library:

1. Add dependency to `build.gradle`
2. Run `fetch_all.sh`
3. Upload the new and updated packages via cipd. The script from step 2 should
   provide hints for this step when it successfully completes.
4. Create a commit & follow [`//docs/adding_to_third_party.md`][docs_link] for
   the review.

[docs_link]: ../../../../docs/adding_to_third_party.md

### Implementation notes:
The script is written as a Gradle plugin to leverage its dependency resolution
features. An alternative way to implement it is to mix gradle to purely fetch
dependencies and their pom.xml files, and use Python to process and generate
the files. This approach was not as successful, as some information about the
dependencies does not seem to be available purely from the POM file, which
resulted in expecting dependencies that gradle considered unnecessary.
