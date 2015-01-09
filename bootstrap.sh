#!/usr/bin/env bash

aclocal
autoheader
autoconf
automake --copy --add-missing --foreign
