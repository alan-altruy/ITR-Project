================================================================================
                    PROJET TEMPS RÉEL - PROTECTION DE POULES
================================================================================

Auteur : [Votre nom à compléter]
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

3. main-template.c (180 lignes) :
   - Point d'entrée du programme
   - Création des deux threads (renard et aigle)
   - Ordonnancement off-line avec clock_nanosleep(TIMER_ABSTIME)


Contraintes respectées
-----------------------

1. CONTRAINTE MONO-PROCESSEUR :
   Les fonctions sense() et sound_alarm() utilisent des mutex internes
   (action_mutex dans chickens.c) pour garantir l'exclusion mutuelle.
   Une seule opération de détection ou d'alarme peut s'exécuter à la fois.

2. CONTRAINTE DE PÉRIODICITÉ :
   Les threads utilisent clock_nanosleep() avec TIMER_ABSTIME pour
   respecter exactement les périodes FOX_TIME et EAGLE_TIME.
   → Pas de dérive temporelle accumulée (absolute time, pas relative time)

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
   lorsque toutes les poules ont été volées. Les threads tournent en
   boucle infinie et ne se terminent jamais naturellement.


Section 3.1 - Tâche optionnelle de remplacement (NON IMPLÉMENTÉE)
-------------------------------------------------------------------

La section 3.1 de l'énoncé propose une extension optionnelle avec une
troisième tâche pour remplacer les poules volées.

Cette fonctionnalité n'a PAS été implémentée dans ce projet.

Principe (si on devait l'implémenter) :
• Un sémaphore serait utilisé pour signaler les poules volées
• Une tâche "handler différé" attendrait sur ce sémaphore
• Quand une poule est volée, le handler ajouterait une nouvelle poule


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
• Se termine automatiquement quand toutes les poules sont volées


Test et vérification
---------------------

Pour tester l'ordonnancement sur 10 secondes :

    timeout 10 ./chickens

Observations attendues :
• Tâche Renard : s'active toutes les 4 secondes (t=0s, 4s, 8s, ...)
• Tâche Aigle : s'active toutes les 2 secondes (t=0s, 2s, 4s, 6s, 8s, ...)
• Les deux tâches ne se bloquent jamais mutuellement
• Le programme respecte strictement les contraintes temporelles


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


Points d'amélioration possibles
---------------------------------

1. Ordonnancement avec priorités explicites :
   Utiliser pthread_setschedparam() pour définir des priorités temps-réel
   (SCHED_FIFO ou SCHED_RR) au lieu de laisser l'ordonnanceur Linux décider.

2. Monitoring du temps d'exécution réel :
   Mesurer avec clock_gettime() si les WCET sont respectés en pratique.

3. Implémentation de la section 3.1 :
   Ajouter la tâche de remplacement des poules avec sémaphores.

4. Gestion de la surcharge :
   Détecter si une période est ratée (missed deadline) et signaler l'erreur.


================================================================================
FIN DU RAPPORT
================================================================================
