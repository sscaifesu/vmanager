/*
 * TUI 界面实现
 * 使用 ncurses 库
 */

#include "../../include/vmanager.h"
#include <ncurses.h>
#include <string.h>
#include <time.h>

// TUI 状态
static WINDOW *header_win = NULL;
static WINDOW *list_win = NULL;
static WINDOW *status_win = NULL;
static WINDOW *help_win = NULL;

static VMInfo *tui_vm_list = NULL;
static int vm_count = 0;
static int selected_index = 0;
static int scroll_offset = 0;
static bool running = true;
static time_t last_refresh = 0;

// 颜色对
#define COLOR_HEADER 1
#define COLOR_SELECTED 2
#define COLOR_RUNNING 3
#define COLOR_STOPPED 4
#define COLOR_HELP 5

// 窗口高度
#define HEADER_HEIGHT 3
#define HELP_HEIGHT 3
#define STATUS_HEIGHT 8

// 初始化 ncurses
void tui_init(void) {
    initscr();              // 初始化屏幕
    cbreak();               // 禁用行缓冲
    noecho();               // 不显示输入
    keypad(stdscr, TRUE);   // 启用功能键
    curs_set(0);            // 隐藏光标
    
    // 初始化颜色
    if (has_colors()) {
        start_color();
        init_pair(COLOR_HEADER, COLOR_WHITE, COLOR_BLUE);
        init_pair(COLOR_SELECTED, COLOR_BLACK, COLOR_CYAN);
        init_pair(COLOR_RUNNING, COLOR_GREEN, COLOR_BLACK);
        init_pair(COLOR_STOPPED, COLOR_RED, COLOR_BLACK);
        init_pair(COLOR_HELP, COLOR_YELLOW, COLOR_BLACK);
    }
    
    refresh();
}

// 清理 ncurses
void tui_cleanup(void) {
    if (header_win) delwin(header_win);
    if (list_win) delwin(list_win);
    if (status_win) delwin(status_win);
    if (help_win) delwin(help_win);
    
    if (tui_vm_list) {
        free(tui_vm_list);
        tui_vm_list = NULL;
    }
    
    endwin();
}

// 创建窗口
static void create_windows(void) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    // 删除旧窗口
    if (header_win) delwin(header_win);
    if (list_win) delwin(list_win);
    if (status_win) delwin(status_win);
    if (help_win) delwin(help_win);
    
    // 创建新窗口
    header_win = newwin(HEADER_HEIGHT, max_x, 0, 0);
    help_win = newwin(HELP_HEIGHT, max_x, max_y - HELP_HEIGHT, 0);
    
    int list_height = max_y - HEADER_HEIGHT - HELP_HEIGHT - STATUS_HEIGHT;
    int list_width = max_x * 2 / 3;
    
    list_win = newwin(list_height, list_width, HEADER_HEIGHT, 0);
    status_win = newwin(list_height, max_x - list_width, HEADER_HEIGHT, list_width);
}

// 绘制标题栏
static void draw_header(void) {
    if (!header_win) return;
    
    werase(header_win);
    wbkgd(header_win, COLOR_PAIR(COLOR_HEADER));
    
    // 标题
    mvwprintw(header_win, 0, 2, "vmanager v%s - Proxmox VM Manager", VERSION);
    
    // 时间
    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
    mvwprintw(header_win, 0, COLS - strlen(time_str) - 2, "%s", time_str);
    
    // VM 统计
    int running_count = 0;
    for (int i = 0; i < vm_count; i++) {
        if (strcmp(tui_vm_list[i].status, "running") == 0) {
            running_count++;
        }
    }
    
    mvwprintw(header_win, 1, 2, "Total VMs: %d | Running: %d | Stopped: %d",
              vm_count, running_count, vm_count - running_count);
    
    wrefresh(header_win);
}

// 绘制 VM 列表
static void draw_vm_list(void) {
    if (!list_win) return;
    
    werase(list_win);
    box(list_win, 0, 0);
    mvwprintw(list_win, 0, 2, " Virtual Machines ");
    
    int max_y, max_x;
    getmaxyx(list_win, max_y, max_x);
    
    // 表头
    wattron(list_win, A_BOLD);
    mvwprintw(list_win, 1, 2, "%-6s %-20s %-10s %6s %10s",
              "VMID", "NAME", "STATUS", "CPU%", "MEMORY");
    wattroff(list_win, A_BOLD);
    
    // VM 列表
    int visible_lines = max_y - 4;
    
    for (int i = 0; i < vm_count && i < visible_lines; i++) {
        int vm_index = scroll_offset + i;
        if (vm_index >= vm_count) break;
        
        VMInfo *vm = &tui_vm_list[vm_index];
        int y = i + 2;
        
        // 选中高亮
        if (vm_index == selected_index) {
            wattron(list_win, COLOR_PAIR(COLOR_SELECTED) | A_BOLD);
        }
        
        // 状态颜色
        if (strcmp(vm->status, "running") == 0) {
            wattron(list_win, COLOR_PAIR(COLOR_RUNNING));
        } else {
            wattron(list_win, COLOR_PAIR(COLOR_STOPPED));
        }
        
        mvwprintw(list_win, y, 2, "%-6d %-20s %-10s %5.1f%% %10s",
                  vm->vmid,
                  vm->name,
                  vm->status,
                  vm->cpu_percent,
                  format_bytes(vm->mem));
        
        wattroff(list_win, COLOR_PAIR(COLOR_SELECTED) | A_BOLD);
        wattroff(list_win, COLOR_PAIR(COLOR_RUNNING));
        wattroff(list_win, COLOR_PAIR(COLOR_STOPPED));
    }
    
    // 滚动指示器
    if (vm_count > visible_lines) {
        mvwprintw(list_win, max_y - 1, max_x - 15, "[%d/%d]",
                  selected_index + 1, vm_count);
    }
    
    wrefresh(list_win);
}

