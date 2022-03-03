#!/bin/bash
# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Copyright (c) 2019 Amlogic, Inc. All rights reserved.
#

ROOT_DIR=`pwd`

ARCH=arm
DEFCONFIG=meson64_a32_smarthome_defconfig
CROSS_COMPILE_TOOL=/opt/gcc-linaro-7.3.1-2018.05-x86_64_armv8l-linux-gnueabihf/bin/armv8l-linux-gnueabihf-
MAKE='make'

source ${ROOT_DIR}/common/common_drivers/scripts/amlogic/mk_smarthome_common.sh $@
