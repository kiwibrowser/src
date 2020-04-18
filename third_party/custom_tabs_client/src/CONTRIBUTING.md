# Contributing to the Custom Tabs Examples
Want to contribute? Great! First, read this page.

## Before you contribute
Before we can use your code, you must sign the
[Google Individual Contributor License Agreement](https://developers.google.com/open-source/cla/individual?csw=1)
(CLA), which you can do online, and it only takes a minute.

The CLA is necessary mainly because you own the copyright to your changes, even
after your contribution becomes part of our codebase, so we need your permission
to use and distribute your code. We also need to be sure of various other
things—for instance that you'll tell us if you know that your code infringes on
other people's patents. You don't have to sign the CLA until after you've
submitted your code for review and a member has approved it, but you must do it
before we can put your code into our codebase.  Before you start working on a
larger contribution, you should get in touch with us first through the issue
tracker with your idea so that we can help out and possibly guide you.
Coordinating up front makes it much easier to avoid frustration later on.

If you are contributing on behalf of a corporation, you must fill out the
[Corporate Contributor License Agreement](https://cla.developers.google.com/about/google-corporate?csw=1)
and send it to us as described on that page.

If you've never submitted code before, you must add your (or your
organization's) name and contact info to the Chromium AUTHORS file.

## Contributing
All submissions, including submissions by project members, require review. We
use [Gerrit](http://chromium-review.googlesource.com) for this purpose.

Install [depot_tools](https://www.chromium.org/developers/how-tos/install-depot-tools).

Then checkout the repo.

`git clone https://chromium.googlesource.com/custom-tabs-client`

You can then create a local branch, make and commit your change.

```
cd custom-tabs-client
git checkout -b foo origin/master
... edit files ...
git commit -a
```

Once you're ready for a review do:

`git cl upload`

Once uploaded you can view the CL in Gerrit and request a review by clicking
the 'publish & mail' link. The
[OWNERS](https://chromium.googlesource.com/custom-tabs-client/+/master/OWNERS)
file suggests relevant reviewers, but does not have any real power, any Chromium
committer has the power to approve the change.

If you get review feedback, edit and commit locally and then do another upload
with the new files. Before you commit you'll want to sync to the tip-of-tree.
You can either merge or rebase, it's up to you.

Then, submit your changes through the commit queue by checking the "Commit" box.

Once everything is landed, you can cleanup your branch.

```
git checkout master
git branch -D foo
```

## Contributing from a Chromium checkout

If you already have this repo checked out as part of a Chromium checkout and want
to edit it in place (instead of having a separate clone of the repository), you
can use the exact same process as above.

When doing `gclient sync` in the Chromium tree, remember to switch back to the
local branch `master`.

## Updating Custom Tabs Examples in the Chromium tree (rolling DEPS)
To get your commit to be tested as part of the Chromium tree in
`src/third_party/custom_tabs_client`, find the git hash of your landed commit
in the [repo](https://chromium.googlesource.com/custom-tabs-client/+log/).

Then edit Chrome's
[src/DEPS](https://chromium.googlesource.com/chromium/src/+/master/DEPS) file.
Look for a line like:

```
'src/third_party/custom_tabs_client/src':
  Var('chromium_git') + '/custom-tabs-client.git' + '@' +
    'bbbf71f41e79b0cfe21199220f495cbd0a3a4ffb',
```

Update the value to the git hash you want to roll to, and [contribute a
codereview to Chromium](http://www.chromium.org/developers/contributing-code)
for your edit. If you are a Chromium committer, feel free to TBR this.
