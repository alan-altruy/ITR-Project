#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

#include "chickens.h"

#define NUM_ACTIVE_POS (MAX_ACTIVE_POS - MIN_ACTIVE_POS + 1)
#define MIN_ACTIVE_POS 1
#define MAX_ACTIVE_POS 4


typedef struct coop coop_t;
typedef enum side side_t;
typedef struct threat threat_t;



struct coop {
    pthread_mutex_t coop_mutex;
    int chickens;
};

error_t init_coop(coop_t **c) {
    if (!c) {
        return NULL_PTR;
    }
    *c = NULL;
    coop_t *coop_ptr = malloc(sizeof(coop_t));
    if (!coop_ptr) {
        return MALLOC;
    }
    if (pthread_mutex_init(&coop_ptr->coop_mutex, NULL)) {
        free(coop_ptr);
        return MUTEX;
    }
    coop_ptr->chickens = INIT_CHICKENS;
    *c = coop_ptr;
    return OK;
}

error_t free_coop(coop_t *c) {
    if (c == NULL) {
        return NULL_PTR;
    }
    error_t res = OK;
    if (pthread_mutex_destroy(&c->coop_mutex)) {
        res = MUTEX;
    }
    free(c);
    return res;
}

error_t steal(coop_t *c) {
    if (!c) {
        return NULL_PTR;
    }
    c->chickens--;
    printf("A chicken has been stolen! (%d left)\n", c->chickens);
    if (!c->chickens) {
        printf("No chickens left...\n");
        exit(1);
    }
    return OK;
}

error_t get_chickens(coop_t *c, int* chickens) {
    *chickens = 0;
    if (!c) {
        return NULL_PTR;
    }
    if (pthread_mutex_lock(&c->coop_mutex)) {
        return MUTEX;
    }
    *chickens = c->chickens;
    if (pthread_mutex_unlock(&c->coop_mutex)) {
        return MUTEX;
    }
    return OK;
}

error_t add_chicken(coop_t *c) {
    if (!c) {
        return NULL_PTR;
    }
    if (pthread_mutex_lock(&c->coop_mutex)) {
        return MUTEX;
    }
    if (c->chickens < INIT_CHICKENS) {
        printf("Adding a new chicken\n");
        c->chickens++;
    }
    if (pthread_mutex_unlock(&c->coop_mutex)) {
        return MUTEX;
    }
    return OK;
}


struct threat {
    timer_t timer;
    unsigned long long time;
    pthread_mutex_t side_mutex;
    side_t minside;
    side_t maxside;
    side_t side;
    coop_t* coop;
    char* name;
};

error_t reset_timer(timer_t timer_id, unsigned long long time) {
    struct itimerspec ts;
    ts.it_value.tv_sec = time / 1000;
    ts.it_value.tv_nsec = (time % 1000) * 1000000;
    ts.it_interval.tv_sec = ts.it_value.tv_sec;
    ts.it_interval.tv_nsec = ts.it_value.tv_nsec;
    if (timer_settime(timer_id, 0, &ts, NULL)) {
        return TIMER_SETTIME;
    }
    return OK;
}

error_t stop_timer(timer_t timer_id) {
    struct itimerspec ts = {0};
    if (timer_settime(timer_id, 0, &ts, NULL)) {
        return TIMER_SETTIME;
    }
    return OK;
}

error_t get_minside(threat_t *threat, side_t *side) {
    if (!threat || !side) {
        return NULL_PTR;
    }
    *side = threat->minside;
    return OK;
}

error_t get_maxside(threat_t *threat, side_t *side) {
    if (!threat || !side) {
        return NULL_PTR;
    }
    *side = threat->maxside;
    return OK;
}

error_t set_side(threat_t *threat, side_t side) {
    if (pthread_mutex_lock(&threat->side_mutex)) {
        return MUTEX;
    }
    threat->side = side;
    if (pthread_mutex_unlock(&threat->side_mutex)) {
        return MUTEX;
    }
    return OK;
}

error_t get_side(threat_t *threat, side_t *side) {
    if (!side) {
        return NULL_PTR;
    }
    *side = AWAY;
    if (pthread_mutex_lock(&threat->side_mutex)) {
        return MUTEX;
    }
    *side = threat->side;
    if (pthread_mutex_unlock(&threat->side_mutex)) {
        return MUTEX;
    }
    return OK;
}

error_t chose_side(threat_t *threat) {
    if (!threat) {
        return NULL_PTR;
    }
    side_t side = rand() % (threat->maxside - threat->minside + 2);
    if (side != 0) {
        side += threat->minside - 1;
    }
    error_t res = OK;
    if ((res = set_side(threat, side)) != OK) {
        return res;
    }
    return reset_timer(threat->timer, threat->time);
}

error_t steal_chicken(coop_t* coop) {
    return steal(coop);
}

