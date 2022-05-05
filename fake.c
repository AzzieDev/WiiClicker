//compile using:
//gcc fake.c -lxdo -o fake
#include <stdio.h>
#include <stdlib.h>
#include <xdo.h>
#include <unistd.h>
int main() {
    xdo_t * x = xdo_new(NULL);

    while(1) {
        printf("simulating Right keypress\n");
        xdo_send_keysequence_window(x, CURRENTWINDOW, "Right", 0);
        sleep(10);
    }

        return 0; 
}