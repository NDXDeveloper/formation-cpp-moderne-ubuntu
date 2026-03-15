🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 23 — Processus et IPC (Inter-Process Communication)

## Module 7 : Programmation Système sur Linux — Niveau Avancé

---

## Vue d'ensemble

Le chapitre 22 a couvert la communication entre machines via le réseau — sockets, HTTP, gRPC. Ce chapitre traite de l'autre face de la communication : l'échange de données **entre processus sur la même machine**.

Sur un système Linux typique, des dizaines (voire des centaines) de processus coexistent et coopèrent. Un serveur web lance des workers pour traiter les requêtes. Un gestionnaire de services (`systemd`) orchestre le démarrage et l'arrêt de démons. Un pipeline de traitement de données enchaîne des programmes reliés par des pipes. Un outil CLI lance un processus enfant pour exécuter une commande externe. Un conteneur Docker est lui-même un processus isolé qui communique avec le démon Docker via un socket Unix.

Comprendre comment les processus naissent, s'exécutent, meurent et communiquent est une compétence fondamentale de la programmation système Linux — et une compétence distincte du networking. Les mécanismes sont différents (pas de pile TCP/IP, pas de sérialisation réseau), les contraintes sont différentes (mémoire partageable, latence quasi nulle), et les API sont différentes (fork/exec, pipes, shared memory, message queues).

---

## Ce que vous allez apprendre

Ce chapitre couvre les quatre piliers de la gestion de processus et de la communication inter-processus sous Linux :

**Création et gestion de processus** — Le modèle Unix de création de processus est unique : `fork()` clone le processus courant, `exec()` remplace son image. Ce modèle, vieux de plus de 50 ans, reste le fondement de tout lancement de programme sur Linux. Vous comprendrez le cycle de vie complet d'un processus — de sa naissance par `fork` à sa mort détectée par `wait` — et les subtilités qui en découlent : processus zombies, orphelins, et les alternatives modernes comme `posix_spawn`.

**Pipes et communication directionnelle** — Les pipes sont le mécanisme IPC le plus simple et le plus utilisé. Chaque commande shell reliée par `|` utilise un pipe. Vous verrez les pipes anonymes (pour la communication parent-enfant) et les pipes nommées (FIFOs, pour la communication entre processus indépendants), et comment les intégrer dans du code C++ avec une gestion RAII propre.

