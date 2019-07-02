#!/bin/bash
THREADS=1
if [ $# -ge 1 ]; then
	THREADS=$1
fi

CPUTYPE=$(lscpu | awk '/Model/{if($0 ~ "POWER") print($3)}')
echo $CPUTYPE
if [ "$CPUTYPE" = "POWER9" ] ; then
	make install T=ppc_64-power9-linuxapp-gcc DESTDIR=./build EXTRA_CFLAGS="-g -fPIC" -j ${THREADS}
elif [ "$CPUTYPE" = "POWER8" ] ; then
	make install T=ppc_64-power8-linuxapp-gcc DESTDIR=./build EXTRA_CFLAGS="-g -fPIC" -j ${THREADS}
else
	make install T=x86_64-native-linuxapp-gcc DESTDIR=./build EXTRA_CFLAGS="-g -fPIC" -j ${THREADS}
fi
cd ..
