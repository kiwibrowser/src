#!/bin/sh

# Script for generating a list of candidates for cherry-picking to a stable branch

# Grep for commits with "cherry picked from commit" in the commit message.
git log --reverse --grep="cherry picked from commit" origin/master..HEAD |\
	grep "cherry picked from commit" |\
	sed -e 's/^[[:space:]]*(cherry picked from commit[[:space:]]*//' -e 's/)//' > already_picked

# Grep for commits that were marked as a candidate for the stable tree.
git log --reverse --pretty=%H -i --grep='^[[:space:]]*NOTE: This is a candidate' HEAD..origin/master |\
while read sha
do
	# Check to see whether the patch is on the ignore list.
	if [ -f bin/.cherry-ignore ] ; then
		if grep -q ^$sha bin/.cherry-ignore ; then
			continue
		fi
	fi

	# Check to see if it has already been picked over.
	if grep -q ^$sha already_picked ; then
		continue
	fi

	git log -n1 --pretty=oneline $sha | cat
done

rm -f already_picked
