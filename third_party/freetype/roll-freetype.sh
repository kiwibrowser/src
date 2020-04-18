#!/bin/bash

rolldeps() {
  STEP="roll-deps" &&
  REVIEWERS=$(paste -s -d, third_party/freetype/OWNERS) &&
  roll-dep -r "${REVIEWERS}" "$@" src/third_party/freetype/src/
}

previousrev() {
  STEP="original revision" &&
  PREVIOUS_FREETYPE_REV=$(git grep "'freetype_revision':" HEAD~1 -- DEPS | grep -Eho "[0-9a-fA-F]{32}")
}

addtrybots() {
  STEP="add trybots" &&
  OLD_MSG=$(git show -s --format=%B HEAD) &&
  git commit --amend -m"$OLD_MSG" -m"CQ_INCLUDE_TRYBOTS=master.tryserver.chromium.linux:linux_chromium_msan_rel_ng"
}

addotherprojectbugs() {
  STEP="add pdfium bug" &&
  OLD_MSG=$(git show -s --format=%B HEAD) &&
  git commit --amend -m"$OLD_MSG" -m"
PDFium-Issue: pdfium:"
}

checkmodules() {
  STEP="check modules.cfg: check list of modules and dependencies" &&
  ! git -C third_party/freetype/src/ diff --name-only ${PREVIOUS_FREETYPE_REV} | grep -q modules.cfg
}

mergeinclude() {
  INCLUDE=$1 &&
  STEP="merge ${INCLUDE}: check for merge conflicts" &&
  TMPFILE=$(mktemp) &&
  git -C third_party/freetype/src/ cat-file blob ${PREVIOUS_FREETYPE_REV}:include/freetype/config/${INCLUDE} >> ${TMPFILE} &&
  git merge-file third_party/freetype/include/freetype-custom-config/${INCLUDE} ${TMPFILE} third_party/freetype/src/include/freetype/config/${INCLUDE} &&
  rm ${TMPFILE} &&
  git add third_party/freetype/include/freetype-custom-config/${INCLUDE}
}

updatereadme() {
  STEP="update README.chromium" &&
  FTVERSION=$(git -C third_party/freetype/src/ describe --long) &&
  FTCOMMIT=$(git -C third_party/freetype/src/ rev-parse HEAD) &&
  sed -i "s/^Version: .*\$/Version: ${FTVERSION%-*}/" third_party/freetype/README.chromium &&
  sed -i "s/^Revision: .*\$/Revision: ${FTCOMMIT}/" third_party/freetype/README.chromium &&
  git add third_party/freetype/README.chromium
}

commit() {
  STEP="commit" &&
  git commit --quiet --amend --no-edit
}

rolldeps "$@" &&
previousrev &&
addtrybots &&
addotherprojectbugs &&
checkmodules &&
mergeinclude ftoption.h &&
mergeinclude ftconfig.h &&
updatereadme &&
commit &&
true || echo "Failed step ${STEP}" && false
