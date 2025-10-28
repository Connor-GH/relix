#!/usr/bin/env bash

TOOLCHAINS=("GNU" "LLVM")
BUILD_TYPES=("DEBUG" "RELEASE")
SATA_FILENAME=test_sata
SATA_DISK=("-drive id=disk,file=$SATA_FILENAME.img,if=none -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0")

panic() {
	echo "$1"
	exit 1
}

# Create the disk image if it doesn't exist
! [ -f ./$SATA_FILENAME.img ] && (qemu-img create -f raw $SATA_FILENAME.img 2G || panic "Could not create disk \`$SATA_FILENAME.img'!")

for TOOLCHAIN in "${TOOLCHAINS[@]}"; do
	for BUILD_TYPE in "${BUILD_TYPES[@]}"; do
		LLVM=
		RELEASE=
		if [ "$TOOLCHAIN" == "GNU" ]; then
			LLVM=
		elif [ "$TOOLCHAIN" == "LLVM" ]; then
			LLVM=1
		fi
		if [ "$BUILD_TYPE" == "DEBUG" ]; then
			RELEASE=
		elif [ "$BUILD_TYPE" == "RELEASE" ]; then
			RELEASE=1
		fi

		make clean
		make iso RELEASE=$RELEASE LLVM=$LLVM -j$(nproc)

		if [ "$?" != "0" ]; then
			panic "Error: build $TOOLCHAIN $BUILD_TYPE exited with an error"
		fi

		make qemu RELEASE=$RELEASE LLVM=$LLVM CONSOLE_LOG=1 QEMUEXTRA="$SATA_DISK"
	done
done
