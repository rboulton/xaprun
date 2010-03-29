#!/bin/bash

aclocal -I"m4-macros" && autoheader && libtoolize && automake --add-missing && autoconf
