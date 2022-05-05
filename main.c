//compile using
// gcc main.c -lxwiimote -lncurses -o main

#include <errno.h>
#include <ncurses.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xwiimote.h>

enum window_mode {
    MODE_ERROR,
    MODE_NORMAL,
    MODE_EXTENDED,
};



static struct xwii_iface *iface;
static unsigned int mode = MODE_ERROR;
static bool freeze = false;

// static void nextSlide() {
//     hThread = GetWindowThreadProcessId(hwnd,&dwPID);
//     if (dwPID == dwProcessID && hThread!= NULL ) {
//         PostThreadMessage( hThread, WM_KEYDOWN,'A',1);
//     }
// }

/* key events */

static void key_show(const struct xwii_event *event) {
    unsigned int code = event->v.key.code;
    bool pressed = event->v.key.state;
    char *str = NULL;

    if (pressed)
        str = "X";
    else
        str = " ";

    if (code == XWII_KEY_LEFT) {
        mvprintw(4, 7, "%s", str);
    } else if (code == XWII_KEY_RIGHT) {
        mvprintw(4, 11, "%s", str);
    } else if (code == XWII_KEY_UP) {
        mvprintw(2, 9, "%s", str);
    } else if (code == XWII_KEY_DOWN) {
        mvprintw(6, 9, "%s", str);
    } else if (code == XWII_KEY_A) {
        if (pressed)
            str = "A";
        mvprintw(10, 5, "%s", str);
    } else if (code == XWII_KEY_B) {
        if (pressed)
            str = "B";
        mvprintw(10, 13, "%s", str);
    } else if (code == XWII_KEY_HOME) {
        if (pressed)
            str = "HOME+";
        else
            str = "     ";
        mvprintw(13, 7, "%s", str);
    } else if (code == XWII_KEY_MINUS) {
        if (pressed)
            str = "-";
        mvprintw(13, 3, "%s", str);
    } else if (code == XWII_KEY_PLUS) {
        if (pressed)
            str = "+";
        mvprintw(13, 15, "%s", str);
    } else if (code == XWII_KEY_ONE) {
        if (pressed)
            str = "1";
        mvprintw(20, 9, "%s", str);
    } else if (code == XWII_KEY_TWO) {
        if (pressed)
            str = "2";
        mvprintw(21, 9, "%s", str);
    }
}

static void key_clear(void) {
    struct xwii_event ev;
    unsigned int i;

    ev.type = XWII_EVENT_KEY;
    for (i = 0; i < XWII_KEY_NUM; ++i) {
        ev.v.key.code = i;
        ev.v.key.state = 0;
        key_show(&ev);
    }
}



/* rumble events */

static void rumble_show(bool on) {
    printf("Toggled Rumble");
    mvprintw(1, 21, on ? "RUMBLE" : "      ");
}

static void rumble_toggle(void) {
    static bool on = false;
    int ret;

    on = !on;
    ret = xwii_iface_rumble(iface, on);
    if (ret) {
        on = !on;
    }

    rumble_show(on);
}



/* battery status */

static void battery_show(uint8_t capacity) {
    int i;

    mvprintw(7, 29, "%3u%%", capacity);

    mvprintw(7, 35, "          ");
    for (i = 0; i * 10 < capacity; ++i)
        mvprintw(7, 35 + i, "#");
}

static void battery_refresh(void) {
    int ret;
    uint8_t capacity;

    ret = xwii_iface_get_battery(iface, &capacity);
    if (!ret)
        //print_error("Error: Cannot read battery capacity");
    //else
        battery_show(capacity);
}

/* device type */

static void devtype_refresh(void) {
    int ret;
    char *name;

    ret = xwii_iface_get_devtype(iface, &name);
    if (!ret) {
       // print_error("Error: Cannot read device type");
    //} else {
        mvprintw(9, 28, "                                                   ");
        mvprintw(9, 28, "%s", name);
        free(name);
    }
}


/* basic window setup */

static void refresh_all(void) {
    battery_refresh();
    devtype_refresh();

    if (geteuid() != 0)
        mvprintw(20, 22, "Warning: Please run as root! (sysfs+evdev access needed)");
}

static void setup_window(void) {
    size_t i;

    i = 0;
    /* 80x24 Box */
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
    mvprintw(i++, 0, "                                                                                ");
}


static void handle_resize(void) {
    if (LINES < 24 || COLS < 80) {
        mode = MODE_ERROR;
        erase();
        mvprintw(0, 0, "Error: Screen smaller than 80x24; no view");
    } else {
        mode = MODE_NORMAL;
        erase();
        setup_window();
        refresh_all();
     //   print_info("Info: Screen smaller than 160x48; limited view");
    }
}

