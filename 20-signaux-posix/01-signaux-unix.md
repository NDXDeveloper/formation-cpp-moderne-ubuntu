🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 20.1 — Comprendre les signaux Unix (SIGINT, SIGTERM, SIGSEGV)

## Module 7 — Programmation Système sur Linux *(Niveau Avancé)*

---

## Introduction

Avant d'installer des handlers ou de gérer les signaux dans un programme multi-threadé, il faut comprendre le mécanisme lui-même : qu'est-ce qu'un signal, comment il est délivré, quelles sont les actions par défaut, et quels signaux un programme C++ est susceptible de recevoir en production. Cette section pose les fondations conceptuelles nécessaires à une gestion correcte des signaux dans les sections 20.2 et 20.3.

---

## Qu'est-ce qu'un signal ?

Un signal est une **notification asynchrone** envoyée à un processus pour l'informer qu'un événement s'est produit. Du point de vue du processus, un signal est une interruption logicielle : le flux d'exécution normal est suspendu, une action est effectuée (action par défaut ou handler personnalisé), puis l'exécution reprend — ou se termine, selon l'action.

Les signaux sont identifiés par un numéro entier et un nom symbolique. Les numéros varient selon les plateformes Unix, mais les noms sont standardisés par POSIX. Sur Linux x86_64, les signaux standard sont numérotés de 1 à 31, et les signaux temps réel de 32 à 64.

### Sources d'émission

Un signal peut être émis par trois sources distinctes :

**Le noyau** — en réponse à un événement matériel ou logiciel : accès mémoire invalide (`SIGSEGV`), instruction illégale (`SIGILL`), division par zéro (`SIGFPE`), écriture dans un pipe sans lecteur (`SIGPIPE`), expiration d'un timer (`SIGALRM`), changement d'état d'un processus enfant (`SIGCHLD`).

**Un autre processus** — via l'appel système `kill()` (malgré son nom, il envoie n'importe quel signal, pas seulement un signal de terminaison). C'est le mécanisme utilisé par la commande shell `kill`, par `systemctl stop`, par Docker et par Kubernetes pour gérer le cycle de vie des processus.

**Le processus lui-même** — via `raise()` (signal à soi-même), `abort()` (envoie `SIGABRT`), ou indirectement via une opération qui déclenche un signal synchrone (déréférencement d'un pointeur nul → `SIGSEGV`).

---

## Le cycle de vie d'un signal

Le parcours d'un signal depuis son émission jusqu'à son traitement comporte plusieurs étapes :

```
  Émission (kill, noyau, raise)
       │
       ▼
  Signal GÉNÉRÉ ──── ajouté à l'ensemble des signaux "pending" du processus
       │
       │  Le processus est-il en train de bloquer ce signal ?
       │
       ├── OUI → Le signal reste PENDING jusqu'au déblocage
       │
       └── NON → Le signal est DÉLIVRÉ
                     │
                     │  Quelle disposition est configurée ?
                     │
                     ├── SIG_DFL → Action par défaut (terminate, core dump, ignore, stop)
                     ├── SIG_IGN → Signal ignoré silencieusement
                     └── Handler → Le handler personnalisé est exécuté
```

Plusieurs concepts clés émergent de ce cycle :

**Pending** — Un signal généré mais pas encore délivré est dit "pending". Si le même signal est envoyé plusieurs fois pendant qu'il est masqué, un seul exemplaire est conservé (les signaux standard ne s'accumulent pas — contrairement aux signaux temps réel). Cela signifie qu'il est impossible de "compter" les signaux standard : si 10 `SIGCHLD` arrivent pendant que le signal est masqué, un seul sera délivré au déblocage.

**Masquage (blocking)** — Un processus peut masquer des signaux individuels via son **signal mask**. Un signal masqué est retenu en pending mais pas délivré. Le masquage est différent de l'ignorance : un signal ignoré (`SIG_IGN`) est définitivement écarté, tandis qu'un signal masqué est juste retardé.

**Disposition** — Chaque signal a une disposition qui détermine le comportement à la délivrance : action par défaut (`SIG_DFL`), ignorance (`SIG_IGN`), ou handler personnalisé. La disposition est un attribut du processus, partagé entre tous les threads.

---

## Les signaux standard de Linux

### Signaux de terminaison

Ces signaux provoquent, par défaut, la terminaison du processus :

