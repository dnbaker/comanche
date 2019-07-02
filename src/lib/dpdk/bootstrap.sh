#!/bin/bash
echo "Boot-strapping DPDK ..."
set -e
DPDK_VERSION=dpdk-18.08

if [ ! -d ./$DPDK_VERSION ] ; then
    echo "Downloading DPDK source ...."
    wget https://fast.dpdk.org/rel/$DPDK_VERSION.tar.xz
    tar -xf $DPDK_VERSION.tar.xz
    ln -s ./$DPDK_VERSION dpdk
fi

if [ ! -d ./dpdk ]; then
	echo "FAIL: not present"
	exit 1
fi

CPUTYPE=$(lscpu | awk '/Model/{if($0 ~ "POWER") print($3)}')
if [ "$CPUTYPE" = "POWER9" ] ; then
    sed 's:power8:power9:g' dpdk/config/defconfig_ppc_64-power8-linuxapp-gcc > dpdk/config/defconfig_ppc_64-power9-linuxapp-gcc
    sed -i 's:-march=$(RTE_MACHINE):-mcpu=$(RTE_MACHINE) -mtune=$(RTE_MACHINE):g' dpdk/mk/target/generic/rte.vars.mk
    sed -i 's:-march=native:-mcpu=native -mtune=native:g' dpdk/mk/target/generic/rte.vars.mk
    sed -i 's:-march=$(RTE_MACHINE):-mcpu=$(RTE_MACHINE) -mtune=$(RTE_MACHINE):g' dpdk/mk/machine/native/rte.vars.mk
    sed -i 's:-march=native:-mcpu=native -mtune=native:g' dpdk/mk/machine/native/rte.vars.mk
    cp  dpdk/config/defconfig_ppc_64-power9-linuxapp-gcc ./$DPDK_VERSION/config || echo "present"
elif [ "$CPUTYPE" = "POWER8" ] ; then
    sed -i 's:-march=$(RTE_MACHINE):-mcpu=$(RTE_MACHINE) -mtune=$(RTE_MACHINE):g' dpdk/mk/target/generic/rte.vars.mk
    sed -i 's:-march=native:-mcpu=native -mtune=native:g' dpdk/mk/target/generic/rte.vars.mk
    sed -i 's:-march=$(RTE_MACHINE):-mcpu=$(RTE_MACHINE) -mtune=$(RTE_MACHINE):g' dpdk/mk/machine/native/rte.vars.mk
    sed -i 's:-march=native:-mcpu=native -mtune=native:g' dpdk/mk/machine/native/rte.vars.mk
    cp  dpdk/config/defconfig_ppc_64-power8-linuxapp-gcc ./$DPDK_VERSION/config || echo "present2"
else
    echo "processing for x86-64"
    cp defconfig_x86_64-native-linuxapp-gcc ./$DPDK_VERSION/config
fi

cp eal_memory.c ./$DPDK_VERSION/lib/librte_eal/linuxapp/eal/eal_memory.c
cp build.sh ./$DPDK_VERSION/build.sh

cd ./$DPDK_VERSION/ ; ./build.sh $1
