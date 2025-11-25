#include "pico/stdlib.h"
#include <stdio.h>

int main(void) {
    // Initialisiert UART (für printf)
    setup_default_uart();

    // Standard LED ansteuern (Pico 2 nutzt auch GPIO 25 oder WL_GPIO 0 je nach Typ)
    // Wir nutzen hier nur printf als Test.
    printf("Hello GitLab CI! Pico 2 Build success.\n");

    while (true) {
        printf("Looping...\n");
        sleep_ms(1000);
    }

    return 0;
}