# Building and running tests


The common (non-UI) parts of libaddressinput are built and run using the Gradle
project automation tool:

http://tools.android.com/tech-docs/new-build-system
http://www.gradle.org/


## Prerequisite dependencies for using Gradle

Gradle (latest version):
  https://services.gradle.org/distributions/gradle-2.3-bin.zip

Note: Additionally you must take care to avoid having multiple versions of
Gradle on your path, as this can cause problems.


## Building and Running

After installing all the prerequisites, check that everything is working by
running:

$ gradle build
$ gradle test