/* device watch events */

static void handle_watch(void) {
    static unsigned int num;
    int ret;


    ret = xwii_iface_open(iface, xwii_iface_available(iface) |
                                 XWII_IFACE_WRITABLE);

    refresh_all();
}

/* keyboard handling */

static int keyboard(void) {
    int key;

    key = getch();
    if (key == ERR)
        return 0;

    switch (key) {
        case KEY_RESIZE:
            handle_resize();
            break;
        case 'r':
            rumble_toggle();
            break;
        case 's':
            refresh_all();
            break;
        case 'q':
            return -ECANCELED;
    }

    return 0;
}

static int run_iface(struct xwii_iface *iface) {
    struct xwii_event event;
    int ret = 0, fds_num;
    struct pollfd fds[2];

    memset(fds, 0, sizeof(fds));
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[1].fd = xwii_iface_get_fd(iface);
    fds[1].events = POLLIN;
    fds_num = 2;

    ret = xwii_iface_watch(iface, true);

    while (true) {
        ret = poll(fds, fds_num, -1);
        if (ret < 0) {
            if (errno != EINTR) {
                ret = -errno;
                break;
            }
        }

        ret = xwii_iface_dispatch(iface, &event, sizeof(event));
        if (ret) {

        } else if (!freeze) {
            switch (event.type) {
                case XWII_EVENT_WATCH:
                    handle_watch();
                    break;
                case XWII_EVENT_KEY:
                    if (mode != MODE_ERROR)
                        key_show(&event);
                    break;

            }
        }

        ret = keyboard();
        if (ret == -ECANCELED)
            return 0;
        else if (ret)
            return ret;
        refresh();
    }

    return ret;
}

static int enumerate() {
    struct xwii_monitor *mon;
    char *ent;
    int num = 0;

    mon = xwii_monitor_new(false, false);
    if (!mon) {
        printf("Cannot create monitor\n");
        return -EINVAL;
    }

    while ((ent = xwii_monitor_poll(mon))) {
        printf("  Found device #%d: %s\n", ++num, ent);
        free(ent);
    }

    xwii_monitor_unref(mon);
    return 0;
}

static char *get_dev() {
    struct xwii_monitor *mon;
    char *ent;
    int i = 0;

    mon = xwii_monitor_new(false, false);
    if (!mon) {
        printf("Cannot create monitor\n");
        return NULL;
    }

    while ((ent = xwii_monitor_poll(mon))) {
        if (++i == 1)
            break;
        free(ent);
    }

    xwii_monitor_unref(mon);

    if (!ent)
        printf("Cannot find device.");

    return ent;
}

int main(int argc, char **argv) {
    int ret = 0;
    char *path = NULL;

//    if (argc < 2 || !strcmp(argv[1], "-h")) {
//        printf("Usage:\n");
//        printf("\txwiishow [-h]: Show help\n");
//        printf("\txwiishow list: List connected devices\n");
//        printf("\txwiishow <num>: Show device with number #num\n");
//        printf("\txwiishow /sys/path/to/device: Show given device\n");
//        printf("UI commands:\n");
//        printf("\tq: Quit application\n");
//        printf("\tf: Freeze/Unfreeze screen\n");
//        printf("\ts: Refresh static values (like battery or calibration)\n");
//        printf("\tk: Toggle key events\n");
//        printf("\tr: Toggle rumble motor\n");
//        ret = -1;
//    } else
//        if (!strcmp(argv[1], "list")) {
//        printf("Listing connected Wii Remote devices:\n");
//        ret = enumerate();
//        printf("End of device list\n");
//    } else {
//        if (argv[1][0] != '/')
    path = get_dev(1);

    ret = xwii_iface_new(&iface, path ? path : argv[1]);
    free(path);
    if (ret) {
        printf("Cannot create xwii_iface '%s' err:%d\n",
               argv[1], ret);
    } else {

        initscr();
        curs_set(0);
        raw();
        noecho();
        timeout(0);

        handle_resize();
        key_clear();
        refresh_all();
        refresh();

        ret = xwii_iface_open(iface,
                              xwii_iface_available(iface) |
                              XWII_IFACE_WRITABLE);
        ret = run_iface(iface);
        xwii_iface_unref(iface);
        if (ret) {
            refresh();
            timeout(-1);
            getch();
        }
        endwin();
    }

    return abs(ret);
}