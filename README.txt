================================================================================
                    PROJET TEMPS RÉEL - PROTECTION DE POULES
================================================================================

Auteur : Alan Altruy
Cours : S-INFO-111 - Informatique Temps Réel


================================================================================
SECTION 2 - RÉPONSES AUX QUESTIONS D'ORDONNANCEMENT
================================================================================

Question 1 : Quelles sont les périodes des tâches « renard » et « aigle » ?
-----------------------------------------------------------------------------

Les périodes sont données par les constantes définies dans chickens.h :

• Tâche Renard : période = FOX_TIME = 4000ms = 4 secondes
  Le renard attaque depuis le nord, le sud ou l'est toutes les 4 secondes.

• Tâche Aigle : période = EAGLE_TIME = 2000ms = 2 secondes
  L'aigle attaque depuis le dessus (ABOVE) toutes les 2 secondes.
  
Remarque : EAGLE_TIME = FOX_TIME / 2, donc l'aigle est deux fois plus rapide.


Question 2 : Combien de pas au pire des cas doivent faire les deux tâches ?
-----------------------------------------------------------------------------

Un "pas" correspond à une opération sense() ou sound_alarm(), qui prend 
STEP_TIME = 500ms (FOX_TIME / 8).

• Tâche Renard - pire cas = 4 pas :
  1. sense(NORTH)      → 1 pas
  2. sense(SOUTH)      → 1 pas
  3. sense(EAST)       → 1 pas
  4. sound_alarm(XXX)  → 1 pas
  TOTAL : 4 × 500ms = 2000ms
  
  Scénario du pire cas : le renard teste les 3 directions (NORTH, SOUTH, EAST)
  et détecte une menace à la dernière direction testée, puis déclenche l'alarme.

• Tâche Aigle - pire cas = 2 pas :
  1. sense(ABOVE)      → 1 pas
  2. sound_alarm(ABOVE) → 1 pas
  TOTAL : 2 × 500ms = 1000ms
  
  Scénario du pire cas : l'aigle teste la direction ABOVE et détecte une menace,
  puis déclenche l'alarme immédiatement.


Question 3 : Quels sont les temps d'exécution WCET de chaque tâche ?
----------------------------------------------------------------------

Le WCET (Worst-Case Execution Time) est le temps maximum d'exécution :

• Tâche Renard :
  WCET = 4 pas × STEP_TIME = 4 × 500ms = 2000ms
  
• Tâche Aigle :
  WCET = 2 pas × STEP_TIME = 2 × 500ms = 1000ms


Question 4 : Diagramme d'ordonnancement
-----------------------------------------

Unité de temps : STEP_TIME = 500ms
Période Renard (R) : 8 unités = 4000ms
Période Aigle (A) : 4 unités = 2000ms

Légende :
  R = Tâche Renard en exécution (WCET = 4 unités max)
  A = Tâche Aigle en exécution (WCET = 2 unités max)
  . = Processeur inactif (idle)

Temps (unités) :  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
                  |---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
Tâche Renard :    R   R   .   .   .   .   .   .   R   R   .   .   .   .   .   .
Tâche Aigle  :    .   .   A   A   .   .   A   A   .   .   A   A   .   .   A   A

Explication :
• Instant 0 : Le renard commence (prioritaire car période plus longue)
• Instant 2 : Le renard a fini, l'aigle s'exécute (première activation)
• Instant 4 : Seconde activation de l'aigle
• Instant 6 : Troisième activation de l'aigle
• Instant 8 : Seconde activation du renard (période = 8 unités)
• Instant 10 : Quatrième activation de l'aigle (période = 4 unités)

Vérifications :
✓ La période du renard (8 unités) est respectée
✓ La période de l'aigle (4 unités) est respectée
✓ Aucune surcharge : WCET_Renard/Période_Renard + WCET_Aigle/Période_Aigle
                      = 4/8 + 2/4 = 0.5 + 0.5 = 1.0 = 100% (limite théorique)
✓ L'ordonnancement est faisable (U = 100% exactement)


================================================================================
SECTION 3 - DÉTAILS D'IMPLÉMENTATION
================================================================================

Architecture du code
---------------------

Le programme est structuré en trois fichiers :

1. chickens.h (258 lignes) :
   - Interface publique (API)
   - Définitions des types opaques (coop_t, sensors_t)
   - Énumérations (error_t, side_t, sense_t)
   - Déclarations des fonctions

