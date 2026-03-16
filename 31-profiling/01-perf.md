🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 31.1 — perf : Profiling CPU et sampling

## Introduction

`perf` (aussi appelé `perf_events` ou `Linux perf`) est l'outil de profiling natif du noyau Linux. Développé dans le cadre du noyau lui-même depuis la version 2.6.31 (2009), il s'appuie directement sur les compteurs de performance matériels (PMU — *Performance Monitoring Unit*) intégrés à chaque processeur moderne. Cette intégration au noyau lui confère un avantage fondamental sur les outils en espace utilisateur : `perf` peut profiler n'importe quel processus, y compris le noyau, les pilotes, les librairies système et les programmes utilisateur, avec un surcoût minimal.

Pour un développeur C++ sur Linux, `perf` est l'outil de première intention pour le profiling de performance. Il ne nécessite aucune recompilation (même si les symboles de débogage améliorent considérablement la lisibilité), il n'altère pas significativement le comportement du programme profilé, et il produit des données exploitables aussi bien en ligne de commande qu'à travers des outils de visualisation graphiques comme Hotspot ou les flamegraphs.

---

## Architecture et principe du sampling

`perf` repose sur le sous-système `perf_events` du noyau Linux. Ce sous-système expose les compteurs de performance matériels du processeur — des registres spéciaux capables de compter des événements micro-architecturaux comme les cycles CPU, les instructions exécutées, les cache misses ou les branch mispredictions — ainsi que des événements logiciels (changements de contexte, page faults, migrations de CPU).

Le principe du profiling par sampling avec `perf` fonctionne ainsi :

1. **Configuration.** L'utilisateur demande au noyau de surveiller un événement précis (par défaut, les cycles CPU) sur un processus ou sur l'ensemble du système.

2. **Comptage matériel.** Le processeur incrémente un compteur matériel à chaque occurrence de l'événement. Ce comptage s'effectue dans le silicium même, sans aucun surcoût logiciel.

