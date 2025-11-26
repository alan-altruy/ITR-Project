================================================================================
                    PROJET TEMPS RÉEL - PROTECTION DE POULES
================================================================================

Membres du groupe :
- Alan Altruy
- [Nom du second membre]


================================================================================
SECTION 2 - RÉPONSES AUX QUESTIONS D'ORDONNANCEMENT
================================================================================

Question 1 : Périodes des tâches
---------------------------------
• Tâche Renard : FOX_TIME = 4000ms
• Tâche Aigle : EAGLE_TIME = 2000ms


Question 2 : Nombre de pas au pire des cas
-------------------------------------------
Un pas = STEP_TIME = 500ms

• Tâche Renard : 4 pas
  - sense(NORTH), sense(SOUTH), sense(EAST), sound_alarm() = 2000ms

• Tâche Aigle : 2 pas
  - sense(ABOVE), sound_alarm() = 1000ms


Question 3 : WCET de chaque tâche
----------------------------------
• Tâche Renard : WCET = 2000ms
• Tâche Aigle : WCET = 1000ms


Question 4 : Diagramme d'ordonnancement
----------------------------------------
Unité de temps : STEP_TIME = 500ms

Temps (unités) :  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
                  |---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
Tâche Renard :    R   R   .   .   .   .   .   .   R   R   .   .   .   .   .   .
Tâche Aigle  :    .   .   A   A   .   .   A   A   .   .   A   A   .   .   A   A

Légende : R = Renard, A = Aigle, . = idle

Taux d'utilisation : U = 2000/4000 + 1000/2000 = 1.0 (100%)


================================================================================
SECTION 3 - IMPLÉMENTATION
================================================================================

Tâche de remplacement (Section 3.1)
------------------------------------
• Synchronisation par sémaphore (sem_t sem_replacement)
• La tâche reste bloquée sur sem_wait() (pas de CPU utilisé)
• Déclenchement : les tâches renard et aigle font sem_post() quand aucune 
  menace n'est détectée (temps CPU libre)
• Vérifie get_chickens() et appelle add_chicken() si nécessaire
• Durée d'exécution négligeable (< STEP_TIME)


Décisions importantes
---------------------
• Ordonnancement off-line avec clock_nanosleep(TIMER_ABSTIME) pour éviter 
  les dérives temporelles
• Détection séquentielle pour le renard (NORTH, SOUTH, EAST) avec arrêt 
  dès qu'une menace est détectée
• Gestion propre de SIGINT avec should_stop et nettoyage des ressources
 • Direction AWAY : représente l'absence de menace (renard ou aigle éloigné). Les timers
   placent initialement les menaces sur AWAY et l'alarme force leur retour sur AWAY.
   Dans l'implémentation finale de patrouille, AWAY n'est pas scannée (ignorée) pour
   garantir que chaque période est dédiée uniquement aux côtés susceptibles de vol,
   réduisant le WCET effectif et évitant des fenêtres où le vol arriverait avant l'alarme.
   La troisième tâche (remplacement) est réveillée uniquement quand aucune menace n'a
   été détectée sur les côtés actifs; on ne réveille pas spécifiquement sur AWAY pour ne
   pas sur-signaler du temps libre artificiel.