void handle_timer(union sigval sig) {
    threat_t *threat = sig.sival_ptr;
    if (threat) {
        side_t side;
        get_side(threat, &side);
        if (side >= threat->minside && side <= threat->maxside) {
            if (threat->coop) {
                steal_chicken(threat->coop);
            }
        }
        chose_side(threat);
    }
}

error_t create_timer(threat_t *threat, timer_t *timer_id) {
    if (!timer_id) {
        return NULL_PTR;
    }
    struct sigevent se;
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_notify_function = handle_timer;
    se.sigev_value.sival_ptr = threat;
    se.sigev_notify_attributes = NULL;
    if (timer_create(CLOCK_REALTIME, &se, timer_id)) {
        return TIMER_CREATE;
    }
    return OK;
}

error_t threat_hunt(threat_t *threat, coop_t *coop) {
    if (!threat || !coop) {
        return NULL_PTR;
    }
    threat->coop = coop;
    error_t res = OK;
    if ((res = set_side(threat, AWAY)) != OK) {
        return res;
    }
    if ((res = reset_timer(threat->timer, threat->time)) != OK) {
        return res;
    }
    return OK;
}

error_t threat_stop_hunt(threat_t *threat) {
    if (!threat) {
        return NULL_PTR;
    }
    error_t res = OK;
    if ((res = stop_timer(threat->timer)) != OK) {
        return res;
    }
    if ((res = set_side(threat, AWAY)) != OK) {
        return res;
    }
    threat->coop = NULL;
    return res;
}

error_t init_threat(threat_t **threat) {
    if (!threat) {
        return NULL_PTR;
    }
    *threat = NULL;
    threat_t *threat_ptr = calloc(1, sizeof(struct threat));
    if (!threat_ptr) {
        return MALLOC;
    }
    threat_ptr->side = AWAY;
    threat_ptr->coop = NULL;
    error_t res = OK;
    if ((res = create_timer(threat_ptr, &threat_ptr->timer)) != OK) {
        return res;
    }
    if (pthread_mutex_init(&threat_ptr->side_mutex, NULL)) {
        if (timer_delete(threat_ptr->timer)) {
            return TIMER_DELETE;
        }
        return MUTEX;
    }
    *threat = threat_ptr;
    return res;

}

error_t free_threat(threat_t *threat) {
    if (!threat) {
        return NULL_PTR;
    }
    error_t res = OK;
    error_t temp_res = OK;
    res = threat_stop_hunt(threat);
    if ((temp_res = timer_delete(threat->timer)) != OK) {
        res = temp_res;
    }
    if ((temp_res = pthread_mutex_destroy(&threat->side_mutex)) != OK) {
        res = temp_res;
    }
    free(threat);
    return res;
}

error_t init_fox(threat_t **fox) {
    error_t res = OK;
    if ((res = init_threat(fox)) != OK) {
        return res;
    }
    (*fox)->minside = NORTH;
    (*fox)->maxside = EAST;
    (*fox)->time = FOX_TIME;
    (*fox)->name = "FOX";
    return OK;
}

error_t init_eagle(threat_t **eagle) {
    error_t res = OK;
    if ((res = init_threat(eagle)) != OK) {
        return res;
    }
    (*eagle)->minside = ABOVE;
    (*eagle)->maxside = ABOVE;
    (*eagle)->time = EAGLE_TIME;
    (*eagle)->name = "EAGLE";
    return OK;
}


unsigned long long int _measure_loop(unsigned long long int iter, int *r) {
    int local;
    if (r == NULL) {
        r = &local; 
    }
    *r = 0;
    unsigned long long int nanotime = 0;
    struct timespec begin = { 0 }, end = { 0 };
    clock_gettime(CLOCK_REALTIME, &begin);
    for (unsigned long long int i = 0; i < iter; ++i) {
        *r ^= i;
    }
    clock_gettime(CLOCK_REALTIME, &end);
    nanotime = (end.tv_sec - begin.tv_sec) * 1000000000 + end.tv_nsec - begin.tv_nsec;
    return nanotime;
}

unsigned long long int _compute_iterations(unsigned long long int delay) {
    unsigned long long int lower = 1;
    unsigned long long int upper = 1;
    unsigned long long int nanotime = 0;
    while (nanotime < 2*delay) {
        upper *= 2;
        nanotime = _measure_loop(upper, NULL);
    }
    unsigned long long int middle = (lower + upper) / 2;
    nanotime = _measure_loop(middle, NULL);
    while ((lower < upper) && (nanotime > delay || delay > nanotime)) {
        if (nanotime > delay) {
            upper = middle-1;
         } else if (delay > nanotime) {
            lower = middle+1;
        }
        middle = (lower+upper) / 2;
        nanotime = _measure_loop(middle, NULL);
    }
    return middle;
}

