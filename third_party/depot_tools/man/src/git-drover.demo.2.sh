#!/usr/bin/env bash
. git-drover.demo.common.sh

echo "# Make sure we have the most up-to-date branch sources."
run git fetch
echo
echo "# Checkout the branch with the change we want to revert."
run git checkout -b drover_9999 branch-heads/9999
echo
drover_c "This change is horribly broken."
echo "# Here's the commit we want to revert."
run git log -n 1
echo
echo "# Now do the revert."
silent git revert --no-edit $(git show-ref -s pick_commit)
pcommand git revert $(git show-ref -s pick_commit)
echo
echo "# That reverted the change and committed the revert."
run git log -n 1
echo
echo "# As with old drover, reverts are generally OK to commit without LGTM."
pcommand git cl upload -r some.committer@chromium.org --send-mail
run git cl land --bypass-hooks
