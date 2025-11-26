#include "chickens.h"
#include <stdio.h>
#include <unistd.h>

coop_t *c;
sensors_t *sensors;

// Important: this code ignores most errors! Do not do this.

int main(int argc, char *argv[]) {

    // Initialize
    init_coop(&c);
    init_sensors(&sensors);

    // Starts the fox and eagle timers.
    start_hunt(sensors, c);

    // Gets the number of chickens
    int chickens;
    error_t res = get_chickens(c, &chickens);

    // Small check for errors
    if (res != OK) {
        printf("Error\n");
    } else {
        printf("%d chickens\n", chickens);
    }

    while (1) {
        // Wait until some signal. We are not doing anything.
        pause();
    }

    // Stops the eagle and fox
    stop_hunt(sensors);

    // Free resources
    free_sensors(sensors);
    free_coop(c);
    return 0;
}
