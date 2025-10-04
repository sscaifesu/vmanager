# Makefile for vmanager v2.0
# Proxmox 虚拟机管理工具

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
TARGET = vmanager
SOURCE = vmanager.c
INSTALL_DIR = /usr/local/bin

.PHONY: all clean install uninstall test help

all: $(TARGET)

$(TARGET): $(SOURCE)
	@echo "🔨 编译 $(TARGET) v2.0..."
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)
	@echo "✅ 编译完成！"
	@echo ""
	@echo "使用方法:"
	@echo "  sudo ./$(TARGET) --help      - 查看帮助"
	@echo "  sudo ./$(TARGET) --version   - 查看版本"
	@echo "  sudo ./$(TARGET) start 100   - 启动 VM 100"
	@echo "  make install                 - 安装到系统"

clean:
	@echo "🧹 清理编译文件..."
	rm -f $(TARGET)
	@echo "✅ 清理完成！"

install: $(TARGET)
	@echo "📦 安装 $(TARGET) 到 $(INSTALL_DIR)..."
	install -m 755 $(TARGET) $(INSTALL_DIR)/$(TARGET)
	@echo "✅ 安装完成！"
	@echo ""
	@echo "现在可以直接使用:"
	@echo "  sudo $(TARGET) --help"
	@echo "  sudo $(TARGET) start 100"

uninstall:
	@echo "🗑️  卸载 $(TARGET)..."
	rm -f $(INSTALL_DIR)/$(TARGET)
	@echo "✅ 卸载完成！"

test: $(TARGET)
	@echo "🧪 运行测试..."
	@echo ""
	@echo "测试 1: 显示帮助"
	@./$(TARGET) --help | head -20
	@echo ""
	@echo "测试 2: 显示版本"
	@./$(TARGET) --version
	@echo ""
	@echo "✅ 测试完成！"

help:
	@echo "vmanager v2.0 - Makefile 帮助"
	@echo ""
	@echo "可用目标:"
	@echo "  make          - 编译程序"
	@echo "  make clean    - 清理编译文件"
	@echo "  make install  - 安装到系统 (需要 root)"
	@echo "  make uninstall- 从系统卸载 (需要 root)"
	@echo "  make test     - 运行测试"
	@echo "  make help     - 显示此帮助信息"
	@echo ""
	@echo "使用示例:"
	@echo "  make && sudo ./$(TARGET) --help"
	@echo "  sudo make install && sudo $(TARGET) start 100"
