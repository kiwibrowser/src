#!/usr/bin/env bash
. git-drover.demo.common.sh

drover_c "This change needs to go to branch 9999"

echo "# Here's a commit (from some.committer) that we want to 'drover'."
run git log -n 1 --pretty=fuller
echo
echo "# Now do the 'drover'."
pcommand git drover --branch 9999 \
  --cherry-pick $(git show-ref -s pick_commit)

echo "Going to cherry-pick"
echo '"""'
output git log -n 1
echo '"""'
echo "to 9999. Continue (y/n)? y"
echo
echo "Error: Patch failed to apply."
echo
echo "A workdir for this cherry-pick has been created in"
echo "  /tmp/drover_9999"
echo
echo "To continue, resolve the conflicts there and run"
echo "  git drover --continue /tmp/drover_9999"
echo
echo "To abort this cherry-pick run"
echo "  git drover --abort /tmp/drover_9999"
echo
pcommand pushd /tmp/drover_9999
echo "# Manually resolve conflicts."
pcommand git add path/to/file_with_conflicts
pcommand popd
pcommand git drover --continue /tmp/drover_9999
echo
echo "# A cl is uploaded to rietveld, where it can be reviewed before landing."
echo
echo "About to land on 9999. Continue (y/n)? y"
echo "# The cherry-pick cl is landed on the branch 9999."
