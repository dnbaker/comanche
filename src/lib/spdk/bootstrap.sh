#!/bin/bash
#
# version should correspond to DPDK
#
echo "Boot-strapping SPDK ..."

if [ ! -d ./spdk ] ; then
    git clone -b v18.10 https://github.com/spdk/spdk.git
fi

CPUTYPE=$(lscpu | awk '/Model/{if($0 ~ "POWER") print($3)}')
if [ "$CPUTYPE" = "POWER9" ] ; then
    BTYPE="ppc_64-power9-linuxapp-gcc"
elif [ "$CPUTYPE" = "POWER8" ] ; then
    BTYPE="ppc_64-power8-linuxapp-gcc"
else
    BTYPE="x86_64-native-linuxapp-gcc"
fi
cd spdk ;
cp ../save.patch .
git apply save.patch
echo ./configure --with-dpdk=../../dpdk/dpdk/build/share/dpdk/${BTYPE} --without-vhost --without-virtio
./configure --with-dpdk=../../dpdk/dpdk/build/share/dpdk/${BTYPE} --without-vhost --without-virtio
DPDKDIR=`readlink -f ../../dpdk/dpdk/build/share/dpdk/$BTYPE`
make DPDK_DIR=$DPDKDIR CONFIG_RDMA=y -j40
