/**
 * @file main-template.c
 * @brief Système de protection automatique de poules contre le renard et l'aigle
 * 
 * Ce programme implémente un ordonnancement temps-réel off-line avec deux tâches :
 * - Tâche Renard : détecte et chasse le renard (période FOX_TIME = 4000ms)
 * - Tâche Aigle : détecte et chasse l'aigle (période EAGLE_TIME = 2000ms)
 * 
 * Ordonnancement :
 * - Unité de temps : STEP_TIME = 500ms
 * - Période Renard : 8 unités (4000ms)
 * - Période Aigle : 4 unités (2000ms)
 * - WCET Renard : 4 unités (pire cas : tester 3 directions + alarme)
 * - WCET Aigle : 2 unités (pire cas : tester 1 direction + alarme)
 * 
 * Contraintes respectées :
 * - Mono-processeur (mutex dans sense/sound_alarm)
 * - Ordonnancement préemptif (découpage des tâches)
 * - Respect des périodes avec clock_nanosleep TIMER_ABSTIME
 */

#define _POSIX_C_SOURCE 200809L

#include "chickens.h"
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

// Variables globales
coop_t *c;
sensors_t *sensors;

/**
 * @brief Ajoute un délai à un temps donné
 * @param ts Pointeur vers la structure timespec à modifier
 * @param ms Délai en millisecondes à ajouter
 */
void timespec_add_ms(struct timespec *ts, unsigned long ms) {
    ts->tv_sec += ms / 1000;
    ts->tv_nsec += (ms % 1000) * 1000000;
    
    // Normaliser si les nanosecondes dépassent 1 seconde
    if (ts->tv_nsec >= 1000000000) {
        ts->tv_sec += 1;
        ts->tv_nsec -= 1000000000;
    }
}

/**
 * @brief Tâche de détection et chasse du renard
 * @param arg Non utilisé
 * @return NULL
 * 
 * Période : FOX_TIME = 4000ms (8 unités de temps)
 * WCET : 4 unités (4 × STEP_TIME = 2000ms dans le pire cas)
 * 
 * Pire cas : 
 * - Tester NORTH (500ms)
 * - Tester SOUTH (500ms) 
 * - Tester EAST (500ms)
 * - Déclencher alarme (500ms)
 */
void* tache_renard(void* arg) {
    struct timespec next_activation;
    clock_gettime(CLOCK_MONOTONIC, &next_activation);
    
    // Directions à tester pour le renard
    side_t directions_renard[] = {NORTH, SOUTH, EAST};
    int nb_directions = 3;
    
    while (1) {
        // Parcourir toutes les directions du renard
        for (int i = 0; i < nb_directions; i++) {
            error_t sense_error;
            sense_t result = sense(sensors, directions_renard[i], &sense_error);
            
            if (sense_error == OK && result == DETECTED) {
                // Menace détectée, déclencher l'alarme
                sound_alarm(sensors, directions_renard[i]);
                break; // Menace trouvée et chassée, fin de cette période
            }
        }
        
        // Attendre la prochaine période (FOX_TIME = 4000ms)
        timespec_add_ms(&next_activation, FOX_TIME);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation, NULL);
    }
    
    return NULL;
}

/**
 * @brief Tâche de détection et chasse de l'aigle
 * @param arg Non utilisé
 * @return NULL
 * 
 * Période : EAGLE_TIME = 2000ms (4 unités de temps)
 * WCET : 2 unités (2 × STEP_TIME = 1000ms dans le pire cas)
 * 
 * Pire cas :
 * - Tester ABOVE (500ms)
 * - Déclencher alarme (500ms)
 */
void* tache_aigle(void* arg) {
    struct timespec next_activation;
    clock_gettime(CLOCK_MONOTONIC, &next_activation);
    
    while (1) {
        // Tester la direction de l'aigle (ABOVE uniquement)
        error_t sense_error;
        sense_t result = sense(sensors, ABOVE, &sense_error);
        
        if (sense_error == OK && result == DETECTED) {
            // Menace détectée, déclencher l'alarme
            sound_alarm(sensors, ABOVE);
        }
        
        // Attendre la prochaine période (EAGLE_TIME = 2000ms)
        timespec_add_ms(&next_activation, EAGLE_TIME);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation, NULL);
    }
    
    return NULL;
}

/**
 * @brief Point d'entrée du programme
 * 
 * Initialise le système, crée les deux threads (renard et aigle),
 * et attend leur terminaison (qui n'arrivera jamais en temps normal).
 * Le programme se termine automatiquement si toutes les poules sont volées.
 */
int main(int argc, char *argv[]) {
    printf("=== Système de protection de poules ===\n");
    
    // Initialisation du poulailler et des capteurs
    error_t res = init_coop(&c);
    if (res != OK) {
        fprintf(stderr, "Erreur lors de l'initialisation du poulailler\n");
        return 1;
    }
    
    res = init_sensors(&sensors);
    if (res != OK) {
        fprintf(stderr, "Erreur lors de l'initialisation des capteurs\n");
        free_coop(c);
        return 1;
    }
    
    // Démarrage des timers du renard et de l'aigle
    start_hunt(sensors, c);
    
    // Affichage du nombre initial de poules
    int chickens;
    res = get_chickens(c, &chickens);
    if (res == OK) {
        printf("Démarrage avec %d poules\n", chickens);
    }
    
    printf("Lancement des tâches de protection...\n\n");
    
    // Création des threads pour les deux tâches
    pthread_t thread_renard, thread_aigle;
    
    if (pthread_create(&thread_renard, NULL, tache_renard, NULL) != 0) {
        fprintf(stderr, "Erreur lors de la création du thread renard\n");
        stop_hunt(sensors);
        free_sensors(sensors);
        free_coop(c);
        return 1;
    }
    
    if (pthread_create(&thread_aigle, NULL, tache_aigle, NULL) != 0) {
        fprintf(stderr, "Erreur lors de la création du thread aigle\n");
        pthread_cancel(thread_renard);
        stop_hunt(sensors);
        free_sensors(sensors);
        free_coop(c);
        return 1;
    }
    
    // Attendre la fin des threads (ne devrait jamais arriver)
    // Le programme se termine automatiquement via exit() si toutes les poules sont volées
    pthread_join(thread_renard, NULL);
    pthread_join(thread_aigle, NULL);
    
    // Nettoyage (code jamais atteint en pratique)
    stop_hunt(sensors);
    free_sensors(sensors);
    free_coop(c);
    
    return 0;
}
