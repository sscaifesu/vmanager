#!/bin/bash
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "vmanager v3 完整功能测试"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

echo -e "\n【测试 1】本地模式 - list"
./vmanager --mode local list
echo "状态: $?"

echo -e "\n【测试 2】远程模式 - list"
./vmanager --mode remote list
echo "状态: $?"

echo -e "\n【测试 3】远程模式 - list -v"
./vmanager --mode remote list -v
echo "状态: $?"

echo -e "\n【测试 4】远程模式 - status 112"
./vmanager --mode remote status 112 | head -20
echo "状态: $?"

echo -e "\n【测试 5】本地模式 - status 112"
./vmanager --mode local status 112 | head -20
echo "状态: $?"

echo -e "\n【测试 6】帮助信息"
./vmanager --help | head -10
echo "状态: $?"

echo -e "\n【测试 7】版本信息"
./vmanager --version
echo "状态: $?"

echo -e "\n【测试 8】配置文件检查"
if [ -f ~/.vmanager.conf ]; then
    echo "✓ 配置文件存在"
    head -5 ~/.vmanager.conf
else
    echo "✗ 配置文件不存在"
fi

echo -e "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "测试完成"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
