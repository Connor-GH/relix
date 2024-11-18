#!/usr/bin/env bash

TOOLCHAINS=("GNU" "LLVM")
BUILD_TYPES=("DEBUG" "RELEASE")

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

		make clean default RELEASE=$RELEASE LLVM=$LLVM -j$(nproc)

		if [ "$?" != "0" ]; then
			echo "Error: build $TOOLCHAIN $BUILD_TYPE exited with an error"
			exit 1
		fi

		make qemu
	done
done