struct sensors {
    unsigned long long int iter_for_ms;
    pthread_mutex_t action_mutex;
    unsigned int positions[NUM_ACTIVE_POS];
    threat_t *threats[2];
};

error_t init_sensors(sensors_t **sensors_v) {
    if (!sensors_v) {
        return NULL_PTR;
    }
    error_t res = OK;
    struct sensors* sensors = calloc(1, sizeof(struct sensors));
    if (!sensors) {
        return MALLOC;
    }
    if ((res = init_fox(&sensors->threats[0])) != OK) {
        goto fox_error;
    }
    if ((res = init_eagle(&sensors->threats[1])) != OK) {
        goto eagle_error;
    }
    for (int i = 0; i < 2; ++i) {
        side_t minside;
        side_t maxside;
        (void) get_maxside(sensors->threats[i], &maxside);
        (void) get_minside(sensors->threats[i], &minside);
        for (side_t j = minside; j <= maxside; ++j) {
            sensors->positions[j-1] = i;
        }
    }
    sensors->iter_for_ms = _compute_iterations(1000000ULL);
    pthread_mutex_init(&sensors->action_mutex, NULL);
    if (res != OK) {
eagle_error:
        free_threat(sensors->threats[0]);
fox_error:
        free(sensors);
        sensors = NULL;
    }
    *sensors_v = sensors;
    return res;
}

error_t free_sensors(sensors_t *sensors) {
    if (!sensors) {
        return NULL_PTR;
    }
    error_t res = OK;
    error_t tmp_res = OK;
    if (pthread_mutex_destroy(&sensors->action_mutex)) {
        res = MUTEX;
    }
    for (int i = 0; i < 2; ++i) {
        if ((tmp_res = free_threat(sensors->threats[i])) != OK) {
            res = tmp_res;
        }
    }
    free(sensors);
    return res;
}

error_t start_hunt(sensors_t* sensors, coop_t* coop) {
    for (int i = 0; i < 2; ++i) {
        threat_hunt(sensors->threats[i], coop);
    }
    return OK;
}

error_t stop_hunt(sensors_t* sensors) {
    for (int i = 0; i < 2; ++i) {
        threat_stop_hunt(sensors->threats[i]);
    }
    return OK;
}

sense_t sense(sensors_t* sensors, side_t side, error_t *error_ptr) {
    sense_t res = NORMAL;
    error_t err = OK;
    if (!sensors) {
        res = ERROR;
        err = NULL_PTR;
        goto mutex_error;
    }
    if (pthread_mutex_lock(&sensors->action_mutex)) {
        res = ERROR;
        err = MUTEX;
        goto mutex_error;
    }
    for (unsigned long long int i = 0; i < sensors->iter_for_ms*(STEP_TIME-JITTER); ++i) {}
    if (side < MIN_ACTIVE_POS || side > MAX_ACTIVE_POS) {
        res = ERROR;
        err = INVALID_POSITION;
        goto mutex_unlock;
    }
    int threat_pos = sensors->positions[side-1];
    threat_t *threat = sensors->threats[threat_pos];
    if (!threat) {
        res = ERROR;
        err = NULL_PTR;
        goto mutex_unlock;
    }
    side_t threat_side;
    if ((err = get_side(threat, &threat_side)) != OK) {
        res = ERROR;
        goto mutex_unlock;
    }
    if (threat_side == side) {
        res = DETECTED;
    }
mutex_unlock:
    pthread_mutex_unlock(&sensors->action_mutex);
mutex_error:
    if (error_ptr) {
        *error_ptr = err;
    }
    return res;
}


error_t sound_alarm(sensors_t *sensors, side_t side) {
    if (!sensors) {
        return NULL_PTR;
    }
    error_t res = OK;
    if (pthread_mutex_lock(&sensors->action_mutex)) {
        res = MUTEX;
        goto mutex_lock_error;
    }
    for (unsigned long long int i = 0; i < sensors->iter_for_ms*(STEP_TIME-JITTER); ++i) {}
    if (side < MIN_ACTIVE_POS || side > MAX_ACTIVE_POS) {
        res = INVALID_POSITION;
        goto unlock_mutex;
    }
    int threat_pos = sensors->positions[side-1];
    threat_t *threat = sensors->threats[threat_pos];
    if (!threat) {
        res = NULL_PTR;
        goto unlock_mutex;
    }
    side_t threat_side;
    error_t tmp_res = OK;
    if ((tmp_res = get_side(threat, &threat_side)) != OK) {
        res = tmp_res;
        goto unlock_mutex;
    }
    if (sensors && threat_side == side) {
        if ((tmp_res = set_side(threat, AWAY)) != OK) {
            res = tmp_res;
            goto unlock_mutex;
        }
    } else {
        res = chose_side(threat);
    }
unlock_mutex:
    pthread_mutex_unlock(&sensors->action_mutex);
mutex_lock_error:
    return res;
}


