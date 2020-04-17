# Third-Party Dependencies

The Open Screen library includes its dependencies as DEPS in the source
tree under the `//third_party/` directory.  They are structured as follows:

```
  //third_party/<library>
   BUILD.gn
   ...other necessary adapter files...
   src/
     <library>'s source
```

## Adding a new dependency

When adding a new dependency to the project, you should first add an entry
to the DEPS file. For example, let's say we want to add a
new library called `alpha`. Opening up DEPS, you would add

``` python
  deps = {
    ...
    'src/third_party/alpha/src': 'https://repo.com/path/to/alpha.git'
        + '@' + '<revision>'
```

Then you need to add a BUILD.gn file for it under `//third_party/alpha`,
assuming it doesn't already provide its own BUILD.gn.

Finally, add a new entry for the "src" directory of your dependency to
the //third_party/.gitignore.

## Roll a dependency to a new version

Rolling a dependency forward (or to any different version really) consists of
two steps:
  1. Update the revision string for the dependency in the DEPS file.
  1. `git add` the DEPS file and commit, then run gclient sync.

Of course, you should also make sure that the new change is still compatible
with the rest of the project, including any adapter files under
`//third_party/<library>` (e.g. BUILD.gn).  Any necessary updates to make the
rest of the project work with the new dependency version should happen in the
same change.
