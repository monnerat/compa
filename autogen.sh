#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="compa"

(test -f $srcdir/configure.ac \
  && test -d $srcdir/src) || {
	echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
	echo " top-level $PKG_NAME directory"
	exit 1
}

which mate-autogen || {
	echo "You need to install mate-common from the MATE Git and make"
	echo "sure the mate-autogen script is in your \$PATH."
	exit 1
}

REQUIRED_AUTOMAKE_VERSION=1.9 USE_MATE2_MACROS=1 . mate-autogen
