🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 36 : Interfaces en Ligne de Commande Modernes 🔥

## Module 12 : Création d'Outils CLI — Niveau Avancé

---

## Introduction

Les outils en ligne de commande constituent l'épine dorsale de l'écosystème Linux et du workflow DevOps. Qu'il s'agisse de `git`, `kubectl`, `docker`, `systemctl` ou `cmake` — les programmes CLI les plus utilisés au quotidien partagent des caractéristiques communes : un parsing d'arguments robuste, une aide auto-générée claire, un formatage de sortie lisible et une architecture extensible par sous-commandes.

Ce chapitre vous donne les clés pour concevoir des outils CLI de qualité professionnelle en C++ moderne. Là où un script Bash ou Python atteint ses limites — performance, typage, distribution sous forme de binaire statique — un outil CLI en C++ compilé devient un choix naturel, en particulier dans les contextes suivants :

- **Outils d'infrastructure** : agents de monitoring, collecteurs de métriques, proxies réseau.  
- **Utilitaires système** : manipulation de fichiers, transformation de données, orchestration de processus.  
- **Tooling interne** : CLI spécifiques à un projet ou une équipe, intégrés dans des pipelines CI/CD.  
- **Outils de diagnostic** : profiling, analyse de logs, inspection de conteneurs.

Historiquement, parser des arguments en C++ impliquait soit d'écrire du code `getopt` verbeux hérité du C, soit de réinventer la roue avec des boucles `if/else` fragiles sur `argv`. L'écosystème moderne a profondément changé la donne : des librairies comme **CLI11**, **argparse** et **fmt** permettent aujourd'hui de rivaliser avec l'ergonomie des frameworks CLI de Go ou Rust, tout en conservant les avantages du C++ — performance native, contrôle mémoire fin et compilation statique.

---

## Objectifs du chapitre

À l'issue de ce chapitre, vous serez capable de :

- Parser des arguments, des flags et des sous-commandes de manière professionnelle avec **CLI11**.  
- Formater des sorties riches et colorées avec **fmt** (et comprendre sa relation avec `std::format` / `std::print` en C++23).  
- Gérer correctement la détection de terminal (TTY) pour adapter le comportement de votre programme (couleurs, pagination, mode interactif vs pipeline).  
- Concevoir l'**architecture** d'un outil CLI maintenable et extensible, en suivant les patterns des outils reconnus de l'industrie.

---

## Pourquoi ne pas simplement utiliser `argc` / `argv` ?

Considérons un outil fictif `deptool` qui gère des dépendances de projet. Un appel typique ressemblerait à :

```bash
deptool install boost --version 1.87 --system-wide --jobs 4  
deptool search "json" --limit 20 --format json  
deptool list --outdated --verbose  
```

Implémenter le parsing de ces commandes manuellement avec `argc`/`argv` nécessite de gérer :

