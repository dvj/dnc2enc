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
	unzip -qq -d $TMPDIR $DNCDIR/$dnc.zip
	chmod -R u+w $TMPDIR/DNC$dnc

	src="$TMPDIR/DNC$dnc"
	dest="$ENCDIR/$dnc.000"
	echo "CONVERTING DNC $i OF $TOTAL ($src TO $dest)..."
	mkdir -p $ENCDIR/$(dirname $dnc)
	$DNC2ENC $src $dest

	rm -rf $TMPDIR/*
done

rm -rf $TMPDIR
