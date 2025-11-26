#define _POSIX_C_SOURCE 200809L

#include "chickens.h"
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>
#include <semaphore.h>
#include <stdlib.h>

// Prototype de la fonction utilitaire d'affichage des directions
const char* dir_name(side_t d);

// Variables globales
coop_t *c;
sensors_t *sensors;

// Sémaphore pour la tâche de remplacement (gestionnaire différé)
sem_t sem_replacement;

// Flag pour arrêter proprement les threads lors de SIGINT
volatile sig_atomic_t should_stop = 0;

// Gestionnaire SIGINT : arrête proprement et débloque le sémaphore.
void sigint_handler(int signum) {
    printf("\n[SIGNAL] Réception de SIGINT, arrêt en cours...\n");
    should_stop = 1;
    
    // Libérer le sémaphore pour débloquer la tâche de remplacement
    sem_post(&sem_replacement);
}

// Ajoute `ms` millisecondes au `timespec` fourni.
void timespec_add_ms(struct timespec *ts, unsigned long ms) {
    ts->tv_sec += ms / 1000;
    ts->tv_nsec += (ms % 1000) * 1000000;
    
    // Normaliser si les nanosecondes dépassent 1 seconde
    if (ts->tv_nsec >= 1000000000) {
        ts->tv_sec += 1;
        ts->tv_nsec -= 1000000000;
    }
}

// Tâche de remplacement : débloquée par sémaphore pour restaurer les poules perdues.
void* tache_remplacement(void* arg) {
    while (1) {
        // Attendre d'être libéré par une autre tâche
        sem_wait(&sem_replacement);
        
        // Vérifier si on doit s'arrêter
        if (should_stop) {
            printf("[REMPLACEMENT] Arrêt de la tâche\n");
            break;
        }
        
        // Vérifier combien de poules restent
        int chickens_count;
        error_t res = get_chickens(c, &chickens_count);
        
        if (res == OK && chickens_count < INIT_CHICKENS) {
            // Il manque des poules, en ajouter une
            res = add_chicken(c);
            if (res == OK) {
                printf("[REMPLACEMENT] Poule ajoutée! Total: %d/%d\n", 
                       chickens_count + 1, INIT_CHICKENS);
            } else {
                fprintf(stderr, "[REMPLACEMENT] Erreur lors de l'ajout d'une poule\n");
            }
        }
    }
    
    return NULL;
}

// Tâche renard : patrouille périodique sur les côtés NORTH, SOUTH, EAST.
void* tache_renard(void* arg) {
    struct timespec next_activation;
    clock_gettime(CLOCK_MONOTONIC, &next_activation);
    
    // Côtés actifs à surveiller (WEST est mur, AWAY = aucun vol)
    side_t directions_renard[] = {NORTH, SOUTH, EAST};
    int nb_directions = 3;
    
    while (!should_stop) {
        printf("[RENARD] Patrouille période FOX_TIME: scan des côtés actifs\n");
        bool menace_trouvee = false;
        for (int i = 0; i < nb_directions && !should_stop; ++i) {
            side_t side = directions_renard[i];
            error_t sense_error = OK;
            sense_t result = sense(sensors, side, &sense_error);
            if (sense_error == OK && result == DETECTED) {
                printf("[RENARD] Menace DETECTED sur %s -> Alarme avant fin période\n", dir_name(side));
                sound_alarm(sensors, side);
                menace_trouvee = true;
                break; // Menace neutralisée pour cette période
            } else if (sense_error != OK) {
                printf("[RENARD] Erreur sense (%d) sur %s\n", sense_error, dir_name(side));
            }
        }
        if (!menace_trouvee) {
            printf("[RENARD] Aucune menace détectée sur N/S/E cette période -> temps libre\n");
            sem_post(&sem_replacement); // Une seule libération
        }
        
        // Attendre la prochaine période (FOX_TIME = 4000ms)
        timespec_add_ms(&next_activation, FOX_TIME);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation, NULL);
    }
    
    printf("[RENARD] Arrêt de la tâche\n");
    return NULL;
}