3. **Interruption par overflow.** Lorsque le compteur atteint un seuil configuré (déterminant la fréquence d'échantillonnage), le processeur déclenche une interruption non masquable (NMI). Le noyau intercepte cette interruption et enregistre un *sample* : l'adresse de l'instruction en cours, le PID du processus, le TID du thread, et optionnellement la pile d'appels complète.

4. **Agrégation.** Les milliers de samples collectés sont agrégés pour produire un profil statistique. Les fonctions qui consomment le plus de cycles CPU apparaissent le plus fréquemment dans les échantillons.

```
     Programme en exécution
     ┌──────────────────────────────────────┐
     │  func_A()  func_B()  func_C()  ...   │
     └────┬──────────┬─────────┬────────────┘
          │          │         │
          ▼          ▼         ▼
     ┌──────────────────────────────────────┐
     │   Compteur matériel (PMU)            │
     │   cycles: 0 → 1M → overflow!         │
     └────────────────┬─────────────────────┘
                      │ NMI (interruption)
                      ▼
     ┌──────────────────────────────────────┐
     │   Noyau Linux (perf_events)          │
     │   → Enregistre: IP, PID, callchain   │
     └────────────────┬─────────────────────┘
                      │
                      ▼
     ┌──────────────────────────────────────┐
     │   Fichier perf.data                  │
     │   (samples accumulés)                │
     └──────────────────────────────────────┘
```

L'élégance de cette approche réside dans le fait que le comptage est réalisé par le matériel. Le logiciel n'intervient qu'aux moments d'overflow pour enregistrer un sample — typiquement quelques milliers de fois par seconde. Le surcoût sur le programme profilé est de l'ordre de 1 à 5%, rendant `perf` utilisable même sur des programmes en conditions quasi-production.

---

## Installation sur Ubuntu

`perf` est distribué avec le paquet des outils du noyau correspondant à la version installée :

```bash
sudo apt update  
sudo apt install linux-tools-common linux-tools-$(uname -r)  
```

Le paquet `linux-tools-common` fournit le binaire `perf`, tandis que `linux-tools-$(uname -r)` fournit le module spécifique à la version exacte de votre noyau. Si vous mettez à jour le noyau, il faut également mettre à jour ce paquet.

Vérification de l'installation :

```bash
perf version
```

### Configuration des permissions

Par défaut, Ubuntu restreint l'accès aux compteurs de performance pour des raisons de sécurité. Le paramètre `perf_event_paranoid` contrôle le niveau de restriction :

```bash
cat /proc/sys/kernel/perf_event_paranoid
```

Les valeurs possibles sont :

| Valeur | Accès autorisé |
|---|---|
| `-1` | Aucune restriction (root-like pour tout le monde) |
| `0` | Tous les événements, mais uniquement pour ses propres processus |
| `1` | Événements CPU et logiciels pour ses propres processus (défaut Ubuntu) |
| `2` | Uniquement les événements logiciels pour ses propres processus |
| `3` | Aucun accès sans privilèges root |

Pour le développement, la valeur `0` ou `-1` est la plus pratique. Elle permet de profiler ses propres programmes sans `sudo` et d'accéder à tous les compteurs matériels :

```bash
# Temporaire (jusqu'au prochain redémarrage)
sudo sysctl kernel.perf_event_paranoid=0

# Permanent
echo 'kernel.perf_event_paranoid=0' | sudo tee -a /etc/sysctl.d/99-perf.conf  
sudo sysctl --system  
```

> ⚠️ **En production ou sur des machines partagées**, conservez la valeur par défaut (`1` ou `2`) et utilisez `sudo` uniquement lorsque nécessaire. Réduire `perf_event_paranoid` sur une machine multi-utilisateurs permet à n'importe quel utilisateur de profiler les processus d'autres utilisateurs, ce qui constitue un risque de fuite d'information.

### Activation des symboles du noyau

Pour profiler des interactions avec le noyau (appels système, drivers, scheduler), les symboles du noyau sont nécessaires :

```bash
sudo apt install linux-image-$(uname -r)-dbgsym
```

Ce paquet volumineux (plusieurs centaines de Mo) n'est utile que si vous profilez des chemins traversant le noyau. Pour du profiling purement applicatif en espace utilisateur, il n'est pas nécessaire.

---

## Événements disponibles

`perf` peut échantillonner sur une grande variété d'événements. La liste complète dépend de votre processeur et de votre version du noyau :

```bash
perf list
```

Cette commande affiche les centaines d'événements disponibles, regroupés par catégorie. Les plus utilisés pour le profiling C++ sont :

### Événements matériels (Hardware events)

Ces événements sont comptés directement par les registres PMU du processeur :

| Événement | Description | Usage typique |
|---|---|---|
| `cycles` | Cycles CPU consommés | Profiling général (défaut) |
| `instructions` | Instructions exécutées | Mesure de l'efficacité (IPC) |
| `cache-references` | Accès au cache (tous niveaux) | Diagnostic cache |
| `cache-misses` | Échecs d'accès au cache | Problèmes de localité mémoire |
| `branch-instructions` | Branches exécutées | Analyse du flux de contrôle |
| `branch-misses` | Mauvaises prédictions de branche | Problèmes de branch prediction |
| `L1-dcache-load-misses` | Misses en lecture cache L1 données | Problèmes de localité fine |
| `LLC-load-misses` | Misses en lecture Last Level Cache | Accès mémoire coûteux |

### Événements logiciels (Software events)

Ces événements sont comptés par le noyau :

| Événement | Description | Usage typique |
|---|---|---|
| `task-clock` | Temps CPU consommé (en ms) | Mesure du temps CPU vs temps réel |
| `context-switches` | Changements de contexte | Contention, scheduling |
| `page-faults` | Défauts de page | Allocations mémoire, accès I/O |
| `cpu-migrations` | Migrations entre cœurs CPU | Affinité CPU, NUMA |

### Événements de tracepoints

Le noyau expose des milliers de points de trace (*tracepoints*) dans ses sous-systèmes. Par exemple :

```bash
# Lister les tracepoints liés au scheduler
perf list 'sched:*'

# Lister les tracepoints liés aux syscalls
perf list 'syscalls:*'
```

Ces tracepoints sont utiles pour diagnostiquer des interactions entre le programme et le noyau (latence de scheduling, contention de verrous noyau, I/O).

---

## Sous-commandes principales de `perf`

`perf` est un outil multi-commandes, à la manière de `git`. Chaque sous-commande correspond à un mode d'utilisation :

| Sous-commande | Rôle |
|---|---|
| `perf stat` | Comptage d'événements sur une exécution complète |
| `perf record` | Enregistrement de samples dans un fichier `perf.data` |
| `perf report` | Analyse interactive d'un fichier `perf.data` |
| `perf annotate` | Affichage source/assembleur avec les hotspots annotés |
| `perf top` | Profiling en direct (comme `top` pour le CPU) |
| `perf script` | Export des samples bruts (pour flamegraphs, etc.) |
| `perf diff` | Comparaison de deux profils |
| `perf list` | Liste des événements disponibles |

Les deux flux de travail principaux sont :

- **`perf stat`** pour une vue macroscopique : « combien de cycles, d'instructions, de cache misses *au total* ? ». Pas de localisation par fonction, mais une mesure globale rapide et précise. Couvert en section 31.1.2.

- **`perf record` → `perf report`** pour une vue détaillée : « *quelles fonctions* consomment le plus de cycles ? ». C'est le workflow de profiling classique. Couvert en section 31.1.1.

---

## Reconstruction des piles d'appels

Pour qu'un profil soit exploitable, `perf` doit reconstruire la pile d'appels (*call stack*) à chaque sample. Sans cette information, on sait *quelle* fonction était en cours d'exécution, mais pas *comment* on y est arrivé — ce qui rend le diagnostic beaucoup plus difficile.

`perf` propose trois méthodes de reconstruction, chacune avec ses compromis :

### Frame pointer (fp)

```bash
perf record --call-graph fp ./mon_programme
```

La méthode la plus rapide et la plus légère. `perf` remonte la pile en suivant la chaîne des frame pointers (`rbp` sur x86_64). Elle nécessite que le programme ait été compilé avec `-fno-omit-frame-pointer` — sans quoi les frame pointers sont absents et les piles reconstruites sont tronquées ou incorrectes.

C'est la méthode recommandée lorsque vous maîtrisez la compilation du programme. Le surcoût est quasi nul.

### DWARF (dwarf)

```bash
perf record --call-graph dwarf ./mon_programme
```

`perf` utilise les informations de débogage DWARF (générées par `-g`) pour reconstruire les piles. Cette méthode fonctionne même sans frame pointers, ce qui la rend compatible avec les binaires optimisés standards. En revanche, elle est plus coûteuse : `perf` copie une portion de la pile à chaque sample (par défaut 8 Ko) et la déchiffre ultérieurement.

Le surcoût est modéré (les fichiers `perf.data` sont plus volumineux), mais la précision est excellente. C'est le choix par défaut lorsque vous ne pouvez pas recompiler avec `-fno-omit-frame-pointer`.

### Last Branch Record (lbr)

```bash
perf record --call-graph lbr ./mon_programme
```

Exploite le registre LBR du processeur, qui enregistre les dernières branches exécutées. C'est la méthode la plus légère et elle ne nécessite aucune option de compilation particulière, mais la profondeur de pile est limitée (typiquement 8 à 32 niveaux selon le processeur). Disponible uniquement sur les processeurs Intel récents et certains AMD.

### Choix de la méthode

| Méthode | Surcoût | Profondeur | Prérequis de compilation | Disponibilité |
|---|---|---|---|---|
| `fp` | Minimal | Illimitée | `-fno-omit-frame-pointer` | Tous |
| `dwarf` | Modéré | Illimitée | `-g` | Tous |
| `lbr` | Minimal | Limitée (8–32) | Aucun | Intel/AMD récents |

En pratique, pour un projet dont vous contrôlez la compilation, ajoutez `-fno-omit-frame-pointer` à vos flags de production (l'impact sur les performances est négligeable) et utilisez `--call-graph fp`. Pour profiler des binaires tiers ou des librairies système, utilisez `--call-graph dwarf`.

---

## Première utilisation : `perf top`

Avant d'aborder les workflows `record`/`report` (section 31.1.1) et `stat` (section 31.1.2), la sous-commande `perf top` offre un premier contact immédiat avec le profiling :

```bash
perf top
```

`perf top` affiche en temps réel les fonctions les plus actives sur l'ensemble du système, à la manière de `top` pour la charge CPU. L'affichage se rafraîchit toutes les secondes :

```
Samples: 42K of event 'cycles', 4000 Hz, Event count (approx.): 18523741200  
Overhead  Shared Object        Symbol  
  12.34%  mon_programme        [.] Parser::analyser_token
   8.67%  mon_programme        [.] Buffer::copier_donnees
   6.12%  libc.so.6            [.] __memcpy_avx2_unaligned
   4.89%  mon_programme        [.] Reseau::lire_socket
   3.45%  [kernel]             [k] copy_user_enhanced_fast_string
   ...
```

Chaque ligne indique le pourcentage du temps CPU (*overhead*), la librairie ou le binaire contenant la fonction (*shared object*), et le nom de la fonction (*symbol*). Le préfixe `[.]` indique du code en espace utilisateur, `[k]` du code noyau.

`perf top` est l'équivalent d'un stéthoscope : il donne une impression générale immédiate de ce qui consomme du CPU, sans rien enregistrer. Pour une analyse approfondie, il faut passer à `perf record`.

Pour cibler un processus spécifique plutôt que l'ensemble du système :

```bash
# Par PID
perf top -p $(pgrep mon_service)

# Lancer et profiler directement
perf top -- ./mon_programme --ses-arguments
```

---

## Limites et considérations

### Précision statistique

Le profiling par sampling est par nature statistique. Une fonction qui ne représente que 0.1% du temps CPU peut ne pas apparaître du tout dans un profil de 10 000 samples. La précision du profil augmente avec le nombre de samples collectés, donc avec la durée d'exécution du programme. Pour les programmes de très courte durée (< 1 seconde), `perf stat` (section 31.1.2) est souvent plus informatif que `perf record`.

La fréquence d'échantillonnage par défaut est de 4 000 Hz (4 000 samples par seconde). Elle peut être ajustée avec l'option `-F` :

```bash
# Fréquence plus élevée : profil plus précis, surcoût plus important
perf record -F 10000 ./mon_programme

# Fréquence plus basse : moins de données, moins de surcoût
perf record -F 1000 ./mon_programme
```

### Skid

Le *skid* est l'écart entre l'instruction qui a réellement causé l'événement et l'instruction enregistrée dans le sample. Lorsqu'un compteur matériel overflow et déclenche une interruption, le processeur a déjà avancé de quelques instructions dans son pipeline. Le sample pointe donc vers une instruction légèrement postérieure à celle qui a causé l'événement. Sur les processeurs modernes, ce skid est typiquement de 0 à quelques dizaines d'instructions.

En pratique, le skid est rarement un problème pour le profiling au niveau des fonctions. Il peut devenir pertinent lors de l'annotation au niveau de l'instruction (`perf annotate`), où des lignes voisines de la ligne réellement responsable peuvent être sur-représentées.

### Programmes multithreads

`perf` profile tous les threads d'un processus par défaut. Les samples sont annotés avec le TID du thread, ce qui permet de filtrer par thread dans `perf report`. Cependant, pour les programmes à forte concurrence, les interactions entre threads (contention de locks, faux partage de cache lines) ne sont pas directement visibles dans un profil CPU classique. Ces problèmes relèvent du profiling de contention, que `perf` peut aborder via les tracepoints du scheduler ou les événements `offcpu`.

### Conteneurs et machines virtuelles

`perf` fonctionne dans les conteneurs Docker/Podman, mais nécessite des privilèges : soit `--privileged`, soit les capacités `CAP_SYS_ADMIN` et `CAP_PERFMON`. Dans les machines virtuelles, l'accès aux compteurs matériels dépend de la virtualisation des PMU par l'hyperviseur (supporté par KVM, partiellement par VMware, pas par tous les providers cloud).

```bash
# Exécuter perf dans un conteneur Docker
docker run --cap-add=SYS_ADMIN --cap-add=PERFMON --security-opt seccomp=unconfined \
    mon_image perf record ./mon_programme
```

---

## Plan des sous-sections

Les sous-sections qui suivent détaillent les deux workflows principaux de `perf` :

- **31.1.1 — perf record et perf report** : enregistrement d'un profil détaillé, analyse interactive des hotspots par fonction et par ligne, annotation du code source et de l'assembleur, comparaison de profils avec `perf diff`.

- **31.1.2 — perf stat : Compteurs matériels** : mesure globale des compteurs de performance du processeur, diagnostic des problèmes d'IPC, de cache et de branch prediction, et interprétation des métriques matérielles pour guider l'optimisation du code C++.

⏭️ [perf record et perf report](/31-profiling/01.1-perf-record-report.md)