| Signal | N° | Source typique | Action par défaut | Description |
|---|---|---|---|---|
| `SIGTERM` | 15 | `kill`, systemd, Docker, K8s | Terminate | Demande de terminaison propre |
| `SIGINT` | 2 | Ctrl+C dans le terminal | Terminate | Interruption interactive |
| `SIGQUIT` | 3 | Ctrl+\ dans le terminal | Core dump | Interruption avec dump mémoire |
| `SIGKILL` | 9 | `kill -9`, OOM killer | Terminate | Terminaison immédiate **non interceptable** |
| `SIGHUP` | 1 | Fermeture du terminal, `systemctl reload` | Terminate | Perte du terminal contrôlant / rechargement |
| `SIGPIPE` | 13 | Écriture dans un pipe/socket sans lecteur | Terminate | Pipe cassé |

**`SIGTERM`** est le signal d'arrêt propre par excellence. C'est le premier signal envoyé par `systemctl stop`, par Docker lors de l'arrêt d'un conteneur, et par Kubernetes lors de la terminaison d'un pod. Un programme bien conçu intercepte `SIGTERM` pour sauvegarder son état, fermer ses connexions et terminer les opérations en cours avant de quitter. C'est le signal le plus important à gérer correctement.

**`SIGINT`** est envoyé quand l'utilisateur presse Ctrl+C. Contrairement à une idée reçue, `SIGINT` n'est pas envoyé au programme en premier plan uniquement : il est envoyé à **tout le groupe de processus** en premier plan du terminal. C'est une distinction importante si votre programme lance des processus enfants.

**`SIGKILL`** ne peut être ni intercepté, ni ignoré, ni masqué. C'est l'arme de dernier recours du système. Un programme ne peut rien faire face à `SIGKILL` — c'est pourquoi la gestion de `SIGTERM` est si importante : c'est la dernière chance de nettoyage avant le `SIGKILL` éventuel.

**`SIGHUP`** a un double usage historique. À l'origine, il signalait la perte de connexion du terminal (hangup). Par convention, de nombreux démons Unix l'utilisent pour déclencher une relecture de la configuration sans redémarrage. Si votre programme est un service long-running, décidez explicitement si `SIGHUP` doit provoquer un arrêt ou un rechargement.

**`SIGPIPE`** est un piège classique pour les programmes réseau et les outils CLI. Par défaut, écrire dans un pipe ou une socket dont le lecteur a fermé son extrémité provoque `SIGPIPE`, qui termine le processus. La plupart des programmes réseau ignorent `SIGPIPE` globalement et gèrent l'erreur `EPIPE` retournée par `write()` à la place :

```cpp
#include <csignal>

int main() {
    // Ignorer SIGPIPE globalement — bonne pratique pour les programmes réseau
    std::signal(SIGPIPE, SIG_IGN);

    // Désormais, write() sur un pipe cassé retourne -1 avec errno == EPIPE
    // au lieu de tuer le processus
}
```

### Signaux d'erreur (synchrones)

Ces signaux sont déclenchés par le processus lui-même suite à une erreur :

| Signal | N° | Cause | Action par défaut | Description |
|---|---|---|---|---|
| `SIGSEGV` | 11 | Accès mémoire invalide | Core dump | Segmentation fault |
| `SIGBUS` | 7 | Accès mémoire mal aligné, mmap invalide | Core dump | Bus error |
| `SIGFPE` | 8 | Division entière par zéro, overflow | Core dump | Floating point exception |
| `SIGILL` | 4 | Instruction CPU invalide | Core dump | Illegal instruction |
| `SIGABRT` | 6 | `abort()`, `assert()` échoué | Core dump | Abandon du programme |
| `SIGSYS` | 31 | Appel système invalide (seccomp) | Core dump | Bad system call |

Ces signaux sont dits **synchrones** car ils sont causés par l'exécution du processus lui-même (par opposition aux signaux asynchrones envoyés par le noyau ou un autre processus). Ils sont toujours délivrés au thread qui a provoqué l'erreur.

**`SIGSEGV`** est le signal le plus rencontré en développement C++. Il est déclenché par le noyau quand le processus tente d'accéder à une adresse mémoire qui ne lui appartient pas : déréférencement d'un pointeur nul, accès après `delete` (use-after-free), débordement de pile (stack overflow), écriture dans une zone mémoire en lecture seule.

