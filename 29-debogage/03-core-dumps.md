🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 29.3 — Core dumps et post-mortem debugging

## Chapitre 29 : Débogage Avancé · Module 10

---

## Introduction

Jusqu'ici, vous avez débogué des programmes en temps réel — vous les lancez sous GDB, vous posez des breakpoints, vous avancez pas à pas. C'est la méthode idéale quand le bug est reproductible sur votre machine de développement.

Mais certains bugs refusent de coopérer. Le programme crash en production à 3h du matin, sous une charge que vous ne pouvez pas simuler. Il crash sur le serveur de CI, mais jamais sur votre poste. Il crash une fois par semaine, sans pattern identifiable. Vous ne pouvez pas "lancer le programme sous GDB" parce que le bug ne se manifeste qu'en dehors de votre environnement de développement.

C'est le territoire du **post-mortem debugging**. Au lieu de déboguer un programme vivant, vous analysez son cadavre : le **core dump**, un fichier qui capture l'état complet de la mémoire du processus au moment exact du crash. Avec un core dump et le binaire correspondant, GDB vous transporte dans l'état du programme à l'instant de sa mort — vous pouvez inspecter les variables, parcourir la pile d'appels, examiner la mémoire — comme si vous aviez posé un breakpoint sur la ligne qui a crashé.

Le post-mortem debugging est la technique fondamentale pour diagnostiquer des crashes en production. Ce n'est pas une compétence de niche — c'est ce qui sépare un développeur qui dit "ça crash, je ne sais pas pourquoi" d'un développeur qui livre un diagnostic précis en 30 minutes.

---

## Qu'est-ce qu'un core dump ?

Un core dump (historiquement "core file") est un fichier généré par le système d'exploitation quand un processus se termine de manière anormale — typiquement suite à un signal fatal comme SIGSEGV (segmentation fault), SIGABRT (abort), ou SIGFPE (floating-point exception).

Ce fichier contient :

- **L'intégralité de la mémoire du processus** — la pile (stack) de chaque thread, le tas (heap), les données globales, les segments mappés.
- **L'état des registres CPU** de chaque thread au moment du crash — dont le compteur programme (`rip` sur x86_64) qui indique l'instruction exacte qui a provoqué le crash.
- **Les informations sur les fichiers mappés** — quelles bibliothèques partagées étaient chargées et à quelles adresses.

En substance, un core dump est une photographie exhaustive du processus à l'instant de sa mort. Quand vous le chargez dans GDB avec le binaire correspondant, GDB reconstruit le contexte complet : vous voyez le code source, les variables, la pile d'appels — exactement comme si le programme était en pause sur un breakpoint.

---

## Activer la génération de core dumps

Par défaut, sur la plupart des distributions Linux modernes, la génération de core dumps est désactivée. C'est une mesure de sécurité et d'économie de disque : un processus qui utilise plusieurs gigaoctets de mémoire produirait un core dump de plusieurs gigaoctets, et ce fichier pourrait contenir des données sensibles (mots de passe en mémoire, clés de chiffrement, données utilisateur).

Pour le débogage, vous devez activer explicitement la génération.

### Vérifier la configuration actuelle

```bash
# Taille maximale des core dumps (en blocs de 512 octets)
ulimit -c
# 0 = désactivé (par défaut sur la plupart des systèmes)
```

### Activer pour la session courante

```bash
# Taille illimitée
ulimit -c unlimited

# Ou taille maximale spécifique (en blocs de 512 octets)
ulimit -c 1048576    # ~512 Mo
```

Cette modification s'applique uniquement au shell courant et à ses processus enfants. Quand vous fermez le terminal, la configuration revient à sa valeur par défaut.

### Activer de manière permanente

Pour que la configuration persiste entre les sessions, modifiez `/etc/security/limits.conf` :

```bash
# /etc/security/limits.conf
# <domain>  <type>  <item>   <value>
*           soft    core     unlimited
*           hard    core     unlimited
```

Puis redémarrez votre session (logout/login). Vérifiez avec `ulimit -c`.

### Contrôler l'emplacement des core dumps

