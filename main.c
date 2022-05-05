//compile using
// gcc main.c -lxwiimote -lncurses -lxdo -o main

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
#include <xdo.h>


enum window_mode {
    MODE_ERROR,
    MODE_NORMAL,
    MODE_EXTENDED,
};


static struct xwii_iface *iface;
static unsigned int mode = MODE_ERROR;
static bool freeze = false;

static bool locker = false;


/* key events */

static void key_show(const struct xwii_event *event) {

    //code to map presses to keyboard
    xdo_t *x = xdo_new(NULL);
    printf("Simulating keypress: ");

    unsigned int code = event->v.key.code;
    bool pressed = event->v.key.state;
    char *str = NULL;

    if (pressed)
        str = "X";
    else
        str = " ";
    if (!locker) {
        if (code == XWII_KEY_LEFT) {
            printf("Left arrow\n");
            xdo_send_keysequence_window(x, CURRENTWINDOW, "Left", 0);
        } else if (code == XWII_KEY_RIGHT) {
            printf("Right arrow\n");
            xdo_send_keysequence_window(x, CURRENTWINDOW, "Right", 0);
        } else if (code == XWII_KEY_UP) {
            printf("Up arrow\n");
            xdo_send_keysequence_window(x, CURRENTWINDOW, "Up", 0);
        } else if (code == XWII_KEY_DOWN) {
            printf("Down arrow\n");
            xdo_send_keysequence_window(x, CURRENTWINDOW, "Down", 0);
        } else if (code == XWII_KEY_A) {
            printf("A button - Spacebar\n");
            xdo_send_keysequence_window(x, CURRENTWINDOW, "space", 0);
        } else if (code == XWII_KEY_B) {
            printf("B button - F5 key\n");
            xdo_send_keysequence_window(x, CURRENTWINDOW, "F5", 0);
        } else if (code == XWII_KEY_HOME) {
            printf("Home button - Escape key\n");
            xdo_send_keysequence_window(x, CURRENTWINDOW, "Escape", 0);
        } else if (code == XWII_KEY_MINUS) {
            printf("Minus button - Decrease volume\n");
            xdo_send_keysequence_window(x, CURRENTWINDOW, "XF86AudioLowerVolume", 0);
        } else if (code == XWII_KEY_PLUS) {
            printf("Plus button - Increase volume\n");
            xdo_send_keysequence_window(x, CURRENTWINDOW, "XF86AudioRaiseVolume", 0);
        } else if (code == XWII_KEY_ONE) {
            printf("One button - Page Up key\n");
            xdo_send_keysequence_window(x, CURRENTWINDOW, "Page_Up", 0);
        } else if (code == XWII_KEY_TWO) {
            printf("Two button - Page Down key\n");
            xdo_send_keysequence_window(x, CURRENTWINDOW, "Page_Down", 0);
        }
    }
    locker = !locker;

    xdo_free(x);
}

static void key_clear(void) {
    struct xwii_event ev;
    unsigned int i;

    ev.type = XWII_EVENT_KEY;
    for (i = 0; i < XWII_KEY_NUM; ++i) {
        ev.v.key.code = i;
        ev.v.key.state = 0;
        //key_show(&ev);
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
        battery_show(capacity);
}

/* device type */

static void devtype_refresh(void) {
    int ret;
    char *name;

    ret = xwii_iface_get_devtype(iface, &name);
    if (!ret) {
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

    if (!ent) {
        printf("Cannot find device.\n");
        // quit since there is no wii remote
        exit(0);
    }

    return ent;
}

int main(char **argv) {
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
//        printf("\ts: Refresh static values (like battery or calibration)\n");
//        printf("\tr: Toggle rumble motor\n");

    path = get_dev(1);

    ret = xwii_iface_new(&iface, path ? path : argv[1]);
    free(path);
    if (!ret) {
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