2. chickens.c (520 lignes) :
   - Implémentation complète fournie par le prof
   - Simulation du renard et de l'aigle avec des timers POSIX
   - Gestion des mutex pour la protection des sections critiques
   - Calibration automatique pour STEP_TIME précis

3. main-template.c (320 lignes) :
   - Point d'entrée du programme
   - Création des trois threads (renard, aigle, remplacement)
   - Ordonnancement off-line avec clock_nanosleep(TIMER_ABSTIME)
   - Gestion du signal SIGINT pour arrêt propre
   - Synchronisation par sémaphore pour la tâche de remplacement


Contraintes respectées
-----------------------

1. CONTRAINTE MONO-PROCESSEUR :
   Les fonctions sense() et sound_alarm() utilisent des mutex internes
   (action_mutex dans chickens.c) pour garantir l'exclusion mutuelle.
   Une seule opération de détection ou d'alarme peut s'exécuter à la fois.

2. CONTRAINTE DE PÉRIODICITÉ :
   Les threads utilisent clock_nanosleep() avec TIMER_ABSTIME pour
   respecter exactement les périodes FOX_TIME et EAGLE_TIME.

3. CONTRAINTE D'ORDONNANCEMENT PRÉEMPTIF :
   Les threads peuvent être préemptés par le système d'exploitation.
   La priorité implicite est gérée par l'ordonnanceur Linux.


Décisions de conception importantes
-------------------------------------

1. ORDONNANCEMENT OFF-LINE :
   Les deux tâches sont périodiques avec des périodes fixes connues
   à l'avance (4000ms et 2000ms). L'ordonnancement est donc déterminé
   statiquement avant l'exécution.

2. UTILISATION DE clock_nanosleep(TIMER_ABSTIME) :
   Au lieu de sleep() ou usleep() qui causent des dérives cumulatives,
   nous utilisons clock_nanosleep() avec des temps absolus :
   
   next_activation += période
   clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation, NULL)
   
   Avantages :
   • Pas de dérive temporelle
   • Respect exact des périodes
   • Résistant aux variations du temps d'exécution (dans la limite du WCET)

3. STRATÉGIE DE DÉTECTION DU RENARD :
   Le renard peut attaquer depuis 3 directions (NORTH, SOUTH, EAST).
   La tâche renard parcourt séquentiellement ces 3 directions et s'arrête
   dès qu'une menace est détectée (optimisation du cas moyen).
   
   Pire cas = tester les 3 directions + alarme = 4 pas = 2000ms

4. STRATÉGIE DE DÉTECTION DE L'AIGLE :
   L'aigle attaque uniquement depuis le dessus (ABOVE).
   La tâche aigle teste cette unique direction puis déclenche l'alarme
   si nécessaire.
   
   Pire cas = tester ABOVE + alarme = 2 pas = 1000ms

5. TERMINAISON DU PROGRAMME :
   Le programme se termine automatiquement via exit() (dans chickens.c)
   lorsque toutes les poules ont été volées, ou proprement lors de SIGINT (Ctrl+C).
   Les threads vérifient le flag should_stop et se terminent proprement.


Section 3.1 - Tâche de remplacement des poules
-------------------------------------------------------------

La section 3.1 de l'énoncé demande une troisième tâche pour remplacer les 
poules volées, se comportant comme un gestionnaire d'interruption différé.

Cette fonctionnalité a été IMPLÉMENTÉE dans ce projet.

Implémentation :
• Un sémaphore (sem_t sem_replacement) est utilisé pour synchroniser la tâche
• La tâche reste bloquée sur sem_wait() la plupart du temps (pas de CPU utilisé)
• Les tâches renard et aigle font sem_post() quand aucune menace n'est détectée
• Cela libère la tâche de remplacement qui vérifie le nombre de poules
• Si chickens_count < INIT_CHICKENS, elle ajoute une poule avec add_chicken()

Stratégie de déclenchement :
• Tâche Renard : si aucune menace détectée dans les 3 directions → sem_post()
• Tâche Aigle : si aucune menace détectée en ABOVE → sem_post()
• Cela garantit que la tâche de remplacement s'exécute pendant le temps CPU libre

Fonctions utilisées (thread-safe) :
• get_chickens(c, &chickens_count) : obtenir le nombre de poules
• add_chicken(c) : ajouter une poule au poulailler
• Durée d'exécution : négligeable (< STEP_TIME) comme spécifié dans l'énoncé

