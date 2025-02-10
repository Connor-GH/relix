#!/bin/sh
# we only want "symbols in .text" (functions, mostly), which is T/t
# delete a space in the middle of the data using \b
# the last sed command adds quotation marks and such.
 nm -C bin/kernel \
	| sort \
	| grep " [Tt] " \
	| awk '{$2="\b"; print $0}' \