// 绘制 VM 详情
static void draw_vm_status(void) {
    if (!status_win || vm_count == 0) return;
    
    werase(status_win);
    box(status_win, 0, 0);
    mvwprintw(status_win, 0, 2, " VM Details ");
    
    VMInfo *vm = &tui_vm_list[selected_index];
    
    int y = 2;
    mvwprintw(status_win, y++, 2, "VMID: %d", vm->vmid);
    mvwprintw(status_win, y++, 2, "Name: %s", vm->name);
    mvwprintw(status_win, y++, 2, "Status: %s", vm->status);
    y++;
    
    mvwprintw(status_win, y++, 2, "CPU: %d cores (%.1f%%)",
              vm->cpus, vm->cpu_percent);
    mvwprintw(status_win, y++, 2, "Memory: %s / %s",
              format_bytes(vm->mem), format_bytes(vm->maxmem));
    mvwprintw(status_win, y++, 2, "Disk: %s / %s",
              format_bytes(vm->disk), format_bytes(vm->maxdisk));
    y++;
    
    if (strcmp(vm->status, "running") == 0) {
        mvwprintw(status_win, y++, 2, "Uptime: %s", format_uptime(vm->uptime));
    }
    y++;
    
    mvwprintw(status_win, y++, 2, "Network:");
    mvwprintw(status_win, y++, 2, "  Bridge: %s", vm->bridge);
    mvwprintw(status_win, y++, 2, "  IP: %s", vm->ip_address);
    y++;
    
    mvwprintw(status_win, y++, 2, "Storage:");
    mvwprintw(status_win, y++, 2, "  %s", vm->storage);
    
    wrefresh(status_win);
}

// 绘制帮助栏
static void draw_help(void) {
    if (!help_win) return;
    
    werase(help_win);
    wbkgd(help_win, COLOR_PAIR(COLOR_HELP));
    
    mvwprintw(help_win, 0, 2, "Navigation: Up/Down or j/k Select | Home/End Jump");
    mvwprintw(help_win, 1, 2, "Actions: S Start | T Stop | R Reboot | D Destroy");
    mvwprintw(help_win, 2, 2, "Other: F5 Refresh | Q Quit");
    
    wrefresh(help_win);
}

// 刷新所有窗口
static void refresh_all(void) {
    draw_header();
    draw_vm_list();
    draw_vm_status();
    draw_help();
}

// 加载 VM 列表
static int load_tui_vm_list(void) {
    if (tui_vm_list) {
        free(tui_vm_list);
        tui_vm_list = NULL;
    }
    
    int ret = api_get_vm_list(&tui_vm_list, &vm_count);
    if (ret != 0) {
        return -1;
    }
    
    // 获取详细信息
    for (int i = 0; i < vm_count; i++) {
        api_get_vm_config_details(tui_vm_list[i].vmid, &tui_vm_list[i]);
        if (strcmp(tui_vm_list[i].status, "running") == 0) {
            api_get_vm_ip(tui_vm_list[i].vmid, &tui_vm_list[i]);
        }
    }
    
    return 0;
}

// 显示消息对话框
static void show_message(const char *title, const char *message) {
    // 禁用超时，防止自动刷新干扰
    timeout(-1);
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int width = 50;
    int height = 7;
    int start_y = (max_y - height) / 2;
    int start_x = (max_x - width) / 2;
    
    WINDOW *msg_win = newwin(height, width, start_y, start_x);
    box(msg_win, 0, 0);
    
    wattron(msg_win, A_BOLD);
    mvwprintw(msg_win, 0, (width - strlen(title)) / 2, " %s ", title);
    wattroff(msg_win, A_BOLD);
    
    mvwprintw(msg_win, 2, 2, "%s", message);
    mvwprintw(msg_win, 4, (width - 20) / 2, "Press any key...");
    
    wrefresh(msg_win);
    getch();
    
    delwin(msg_win);
    refresh_all();
    
    // 恢复超时
    timeout(1000);
}

