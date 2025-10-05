#!/bin/bash
# 构建脚本 for vmanager v4.0.0

set -e

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Building vmanager v4.0.0..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# 清理
echo "Cleaning..."
rm -f vmanager src/*.o src/core/*.o src/ui/*.o src/utils/*.o *.o

# 编译标志
CFLAGS="-Wall -Wextra -O2 -std=c11 -Iinclude"
LDFLAGS="-lcurl -lncurses -lm"

# 编译源文件
echo "Compiling cJSON.c..."
gcc $CFLAGS -c cJSON.c -o cJSON.o

echo "Compiling src/utils/json.c..."
gcc $CFLAGS -c src/utils/json.c -o src/utils/json.o

echo "Compiling src/utils/common.c..."
gcc $CFLAGS -c src/utils/common.c -o src/utils/common.o

echo "Compiling src/core/config.c..."
gcc $CFLAGS -c src/core/config.c -o src/core/config.o

echo "Compiling src/core/api.c..."
gcc $CFLAGS -c src/core/api.c -o src/core/api.o

gcc $CFLAGS -c src/core/vm.c -o src/core/vm.o

echo "Compiling src/ui/cli.c..."
gcc $CFLAGS -c src/ui/cli.c -o src/ui/cli.o

echo "Compiling src/ui/tui.c..."
gcc $CFLAGS -c src/ui/tui.c -o src/ui/tui.o

echo "Compiling src/main.c..."
gcc $CFLAGS -c src/main.c -o src/main.o

# 链接
echo "Linking vmanager..."
gcc $CFLAGS -o vmanager src/main.o src/core/api.o src/core/config.o src/core/vm.o src/ui/cli.o src/ui/tui.o src/utils/json.o src/utils/common.o cJSON.o $LDFLAGS

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✓ Build complete: vmanager"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Run './vmanager --help' to get started"