// Tâche aigle : unique côté ABOVE à surveiller chaque période EAGLE_TIME.
void* tache_aigle(void* arg) {
    struct timespec next_activation;
    clock_gettime(CLOCK_MONOTONIC, &next_activation);
    while (!should_stop) {
        printf("[AIGLE] Patrouille ABOVE période EAGLE_TIME\n");
        error_t sense_error = OK;
        sense_t result = sense(sensors, ABOVE, &sense_error);
        if (sense_error == OK && result == DETECTED) {
            printf("[AIGLE] Menace DETECTED ABOVE -> Alarme avant fin période\n");
            sound_alarm(sensors, ABOVE);
        } else if (sense_error == OK && result == NORMAL) {
            printf("[AIGLE] Rien ABOVE cette période -> temps libre\n");
            sem_post(&sem_replacement);
        } else {
            printf("[AIGLE] Erreur sense (%d) ABOVE\n", sense_error);
        }
        timespec_add_ms(&next_activation, EAGLE_TIME);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation, NULL);
    }
    printf("[AIGLE] Arrêt de la tâche\n");
    return NULL;
}


int main(int argc, char *argv[]) {
    printf("=== Système de protection de poules ===\n");

    // Initialiser RNG pour choix du renard
    srand((unsigned)time(NULL));
    
    // Initialiser le sémaphore pour la tâche de remplacement
    if (sem_init(&sem_replacement, 0, 0) != 0) {
        fprintf(stderr, "Erreur lors de l'initialisation du sémaphore\n");
        return 1;
    }
    
    // Installer le gestionnaire de signal SIGINT
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) != 0) {
        fprintf(stderr, "Erreur lors de l'installation du gestionnaire SIGINT\n");
        sem_destroy(&sem_replacement);
        return 1;
    }
    
    // Initialisation du poulailler et des capteurs
    error_t res = init_coop(&c);
    if (res != OK) {
        fprintf(stderr, "Erreur lors de l'initialisation du poulailler\n");
        sem_destroy(&sem_replacement);
        return 1;
    }
    
    res = init_sensors(&sensors);
    if (res != OK) {
        fprintf(stderr, "Erreur lors de l'initialisation des capteurs\n");
        free_coop(c);
        sem_destroy(&sem_replacement);
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
    
    printf("Lancement des tâches de protection...\n");
    printf("Appuyez sur Ctrl+C pour arrêter proprement le programme.\n\n");
    
    // Création des threads pour les trois tâches
    pthread_t thread_renard, thread_aigle, thread_remplacement;
    
    if (pthread_create(&thread_remplacement, NULL, tache_remplacement, NULL) != 0) {
        fprintf(stderr, "Erreur lors de la création du thread de remplacement\n");
        stop_hunt(sensors);
        free_sensors(sensors);
        free_coop(c);
        sem_destroy(&sem_replacement);
        return 1;
    }
    
    if (pthread_create(&thread_renard, NULL, tache_renard, NULL) != 0) {
        fprintf(stderr, "Erreur lors de la création du thread renard\n");
        should_stop = 1;
        sem_post(&sem_replacement);
        pthread_join(thread_remplacement, NULL);
        stop_hunt(sensors);
        free_sensors(sensors);
        free_coop(c);
        sem_destroy(&sem_replacement);
        return 1;
    }
    
    if (pthread_create(&thread_aigle, NULL, tache_aigle, NULL) != 0) {
        fprintf(stderr, "Erreur lors de la création du thread aigle\n");
        should_stop = 1;
        pthread_cancel(thread_renard);
        sem_post(&sem_replacement);
        pthread_join(thread_renard, NULL);
        pthread_join(thread_remplacement, NULL);
        stop_hunt(sensors);
        free_sensors(sensors);
        free_coop(c);
        sem_destroy(&sem_replacement);
        return 1;
    }
    
    // Attendre la fin des threads (lors de SIGINT ou fin du jeu)
    pthread_join(thread_renard, NULL);
    pthread_join(thread_aigle, NULL);
    pthread_join(thread_remplacement, NULL);
    
    // Nettoyage des ressources
    printf("\n[MAIN] Nettoyage des ressources...\n");
    stop_hunt(sensors);
    free_sensors(sensors);
    free_coop(c);
    sem_destroy(&sem_replacement);
    
    printf("[MAIN] Programme terminé proprement.\n");
    return 0;
}

// Fonction utilitaire déplacée en bas pour compatibilité C
const char* dir_name(side_t d) {
    switch (d) {
        case AWAY: return "AWAY";
        case NORTH: return "NORTH";
        case SOUTH: return "SOUTH";
        case EAST: return "EAST";
        case ABOVE: return "ABOVE";
        default: return "UNKNOWN";
    }
}

