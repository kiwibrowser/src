#!/bin/bash
for i in "$@"; do
	n=$(basename "$i" .txt|sed -e 's/s$//')
	if test "$n" == "shortfile"; then continue; fi
	echo "enum sm4_$n"
	echo "{"
	while read j; do
		echo $'\t'"SM4_${n}_$j",
	done < "$i" |tr '[a-z]' '[A-Z]'|tr ' ' '_'
	echo $'\t'"SM4_${n}_COUNT"|tr '[a-z]' '[A-Z]'
	echo "};"
	echo
done