- La distinction entre **sous-commandes** (`install`, `search`, `list`).  
- Les **options longues** (`--version`, `--format`) et **courtes** (`-j`, `-v`).  
- Les **valeurs par défaut** (si `--jobs` n'est pas spécifié, utiliser le nombre de cœurs).  
- La **validation** (le numéro de version doit être valide, `--limit` doit être un entier positif).  
- La **génération d'aide** (`--help` pour chaque sous-commande).  
- Les **messages d'erreur explicites** quand l'utilisateur se trompe.

Écrire tout cela à la main, c'est plusieurs centaines de lignes de code boilerplate, fragile et difficile à maintenir. Avec CLI11, le même résultat s'obtient en quelques dizaines de lignes déclaratives, avec validation, aide automatique et complétion incluses.

---

## Panorama des librairies CLI en C++ (2026)

| Librairie | Style | Header-only | Points forts | Cas d'usage principal |
|-----------|-------|:-----------:|-------------|----------------------|
| **CLI11** | Déclaratif | ✅ | Sous-commandes, validation, génération d'aide, très complet | Outils professionnels complexes |
| **argparse** | Déclaratif (style Python) | ✅ | API simple et familière, prise en main rapide | Outils simples à modérés |
| **cxxopts** | Déclaratif | ✅ | Léger, syntaxe concise | Scripts et petits utilitaires |
| **Boost.Program_options** | Déclaratif | ❌ | Écosystème Boost, très configurable | Projets déjà liés à Boost |
| **getopt / getopt_long** | Impératif (C) | — | Disponible partout, aucune dépendance | Code legacy, contraintes minimales |
| **gflags** | Global flags | ❌ | Flags déclarés où ils sont utilisés | Large codebase (style Google) |

> 💡 **Recommandation 2026** : **CLI11** est le choix par défaut pour tout nouveau projet. Il offre le meilleur rapport fonctionnalités/simplicité, n'a aucune dépendance externe, et s'intègre naturellement avec CMake via `FetchContent`. Pour un script rapide ou un outil minimaliste, **argparse** est une alternative parfaitement viable.

---

## Anatomie d'un outil CLI bien conçu

Les meilleurs outils CLI partagent des conventions que les utilisateurs ont intériorisées. Les respecter rend votre programme immédiatement familier :

### Conventions standard

- **`--help` / `-h`** : affiche l'aide et quitte.  
- **`--version` / `-V`** : affiche la version et quitte.  
- **`--verbose` / `-v`** : augmente la verbosité (empilable : `-vvv`).  
- **`--quiet` / `-q`** : supprime les sorties non essentielles.  
- **`--output` / `-o`** : spécifie un fichier de sortie.  
- **`--format`** : contrôle le format de sortie (`text`, `json`, `table`).  
- **`--` (double tiret)** : sépare les options des arguments positionnels.

### Codes de retour

Votre programme doit retourner des codes de sortie cohérents :

```
0   — Succès
1   — Erreur générale
2   — Mauvaise utilisation (arguments invalides)
64  — Erreur d'usage (convention sysexits.h : EX_USAGE)
```

### Sortie standard vs sortie d'erreur

Une règle simple mais souvent négligée :

- **`stdout`** : les données utiles (résultats, JSON, contenu exploitable par un pipe).  
- **`stderr`** : les messages de diagnostic (progression, warnings, erreurs).

Cette séparation permet le chaînage naturel dans un pipeline :

```bash
deptool list --format json 2>/dev/null | jq '.[] | .name'
```

---

## Formatage de sortie : l'importance de la présentation

Un outil CLI professionnel ne se contente pas de fonctionner correctement — il communique clairement avec l'utilisateur. Cela implique un formatage de sortie soigné, et en 2026, deux approches coexistent :

- **`std::print` / `std::format` (C++23)** : intégrés au standard, suffisants pour du formatage basique type-safe. C'est le choix naturel si vous n'avez pas besoin de couleurs ni de formatage avancé.  
- **`fmt`** : la librairie dont `std::format` est issu. Elle reste pertinente pour ses fonctionnalités supplémentaires — notamment le support des couleurs et des styles terminaux, les performances de compilation, et certaines fonctionnalités non encore standardisées.

> 💡 Nous avons couvert `std::print` en **section 2.7** (prise en main) et **section 12.7** (formatage approfondi). La **section 36.3** se concentre sur les capacités spécifiques de `fmt` qui vont au-delà du standard : couleurs, styles et intégration dans un contexte CLI.

---

## Gestion du terminal : TTY ou pipe ?

Un point souvent négligé par les développeurs débutants en CLI : votre programme doit se comporter différemment selon qu'il écrit dans un terminal interactif ou dans un pipe/fichier. La fonction POSIX `isatty()` permet cette détection :

```cpp
#include <unistd.h>

bool is_interactive = isatty(STDOUT_FILENO);  // true si stdout est un terminal
```

Quand `stdout` est un terminal, vous pouvez afficher des couleurs, des barres de progression et du texte formaté. Quand la sortie est redirigée vers un pipe ou un fichier, ces éléments doivent être désactivés automatiquement — sous peine de polluer la sortie avec des codes ANSI illisibles.

Ce comportement adaptatif est un marqueur de qualité professionnelle. Nous le détaillerons en **section 36.4**.

---

## Organisation du chapitre

Ce chapitre est structuré en cinq sections progressives :

| Section | Sujet | Ce que vous apprendrez |
|---------|-------|----------------------|
| **36.1** | CLI11 | Parsing d'arguments professionnel : options, flags, sous-commandes, validation |
| **36.2** | argparse | Alternative légère pour les outils plus simples |
| **36.3** | fmt | Formatage avancé, couleurs et styles pour les sorties CLI |
| **36.4** | Couleurs et TTY | Détection de terminal, codes ANSI, adaptation automatique |
| **36.5** | Architecture CLI | Conception d'un outil maintenable et extensible à la `kubectl` / `git` |

Chaque section est conçue pour être autonome, mais elles se complètent naturellement : un outil CLI complet combine le parsing de CLI11 (36.1), le formatage de fmt (36.3), la gestion intelligente du terminal (36.4), le tout structuré selon une architecture propre (36.5).

---

> 🔥 **Ce chapitre est un différenciateur professionnel.** La capacité à livrer des outils CLI solides, ergonomiques et bien architecturés est une compétence très recherchée dans les équipes DevOps et infrastructure. C'est aussi un excellent terrain de pratique pour appliquer les concepts vus dans les modules précédents — smart pointers, gestion d'erreurs avec `std::expected`, sérialisation JSON/YAML, et programmation système Linux.

⏭️ [CLI11 : Parsing d'arguments professionnel](/36-interfaces-cli/01-cli11.md)
