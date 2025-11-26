
/**
 * @file chickens.h
 * @brief API for a real-time chicken coop protection system simulation.
 *
 * This header defines constants, data types, and functions used to simulate
 * a chicken coop threatened by a fox and an eagle, and protected by a sensor/alarm system.
 */


#pragma once


// --- Constants and Macros ---

/**
 * @def INIT_CHICKENS
 * @brief Number of initial chickens in the coop.
 */
#define INIT_CHICKENS 5

/**
 * @def FOX_TIME
 * @brief Period of the fox (in ms). It hunts every FOX_TIME.
 */
#define FOX_TIME 4000ULL

/**
 * @def EAGLE_TIME
 * @brief Period of the eagle (in ms). It hunts every EAGLE_TIME.
 */
#define EAGLE_TIME (FOX_TIME/2)

/**
 * @def STEP_TIME
 * @brief Time of a "step" (in ms). It takes STEP_TIME to use sense or sound_alarm
 * on a given side.
 */
#define STEP_TIME (FOX_TIME/8)

/**
 * @def JITTER
 * @brief Small variation in delays (in ms).
 * The sense and sound_alarm functions take a time STEP_TIME - JITTER to account for external delays from the OS.
 */
#define JITTER 100ULL


// --- Enumerations and Typedefs ---

/**
 * @enum error
 * @brief Possible results and errors returned by functions.
 */
enum error {
    /// No error, normal result
    OK = 0,
    /// Code errors, probably an error in the code.
    /// The side given as input was invalid (e.g., out of range)
    INVALID_POSITION = 1,
    /// An argument was a NULL pointer
    NULL_PTR = 2,
    /// System errors, could come from the OS.
    /// Malloc error, lack of memory
    MALLOC = 3,
    /// Error when setting the time of a timer
    TIMER_SETTIME = 4,
    /// Error when creating a timer
    TIMER_CREATE = 5,
    /// Error when deleting a timer
    TIMER_DELETE = 6,
    /// Mutex error. Concurrency problem.  (the mutex could not be created, destroyed, locked, unlocked, etc.)
    MUTEX = 7,
};

/**
 * @typedef error_t
 * @brief Value indicating an error.
 * @see enum error
 */
typedef enum error error_t;



/**
 * @enum side
 * @brief The different sides of the coop.
 * Note that there is no WEST since there is a wall on the west side.
 */
enum side {
    /// Away from the coop, not stealing chicken
    AWAY = 0,
    /// North of the coop
    NORTH = 1,
    /// South of the coop
    SOUTH = 2,
    /// East of the coop
    EAST = 3,
    /// Above the coop, the eagle is flying
    ABOVE = 4
};

/**
 * @typedef side_t
 * @brief Typedef for the different sides available.
 * @see enum side
 */
typedef enum side side_t;

/**
 * @enum sense_result
 * @brief Possible results of the sensor
 */
enum sense_result {
    /// Threat detected
    DETECTED = 0,
    /// Nothing detected, all normal
    NORMAL = 1,
    /// An error occurred (malloc, mutex, ...)
    ERROR = 2,
};

/**
 * @typedef sense_t
 * @brief Result of the sensor.
 * @see enum sense_result
 */
typedef enum sense_result sense_t;


// --- Opaque Structures ---

/**
 * @typedef coop_t
 * @brief Place where the chickens roam free (Opaque structure).
 */
typedef struct coop coop_t;

/**
 * @typedef sensors_t
 * @brief The sensor system itself (Opaque structure).
 */
typedef struct sensors sensors_t;


// --- Sensor Functions ---

/**
 * @brief Initialize the sensor using dynamic memory allocation.
 *
 * Sensor must be freed after using free_sensors.
 *
 * @param sensors Takes a pointer to a pointer of type sensors_t, which will be set.
 * @return error_t Returns an error_t (OK or error code).
 */
error_t init_sensors(sensors_t **sensors);

/**
 * @brief Frees the dynamically allocated resources.
 *
 * @param sensors Takes a pointer to a sensor.
 * @return error_t Returns an error_t (OK or error code).
 */
error_t free_sensors(sensors_t *sensors);

/**
 * @brief Starts the two threats (eagle and fox).
 *
 * **Important**: If the number of chickens reaches zero, the code calls the
 * exit function, causing the program to stop.
 *
 * @param sensors Takes a pointer to a sensors_t.
 * @param coop Takes a pointer to a coop_t.
 * @return error_t Returns an error_t (OK or error code).
 */
error_t start_hunt(sensors_t* sensors, coop_t* coop);

/**
 * @brief Stops the two threats (eagle and fox).
 *
 * @param sensors Takes a pointer to a sensors_t.
 * @return error_t Returns an error_t (OK or error code).
 */
error_t stop_hunt(sensors_t* sensors);

/**
 * @brief Senses on the given side if a threat is present.
 *
 * The pointer to error_t can be NULL. If it is *not NULL*, it will be set
 * in case of error to give more information.
 * Uses a mutex to be thread-safe.
 * Takes STEP_TIME ms.
 *
 * @param sensors Takes a pointer to a sensors_t.
 * @param side The side to sense on.
 * @param error Pointer to an error_t output parameter (can be NULL).
 * @return sense_t Returns a sense_t (DETECTED, NORMAL, or ERROR).
 */
sense_t sense(sensors_t *sensors, side_t side, error_t *error);

/**
 * @brief Sounds the alarm on the given side.
 *
 * If a threat was on that side, it is chased away and won't steal a
 * chicken until its next period.
 * Takes a pointer to a sensors_t and a side_t.
 * Uses a mutex to be thread-safe.
 * Takes STEP_TIME ms.
 *
 * @param sensors Takes a pointer to a sensors_t.
 * @param side The side to sound the alarm on.
 * @return error_t Returns an error_t (OK or error code).
 */
error_t sound_alarm(sensors_t *sensors, side_t side);


// --- Coop Functions ---

/**
 * @brief Initializes the coop.
 *
 * @param c Pointer to a pointer of type coop_t, which will be set.
 * @return error_t Returns an error_t (OK or error code).
 */
error_t init_coop(coop_t **c);

/**
 * @brief Frees the resources of the coop.
 *
 * @param c Pointer to the coop instance.
 * @return error_t Returns an error_t (OK or error code).
 */
error_t free_coop(coop_t *c);

/**
 * @brief Writes the number of chickens remaining in the pointer.
 *
 * If the pointer is NULL, does nothing and returns NULL_POINTER.
 * Uses a mutex to be thread-safe.
 *
 * @param c Pointer to the coop instance.
 * @param chickens Pointer to an integer output parameter where the count will be stored.
 * @return error_t Returns an error_t (OK or error code).
 */
error_t get_chickens(coop_t *c, int* chickens);

/**
 * @brief Adds one chicken to the coop.
 *
 * Adds one chicken to the coop if the number of chickens is lower than
 * INIT_CHICKEN. Uses a mutex to be thread-safe.
 *
 * @param c Pointer to the coop instance.
 * @return error_t Returns an error_t (OK or error code).
 */
error_t add_chicken(coop_t *c);