Un point essentiel : intercepter `SIGSEGV` (ou tout signal d'erreur synchrone) avec un handler qui tente de "récupérer" l'erreur et de continuer l'exécution est un **comportement indéfini** selon POSIX. L'état du processus après un `SIGSEGV` est corrompu. Le seul usage légitime d'un handler `SIGSEGV` est de journaliser un diagnostic (stack trace, état des variables critiques) puis de terminer le processus :

```cpp
#include <csignal>
#include <unistd.h>
#include <cstring>

// Ce handler est minimaliste — c'est intentionnel (voir section 20.2)
void sigsegv_handler(int sig) {
    const char msg[] = "FATAL: Segmentation fault — arrêt du programme\n";
    // write() est async-signal-safe, std::println ne l'est PAS
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
    _exit(128 + sig);  // _exit est async-signal-safe, exit() ne l'est PAS
}
```

**`SIGABRT`** est envoyé par `abort()`, qui est appelé par `assert()` quand une assertion échoue, et par certaines implémentations de la STL en cas de violation de précondition (accès hors bornes avec `at()`, par exemple). Il produit un core dump par défaut, ce qui est le comportement souhaité : le core dump permet l'analyse post-mortem avec GDB (section 29.3).

**`SIGSYS`** est déclenché par le mécanisme seccomp (secure computing) quand un processus tente un appel système interdit par son profil de sécurité. C'est courant dans les conteneurs Docker configurés avec un profil seccomp restrictif.

### Signaux de contrôle de processus

| Signal | N° | Source typique | Action par défaut | Description |
|---|---|---|---|---|
| `SIGCHLD` | 17 | Terminaison/arrêt d'un processus enfant | Ignore | Notification de changement d'état enfant |
| `SIGSTOP` | 19 | `kill -STOP` | Stop | Suspension **non interceptable** |
| `SIGTSTP` | 20 | Ctrl+Z dans le terminal | Stop | Suspension interactive (interceptable) |
| `SIGCONT` | 18 | `kill -CONT`, `fg` | Continue | Reprise d'un processus stoppé |

**`SIGCHLD`** est automatiquement envoyé au processus parent quand un enfant termine ou est stoppé. Par défaut, il est ignoré, mais si le parent ne fait pas `wait()` ou `waitpid()` pour collecter le statut de l'enfant, celui-ci reste comme processus **zombie** (visible dans `ps` avec le statut `Z`). La section 23.1 (fork/exec) traite ce sujet en détail.

**`SIGSTOP`** est l'autre signal non interceptable (avec `SIGKILL`). Il suspend immédiatement le processus. Seul `SIGCONT` peut le reprendre.

### Signaux utilisateur et timer

| Signal | N° | Source typique | Action par défaut | Description |
|---|---|---|---|---|
| `SIGUSR1` | 10 | Application définie | Terminate | Signal utilisateur 1 |
| `SIGUSR2` | 12 | Application définie | Terminate | Signal utilisateur 2 |
| `SIGALRM` | 14 | `alarm()`, `setitimer()` | Terminate | Timer expiré |
| `SIGWINCH` | 28 | Redimensionnement du terminal | Ignore | Changement de taille de fenêtre |

**`SIGUSR1`** et **`SIGUSR2`** sont réservés aux applications. Leur sémantique est entièrement définie par le programme. Usages courants : `SIGUSR1` pour augmenter le niveau de verbosité des logs, `SIGUSR2` pour afficher des statistiques internes, rotation de logs, basculement de mode.

**`SIGWINCH`** est envoyé quand la taille du terminal change. Il est ignoré par défaut, mais les programmes interactifs en mode terminal (comme `vim`, `htop` ou une TUI C++) l'interceptent pour recalculer leur mise en page.

---

## Actions par défaut

Chaque signal a une action par défaut, classée en cinq catégories :

| Action | Comportement | Signaux concernés |
|---|---|---|
| **Terminate** | Le processus est terminé | SIGTERM, SIGINT, SIGHUP, SIGPIPE, SIGUSR1, SIGUSR2, SIGALRM |
| **Core dump** | Le processus est terminé et un core dump est généré | SIGQUIT, SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT, SIGSYS |
| **Stop** | Le processus est suspendu | SIGSTOP, SIGTSTP, SIGTTIN, SIGTTOU |
| **Continue** | Le processus suspendu reprend | SIGCONT |
| **Ignore** | Le signal est écarté silencieusement | SIGCHLD, SIGURG, SIGWINCH |

La distinction entre Terminate et Core dump est importante pour le diagnostic : un signal de type Core dump produit un fichier de dump mémoire (si les limites système le permettent — `ulimit -c`) qui peut être analysé avec GDB pour comprendre l'état exact du programme au moment du crash.

---

## Signaux non interceptables

Deux signaux ne peuvent être ni interceptés, ni ignorés, ni masqués :

**`SIGKILL` (9)** — Terminaison immédiate et inconditionnelle. Le processus n'exécute aucun code après la réception de `SIGKILL` : pas de handler, pas de destructeur, pas de `atexit`, rien. C'est le mécanisme de dernier recours utilisé par le système quand un processus ne répond pas à `SIGTERM`, ou par le OOM killer quand la mémoire est épuisée.

**`SIGSTOP` (19)** — Suspension immédiate et inconditionnelle. Le processus est gelé jusqu'à la réception d'un `SIGCONT`.

L'impossibilité d'intercepter `SIGKILL` a une conséquence architecturale directe : un programme ne peut **jamais** garantir qu'il aura l'occasion de nettoyer ses ressources. Le nettoyage doit donc être conçu pour être idempotent et résistant aux interruptions brutales. Les fichiers de verrouillage (lock files) doivent utiliser `O_EXCL` plutôt que de vérifier manuellement l'existence. Les écritures critiques doivent utiliser le pattern write-then-rename (section 19.1.3). Les transactions doivent être atomiques ou recouvrables.

---

## Envoyer un signal

### Depuis le shell

```bash
# Par numéro
kill -15 <pid>       # SIGTERM  
kill -9 <pid>        # SIGKILL  

# Par nom
kill -SIGTERM <pid>  
kill -TERM <pid>  

# À tout un groupe de processus
kill -TERM -<pgid>

# Ctrl+C → SIGINT au groupe de processus en premier plan
# Ctrl+\ → SIGQUIT au groupe de processus en premier plan
# Ctrl+Z → SIGTSTP au groupe de processus en premier plan
```

### Depuis un programme C++

```cpp
#include <csignal>
#include <sys/types.h>
#include <unistd.h>
#include <print>
#include <cerrno>
#include <cstring>

int main() {
    // Envoyer un signal à un processus spécifique
    pid_t target_pid = 1234;
    if (kill(target_pid, SIGTERM) == -1) {
        std::println("kill: {}", strerror(errno));
        // ESRCH : processus inexistant
        // EPERM : permission refusée
    }

    // Vérifier l'existence d'un processus sans envoyer de signal
    if (kill(target_pid, 0) == -1) {
        if (errno == ESRCH) {
            std::println("Le processus {} n'existe pas", target_pid);
        }
    } else {
        std::println("Le processus {} existe", target_pid);
    }

    // Envoyer un signal à tout son groupe de processus
    // kill(0, SIGTERM);  // 0 signifie "mon groupe de processus"

    // Envoyer un signal à soi-même (attention : termine le processus !)
    // raise(SIGTERM);  // Équivalent à kill(getpid(), SIGTERM)
}
```

L'appel `kill(pid, 0)` est une technique standard pour vérifier l'existence d'un processus sans lui envoyer de signal réel. Il retourne 0 si le processus existe et que l'appelant a la permission de lui envoyer un signal, ou -1 avec `errno == ESRCH` si le processus n'existe pas.

---

## Codes de sortie et signaux

Quand un processus est terminé par un signal, son **exit status** encode le numéro du signal. La convention Unix est :

```
exit code = 128 + numéro du signal
```

| Signal | Numéro | Exit code |
|---|---|---|
| SIGHUP | 1 | 129 |
| SIGINT | 2 | 130 |
| SIGKILL | 9 | 137 |
| SIGSEGV | 11 | 139 |
| SIGTERM | 15 | 143 |

```bash
# Vérifier comment un processus a terminé
./mon_programme
echo $?
# 0   → terminaison normale
# 1   → erreur applicative
# 130 → tué par SIGINT (Ctrl+C)
# 137 → tué par SIGKILL
# 139 → segmentation fault
# 143 → tué par SIGTERM
```

Ces codes sont utilisés par les scripts shell, les systèmes de CI/CD et les orchestrateurs de conteneurs pour diagnostiquer la cause de terminaison d'un programme. Un exit code de 137 dans un pipeline CI/CD signifie généralement que le processus a été tué par le OOM killer (manque de mémoire) ou par un timeout.

### Détecter le signal dans le processus parent

Le processus parent utilise `waitpid()` et les macros `WIFEXITED`, `WIFSIGNALED`, `WTERMSIG` pour déterminer comment l'enfant a terminé :

```cpp
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <print>

void check_child_status() {
    int status;
    pid_t pid = wait(&status);

    if (pid == -1) return;

    if (WIFEXITED(status)) {
        std::println("Processus {} terminé normalement, code {}",
            pid, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        std::println("Processus {} tué par signal {} ({})",
            pid, WTERMSIG(status), strsignal(WTERMSIG(status)));

        if (WCOREDUMP(status)) {
            std::println("  Core dump généré");
        }
    } else if (WIFSTOPPED(status)) {
        std::println("Processus {} stoppé par signal {}",
            pid, WSTOPSIG(status));
    }
}
```

---

## Signaux temps réel (SIGRTMIN–SIGRTMAX)

En plus des 31 signaux standard, Linux supporte les **signaux temps réel** (`SIGRTMIN` à `SIGRTMAX`), numérotés de 34 à 64 sur la plupart des architectures. Ils se distinguent des signaux standard par deux propriétés :

**Mise en file d'attente** — Contrairement aux signaux standard qui ne sont pas cumulés (envoyer 10 `SIGUSR1` pendant le masquage ne délivre qu'un seul `SIGUSR1`), les signaux temps réel sont mis en file et délivrés autant de fois qu'ils ont été envoyés.