Par défaut, le core dump est écrit dans le répertoire de travail du processus au moment du crash, sous le nom `core` ou `core.<pid>`. Vous pouvez contrôler le nom et l'emplacement via le noyau :

```bash
# Voir la configuration actuelle
cat /proc/sys/kernel/core_pattern

# Possibilités courantes :
# core                         → fichier "core" dans le répertoire courant
# /tmp/cores/core.%e.%p.%t    → chemin personnalisé avec métadonnées
# |/usr/share/apport/apport   → pipeline vers un gestionnaire (Ubuntu)
```

Les variables de substitution dans `core_pattern` :

| Variable | Signification |
|---|---|
| `%p` | PID du processus |
| `%u` | UID du processus |
| `%g` | GID du processus |
| `%s` | Numéro du signal qui a provoqué le dump |
| `%t` | Timestamp (epoch) |
| `%h` | Hostname |
| `%e` | Nom de l'exécutable (tronqué à 15 caractères) |
| `%E` | Chemin de l'exécutable (/ remplacés par !) |

Une configuration recommandée pour le développement :

```bash
# Créer le répertoire de destination
sudo mkdir -p /var/coredumps  
sudo chmod 1777 /var/coredumps  

# Configurer le pattern
echo "/var/coredumps/core.%e.%p.%t" | sudo tee /proc/sys/kernel/core_pattern

# Rendre persistant (survit au reboot)
echo "kernel.core_pattern = /var/coredumps/core.%e.%p.%t" | \
    sudo tee /etc/sysctl.d/99-coredump.conf
sudo sysctl -p /etc/sysctl.d/99-coredump.conf
```

Avec cette configuration, un crash de `config_parser` (PID 12345) produit un fichier `/var/coredumps/core.config_parser.12345.1710500000`. Le nom est explicite — vous savez quel programme a crashé, quand, et quel PID il avait.

### Le cas Ubuntu : `apport` et `systemd-coredump`

Sur Ubuntu, le `core_pattern` pointe souvent vers `apport` (le gestionnaire de rapports de crash) ou `systemd-coredump`. Ces systèmes interceptent le core dump, le compressent, et le stockent dans leur propre structure.

**Avec `systemd-coredump`** (Ubuntu 22.04+), les core dumps sont stockés dans le journal systemd :

```bash
# Lister les core dumps récents
coredumpctl list

# Voir les détails d'un crash
coredumpctl info <PID>

# Extraire le core dump pour GDB
coredumpctl dump <PID> -o /tmp/core_file

# Lancer GDB directement sur le dernier crash
coredumpctl debug
```

`coredumpctl debug` est remarquablement pratique : il lance GDB avec le bon binaire et le bon core dump automatiquement. Pas besoin de chercher les fichiers.

**Pour désactiver `apport`/`systemd-coredump` et obtenir des core dumps classiques** (fichiers dans le répertoire courant), vous pouvez temporairement écraser le pattern :

```bash
echo "core.%e.%p" | sudo tee /proc/sys/kernel/core_pattern
```

C'est la méthode la plus simple en développement. En production, `systemd-coredump` est préférable car il gère la compression et la rotation automatiquement.

---

## Provoquer et capturer un core dump

### Crash naturel

Le cas le plus courant : votre programme crash tout seul. Si `ulimit -c` est configuré, le core dump est généré automatiquement :

```bash
ulimit -c unlimited
./config_parser nonexistent.conf
# Segmentation fault (core dumped)
ls -la core.*
# -rw------- 1 dev dev 1245184 mars 15 14:23 core.config_parser.12345.1710500000
```

Le message `(core dumped)` confirme que le fichier a été généré. Si vous voyez `Segmentation fault` sans `(core dumped)`, vérifiez votre `ulimit -c` et votre `core_pattern`.

### Provoquer un dump manuellement

Parfois, le programme ne crash pas — il est bloqué, il boucle, ou il se comporte de manière erratique. Vous voulez capturer son état sans le tuer proprement :

```bash
# Envoyer SIGABRT au processus (génère un core dump et le tue)
kill -ABRT <PID>

# Ou depuis GDB attaché au processus
gdb -p <PID>
(gdb) generate-core-file /tmp/core_manual
(gdb) detach
```

