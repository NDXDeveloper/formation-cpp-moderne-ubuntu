🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 20 : Signaux POSIX

## Module 7 — Programmation Système sur Linux *(Niveau Avancé)*

---

## Introduction

Les signaux sont le mécanisme de notification asynchrone le plus ancien et le plus fondamental de Unix. Ils permettent au noyau, à d'autres processus ou au processus lui-même de signaler un événement qui requiert une attention immédiate : l'utilisateur a pressé Ctrl+C, un processus enfant a terminé, une opération a provoqué une erreur fatale, un timer a expiré, ou un administrateur demande l'arrêt propre d'un service.

En tant que développeur C++ sur Linux, vous interagissez avec les signaux que vous le vouliez ou non. Un programme qui ne gère pas les signaux se termine brutalement à chaque Ctrl+C, perd les données non sauvegardées, laisse des fichiers temporaires orphelins et des connexions réseau dans un état indéterminé. Un programme qui les gère mal — et c'est un piège extrêmement courant — introduit des bugs subtils, des race conditions et des comportements indéfinis qui ne se manifestent qu'en production sous charge.

Ce chapitre couvre les signaux POSIX sous l'angle du développeur C++ moderne : comprendre le mécanisme, installer des handlers corrects, éviter les pièges d'async-signal-safety, et gérer la coexistence délicate entre les signaux et les threads.

---

## Pourquoi ce chapitre est essentiel

### Pour le développeur système

Les signaux sont omniprésents dans la vie d'un processus Linux. Même un programme simple reçoit des signaux : `SIGPIPE` lors de l'écriture dans un pipe cassé, `SIGCHLD` quand un processus enfant termine, `SIGWINCH` quand le terminal est redimensionné. Les serveurs long-running reçoivent `SIGHUP` pour recharger leur configuration, `SIGTERM` pour s'arrêter proprement, `SIGUSR1`/`SIGUSR2` pour des actions personnalisées. Un développeur système qui ne maîtrise pas les signaux produit des programmes fragiles qui ne répondent pas correctement aux événements de leur environnement.

### Pour le DevOps et le Cloud Native

Le cycle de vie des conteneurs Docker et des pods Kubernetes repose entièrement sur les signaux. Quand Kubernetes décide de terminer un pod, il envoie `SIGTERM` au processus principal, attend un délai de grâce (30 secondes par défaut), puis envoie `SIGKILL`. Un programme C++ qui ne gère pas `SIGTERM` n'a aucune chance de sauvegarder son état, de fermer proprement ses connexions ou de terminer les requêtes en cours avant d'être tué. Systemd utilise le même mécanisme pour l'arrêt des services. La gestion correcte de `SIGTERM` est un requis non négociable pour tout programme déployé en production.

### Pour la robustesse du code

Les signaux sont la source de certaines des classes de bugs les plus difficiles à diagnostiquer : corruption de données par un handler qui appelle une fonction non-réentrante, deadlock par un handler qui prend un mutex déjà verrouillé, comportement indéfini par un handler qui modifie une variable non-`volatile sig_atomic_t`. Comprendre les contraintes des signal handlers est ce qui permet d'écrire du code qui survit aux conditions réelles de production.

---

## Ce que vous allez apprendre

Ce chapitre se structure autour de trois axes progressifs :

**Comprendre les signaux Unix** — Le mécanisme de délivrance, les signaux standard (SIGINT, SIGTERM, SIGSEGV, SIGPIPE, SIGHUP, SIGCHLD…), les actions par défaut, les signaux non interceptables (SIGKILL, SIGSTOP), et le concept de signal pending vs delivered. Cette section pose les fondations nécessaires pour comprendre *pourquoi* les handlers sont si contraints.

**Installation de handlers** — Les deux API disponibles (`signal()` et `sigaction()`), pourquoi `sigaction()` est la seule option sérieuse, les contraintes d'async-signal-safety, les techniques pour contourner ces contraintes (flag `volatile sig_atomic_t`, self-pipe trick, `signalfd`), et les patterns de gestion propre d'arrêt (graceful shutdown).

**Signaux et threads** — La problématique spécifique de la concurrence entre signaux et threads : quel thread reçoit le signal, comment masquer les signaux dans les worker threads, `pthread_sigmask`, `sigwait` comme alternative aux handlers dans un programme multi-threadé, et l'interaction avec `std::jthread` et les stop tokens C++20.

