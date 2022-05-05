//compile using
// gcc main.c -lxwiimote -lncurses -o main

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <ncurses.h>
#include <poll.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
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

/* error messages */

static void nextSlide() {
    hThread = GetWindowThreadProcessId(hwnd,&dwPID);
    if (dwPID == dwProcessID && hThread!= NULL ) {
        PostThreadMessage( hThread, WM_KEYDOWN,'A',1);
    }
}

static void print_info(const char *format, ...) {
    va_list list;
    char str[58 + 1];

    va_start(list, format);
    vsnprintf(str, sizeof(str), format, list);
    str[sizeof(str) - 1] = 0;
    va_end(list);

    mvprintw(22, 22, "                                                          ");
    mvprintw(22, 22, "%s", str);
    refresh();
}

static void print_error(const char *format, ...) {
    va_list list;
    char str[58 + 80 + 1];

    va_start(list, format);
    vsnprintf(str, sizeof(str), format, list);
    if (mode == MODE_EXTENDED)
        str[sizeof(str) - 1] = 0;
    else
        str[58] = 0;
    va_end(list);

    mvprintw(23, 22, "                                                          ");
    if (mode == MODE_EXTENDED)
        mvprintw(23, 80, "                                                                                ");
    mvprintw(23, 22, "%s", str);
    refresh();
}

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

static void key_toggle(void) {
    int ret;

    if (xwii_iface_opened(iface) & XWII_IFACE_CORE) {
        xwii_iface_close(iface, XWII_IFACE_CORE);
        key_clear();
        print_info("Info: Disable key events");
    } else {
        ret = xwii_iface_open(iface, XWII_IFACE_CORE |
                                     XWII_IFACE_WRITABLE);
        if (ret)
            print_error("Error: Cannot enable key events: %d", ret);
        else
            print_info("Info: Enable key events");
    }
}


/* rumble events */

static void rumble_show(bool on) {
    mvprintw(1, 21, on ? "RUMBLE" : "      ");
}

static void rumble_toggle(void) {
    static bool on = false;
    int ret;

    on = !on;
    ret = xwii_iface_rumble(iface, on);
    if (ret) {
        print_error("Error: Cannot toggle rumble motor: %d", ret);
        on = !on;
    }

    rumble_show(on);
}

/* LEDs */

static bool led_state[4];

static void led_show(int n, bool on) {
    mvprintw(5, 59 + n * 5, on ? "(#%i)" : " -%i ", n + 1);
}

static void led_toggle(int n) {
    int ret;

    led_state[n] = !led_state[n];
    ret = xwii_iface_set_led(iface, XWII_LED(n + 1), led_state[n]);
    if (ret) {
        print_error("Error: Cannot toggle LED %i: %d", n + 1, ret);
        led_state[n] = !led_state[n];
    }
    led_show(n, led_state[n]);
}

static void led_refresh(int n) {
    int ret;

    ret = xwii_iface_get_led(iface, XWII_LED(n + 1), &led_state[n]);
    if (ret)
        print_error("Error: Cannot read LED state");
    else
        led_show(n, led_state[n]);
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
    if (ret)
        print_error("Error: Cannot read battery capacity");
    else
        battery_show(capacity);
}

/* device type */

static void devtype_refresh(void) {
    int ret;
    char *name;

    ret = xwii_iface_get_devtype(iface, &name);
    if (ret) {
        print_error("Error: Cannot read device type");
    } else {
        mvprintw(9, 28, "                                                   ");
        mvprintw(9, 28, "%s", name);
        free(name);
    }
}

/* extension type */

static void extension_refresh(void) {
    int ret;
    char *name;

    ret = xwii_iface_get_extension(iface, &name);
    if (ret) {
        print_error("Error: Cannot read extension type");
    } else {
        mvprintw(7, 54, "                      ");
        mvprintw(7, 54, "%s", name);
        free(name);
    }

    if (xwii_iface_available(iface) & XWII_IFACE_MOTION_PLUS)
        mvprintw(7, 77, "M+");
    else
        mvprintw(7, 77, "  ");
}

/* basic window setup */

static void refresh_all(void) {
    battery_refresh();
    led_refresh(0);
    led_refresh(1);
    led_refresh(2);
    led_refresh(3);
    devtype_refresh();
    extension_refresh();

    if (geteuid() != 0)
        mvprintw(20, 22, "Warning: Please run as root! (sysfs+evdev access needed)");
}

