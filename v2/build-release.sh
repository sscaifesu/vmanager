#!/bin/bash
# vmanager v2.0.0 Release 构建脚本
# 支持多平台交叉编译

set -e

VERSION="2.0.0"
PROGRAM="vmanager"
SOURCE="vmanager.c"

echo "╔══════════════════════════════════════════════════════════════════╗"
echo "║          vmanager v${VERSION} Release 构建                        ║"
echo "╚══════════════════════════════════════════════════════════════════╝"
echo ""

# 创建 release 目录
RELEASE_DIR="release-${VERSION}"
rm -rf "${RELEASE_DIR}"
mkdir -p "${RELEASE_DIR}"

echo "开始构建多平台版本..."
echo ""

# 1. Linux AMD64
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "1. 构建 Linux AMD64..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
gcc -Wall -Wextra -O2 -std=c11 -o "${RELEASE_DIR}/${PROGRAM}-linux-amd64" "${SOURCE}"
if [ $? -eq 0 ]; then
    echo "✓ Linux AMD64 构建成功"
    ls -lh "${RELEASE_DIR}/${PROGRAM}-linux-amd64"
else
    echo "✗ Linux AMD64 构建失败"
    exit 1
fi
echo ""

# 2. Linux ARM64 (交叉编译)
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "2. 构建 Linux ARM64..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if command -v aarch64-linux-gnu-gcc &> /dev/null; then
    aarch64-linux-gnu-gcc -Wall -Wextra -O2 -std=c11 -static \
        -o "${RELEASE_DIR}/${PROGRAM}-linux-arm64" "${SOURCE}"
    if [ $? -eq 0 ]; then
        echo "✓ Linux ARM64 构建成功"
        ls -lh "${RELEASE_DIR}/${PROGRAM}-linux-arm64"
    else
        echo "✗ Linux ARM64 构建失败"
    fi
else
    echo "⚠ 跳过 Linux ARM64 (需要 aarch64-linux-gnu-gcc)"
    echo "  安装: sudo apt-get install gcc-aarch64-linux-gnu"
fi
echo ""

# 3. macOS ARM64 (交叉编译)
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "3. 构建 macOS ARM64..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if command -v aarch64-apple-darwin-gcc &> /dev/null || command -v o64-clang &> /dev/null; then
    # 使用 osxcross 或其他交叉编译工具
    echo "⚠ macOS 交叉编译需要 osxcross 工具链"
    echo "  由于环境限制，建议在 macOS 上本地编译"
else
    echo "⚠ 跳过 macOS ARM64 (需要 osxcross 或在 macOS 上编译)"
    echo "  在 macOS 上编译命令:"
    echo "  clang -Wall -Wextra -O2 -std=c11 -o vmanager-macos-arm64 vmanager.c"
fi
echo ""

# 4. Linux ARM (32-bit)
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "4. 构建 Linux ARM (32-bit)..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if command -v arm-linux-gnueabihf-gcc &> /dev/null; then
    arm-linux-gnueabihf-gcc -Wall -Wextra -O2 -std=c11 -static \
        -o "${RELEASE_DIR}/${PROGRAM}-linux-arm" "${SOURCE}"
    if [ $? -eq 0 ]; then
        echo "✓ Linux ARM 构建成功"
        ls -lh "${RELEASE_DIR}/${PROGRAM}-linux-arm"
    else
        echo "✗ Linux ARM 构建失败"
    fi
else
    echo "⚠ 跳过 Linux ARM (需要 arm-linux-gnueabihf-gcc)"
    echo "  安装: sudo apt-get install gcc-arm-linux-gnueabihf"
fi
echo ""

# 生成校验和
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "生成校验和..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
cd "${RELEASE_DIR}"
sha256sum ${PROGRAM}-* > SHA256SUMS 2>/dev/null || true
cd ..
echo "✓ 校验和已生成"
echo ""

# 创建 README
cat > "${RELEASE_DIR}/README.txt" << 'EOF'
vmanager v2.0.0 - Proxmox 虚拟机管理工具

## 平台支持

- vmanager-linux-amd64   : Linux x86_64 (Intel/AMD 64位)
- vmanager-linux-arm64   : Linux ARM64 (ARMv8 64位)
- vmanager-linux-arm     : Linux ARM (ARMv7 32位)
- vmanager-macos-arm64   : macOS Apple Silicon (M1/M2/M3)

## 安装方法

1. 下载对应平台的二进制文件
2. 添加执行权限: chmod +x vmanager-*
3. 移动到系统路径: sudo mv vmanager-* /usr/local/bin/vmanager
4. 验证安装: vmanager --version

## 使用示例

# 查看帮助
vmanager --help

# 启动 VM
sudo vmanager start 100

# 批量操作
sudo vmanager stop 100-110

## 校验文件完整性

sha256sum -c SHA256SUMS

## 更多信息

项目主页: https://github.com/sscaifesu/vmanager
文档: https://github.com/sscaifesu/vmanager/blob/main/v2/README.md
许可证: GPL v3.0

作者: YXWA Infosec Lab (Crystal & evalEvil)
EOF

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "构建完成！"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Release 文件位于: ${RELEASE_DIR}/"
ls -lh "${RELEASE_DIR}/"
echo ""
echo "下一步:"
echo "1. 压缩 release 文件: tar -czf vmanager-v${VERSION}.tar.gz ${RELEASE_DIR}/"
echo "2. 在 GitHub 创建 Release 并上传文件"
echo "3. 访问: https://github.com/sscaifesu/vmanager/releases/new"
echo ""
