# Makefile for Proxmox VM Manager
# 用法: make 或 make install

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
TARGET = vm_manager
SOURCE = vm_manager.c
INSTALL_DIR = /usr/local/bin

.PHONY: all clean install uninstall help

all: $(TARGET)

$(TARGET): $(SOURCE)
	@echo "🔨 编译 $(TARGET)..."
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)
	@echo "✅ 编译完成！"
	@echo ""
	@echo "使用方法:"
	@echo "  sudo ./$(TARGET)          - 直接运行"
	@echo "  make install              - 安装到系统"
	@echo "  make clean                - 清理编译文件"

clean:
	@echo "🧹 清理编译文件..."
	rm -f $(TARGET)
	@echo "✅ 清理完成！"

install: $(TARGET)
	@echo "📦 安装 $(TARGET) 到 $(INSTALL_DIR)..."
	install -m 755 $(TARGET) $(INSTALL_DIR)/$(TARGET)
	@echo "✅ 安装完成！"
	@echo ""
	@echo "现在可以直接使用: sudo $(TARGET)"

uninstall:
	@echo "🗑️  卸载 $(TARGET)..."
	rm -f $(INSTALL_DIR)/$(TARGET)
	@echo "✅ 卸载完成！"

help:
	@echo "Proxmox VM Manager - Makefile 帮助"
	@echo ""
	@echo "可用目标:"
	@echo "  make          - 编译程序"
	@echo "  make clean    - 清理编译文件"
	@echo "  make install  - 安装到系统 (需要 root)"
	@echo "  make uninstall- 从系统卸载 (需要 root)"
	@echo "  make help     - 显示此帮助信息"
	@echo ""
	@echo "使用示例:"
	@echo "  make && sudo ./$(TARGET)"
	@echo "  sudo make install && sudo $(TARGET)"
