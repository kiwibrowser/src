## Created 7/14/2006 by Tom Whipple <tom.whipple@intel.com>

## create a globally unique identifier (GUID)
## note that this is LIKELY to be unique, but not 100% guaranteed.
get_guid() {
	prefix=$1
	now=`date '+%F-%H%M%S.%N'`
	GUID="$prefix$now-$RANDOM"
}

get_tmpsubdir() {
	if [ ! -z "$1" ] ; then
		tmp="$1"
	else
		tmp=${TMPDIR-/tmp}
	fi
	if [ -z "$GUID" ] ; then
		get_guid
	fi
	TMPSUBDIR="$tmp/$GUID-$$"
	(umask 000 && mkdir -p $TMPSUBDIR) || {
		echo "Could not create temporary directory!" >&2 
		exit 255
	}
}

get_shortid() {
	seqfile=$1

	today=`date '+%m-%d'`

	if [ -f "$seqfile" ] ; then	
		seqdate=`cat $seqfile | cut -d '+' -f 1 -`
		if [ "$today" = "$seqdate" ] ; then
			seq=$(( `cat $seqfile | cut -d '+' -f 2 -` + 1 ))
		else
			seq=1
		fi
	else
		seq=1
	fi

	SHORTID="$today+$seq"
	echo "$SHORTID" > "$seqfile"
	
}

