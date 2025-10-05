# Makefile for vmanager v4.0.1
# 遵循 GNU Make 标准

# 编译器和标志
CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -std=c11 -Iinclude
LDFLAGS = -lcurl -lncurses -lm
DEBUG_FLAGS = -g -DDEBUG

# 目标和源文件
TARGET = vmanager
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man/man1

# 源文件
CORE_SRCS = src/core/api.c src/core/config.c src/core/vm.c
UI_SRCS = src/ui/cli.c src/ui/tui.c
UTILS_SRCS = src/utils/json.c src/utils/common.c
MAIN_SRC = src/main.c
LIB_SRCS = cJSON.c

SRCS = $(MAIN_SRC) $(CORE_SRCS) $(UI_SRCS) $(UTILS_SRCS) $(LIB_SRCS)
OBJS = $(SRCS:.c=.o)

# 默认目标
.PHONY: all
all: $(TARGET)

# 链接目标
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✓ Build complete: $(TARGET)"

# 编译规则
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# 调试版本
.PHONY: debug
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)

# 清理
.PHONY: clean
clean:
	@echo "Cleaning..."
	rm -f $(TARGET) $(OBJS)
	@echo "✓ Clean complete"

# 安装
.PHONY: install
install: $(TARGET)
	@echo "Installing $(TARGET) to $(BINDIR)..."
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/
	@echo "✓ Installation complete"
	@echo "Run '$(TARGET) --help' to get started"

# 卸载
.PHONY: uninstall
uninstall:
	@echo "Uninstalling $(TARGET)..."
	rm -f $(BINDIR)/$(TARGET)
	@echo "✓ Uninstallation complete"

# 测试
.PHONY: test
test: $(TARGET)
	@echo "Running tests..."
	@./$(TARGET) --version
	@./$(TARGET) --help > /dev/null
	@echo "✓ Basic tests passed"

# 检查依赖
.PHONY: check-deps
check-deps:
	@echo "Checking dependencies..."
	@which $(CC) > /dev/null || (echo "Error: gcc not found" && exit 1)
	@pkg-config --exists libcurl || (echo "Error: libcurl not found" && exit 1)
	@echo "✓ All dependencies satisfied"

# 帮助
.PHONY: help
help:
	@echo "vmanager v4.0.1 Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build the program (default)"
	@echo "  debug       - Build with debug symbols"
	@echo "  clean       - Remove built files"
	@echo "  install     - Install to $(BINDIR)"
	@echo "  uninstall   - Remove from $(BINDIR)"
	@echo "  test        - Run basic tests"
	@echo "  check-deps  - Check build dependencies"
	@echo "  help        - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  PREFIX      - Installation prefix (default: /usr/local)"
	@echo "  CC          - C compiler (default: gcc)"
	@echo ""
	@echo "Example:"
	@echo "  make"
	@echo "  make install"
	@echo "  make PREFIX=/opt install"

# 依赖关系
src/main.o: src/main.c include/vmanager.h
src/core/api.o: src/core/api.c include/vmanager.h cJSON.h
src/core/config.o: src/core/config.c include/vmanager.h
src/core/vm.o: src/core/vm.c include/vmanager.h
src/ui/cli.o: src/ui/cli.c include/vmanager.h
src/ui/tui.o: src/ui/tui.c include/vmanager.h
src/utils/json.o: src/utils/json.c include/vmanager.h cJSON.h
src/utils/common.o: src/utils/common.c include/vmanager.h
cJSON.o: cJSON.c cJSON.h
