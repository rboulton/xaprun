#!/bin/bash

# Nasty hack - if we have a xapian-config, try and guess where the macro is based on it.
xapconfig=`which xapian-config`
if [ "x$xapconfig" == "x" ];
then
    xapconfig=`which xapian-config-1.1`;
fi

xapmacropath=`cat $xapconfig | sed -n '/^prefix="/p' | sed 's/prefix=/-I /;s/"//g'`
if [ "x$xapmacropath" != "x" ];
then
    xapmacropath="$xapmacropath/share/aclocal";
fi

aclocal $xapmacropath && autoheader && libtoolize && automake --add-missing && autoconf
