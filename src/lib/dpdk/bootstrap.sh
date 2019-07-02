#!/bin/bash
echo "Boot-strapping DPDK ..."
DPDK_VERSION=dpdk-18.08

if [ ! -d ./$DPDK_VERSION ] ; then
    echo "Downloading DPDK source ...."
    wget https://fast.dpdk.org/rel/$DPDK_VERSION.tar.xz
    tar -xf $DPDK_VERSION.tar.xz
    ln -s ./$DPDK_VERSION dpdk
fi

CPUTYPE=$(lscpu | awk '/Model/{if($0 ~ "POWER") print($3)}')
if ["$CPUTYPE" = "POWER9"] ; then
    cd dpdk
      sed 's:power8:power9:g' < config/defconfig_ppc_64-power8-linuxapp-gcc > config/defconfig_ppc_64-power9-linuxapp-gcc
      sed -i 's:-march=$(RTE_MACHINE):-mcpu=$(RTE_MACHINE) -mtune=$(RTE_MACHINE):g' mk/target/generic/rte.vars.mk
    cd ..
fi
if ["$CPUTYPE" = "POWER8"] ; then
    cd dpdk
      sed -i 's:-march=$(RTE_MACHINE):-mcpu=$(RTE_MACHINE) -mtune=$(RTE_MACHINE):g' mk/target/generic/rte.vars.mk
    cd ..
fi

cp eal_memory.c ./$DPDK_VERSION/lib/librte_eal/linuxapp/eal/eal_memory.c
cp defconfig_x86_64-native-linuxapp-gcc ./$DPDK_VERSION/config
cp build.sh ./$DPDK_VERSION/build.sh

cd ./$DPDK_VERSION/ ; ./build.sh $1