**Mémoire partagée** — Quand les pipes ne suffisent plus (volume de données trop important, besoin d'accès aléatoire plutôt que séquentiel), la mémoire partagée offre le mécanisme IPC le plus rapide possible : **zéro copie**. Les données ne transitent jamais par le noyau — les processus lisent et écrivent directement dans la même zone mémoire physique. Vous couvrirez `mmap` avec des fichiers mappés et la mémoire partagée POSIX (`shm_open`), avec les mécanismes de synchronisation nécessaires pour éviter les race conditions.

**Files de messages POSIX** — Les message queues offrent un modèle producteur-consommateur structuré : des messages typés et prioritisés, avec des garanties d'ordre et de persistance au niveau du noyau. C'est un intermédiaire entre la simplicité des pipes et la puissance de la mémoire partagée.

---

## Pourquoi ce chapitre est important

### Pour le développeur système

La maîtrise de `fork`/`exec` et des mécanismes IPC est le socle de la programmation système Linux. Sans elle, vous ne pouvez pas écrire un démon, un gestionnaire de processus, un shell, un système de build, ou un outil d'orchestration. Les concepts couverts ici sont ceux que manipulent `systemd`, `Docker`, `make`, `bash` et `nginx` en interne.

### Pour le développeur DevOps

Les outils DevOps en C++ lancent régulièrement des processus externes : appeler `git`, exécuter un script de déploiement, lancer un conteneur, invoquer un compilateur. Savoir lancer un processus enfant, capturer sa sortie, détecter son code de retour et gérer les timeouts est une compétence quotidienne.

### Pour la performance

La mémoire partagée est le mécanisme IPC le plus rapide — des ordres de grandeur plus rapide que les sockets TCP sur la même machine, car les données ne traversent jamais la pile réseau du noyau. Pour les systèmes à faible latence (trading, traitement audio/vidéo, bases de données), c'est souvent le seul choix viable.

### Pour la sécurité et l'isolation

Comprendre les processus, c'est aussi comprendre l'isolation. Les conteneurs Docker reposent sur des namespaces et des cgroups — des mécanismes de processus. Les sandboxes de sécurité utilisent `seccomp` pour filtrer les appels système. Le modèle de permissions Linux est fondamentalement un modèle de processus (chaque processus a un UID, un GID, des capabilities).

---

## Prérequis

Ce chapitre s'appuie sur les compétences suivantes :

- **Gestion de la mémoire** (chapitre 5) — La compréhension de la stack, du heap et du layout mémoire d'un processus est essentielle pour saisir ce que `fork` duplique et ce que `mmap` expose.
- **RAII et smart pointers** (chapitres 6, 9) — Les file descriptors de pipes et les segments de mémoire partagée sont des ressources système qui doivent être gérées avec la même rigueur que les sockets du chapitre 22.
- **Signaux POSIX** (chapitre 20) — Les signaux sont le mécanisme de notification principal entre processus. `SIGCHLD` signale la mort d'un enfant, `SIGPIPE` signale l'écriture dans un pipe cassé, `SIGTERM` demande un arrêt propre.
- **Threads et concurrence** (chapitre 21) — La mémoire partagée nécessite des primitives de synchronisation. La distinction entre processus et threads (espaces d'adressage séparés vs partagé) est fondamentale.
- **Networking** (chapitre 22) — Les sockets Unix (`AF_UNIX`) sont un mécanisme IPC couvert au chapitre 22. Ce chapitre les complète avec des mécanismes non-socket.

---

## Processus vs Threads : clarification

Avant de plonger dans le code, clarifions la distinction fondamentale entre processus et threads, car elle détermine le choix du mécanisme de communication :

```
Processus A                          Processus B
┌────────────────────┐               ┌────────────────────┐
│  Espace d'adressage│               │  Espace d'adressage│
│  SÉPARÉ            │               │  SÉPARÉ            │
│                    │               │                    │
│  ┌──────┐ ┌──────┐ │               │  ┌──────┐ ┌──────┐ │
│  │Thread│ │Thread│ │               │  │Thread│ │Thread│ │
│  │  1   │ │  2   │ │               │  │  3   │ │  4   │ │
│  └──────┘ └──────┘ │               │  └──────┘ └──────┘ │
│                    │               │                    │
│  heap, stack,      │               │  heap, stack,      │
│  code, données     │               │  code, données     │
│  (propres)         │               │  (propres)         │
└────────────────────┘               └────────────────────┘
         │                                     │
         └──────── IPC nécessaire ─────────────┘
              (pipes, shm, mqueue, sockets)

Processus A (multi-threadé)
┌───────────────────────────────────┐
│  Espace d'adressage PARTAGÉ       │
│                                   │
│  ┌──────┐ ┌──────┐ ┌──────┐       │
│  │Thread│ │Thread│ │Thread│       │
│  │  1   │ │  2   │ │  3   │       │
│  └──────┘ └──────┘ └──────┘       │
│                                   │
│  heap, code, données PARTAGÉS     │
│  (stacks séparées)                │
│                                   │
│  Communication : mutex,           │
│  variables partagées, atomics     │
└───────────────────────────────────┘
```

**Threads** (chapitre 21) partagent le même espace d'adressage. La communication est triviale — n'importe quelle variable est accessible par tous les threads. Le défi est la **synchronisation** (mutexes, atomics, condition variables).

**Processus** ont des espaces d'adressage **isolés**. Un pointeur dans le processus A ne signifie rien dans le processus B. Toute communication nécessite un mécanisme explicite — c'est l'objet de ce chapitre.

### Pourquoi utiliser des processus plutôt que des threads ?

L'isolation des processus, bien que contraignante pour la communication, offre des avantages importants :

**Isolation des crashes** — Un segfault dans un thread tue tout le processus. Un segfault dans un processus enfant ne touche pas le parent. C'est pourquoi Nginx, Apache et Chrome utilisent des processus workers plutôt que des threads.

**Isolation de sécurité** — Chaque processus peut avoir des privilèges différents (UID, capabilities, seccomp filters). Les threads partagent les mêmes privilèges.

**Isolation mémoire** — Un bug mémoire (buffer overflow, use-after-free) dans un processus ne corrompt pas la mémoire d'un autre processus. Entre threads, un tel bug peut corrompre n'importe quelle donnée partagée.

**Langage hétérogène** — Des processus peuvent être écrits dans des langages différents (votre orchestrateur C++ lance un script Python, qui appelle un outil Go). Les threads doivent être dans le même langage (ou le même runtime).

---

## Panorama des mécanismes IPC

Linux offre une richesse de mécanismes IPC, chacun avec ses forces et ses cas d'usage :

```
Mécanisme          Direction       Latence     Débit      Complexité
─────────────────  ─────────────   ────────    ─────────  ──────────
Pipe anonyme       Unidirectionnel Faible      Moyen      Très faible  
Pipe nommé (FIFO)  Unidirectionnel Faible      Moyen      Faible  
Socket Unix        Bidirectionnel  Faible      Bon        Moyenne  
Mémoire partagée   Bidirectionnel  Très faible Très élevé Élevée  
Message queue      Bidirectionnel  Faible      Moyen      Moyenne  
Signal             Unidirectionnel Très faible Nul (*)    Faible  
Fichier            Bidirectionnel  Élevée      Variable   Faible  

(*) Les signaux ne transportent quasiment pas de données (juste un numéro).
```

Ce chapitre couvre les quatre mécanismes les plus importants : pipes (anonymes et nommés), mémoire partagée avec `mmap`, et message queues POSIX. Les sockets Unix (`AF_UNIX`) ont été abordés au chapitre 22. Les signaux sont couverts au chapitre 20.

### Comment choisir ?

**Communication parent ↔ enfant simple** → Pipe anonyme. C'est le plus simple et le plus courant. Si un pipe suffit, utilisez un pipe.

**Communication entre processus indépendants** → Pipe nommé (FIFO) pour du flux simple, socket Unix pour du bidirectionnel structuré, message queue pour du producteur-consommateur.

**Volume de données important ou accès aléatoire** → Mémoire partagée. C'est le seul mécanisme zéro-copie, et le seul qui permet un accès non-séquentiel.

**Communication structurée avec priorités** → Message queue POSIX. Les messages sont délimités (pas un flux d'octets comme les pipes), optionnellement prioritisés, et persistent dans le noyau jusqu'à leur consommation.

**Communication inter-machines** → Ce n'est plus de l'IPC, c'est du networking — retour au chapitre 22.

---

## Conventions de ce chapitre

Les exemples suivent les mêmes conventions que le chapitre 22 :

- **RAII systématique** — Les file descriptors (pipes), les segments de mémoire partagée, et les message queues sont encapsulés dans des wrappers qui libèrent les ressources automatiquement.
- **Gestion d'erreurs complète** — Chaque appel système est vérifié. Les erreurs utilisent `std::system_error` ou `std::expected` (C++23).
- **C++ moderne** — Les exemples utilisent C++20 au minimum. Les fonctionnalités C++23 (`std::expected`, `std::print`) sont utilisées quand elles apportent de la clarté.
- **Sécurité des processus** — Les exemples gèrent correctement les processus zombies, les pipes cassés, et les signaux d'interruption.

---

## Plan du chapitre

| Section | Sujet | Contenu clé |
|---------|-------|-------------|
| **23.1** | fork, exec et gestion de processus | Cycle de vie d'un processus, `fork()`, famille `exec`, `wait`/`waitpid`, processus zombies, `posix_spawn`, wrapper RAII |
| **23.2** | Pipes et communication inter-processus | Pipes anonymes, redirection de stdout/stderr, pipes nommées (FIFO), communication parent ↔ enfant |
| **23.3** | Shared memory et mmap | `mmap` pour les fichiers mappés et la mémoire anonyme, `shm_open` / `shm_unlink`, synchronisation inter-processus |
| **23.4** | Message queues POSIX | `mq_open`, `mq_send`, `mq_receive`, priorités, notification asynchrone, comparaison avec les pipes |

---

## Connexions avec les autres chapitres

- **Chapitre 20 (Signaux POSIX)** — Les signaux sont le mécanisme de notification entre processus. `SIGCHLD` détecte la mort d'un enfant sans polling, `SIGPIPE` signale un pipe cassé, `SIGTERM`/`SIGKILL` contrôlent l'arrêt des processus.
- **Chapitre 21 (Threads)** — La mémoire partagée inter-processus nécessite des primitives de synchronisation inter-processus (`pthread_mutex` avec attribut `PTHREAD_PROCESS_SHARED`, sémaphores POSIX) — une extension des concepts du chapitre 21.
- **Chapitre 22 (Networking)** — Les sockets Unix (`AF_UNIX`) sont un mécanisme IPC couvert au chapitre 22. Le passage de file descriptors entre processus via `sendmsg` sur un socket Unix est un pattern avancé qui relie les deux chapitres.
- **Chapitre 37 (Dockerisation)** — Les conteneurs Docker sont des processus isolés par des namespaces. La communication entre un conteneur et son hôte passe par les mécanismes de ce chapitre (sockets Unix, volumes montés = fichiers partagés, pipes).
- **Chapitre 38 (CI/CD)** — Les pipelines CI lancent des processus (compilateurs, tests, linters) et capturent leur sortie et leur code de retour. Les concepts de `fork`/`exec`/`wait` sont au cœur de ce mécanisme.

---

> **Prêt ?** Commencez par la section 23.1 pour comprendre comment les processus naissent et meurent sous Linux — le modèle `fork`/`exec` qui est au cœur de tout système Unix.

⏭️ [fork, exec et gestion de processus](/23-processus-ipc/01-fork-exec.md)
