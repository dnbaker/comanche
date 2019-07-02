#!/bin/bash
CPUTYPE=$(lscpu | awk '/Model/{if($0 ~ "POWER") print($3)}')
echo $CPUTYPE
if [ "$CPUTYPE" = "POWER9" ] ; then
	make install T=ppc_64-power9-linuxapp-gcc DESTDIR=./build EXTRA_CFLAGS="-g -fPIC"
elif [ "$CPUTYPE" = "POWER8" ] ; then
	make install T=ppc_64-power8-linuxapp-gcc DESTDIR=./build EXTRA_CFLAGS="-g -fPIC"
else
	make install T=x86_64-native-linuxapp-gcc DESTDIR=./build EXTRA_CFLAGS="-g -fPIC"
fi

