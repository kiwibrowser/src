Library Project including a multidex loader.

This can be used by an Android project to install multiple dexes
in the classloader of an application running on API 4+.

Note that multidexing will allow to go over the dex index limit.
It can also help with the linearalloc limit during installation but it
won't help with linearalloc at execution time. This means that
most applications requiring multidexing because of the dex index
limit won't execute on API below 14 because of linearalloc limit.

There is technically no source, but the src folder is necessary
to ensure that the build system works.  The content is actually
located in libs/android-support-multidex.jar.

