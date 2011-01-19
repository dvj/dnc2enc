#!/bin/sh

DNCDIR=$1
ENCDIR=$2
DNC2ENC=./dnc2enc
TMPDIR=$(mktemp -d /tmp/dnc2enc.XXXXX)

if [ -z "$DNCDIR" -o -z "$ENCDIR" ]; then
	echo "Usage: $0 <dnc-dir> <dest-dir>"
	exit 1
fi

TOTAL=$(cat $DNCDIR/dncs.csv | grep -v BROWSE | wc -l)
i=0

for dnc in $(cat $DNCDIR/dncs.csv | grep -v BROWSE | awk -F, '{print $1 "/" $2}'); do
	i=$(($i+1))

	src="$TMPDIR/DNC$dnc"
	dest="$ENCDIR/$dnc.000"

	echo "CONVERTING DNC $i OF $TOTAL ($src TO $dest)..."

	if [ -f $dest ]; then
		echo "$dest EXISTS, SKIPPING"
		continue
	fi

	unzip -qq -d $TMPDIR $DNCDIR/$dnc.zip

	if [ ! -e $TMPDIR/DNC$dnc -a -e $TMPDIR/dnc$dnc ]; then
		# sometimes when you extract the zips they go in dncNN
		# instead of DNCNN
		mv $TMPDIR/dnc$(dirname $dnc) $TMPDIR/DNC$(dirname $dnc)
	fi

	chmod -R u+w $TMPDIR/DNC$dnc

	mkdir -p $ENCDIR/$(dirname $dnc)
	$DNC2ENC $src $dest

	rm -rf $TMPDIR/*
done

rm -rf $TMPDIR
