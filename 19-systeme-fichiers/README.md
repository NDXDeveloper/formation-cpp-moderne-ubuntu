🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 19 : Manipulation du Système de Fichiers

## Module 7 — Programmation Système sur Linux *(Niveau Avancé)*

---

## Introduction

La manipulation du système de fichiers est l'une des opérations les plus fréquentes en programmation système. Qu'il s'agisse de lire un fichier de configuration, de parcourir une arborescence de logs, de surveiller un répertoire de déploiement ou de gérer des fichiers temporaires dans un pipeline CI/CD, un développeur C++ sur Linux interagit constamment avec le filesystem.

Historiquement, le C++ ne proposait aucune abstraction portable pour ces opérations. Les développeurs devaient se tourner vers les appels système POSIX (`open`, `read`, `write`, `stat`, `opendir`…) ou vers des wrappers comme Boost.Filesystem. Ce manque a été comblé avec l'arrivée de **`std::filesystem`** dans le standard **C++17**, qui offre une API moderne, portable et type-safe pour interagir avec le système de fichiers.

Cependant, comprendre uniquement l'API haut niveau ne suffit pas. Sur Linux, `std::filesystem` repose sur les appels système POSIX sous-jacents, et certaines situations exigent de descendre à ce niveau : contrôle fin des descripteurs de fichiers, opérations non-bloquantes, manipulation directe des permissions Unix, ou encore interopérabilité avec du code C existant. Un développeur système compétent doit maîtriser les deux couches et savoir quand utiliser l'une plutôt que l'autre.

---

## Pourquoi ce chapitre est essentiel

### Pour le développeur système

Le filesystem est le point d'entrée vers la plupart des ressources sur Linux. En vertu de la philosophie Unix **"Everything is a file"**, manipuler des fichiers signifie aussi interagir avec des périphériques (`/dev`), des informations noyau (`/proc`, `/sys`), des sockets Unix et bien d'autres abstractions exposées via le VFS (Virtual File System). Comprendre comment le système de fichiers fonctionne à bas niveau est un prérequis pour écrire du code robuste sur Linux.

### Pour le DevOps et le Cloud Native

Les outils CLI, les agents de monitoring, les processus de build et les conteneurs Docker manipulent tous intensivement le filesystem : lecture de configurations YAML/JSON, écriture de logs structurés, gestion de fichiers temporaires, surveillance de répertoires, création de paquets de distribution. Ce chapitre fournit les bases techniques pour construire ces outils avec fiabilité.

---

## Ce que vous allez apprendre

Ce chapitre se structure autour de trois axes complémentaires :

**`std::filesystem` (C++17)** — L'API moderne et portable de la bibliothèque standard. Vous apprendrez à parcourir des répertoires, manipuler des chemins, effectuer des opérations sur les fichiers et répertoires (copie, déplacement, suppression, création), le tout avec une gestion d'erreurs propre via `std::error_code`. C'est l'approche à privilégier dans tout nouveau projet C++ moderne.

**Les appels système POSIX** — L'interface bas niveau du noyau Linux : `open`, `read`, `write`, `close`, `stat`, `lseek`. Ces fonctions offrent un contrôle total sur les descripteurs de fichiers et sont incontournables lorsque vous avez besoin de performances maximales, d'opérations non-bloquantes, ou d'accès à des fonctionnalités spécifiques à Linux comme `epoll` ou `inotify`.

**La comparaison et l'arbitrage** — Savoir choisir entre l'API C++ et l'API système selon le contexte. Ce n'est pas un choix binaire : dans un même projet, il est courant d'utiliser `std::filesystem` pour la gestion générale de l'arborescence et les appels POSIX pour les opérations critiques en performance ou spécifiques à Linux.

---

## Prérequis

Avant d'aborder ce chapitre, vous devez être à l'aise avec :

- Les **références et pointeurs** (chapitre 4), notamment le passage par référence constante, car les fonctions filesystem travaillent souvent avec des `const std::filesystem::path&`.
- La **gestion de la mémoire** (chapitre 5), en particulier la distinction stack/heap, car les appels POSIX manipulent des buffers bruts.
- Le principe **RAII** (section 6.3), qui est au cœur de la gestion sûre des descripteurs de fichiers : tout descripteur ouvert doit être fermé, et RAII garantit cela même en cas d'exception.
- Les **exceptions et `std::error_code`** (chapitre 17), car `std::filesystem` propose systématiquement deux variantes pour chaque opération : une qui lance une exception et une qui retourne un code d'erreur.
- Les bases de **Linux et du terminal** (chapitre 1), notamment la notion de chemins absolus/relatifs, de permissions Unix et de structure de l'arborescence.

---

## Environnement et compilation

`std::filesystem` est disponible nativement avec **GCC 8+** et **Clang 7+**. Sur les versions plus anciennes de GCC (8 et 9), il était nécessaire de lier explicitement la bibliothèque avec `-lstdc++fs`. Depuis **GCC 10**, ce linkage est automatique.

Avec les compilateurs couverts par cette formation (**GCC 15** et **Clang 20**), aucune configuration particulière n'est requise : il suffit d'inclure le header `<filesystem>` et de compiler en C++17 ou supérieur.

```bash
# Compilation standard — aucun flag supplémentaire nécessaire
g++ -std=c++20 -Wall -Wextra -o mon_programme mon_programme.cpp

# Avec CMake (recommandé)
# std::filesystem est automatiquement disponible via le standard C++17+
```

Pour les appels système POSIX, les headers nécessaires (`<fcntl.h>`, `<unistd.h>`, `<sys/stat.h>`) sont disponibles sur tout système Linux sans dépendance supplémentaire.

---

## Organisation du chapitre

| Section | Contenu | Niveau |
|---|---|---|
| **19.1** | `std::filesystem` — API moderne C++17 | Intermédiaire |
| **19.2** | Appels système POSIX — `open`, `read`, `write`, `close` | Avancé |
| **19.3** | Comparaison API C++ vs API système | Avancé |
| **19.4** | Permissions, droits et gestion des erreurs | Avancé |

---

## Conventions utilisées dans ce chapitre

Les exemples de code utilisent systématiquement l'alias de namespace suivant pour améliorer la lisibilité :

```cpp
namespace fs = std::filesystem;
```

Les exemples POSIX incluent toujours la gestion des erreurs via `errno` et `perror`, car ignorer les codes de retour des appels système est une source majeure de bugs en programmation système.

Lorsqu'un exemple manipule des chemins, ceux-ci suivent les conventions Linux (séparateur `/`). La portabilité Windows n'est pas abordée dans ce chapitre, conformément au périmètre de la formation centrée sur Ubuntu.

---

> **💡 Note** — Ce chapitre marque l'entrée dans la programmation système à proprement parler. Contrairement aux chapitres précédents qui travaillaient principalement en espace utilisateur pur, vous allez ici interagir directement avec le noyau Linux via ses appels système. Cette interaction implique une rigueur supplémentaire : chaque ressource ouverte doit être fermée, chaque code de retour doit être vérifié, et chaque buffer doit être correctement dimensionné. Les sanitizers présentés au chapitre 29 (notamment AddressSanitizer) sont vos alliés pour détecter les erreurs à ce niveau.


⏭️ [std::filesystem (C++17) : API moderne](/19-systeme-fichiers/01-std-filesystem.md)