Remarque importante :
Si l'implémentation est correcte, cette tâche ne devrait jamais avoir besoin 
de remplacer les poules perdues car les menaces sont chassées à temps.


Section 3.2 - Gestion du signal SIGINT (IMPLÉMENTÉE)
-----------------------------------------------------

La section 3.2 de l'énoncé demande une gestion propre du signal SIGINT (Ctrl+C).

Cette fonctionnalité a été IMPLÉMENTÉE dans ce projet.

Implémentation :
• Installation d'un gestionnaire de signal avec sigaction(SIGINT, &sa, NULL)
• Handler sigint_handler() qui met should_stop = 1
• Flag volatile sig_atomic_t should_stop vérifié dans les boucles des 3 threads
• Le handler libère aussi le sémaphore avec sem_post() pour débloquer la tâche

Arrêt propre :
1. Réception de SIGINT (Ctrl+C)
2. should_stop = 1 → les threads sortent de leurs boucles while
3. pthread_join() attend la terminaison propre de tous les threads
4. Nettoyage complet des ressources :
   - stop_hunt(sensors) : arrête les timers du renard et de l'aigle
   - free_sensors(sensors) : libère la structure des capteurs
   - free_coop(c) : libère le poulailler
   - sem_destroy(&sem_replacement) : détruit le sémaphore
5. Message de confirmation : "Programme terminé proprement"

Avantages :
• Pas de fuite mémoire
• Pas de threads zombies
• Arrêt immédiat et propre de toutes les ressources système


================================================================================
SECTION 4 - COMPILATION ET EXÉCUTION
================================================================================

Compilation
-----------

Commande (depuis src/chickens/) :

    gcc -o chickens main-template.c chickens.c

Note : Aucune option supplémentaire n'est nécessaire. Les bibliothèques
pthread et rt sont liées automatiquement sur les systèmes Linux modernes.


Exécution
---------

    ./chickens

Le programme affiche :
• Le nombre initial de poules (5 par défaut)
• Les détections de menaces et les alarmes en temps réel
• Les remplacements de poules (si activés)
• Se termine automatiquement quand toutes les poules sont volées
• Ou proprement avec Ctrl+C (SIGINT)


Test et vérification
---------------------

Pour tester l'ordonnancement sur 10 secondes :

    timeout 10 ./chickens

Observations attendues :
• Tâche Renard : s'active toutes les 4 secondes (t=0s, 4s, 8s, ...)
• Tâche Aigle : s'active toutes les 2 secondes (t=0s, 2s, 4s, 6s, 8s, ...)
• Tâche Remplacement : s'active quand aucune menace n'est détectée
• Les trois tâches ne se bloquent jamais mutuellement
• Le programme respecte strictement les contraintes temporelles
• Ctrl+C arrête proprement toutes les tâches et libère les ressources


================================================================================
SECTION 5 - ANALYSE ET JUSTIFICATION
================================================================================

Faisabilité de l'ordonnancement
---------------------------------

Taux d'utilisation du processeur (U) :

    U = C₁/T₁ + C₂/T₂
    U = WCET_Renard/Période_Renard + WCET_Aigle/Période_Aigle
    U = 2000ms/4000ms + 1000ms/2000ms
    U = 0.5 + 0.5
    U = 1.0 = 100%

Conclusion : L'ordonnancement est faisable mais à la limite théorique.
Le processeur est utilisé à 100% dans le pire des cas.

Théorème de Liu & Layland (Rate Monotonic Scheduling) :
Pour 2 tâches : U_max = 2 × (2^(1/2) - 1) ≈ 0.828 = 82.8%

⚠️ Notre U = 100% > 82.8% → L'ordonnancement RM n'est PAS optimal ici.
Cependant, l'ordonnancement reste faisable car :
1. Les périodes sont harmoniques (EAGLE_TIME = FOX_TIME/2)
2. Les tâches respectent leurs deadlines implicites (deadline = période)


Robustesse et gestion des erreurs
-----------------------------------

Le code vérifie systématiquement les valeurs de retour :
• init_coop() et init_sensors() → sortie en cas d'erreur
• pthread_create() → nettoyage et sortie en cas d'échec
• sense() → paramètre error_t pour détecter les dysfonctionnements
• sound_alarm() → valeur de retour vérifiable (non utilisée ici)

Les mutex dans chickens.c garantissent l'absence de race conditions.