---

## Prérequis

Avant d'aborder ce chapitre, vous devez être à l'aise avec :

- La **gestion de la mémoire** et le concept de **pile d'exécution** (chapitre 5), car les signal handlers s'exécutent sur la pile du thread interrompu (ou sur une pile alternative si configurée).
- Le principe **RAII** (section 6.3), car la gestion propre des ressources lors d'un arrêt par signal repose sur la capacité des destructeurs à nettoyer automatiquement.
- Les **appels système POSIX** (section 19.2), notamment `open`, `read`, `write` et la notion de descripteur de fichier, car les techniques avancées de gestion des signaux (self-pipe, `signalfd`) reposent sur les descripteurs.
- Les bases de la **programmation concurrente** (chapitre 21) seront utiles pour la section 20.3, mais ne sont pas strictement nécessaires pour les sections 20.1 et 20.2.
- Les **variables atomiques** (`std::atomic`, section 21.4) sont fortement recommandées, car elles constituent la manière moderne et sûre de communiquer entre un signal handler et le code principal.

---

## La difficulté particulière des signaux

Les signaux sont souvent sous-estimés parce que les exemples d'introduction sont trompeurs : installer un handler avec `signal(SIGINT, handler)` semble trivial. La réalité est que les signaux sont l'un des mécanismes les plus délicats de la programmation système, pour trois raisons fondamentales :

**L'asynchronisme radical** — Un signal peut arriver à n'importe quel instant, entre n'importe quelles deux instructions machine. Le handler interrompt le flux d'exécution normal et s'exécute dans un contexte où la quasi-totalité des fonctions habituelles (y compris `malloc`, `printf`, `std::println`, et toute opération qui prend un mutex) sont **interdites** car non-réentrantes. La liste des fonctions utilisables dans un handler (les fonctions "async-signal-safe") est très restreinte.

**L'interaction avec les threads** — Dans un programme multi-threadé, un signal peut être délivré à n'importe quel thread (sauf les signaux synchrones comme `SIGSEGV` qui sont délivrés au thread fautif). Cette indétermination complique considérablement la gestion et nécessite des techniques de masquage et de thread dédié.

**Les effets de bord invisibles** — Un signal qui interrompt un appel système (`read`, `write`, `sleep`) peut le faire échouer avec `EINTR` ou le faire se comporter différemment selon les flags de `sigaction`. Ces interactions sont documentées mais rarement évidentes au premier abord.

---

## Organisation du chapitre

| Section | Contenu | Niveau |
|---|---|---|
| **20.1** | Comprendre les signaux Unix — mécanisme, signaux standard, actions par défaut | Avancé |
| **20.2** | Installation de handlers — `signal()` vs `sigaction()`, async-signal-safety, patterns | Avancé |
| **20.3** | Signaux et threads — masquage, `sigwait`, thread dédié, interaction avec C++20 | Expert |

---

## Conventions utilisées dans ce chapitre

Les exemples de ce chapitre utilisent les constantes de signaux définies dans `<csignal>` (interface C++ standard) et `<signal.h>` (interface POSIX). Les deux headers sont interchangeables en pratique sur Linux ; nous utilisons `<csignal>` par défaut pour rester dans l'idiome C++, et `<signal.h>` quand des fonctionnalités spécifiquement POSIX comme `sigaction` sont nécessaires.

Les handlers de signaux sont volontairement minimalistes dans les exemples : c'est une contrainte technique, pas un manque de détail. Un handler complexe est presque toujours un handler incorrect.

---

> **💡 Note** — Les signaux sont un sujet où l'intuition trompe souvent le développeur. Un handler qui "fonctionne" en test peut corrompre silencieusement des données en production, simplement parce que les conditions de timing qui déclenchent le bug sont statistiquement rares. L'approche de ce chapitre est donc délibérément conservatrice : chaque technique présentée est accompagnée des contraintes qui l'encadrent, et les patterns recommandés minimisent systématiquement le travail effectué dans le handler au profit de mécanismes plus sûrs (flags atomiques, self-pipe, `signalfd`, `sigwait`). L'objectif est que vous compreniez non seulement *comment* gérer les signaux, mais surtout *pourquoi* la plupart des approches naïves sont incorrectes.

⏭️ [Comprendre les signaux Unix (SIGINT, SIGTERM, SIGSEGV)](/20-signaux-posix/01-signaux-unix.md)