La commande `generate-core-file` dans GDB est précieuse : elle crée un core dump du processus en pause **sans le tuer**. Vous pouvez analyser le dump plus tard, et le processus continue de tourner après le `detach`. C'est la technique pour diagnostiquer un programme bloqué en production sans interrompre le service.

### Provoquer un crash dans le code

Pour tester votre pipeline de core dumps, ajoutez un crash volontaire :

```cpp
#include <cstdlib>
#include <csignal>

// Option 1 : abort()
std::abort();

// Option 2 : raise un signal
std::raise(SIGSEGV);

// Option 3 : déréférencement null (le classique)
int* ptr = nullptr;
*ptr = 42;
```

---

## Analyser un core dump avec GDB

### Chargement

```bash
gdb ./config_parser /var/coredumps/core.config_parser.12345.1710500000
```

Deux arguments : le binaire (qui fournit les symboles et le code source) et le fichier core dump (qui fournit l'état de la mémoire). Les deux doivent correspondre — le binaire doit être exactement celui qui a produit le core dump, compilé avec les mêmes flags.

GDB affiche immédiatement le contexte du crash :

```
Core was generated by `./config_parser nonexistent.conf'.  
Program terminated with signal SIGSEGV, Segmentation fault.  
#0  0x0000555555555340 in parse_config (filename="nonexistent.conf")
    at config_parser.cpp:25
25              if (line.empty() || line[0] == '#') {
```

Vous savez déjà beaucoup : le programme a reçu un SIGSEGV, dans la fonction `parse_config`, à la ligne 25. GDB vous a placé sur la ligne exacte du crash.

### Examiner la pile d'appels

```
(gdb) backtrace
#0  0x0000555555555340 in parse_config (filename="nonexistent.conf")
    at config_parser.cpp:25
#1  0x0000555555555512 in main (argc=2, argv=0x7fffffffe3a8)
    at config_parser.cpp:53
```

La pile est courte — `main` a appelé `parse_config`, qui a crashé. Dans un programme plus complexe, la pile peut avoir des dizaines de frames, et c'est en la parcourant que vous reconstituez le chemin qui a mené au crash.

### Inspecter les variables

```
(gdb) info locals
entries = std::vector of length 0, capacity 0  
line = <error: Cannot access memory at address 0x7fffff7feff0>  
line_num = 0  
(gdb) info args
filename = "nonexistent.conf"
(gdb) print file.is_open()
$1 = false
```

Le diagnostic se dessine : le fichier n'existe pas (`is_open()` retourne false), mais le code tente quand même de lire des lignes. Le `std::getline` sur un flux invalide produit un comportement qui mène au crash.

### Naviguer dans les frames

```
(gdb) frame 1
#1  0x0000555555555512 in main (argc=2, argv=0x7fffffffe3a8)
    at config_parser.cpp:53
53          auto entries = parse_config(argv[1]);
(gdb) print argv[1]
$2 = 0x7fffffffe5b0 "nonexistent.conf"
```

En remontant dans `main`, vous confirmez que l'argument passé est bien le fichier inexistant.

### Ce que vous NE POUVEZ PAS faire avec un core dump

Un core dump est une image figée. Le programme est mort. Certaines commandes GDB ne fonctionnent pas :

```
(gdb) run
# The "remote" target does not support "run". Try "help target".

(gdb) next
# Cannot execute this command without a live process.

(gdb) continue
# Cannot execute this command without a live process.
```

Les commandes de **navigation** (`run`, `next`, `step`, `continue`) ne fonctionnent pas — il n'y a pas de processus vivant à faire avancer. Les commandes d'**inspection** fonctionnent normalement (`print`, `backtrace`, `info locals`, `x`). Les commandes de **modification** (`set variable`) modifient la mémoire du core dump en lecture mais n'ont pas d'effet pratique — il n'y a pas de processus pour reprendre l'exécution.

En résumé, un core dump vous donne une capacité complète d'observation, mais aucune capacité de contrôle. C'est une autopsie, pas une consultation.

---

## Le problème de la correspondance binaire

Le piège le plus fréquent en analyse de core dump : utiliser le mauvais binaire.

### Symptômes d'une non-correspondance

```bash
gdb ./config_parser_v2 core.config_parser.12345
```

```
warning: core file may not match specified executable file.
(gdb) backtrace
#0  0x0000555555555340 in ?? ()
#1  0x0000555555555512 in ?? ()
```

Les `??` signifient que GDB ne peut pas faire correspondre les adresses du core dump avec des symboles du binaire. Le binaire a changé depuis le crash — les fonctions ne sont plus aux mêmes adresses.

### La règle : même binaire, mêmes bibliothèques

Pour une analyse correcte, vous avez besoin de :

1. **Le binaire exact** qui a produit le core dump — pas une version recompilée, pas la version d'après le fix, pas la version debug si le crash s'est produit en release. Le même fichier, octet pour octet.
2. **Les bibliothèques partagées** chargées au moment du crash — si votre programme utilise `libcurl.so.4`, vous avez besoin de la version exacte qui était installée sur le serveur au moment du crash.

### Stratégies pour garantir la correspondance

**Archiver les binaires avec symboles.** En CI/CD, conservez le binaire compilé avec `-g` (ou un fichier de symboles séparé) à chaque release. Quand un crash survient en production, vous récupérez le binaire correspondant au tag de release déployé.

**Fichiers de symboles séparés.** En production, vous déployez un binaire strippé (sans symboles de débogage) pour réduire la taille. Les symboles sont extraits dans un fichier séparé :

```bash
# Compilation avec symboles
g++ -g -O2 -o config_parser main.cpp

# Extraire les symboles dans un fichier séparé
objcopy --only-keep-debug config_parser config_parser.debug

# Stripper le binaire de production
strip --strip-debug config_parser

# Ajouter un lien vers le fichier de symboles
objcopy --add-gnu-debuglink=config_parser.debug config_parser
```

En production, vous déployez `config_parser` (strippé, léger). En débogage, vous utilisez `config_parser.debug` avec GDB :

```bash
gdb -s config_parser.debug -c core.12345 -e config_parser
```

Ou, si le debug link est configuré, GDB trouve les symboles automatiquement si le fichier `.debug` est dans le même répertoire ou dans `/usr/lib/debug/`.

**Build ID.** GCC et Clang incluent un identifiant unique dans chaque binaire (le Build ID). GDB le vérifie lors du chargement d'un core dump :

```bash
# Voir le Build ID d'un binaire
readelf -n config_parser | grep "Build ID"
  Build ID: 7a3f2b8e9d1c4a5b6e7f8a9b0c1d2e3f4a5b6c7d

# GDB vérifie automatiquement
gdb ./config_parser core.12345
# warning: build-id mismatch → binaire incorrect
```

Le Build ID est votre filet de sécurité. S'il ne correspond pas, GDB vous prévient — vous savez que vous analysez avec le mauvais binaire.

---

## Analyse des bibliothèques partagées

Quand un crash survient dans une bibliothèque partagée (et non dans votre code), vous avez besoin des symboles de cette bibliothèque pour obtenir un backtrace lisible.

### Identifier les bibliothèques chargées

```
(gdb) info sharedlibrary
From                To                  Syms Read   Shared Object Library
0x00007ffff7dd5000  0x00007ffff7df7000  Yes         /lib64/ld-linux-x86-64.so.2
0x00007ffff7a00000  0x00007ffff7bc7000  Yes (*)     /lib/x86_64-linux-gnu/libstdc++.so.6
0x00007ffff7800000  0x00007ffff79f5000  Yes (*)     /lib/x86_64-linux-gnu/libc.so.6
(*): Shared library is missing debugging information.
```

Le `(*)` indique que les symboles de débogage ne sont pas disponibles. Le backtrace dans ces bibliothèques affichera des adresses brutes au lieu de noms de fonctions.

### Installer les symboles de débogage

Sur Ubuntu, les paquets de symboles sont disponibles via `dbgsym` :

```bash
# Activer le dépôt de symboles de débogage
echo "deb http://ddebs.ubuntu.com $(lsb_release -cs) main restricted universe" | \
    sudo tee /etc/apt/sources.list.d/ddebs.list
sudo apt update

# Installer les symboles pour les bibliothèques communes
sudo apt install libc6-dbgsym libstdc++6-dbgsym
```

Après installation, relancez GDB avec le core dump — les backtraces dans les bibliothèques système deviennent lisibles.

### Pointer vers les bibliothèques d'une autre machine

Si le crash s'est produit sur un serveur avec des versions de bibliothèques différentes de votre machine de développement, vous pouvez copier les bibliothèques du serveur et les indiquer à GDB :

```bash
# Sur le serveur : copier les bibliothèques
scp -r server:/lib/x86_64-linux-gnu/ ./server-libs/

# Dans GDB : pointer vers les bibliothèques
(gdb) set sysroot ./server-libs
(gdb) set solib-search-path ./server-libs/lib/x86_64-linux-gnu
```

GDB utilisera ces bibliothèques au lieu de celles installées localement, et les adresses correspondront correctement.

---

## Analyse avancée : threads et signaux

### Crash dans un programme multithreadé

Un core dump multithreadé capture l'état de tous les threads, pas seulement celui qui a crashé :

```
(gdb) info threads
  Id   Target Id                           Frame
* 1    Thread 0x7ffff7c2e740 (LWP 12345)   __GI_raise (sig=sig@entry=6)
                                            at ../sysdeps/unix/sysv/linux/raise.c:50
  2    Thread 0x7ffff6c2d700 (LWP 12346)   futex_wait (...)
                                            at ../sysdeps/nptl/futex-internal.h:146
  3    Thread 0x7ffff5c2c700 (LWP 12347)   0x00007ffff7b12340 in read ()
                                            from /lib/x86_64-linux-gnu/libc.so.6
```

L'astérisque `*` marque le thread qui a déclenché le crash (thread 1 dans cet exemple). Les autres threads sont figés dans l'état où ils se trouvaient à cet instant.

Pour examiner un autre thread :

```
(gdb) thread 2
[Switching to thread 2 (Thread 0x7ffff6c2d700 (LWP 12346))]
#0  futex_wait (...) at ../sysdeps/nptl/futex-internal.h:146
(gdb) backtrace
#0  futex_wait (...) at ../sysdeps/nptl/futex-internal.h:146
#1  __lll_lock_wait (...) at lowlevellock.c:49
#2  pthread_mutex_lock (...) at pthread_mutex_lock.c:108
#3  0x0000555555555a10 in worker_process (id=2) at worker.cpp:45
#4  0x00007ffff7a5b6b3 in start_thread (...) at pthread_create.c:442
```

Ce thread était bloqué sur un mutex. C'est un pattern fréquent dans les crashs multithreadés : un thread crash, et les autres sont en attente de synchronisation.

Pour voir tous les backtraces d'un coup :

```
(gdb) thread apply all backtrace

Thread 3 (Thread 0x7ffff5c2c700 (LWP 12347)):
#0  0x00007ffff7b12340 in read () from /lib/x86_64-linux-gnu/libc.so.6
#1  0x0000555555555c20 in io_worker () at io.cpp:30
[...]

Thread 2 (Thread 0x7ffff6c2d700 (LWP 12346)):
#0  futex_wait (...) at ../sysdeps/nptl/futex-internal.h:146
[...]

Thread 1 (Thread 0x7ffff7c2e740 (LWP 12345)):
#0  __GI_raise (sig=sig@entry=6) at ../sysdeps/unix/sysv/linux/raise.c:50
[...]
```

C'est souvent la première commande à exécuter sur un core dump multithreadé. Vous obtenez une vision complète de ce que faisait chaque thread au moment du crash.

### Identifier le signal

```
(gdb) print $_siginfo
$1 = {
  si_signo = 11,            # SIGSEGV
  si_errno = 0,
  si_code = 1,              # SEGV_MAPERR (accès à une adresse non mappée)
  _sifields = {
    _sigfault = {
      si_addr = 0x0          # L'adresse fautive (ici: NULL)
    }
  }
}
```

La variable spéciale `$_siginfo` contient les détails du signal qui a tué le processus :

- `si_signo = 11` → SIGSEGV (segmentation fault)
- `si_code = 1` → SEGV_MAPERR (l'adresse n'est mappée dans aucun segment mémoire)
- `si_addr = 0x0` → l'adresse fautive est NULL — c'est un déréférencement de pointeur null

Pour les signaux courants :

| Signal | Numéro | Cause typique |
|---|---|---|
| SIGSEGV | 11 | Accès mémoire invalide (pointeur null, buffer overflow, use-after-free) |
| SIGABRT | 6 | Appel à `abort()`, assertion échouée, exception non attrapée |
| SIGFPE | 8 | Division par zéro, overflow flottant |
| SIGBUS | 7 | Accès mémoire mal aligné, fichier mappé tronqué |
| SIGSYS | 31 | Appel système interdit (seccomp) |

---

## Core dumps en production

### Dimensionnement et rotation

Un core dump fait typiquement la taille de la mémoire résidente (RSS) du processus. Un serveur C++ qui utilise 2 Go de RAM produit un core dump de ~2 Go. Trois crashs dans la journée, et vous avez perdu 6 Go de disque.

**Avec `systemd-coredump`**, la rotation est gérée automatiquement. La configuration se trouve dans `/etc/systemd/coredump.conf` :

```ini
# /etc/systemd/coredump.conf
[Coredump]
Storage=external  
Compress=yes  
MaxUse=2G  
KeepFree=1G  
ProcessSizeMax=1G  
```

| Paramètre | Rôle |
|---|---|
| `Storage=external` | Stocke les core dumps sur disque (pas en journal) |
| `Compress=yes` | Compression zstd (réduit la taille de 60-80 %) |
| `MaxUse=2G` | Espace disque maximum pour les core dumps |
| `KeepFree=1G` | Arrêter de stocker si l'espace libre passe sous 1 Go |
| `ProcessSizeMax=1G` | Ignorer les processus de plus de 1 Go |

Après modification, rechargez :

```bash
sudo systemctl daemon-reload
```

### Sécurité

Un core dump contient tout ce qui était en mémoire au moment du crash. Cela inclut potentiellement des mots de passe, des tokens d'authentification, des clés de chiffrement, et des données utilisateur. En production :

- **Restreignez les permissions.** Les core dumps doivent être lisibles uniquement par root et l'utilisateur du processus. `systemd-coredump` s'en charge par défaut.
- **Chiffrez le stockage.** Si les core dumps sont archivés, utilisez un stockage chiffré.
- **Ne transmettez pas de core dumps via des canaux non sécurisés.** Un core dump envoyé par email ou stocké sur un serveur de fichiers accessible à tous expose des données sensibles.
- **Nettoyez après analyse.** Supprimez les core dumps après diagnostic. Ne les accumulez pas.

### Pipeline de collecte en production

Un setup de production robuste pour la gestion des core dumps suit cette architecture :

```
Programme crash
    │
    ▼
systemd-coredump
    │ (compresse, stocke localement)
    ▼
Collecteur (script cron ou agent)
    │ (copie vers stockage centralisé)
    ▼
Stockage sécurisé (S3 chiffré, NFS restreint)
    │
    ▼
Alerte (PagerDuty, Slack, email)
    │
    ▼
Développeur analyse avec GDB
```

Le script de collecte type :

```bash
#!/bin/bash
# /usr/local/bin/collect-coredumps.sh

DEST="s3://my-bucket/coredumps/$(hostname)"  
DUMP_DIR="/var/lib/systemd/coredump"  

for dump in "$DUMP_DIR"/*.zst; do
    [ -f "$dump" ] || continue
    aws s3 cp "$dump" "$DEST/$(basename "$dump")"
    logger "Core dump uploaded: $(basename "$dump")"
done
```

---

## Core dumps et conteneurs Docker

Les conteneurs Docker ajoutent une couche de complexité à la génération de core dumps, car le `core_pattern` du noyau est global — il est partagé entre l'hôte et tous les conteneurs.

### Le problème

Le `core_pattern` est un paramètre du noyau, pas du conteneur. Vous ne pouvez pas le modifier depuis l'intérieur d'un conteneur (même avec `--privileged`). Si le pattern de l'hôte pointe vers `/var/coredumps/`, le core dump sera écrit à cet emplacement sur l'hôte, pas dans le conteneur.

### Solution 1 : volume partagé

Montez un volume pour recevoir les core dumps :

```bash
# Sur l'hôte, configurer le pattern
echo "/coredumps/core.%e.%p.%t" | sudo tee /proc/sys/kernel/core_pattern

docker run \
    --cap-add=SYS_PTRACE \
    --ulimit core=-1 \
    -v /coredumps:/coredumps \
    myapp:debug
```

Le core dump est écrit dans `/coredumps` sur l'hôte, accessible depuis l'extérieur du conteneur.

### Solution 2 : `systemd-coredump` sur l'hôte

Si l'hôte utilise `systemd-coredump`, les core dumps des processus conteneurisés sont capturés par le journal systemd de l'hôte :

```bash
# Sur l'hôte : lister les crashes, y compris ceux des conteneurs
coredumpctl list

# Analyser un crash conteneurisé
coredumpctl debug <PID>
```

Le binaire et les bibliothèques du conteneur doivent être accessibles pour que GDB produise un backtrace lisible. `coredumpctl debug` tente de les localiser automatiquement, mais en pratique il est souvent nécessaire de pointer GDB vers le bon sysroot.

### Solution 3 : gdb et core dump dans le conteneur

Si vous préférez tout faire dans le conteneur :

```dockerfile
FROM ubuntu:24.04 AS debug  
RUN apt-get update && apt-get install -y gdb  
# Le core_pattern sera hérité de l'hôte
```

```bash
docker run -it \
    --cap-add=SYS_PTRACE \
    --ulimit core=-1 \
    -v /coredumps:/coredumps \
    myapp:debug bash

# Dans le conteneur
./config_parser bad_input.conf
# Segmentation fault (core dumped)
gdb ./config_parser /coredumps/core.config_parser.54321.*
```

---

## Workflow complet de diagnostic

Voici le workflow type quand un crash survient en production. Ce n'est pas une procédure rigide, mais un guide qui couvre les étapes les plus courantes :

**1. Récupérer le core dump et le binaire correspondant.**

```bash
# Si systemd-coredump est configuré
coredumpctl info          # Identifier le crash  
coredumpctl dump -o /tmp/core_file  

# Récupérer le binaire exact (même Build ID)
# Depuis l'artefact CI, le registre Docker, ou le serveur de déploiement
```

**2. Charger dans GDB.**

```bash
gdb ./config_parser /tmp/core_file
```

**3. Vue d'ensemble immédiate.**

```
(gdb) backtrace
(gdb) info threads
(gdb) thread apply all backtrace    # Si multithreadé
(gdb) print $_siginfo               # Détails du signal
```

**4. Inspecter le point de crash.**

```
(gdb) frame 0                       # Frame du crash
(gdb) info locals
(gdb) info args
(gdb) list                          # Voir le code source autour du crash
```

**5. Remonter dans la pile pour comprendre le contexte.**

```
(gdb) frame 1
(gdb) info locals
(gdb) print <variable suspecte>
```

**6. Formuler un diagnostic.**

À ce stade, vous avez généralement assez d'information pour identifier la cause : pointeur null, accès hors limites, variable non initialisée, état corrompu. Documentez vos findings — le numéro de ligne, les valeurs des variables, la pile d'appels — dans votre ticket de bug.

**7. Reproduire et corriger.**

Avec le diagnostic du core dump, vous savez exactement quelle condition déclenche le crash. Écrivez un test unitaire qui reproduit cette condition, corrigez le code, et vérifiez que le test passe.

---

> **À retenir** : un core dump est une photographie complète d'un programme au moment de sa mort. Configurer la génération de core dumps (`ulimit -c unlimited`, `core_pattern`), archiver les binaires avec leurs symboles, et maîtriser l'analyse avec GDB — c'est le triptyque qui vous permet de diagnostiquer n'importe quel crash, même s'il survient à 3h du matin sur un serveur que vous ne pouvez pas reproduire localement.

⏭️ [Sanitizers](/29-debogage/04-sanitizers.md)
