#!/bin/sh
set -eu

# Rebuild btctrler.a after bypassing the faulty lmp_rx_au_rand rejection branch.
# Run this script inside debian-x64 with the Jieli LLVM 4.0.1 toolchain.

SDK_ROOT=${SDK_ROOT:-/mnt/mac/Users/awalol/Code/fw-AC792_SDK/sdk}
LIB_DIR="$SDK_ROOT/cpu/wl83/liba"
INPUT_LIB=${1:-"$LIB_DIR/btctrler.a.before-lto-patch"}
OUTPUT_LIB=${2:-"$LIB_DIR/btctrler.a.patched"}

TOOL_COMMON=/opt/jieli/common/bin
TOOL_PI32V2=/opt/jieli/pi32v2/bin
WORK=$(mktemp -d)

trap 'rm -rf "$WORK"' EXIT

"$TOOL_COMMON/llvm-ar" p "$INPUT_LIB" lmp.c.o > "$WORK/lmp.bc"
"$TOOL_COMMON/clang" -S -emit-llvm "$WORK/lmp.bc" -o "$WORK/lmp.ll"

# The rejected edge logs fix lmp_rx_au_not accept, sends LMP_NOT_ACCEPTED,
# and returns before SRES processing. Route both edges to normal processing.
sed 's/br i1 %33, label %47, label %34/br i1 true, label %47, label %47/' \
    "$WORK/lmp.ll" > "$WORK/lmp.patched.ll"

"$TOOL_PI32V2/clang" \
    -target pi32v2 \
    -integrated-as \
    -mcpu=r3 \
    -mfprev1 \
    -Oz \
    -g \
    -flto \
    -fallow-pointer-null \
    -fprefer-gnu-section \
    -femulated-tls \
    -mllvm -pi32v2-large-program=true \
    -fms-extensions \
    -fno-unwind-tables \
    -ffunction-sections \
    -fdata-sections \
    -c "$WORK/lmp.patched.ll" -o "$WORK/lmp.c.o"

cp "$INPUT_LIB" "$OUTPUT_LIB"
"$TOOL_COMMON/llvm-ar" r "$OUTPUT_LIB" "$WORK/lmp.c.o"
"$TOOL_COMMON/llvm-ar" s "$OUTPUT_LIB"

echo "created: $OUTPUT_LIB"
