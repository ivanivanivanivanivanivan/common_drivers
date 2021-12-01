#!/bin/bash

function pre_defconfig_cmds() {
	if [[ ${AMLOGIC_USERDEBUG} -eq "1" ]]; then
		echo "CONFIG_AMLOGIC_USERDEBUG=y" >> ${ROOT_DIR}/${FRAGMENT_CONFIG}
	else
		echo "CONFIG_AMLOGIC_USERDEBUG=n" >> ${ROOT_DIR}/${FRAGMENT_CONFIG}
	fi

	if [[ ${AMLOGIC_NONGKI} -eq "1" ]]; then
		echo "CONFIG_AMLOGIC_NONGKI=y" >> ${ROOT_DIR}/${FRAGMENT_CONFIG}
		SKIP_EXT_MODULES=1
		export SKIP_EXT_MODULES
		EXT_MODULES=
		export EXT_MODULES
	else
		echo "CONFIG_AMLOGIC_NONGKI=n" >> ${ROOT_DIR}/${FRAGMENT_CONFIG}
	fi
	KCONFIG_CONFIG=${ROOT_DIR}/${KERNEL_DIR}/arch/arm64/configs/${DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m -r ${ROOT_DIR}/${KERNEL_DIR}/arch/arm64/configs/gki_defconfig ${ROOT_DIR}/${FRAGMENT_CONFIG}
}
export -f pre_defconfig_cmds

function post_defconfig_cmds() {
	# check_defconfig
	rm ${ROOT_DIR}/${KERNEL_DIR}/arch/arm64/configs/${DEFCONFIG}
	pushd ${ROOT_DIR}/common_drivers
		sed -i '5,${/CONFIG_AMLOGIC_USERDEBUG/d}' ${ROOT_DIR}/${FRAGMENT_CONFIG}
		sed -i '5,${/CONFIG_AMLOGIC_NONGKI/d}' ${ROOT_DIR}/${FRAGMENT_CONFIG}
	popd
}
export -f post_defconfig_cmds

function read_ext_module_config() {
	ALL_LINE=""
	while read LINE
	do
		if [[ $LINE != \#*  &&  $LINE != "" ]]; then
			ALL_LINE="$ALL_LINE"" ""$LINE"
		fi
	done < $1
	export GKI_EXT_MODULE_CONFIG=$ALL_LINE
	echo "GKI_EXT_MODULE_CONFIG=${GKI_EXT_MODULE_CONFIG}"
}

function read_ext_module_predefine() {
	PRE_DEFINE=""

	for y_config in `cat $1 | grep "^CONFIG_.*=y" | sed 's/=y//'`;
	do
		PRE_DEFINE="$PRE_DEFINE"" -D"${y_config}
	done

	for m_config in `cat $1 | grep "^CONFIG_.*=m" | sed 's/=m//'`;
	do
		PRE_DEFINE="$PRE_DEFINE"" -D"${m_config}_MODULE
	done

	export GKI_EXT_MODULE_PREDEFINE=$PRE_DEFINE
	echo "GKI_EXT_MODULE_PREDEFINE=${GKI_EXT_MODULE_PREDEFINE}"
}

function prepare_module_build() {
	if [[ -z ${AMLOGIC_NONGKI} ]]; then
		read_ext_module_config $FRAGMENT_CONFIG && read_ext_module_predefine $FRAGMENT_CONFIG
	fi
}

export -f prepare_module_build

function mod_probe() {
        local ko=$1
        local loop
        for loop in `grep "$ko:" modules.dep | sed 's/.*://'`;
        do
                mod_probe $loop
                echo insmod $loop >> __install.sh
        done
}

function modules_install() {
	rm modules -rf
	mkdir modules
	cp *.ko modules

	local stagin_module=$(echo ${MODULES_STAGING_DIR}/lib/modules/*)
	echo stagin_module=${stagin_module}
	cp ${stagin_module}/modules.dep modules

	cd modules
	sed -i 's#[^ ]*/##g' modules.dep

	touch __install.sh
	for loop in `cat modules.dep | sed 's/:.*//'`; do
	        mod_probe $loop
	        echo insmod $loop >> __install.sh
	done

	cat __install.sh  | awk ' {
		if (!cnt[$2]) {
			print $0;
			cnt[$2]++;
		}
	}' > __install.sh.tmp

	cp __install.sh.tmp __install.sh

	sed -i '1s/^/#!\/bin\/sh\n\nset -ex\n/' __install.sh
	echo "echo Install modules success!" >> __install.sh
	chmod 777 __install.sh

	echo "#!/bin/sh" > install.sh
	# echo "./__install.sh || reboot" >> install.sh
	echo "./__install.sh" >> install.sh
	chmod 777 install.sh

	echo "/modules/: all `wc -l modules.dep | awk '{print $1}'` modules."

	cd ../
}

function rebuild_rootfs() {
	pushd ${DIST_DIR}

	modules_install

	rm rootfs -rf
	mkdir rootfs
	cp ${ROOT_DIR}/common_drivers/rootfs_base.cpio.gz.uboot	rootfs
	cd rootfs
	dd if=rootfs_base.cpio.gz.uboot of=rootfs_base.cpio.gz bs=64 skip=1
	gunzip rootfs_base.cpio.gz
	mkdir rootfs
	cd rootfs
	cpio -i -F ../rootfs_base.cpio
	cp -rf ../../modules .
	find . | cpio -o -H newc | gzip > ../rootfs_new.cpio.gz
	cd ../
	mkimage -A arm64 -O linux -T  ramdisk -C none -d rootfs_new.cpio.gz rootfs_new.cpio.gz.uboot
	mv rootfs_new.cpio.gz.uboot ../
	cd ../

	popd
}
export -f rebuild_rootfs

function check_undefined_symbol() {
	pushd ${DIST_DIR}/rootfs/rootfs/modules
	echo
	echo "========================================================"
	echo "Functions or variables not defined in this module refer to which module."
	nm ../../../vmlinux | grep -E " T | D | B | R | W "> vmlinux_T.txt
	cat __install.sh | grep "insmod" | cut -d ' ' -f 2 > module_list.txt
	while read LINE
	do
		echo ${LINE}
		for U in `nm ${LINE} | grep " U " | sed -e 's/^\s*//' -e 's/\s*$//' | cut -d ' ' -f 2`
		do
			#echo ${U}
			U_v=`grep -w ${U} vmlinux_T.txt`
			in_vmlinux=0
			if [ -n "${U_v}" ];
			then
				#printf "\t%-50s <== vmlinux\n" ${U}
				in_vmlinux=1
				continue
			fi
			in_module=0
			MODULE=
			while read LINE1
			do
				U_m=`nm ${LINE1} | grep -E " T | D | B | R " | grep -v "\.cfi_jt" | grep "${U}"`
				if [ -n "${U_m}" ];
				then
					in_module=1
					MODULE=${LINE1}
				fi
			done < module_list.txt
			if [ ${in_module} -eq "1" ];
			then
				printf "\t%-50s <== %s\n" ${U} ${MODULE}
				continue
			else
				printf "\t%-50s <== none\n" ${U}
			fi
		done
		echo
		echo
	done  < module_list.txt
	rm vmlinux_T.txt
	rm module_list.txt
	popd
}
export -f check_undefined_symbol