// 显示确认对话框
static bool show_confirm(const char *title, const char *message) {
    // 禁用超时，防止自动刷新干扰
    timeout(-1);
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int width = 50;
    int height = 7;
    int start_y = (max_y - height) / 2;
    int start_x = (max_x - width) / 2;
    
    WINDOW *msg_win = newwin(height, width, start_y, start_x);
    box(msg_win, 0, 0);
    
    wattron(msg_win, A_BOLD);
    mvwprintw(msg_win, 0, (width - strlen(title)) / 2, " %s ", title);
    wattroff(msg_win, A_BOLD);
    
    mvwprintw(msg_win, 2, 2, "%s", message);
    mvwprintw(msg_win, 4, (width - 20) / 2, "[Y]es / [N]o");
    
    wrefresh(msg_win);
    
    int ch = getch();
    bool result = (ch == 'y' || ch == 'Y');
    
    delwin(msg_win);
    refresh_all();
    
    // 恢复超时
    timeout(1000);
    
    return result;
}

// 处理按键
static void handle_key(int ch) {
    int max_y = 0;
    if (list_win) {
        int max_x;
        getmaxyx(list_win, max_y, max_x);
        (void)max_x;  // 避免未使用警告
    }
    int visible_lines = max_y - 4;
    
    switch (ch) {
        case KEY_UP:
        case 'k':
            if (selected_index > 0) {
                selected_index--;
                if (selected_index < scroll_offset) {
                    scroll_offset = selected_index;
                }
            }
            break;
            
        case KEY_DOWN:
        case 'j':
            if (selected_index < vm_count - 1) {
                selected_index++;
                if (selected_index >= scroll_offset + visible_lines) {
                    scroll_offset = selected_index - visible_lines + 1;
                }
            }
            break;
            
        case KEY_HOME:
            selected_index = 0;
            scroll_offset = 0;
            break;
            
        case KEY_END:
            selected_index = vm_count - 1;
            scroll_offset = (vm_count > visible_lines) ? vm_count - visible_lines : 0;
            break;
            
        case 's':
        case 'S':
            // 启动 VM
            if (vm_count > 0) {
                VMInfo *vm = &tui_vm_list[selected_index];
                if (show_confirm("Start VM", "Start this VM?")) {
                    if (vm_start(vm->vmid) == 0) {
                        show_message("Success", "VM started successfully");
                        load_tui_vm_list();
                    } else {
                        show_message("Error", "Failed to start VM");
                    }
                }
            }
            break;
            
        case 't':
        case 'T':
            // 停止 VM
            if (vm_count > 0) {
                VMInfo *vm = &tui_vm_list[selected_index];
                if (show_confirm("Stop VM", "Stop this VM?")) {
                    if (vm_stop(vm->vmid) == 0) {
                        show_message("Success", "VM stopped successfully");
                        load_tui_vm_list();
                    } else {
                        show_message("Error", "Failed to stop VM");
                    }
                }
            }
            break;
            
        case 'r':
        case 'R':
            // 重启 VM
            if (vm_count > 0) {
                VMInfo *vm = &tui_vm_list[selected_index];
                if (show_confirm("Reboot VM", "Reboot this VM?")) {
                    if (vm_restart(vm->vmid) == 0) {
                        show_message("Success", "VM rebooted successfully");
                        load_tui_vm_list();
                    } else {
                        show_message("Error", "Failed to reboot VM");
                    }
                }
            }
            break;
            
        case 'd':
        case 'D':
            // 删除 VM
            if (vm_count > 0) {
                VMInfo *vm = &tui_vm_list[selected_index];
                if (show_confirm("Destroy VM", "DESTROY this VM? (Cannot undo!)")) {
                    if (vm_destroy(vm->vmid, true) == 0) {
                        show_message("Success", "VM destroyed successfully");
                        load_tui_vm_list();
                        if (selected_index >= vm_count) {
                            selected_index = vm_count - 1;
                        }
                    } else {
                        show_message("Error", "Failed to destroy VM");
                    }
                }
            }
            break;
            
        case KEY_F(5):
            // 刷新
            load_tui_vm_list();
            show_message("Refresh", "VM list refreshed");
            break;
            
        case 'q':
        case 'Q':
            // 退出
            if (show_confirm("Quit", "Are you sure to quit?")) {
                running = false;
            }
            break;
    }
}

// TUI 主循环
int tui_main(void) {
    tui_init();
    
    // 加载 VM 列表
    if (load_tui_vm_list() != 0) {
        tui_cleanup();
        fprintf(stderr, "错误：无法加载 VM 列表\n");
        return 1;
    }
    
    // 创建窗口
    create_windows();
    
    // 设置超时（用于自动刷新）
    timeout(1000);  // 1 秒超时
    
    // 初始绘制
    refresh_all();
    
    // 主循环
    while (running) {
        int ch = getch();
        
        if (ch != ERR) {
            handle_key(ch);
            refresh_all();
        }
        
        // 自动刷新（每 30 秒）
        time_t now = time(NULL);
        if (now - last_refresh >= 30) {
            load_tui_vm_list();
            refresh_all();
            last_refresh = now;
        }
    }
    
    tui_cleanup();
    return 0;
}