**Données associées** — Les signaux temps réel peuvent transporter une valeur entière ou un pointeur via `sigqueue()`, récupérable dans le handler via la structure `siginfo_t`.

En pratique, les signaux temps réel sont rarement utilisés directement dans les applications C++. Ils sont surtout employés par les bibliothèques de threading (glibc utilise `SIGRTMIN` à `SIGRTMIN+2` en interne pour la gestion des threads NPTL) et par certains mécanismes d'I/O asynchrone.

---

## Consulter l'état des signaux

### Depuis le shell

```bash
# Lister les signaux disponibles
kill -l

# Voir les signaux pending et masqués d'un processus
cat /proc/<pid>/status | grep -E "Sig(Pnd|Blk|Ign|Cgt)"
# SigPnd: signaux pending
# SigBlk: signaux bloqués (masqués)
# SigIgn: signaux ignorés
# SigCgt: signaux interceptés (handler installé)
```

Les valeurs dans `/proc/<pid>/status` sont des masques hexadécimaux. Par exemple, `SigCgt: 0000000180004002` indique que les signaux correspondant aux bits positionnés sont interceptés. Des outils comme `strace` permettent de visualiser les signaux en temps réel :

```bash
# Observer les signaux reçus par un processus
strace -e signal -p <pid>

# Lancer un programme et observer tous ses signaux
strace -e trace=signal ./mon_programme
```

