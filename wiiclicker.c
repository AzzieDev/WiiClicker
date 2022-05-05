// WiiClicker - Created by David Katz at Towson University

// compile using:
// gcc wiiclicker.c -lxwiimote -lncurses -lxdo -o wiiclicker

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

static struct xwii_iface * interface;

// shown whenever the screen updates
void showCommands() {
				mvprintw(1, 0, "WiiClicker by David Katz");
				mvprintw(3, 0, "b: Show battery percentage");
				mvprintw(4, 0, "r: Toggle rumble motor");
				mvprintw(5, 0, "q: Quit application");

}

// locker variable to prevent dupe key-presses
static bool locker = false;

// converts button presses to keyboard mapping
static void key_show(const struct xwii_event * event) {
				// initialize xdo object for keyboard presses
				xdo_t * x = xdo_new(NULL);
				unsigned int code = event -> v.key.code;
				// prevent duplicates for key up vs key down
				if (!locker) {
								clear();
								mvprintw(7, 0, "Wii Remote Button pressed:");
								// check each of our key types and simulate presses accordingly
								if (code == XWII_KEY_LEFT) {
												mvprintw(8, 0, "D-Pad Left - Left Arrow key keyboard press simulated");
												xdo_send_keysequence_window(x, CURRENTWINDOW, "Left", 0);
								} else if (code == XWII_KEY_RIGHT) {
												mvprintw(8, 0, "D-Pad Right - Right arrow key keyboard press simulated");
												xdo_send_keysequence_window(x, CURRENTWINDOW, "Right", 0);
								} else if (code == XWII_KEY_UP) {
												mvprintw(8, 0, "D-Pad Up - Up arrow key keyboard press simulated");
												xdo_send_keysequence_window(x, CURRENTWINDOW, "Up", 0);
								} else if (code == XWII_KEY_DOWN) {
												mvprintw(8, 0, "D-Pad Down - Down arrow key keyboard press simulated");
												xdo_send_keysequence_window(x, CURRENTWINDOW, "Down", 0);
								} else if (code == XWII_KEY_A) {
												mvprintw(8, 0, "A button - Space Bar key keyboard press simulated");
												xdo_send_keysequence_window(x, CURRENTWINDOW, "space", 0);
								} else if (code == XWII_KEY_B) {
												mvprintw(8, 0, "B button - F5 key keyboard press simulated");
												xdo_send_keysequence_window(x, CURRENTWINDOW, "F5", 0);
								} else if (code == XWII_KEY_HOME) {
												mvprintw(8, 0, "Home button - Escape key keyboard press simulated");
												xdo_send_keysequence_window(x, CURRENTWINDOW, "Escape", 0);
								} else if (code == XWII_KEY_MINUS) {
												mvprintw(8, 0, "Minus button - Decreasing volume");
												xdo_send_keysequence_window(x, CURRENTWINDOW, "XF86AudioLowerVolume", 0);
								} else if (code == XWII_KEY_PLUS) {
												mvprintw(8, 0, "Plus button - Increasing volume");
												xdo_send_keysequence_window(x, CURRENTWINDOW, "XF86AudioRaiseVolume", 0);
								} else if (code == XWII_KEY_ONE) {
												mvprintw(8, 0, "One button - Page Up key keyboard press simulated");
												xdo_send_keysequence_window(x, CURRENTWINDOW, "Page_Up", 0);
								} else if (code == XWII_KEY_TWO) {
												mvprintw(8, 0, "Two button - Page Down key keyboard press simulated");
												xdo_send_keysequence_window(x, CURRENTWINDOW, "Page_Down", 0);
								}
				}
				showCommands();
				locker = !locker;
				// necessary to prevent overflow for xdo
				xdo_free(x);
}

// handles rumble on/off event
static void rumble_toggle() {
				static bool on = false;
				int ret;
				on = !on;
				ret = xwii_iface_rumble(interface, on);
				if (ret) {
								on = !on;
				}
				clear();
				mvprintw(7, 0, "Toggled Rumble: ");
				mvprintw(8, 0, on ? "Rumble on\n" : "Rumble off\n");
				showCommands();
}

// display the current battery percentage
static void battery_refresh() {
				int ret;
				uint8_t capacity;
				ret = xwii_iface_get_battery(interface, & capacity);
				if (!ret) {
								clear();
								mvprintw(7, 0, "Battery: %3u%%\n", capacity);
								showCommands();
				}
}

// handle keyboard presses from user
static int keyboard() {
				int key;
				key = getch();
				if (key == ERR)
								return 0;

				switch (key) {
								case 'r':
												rumble_toggle();
												break;
								case 'b':
												battery_refresh();
												break;
								case 'q':
												return -ECANCELED;
				}
				return 0;
}

// terminal interface to display output and accept keystrokes
static int run_interface(struct xwii_iface * interface) {
				struct xwii_event event;
				int ret, fds_num;
				struct pollfd fds[2];
				memset(fds, 0, sizeof(fds));
				fds[0].fd = 0;
				fds[0].events = POLLIN;
				fds[1].fd = xwii_iface_get_fd(interface);
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
								ret = xwii_iface_dispatch(interface, & event, sizeof(event));
								if (!ret) {
												switch (event.type) {
																case XWII_EVENT_WATCH:
																				battery_refresh();
																				break;
																case XWII_EVENT_KEY:
																				key_show( & event);
																				break;
												}
								}
								ret = keyboard();
								if (ret == -ECANCELED) {
												return 0;
								} else if (ret) {
												return ret;
								}
								refresh();
				}
				return ret;
}

// scan for wii remotes
static char * get_dev() {
				struct xwii_monitor * mon;
				char * ent;
				int i = 0;
				mon = xwii_monitor_new(false, false);
				if (!mon) {
								printf("WiiClicker - Failed to monitor for Wii Remotes.\n");
								return NULL;
				}
				// stop after finding the first remote
				while ((ent = xwii_monitor_poll(mon))) {
								if (++i == 1) {
												break;
								}
								free(ent);
				}

				xwii_monitor_unref(mon);

				if (!ent) {
								// quit since there is no wii remote found
								printf("WiiClicker - Please connect a Wii Remote to Bluetooth and try again.\n");
								exit(0);
				}
				return ent;
}

// primary procedure
int main(char ** argv) {
				int ret = 0;
				char * path = NULL;
				clear();

				// we always get the first wii remote connected
				path = get_dev();

				ret = xwii_iface_new( & interface, path ? path : argv[1]);
				free(path);
				if (!ret) {
								initscr();
								curs_set(0);
								raw();
								noecho();
								timeout(0);
								battery_refresh();
								showCommands();
								refresh();
								ret = xwii_iface_open(interface,
								                      xwii_iface_available(interface) |
								                      XWII_IFACE_WRITABLE);
								ret = run_interface(interface);
								xwii_iface_unref(interface);
								if (ret) {
												refresh();
												timeout(-1);
												getch();
								}
								endwin();
				}
				return abs(ret);
}