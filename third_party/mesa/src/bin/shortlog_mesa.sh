#!/bin/bash

# This script is used to generate the list of changes that
# appears in the release notes files, with HTML formatting.


typeset -i in_log=0

git shortlog $* | while read l
do
    if [ $in_log -eq 0 ]; then
	echo '<p>'$l'</p>'
	echo '<ul>'
	in_log=1
    elif echo "$l" | egrep -q '^$' ; then
	echo '</ul>'
	echo
	in_log=0
    else
        mesg=$(echo $l | sed 's/ (cherry picked from commit [0-9a-f]\+)//;s/\&/&amp;/g;s/</\&lt;/g;s/>/\&gt;/g')
	echo '  <li>'${mesg}'</li>'
    fi
done