### Depuis le code C++

```cpp
#include <csignal>
#include <print>

void show_signal_disposition() {
    // sigaction() avec un handler NULL retourne la disposition actuelle
    struct sigaction sa;
    sigaction(SIGTERM, nullptr, &sa);

    if (sa.sa_handler == SIG_DFL) {
        std::println("SIGTERM : action par défaut");
    } else if (sa.sa_handler == SIG_IGN) {
        std::println("SIGTERM : ignoré");
    } else {
        std::println("SIGTERM : handler personnalisé installé");
    }
}
```

---

## Résumé : les signaux à connaître absolument

Pour un développeur C++ travaillant sur des services Linux ou des outils CLI, voici les signaux incontournables classés par priorité :

| Priorité | Signal | Pourquoi |
|---|---|---|
| **Critique** | `SIGTERM` | Arrêt propre — Docker, Kubernetes, systemd |
| **Critique** | `SIGINT` | Ctrl+C — tout programme interactif |
| **Important** | `SIGPIPE` | À ignorer dans les programmes réseau |
| **Important** | `SIGSEGV` | Diagnostic des crashes mémoire |
| **Important** | `SIGCHLD` | Si vous lancez des processus enfants |
| **Utile** | `SIGHUP` | Rechargement de configuration pour les démons |
| **Utile** | `SIGUSR1/2` | Actions personnalisées (logs, stats, debug) |
| **À connaître** | `SIGKILL` | Non interceptable — concevoir pour y résister |
| **À connaître** | `SIGABRT` | `assert()` échoué, core dump |
| **À connaître** | `SIGQUIT` | Ctrl+\ — core dump à la demande |

---

> 💡 **Note** — La tentation naturelle après avoir lu cette section est de vouloir intercepter un maximum de signaux "au cas où". C'est une erreur. Chaque signal intercepté ajoute de la complexité et un risque de bug dans le handler. La bonne approche est de n'intercepter que les signaux pour lesquels vous avez une action spécifique à effectuer : `SIGTERM` et `SIGINT` pour l'arrêt propre, `SIGPIPE` pour l'ignorer, et éventuellement `SIGHUP` pour le rechargement de configuration. Pour les signaux d'erreur (`SIGSEGV`, `SIGABRT`), l'action par défaut (core dump) est généralement le meilleur diagnostic. L'installer un handler sur `SIGSEGV` qui tente de "récupérer" la situation est presque toujours une erreur qui masque le vrai problème et corrompt davantage l'état du programme. La section 20.2 détaillera comment installer ces handlers correctement.

⏭️ [Installation de handlers (signal, sigaction)](/20-signaux-posix/02-handlers.md)