static void setup_window(void) {
    size_t i;

    i = 0;
    /* 80x24 Box */
    mvprintw(i++, 0, "+- Keys ----------+ +------+ +---------------------------------+---------------+");
    mvprintw(i++, 0, "|       +-+       | |      |  Accel x:       y:       z:       | XWIIMOTE SHOW |");
    mvprintw(i++, 0, "|       | |       | +------+ +---------------------------------+---------------+");
    mvprintw(i++, 0, "|     +-+ +-+     | IR #1:     x     #2:     x     #3:     x     #4:     x     |");
    mvprintw(i++, 0, "|     |     |     | +--------------------------------+-------------------------+");
    mvprintw(i++, 0, "|     +-+ +-+     | MP x:        y:        z:        | LED  -0   -1   -2   -3  |");
    mvprintw(i++, 0, "|       | |       | +--------------------------+-----+----------------------+--+");
    mvprintw(i++, 0, "|       +-+       | Battery:      |          | | Ext:                       |  |");
    mvprintw(i++, 0, "|                 | +--------------------------+----------------------------+--+");
    mvprintw(i++, 0, "|   +-+     +-+   | Device:                                                    |");
    mvprintw(i++, 0, "|   | |     | |   | +----------------------------------------------------------+");
    mvprintw(i++, 0, "|   +-+     +-+   |                                                            |");
    mvprintw(i++, 0, "|                 |                                                            |");
    mvprintw(i++, 0, "| ( ) |     | ( ) |                                                            |");
    mvprintw(i++, 0, "|                 |                                                            |");
    mvprintw(i++, 0, "|      +++++      |                                                            |");
    mvprintw(i++, 0, "|      +   +      |                                                            |");
    mvprintw(i++, 0, "|      +   +      | +----------------------------------------------------------+");
    mvprintw(i++, 0, "|      +++++      | HINT: Keep the remote still and press 's' to recalibrate!  |");
    mvprintw(i++, 0, "|                 | +----------------------------------------------------------+");
    mvprintw(i++, 0, "|       | |       | |                                                          |");
    mvprintw(i++, 0, "|       | |       | +----------------------------------------------------------+");
    mvprintw(i++, 0, "|                 | |                                                           ");
    mvprintw(i++, 0, "+-----------------+ |");
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

    print_info("Info: Watch Event #%u", ++num);

    ret = xwii_iface_open(iface, xwii_iface_available(iface) |
                                 XWII_IFACE_WRITABLE);
    if (ret)
        print_error("Error: Cannot open interface: %d", ret);

    refresh_all();
}

/* keyboard handling */

static void freeze_toggle(void) {
    freeze = !freeze;
    print_info("Info: %sreeze screen", freeze ? "F" : "Unf");
}

static int keyboard(void) {
    int key;

    key = getch();
    if (key == ERR)
        return 0;

    switch (key) {
        case KEY_RESIZE:
            handle_resize();
            break;
        case 'k':
            key_toggle();
            break;
        case 'r':
            rumble_toggle();
            break;
        case '1':
            led_toggle(0);
            break;
        case '2':
            led_toggle(1);
            break;
        case '3':
            led_toggle(2);
            break;
        case '4':
            led_toggle(3);
            break;
        case 'f':
            freeze_toggle();
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
    if (ret)
        print_error("Error: Cannot initialize hotplug watch descriptor");

    while (true) {
        ret = poll(fds, fds_num, -1);
        if (ret < 0) {
            if (errno != EINTR) {
                ret = -errno;
                print_error("Error: Cannot poll fds: %d", ret);
                break;
            }
        }

        ret = xwii_iface_dispatch(iface, &event, sizeof(event));
        if (ret) {
            if (ret != -EAGAIN) {
                print_error("Error: Read failed with err:%d",
                            ret);
                break;
            }
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
        printf("Cannot find device with number #%d\n", 1);

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
//        printf("\ta: Toggle accelerometer\n");
//        printf("\ti: Toggle IR camera\n");
//        printf("\tm: Toggle motion plus\n");
//        printf("\tn: Toggle normalization for motion plus\n");
//        printf("\tb: Toggle balance board\n");
//        printf("\tp: Toggle pro controller\n");
//        printf("\tg: Toggle guitar controller\n");
//        printf("\td: Toggle drums controller\n");
//        printf("\t1: Toggle LED 1\n");
//        printf("\t2: Toggle LED 2\n");
//        printf("\t3: Toggle LED 3\n");
//        printf("\t4: Toggle LED 4\n");
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
        if (ret)
            print_error("Error: Cannot open interface: %d",
                        ret);

        ret = run_iface(iface);
        xwii_iface_unref(iface);
        if (ret) {
            print_error("Program failed; press any key to exit");
            refresh();
            timeout(-1);
            getch();
        }
        endwin();
    }
    //}

    return abs(ret);
}