🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Module 12 — Création d'Outils CLI

> 🎯 Niveau : Avancé

Ce module ne contient qu'un seul chapitre, mais c'est un chapitre à forte valeur professionnelle. La capacité à construire des outils en ligne de commande de qualité production — parsing d'arguments propre, sous-commandes, aide auto-générée, sortie formatée, détection TTY, codes de retour corrects — est ce qui distingue un binaire C++ jeté dans un répertoire d'un outil que les équipes adoptent. Dans un contexte DevOps/SRE, les outils CLI sont le livrable le plus courant en C++, et ils sont jugés sur la même exigence d'ergonomie que `kubectl`, `git` ou `docker`.

---

## Objectifs pédagogiques

1. **Implémenter** le parsing d'arguments avec CLI11 : options, flags, sous-commandes, validation, callbacks, génération d'aide automatique.
2. **Utiliser** `fmt` pour le formatage avancé de la sortie (syntaxe Python-like, couleurs, styles), et comprendre sa relation avec `std::print`/`std::format` (C++23).
3. **Gérer** la détection TTY pour adapter la sortie (couleurs ANSI en terminal interactif, texte brut en pipe ou redirection).
4. **Concevoir** l'architecture d'un outil CLI professionnel : structure en sous-commandes, séparation parsing/logique, codes de retour Unix, gestion des erreurs, documentation intégrée.
5. **Appliquer** les conventions Unix : exit codes (`0` = succès, `1` = erreur générale, `2` = usage incorrect), `--help`, `--version`, sortie structurée.

---

## Prérequis

- **Module 4, chapitre 12, section 12.7** : `std::print` et `std::format` (C++23) — `fmt` est la librairie originale dont `std::format` est dérivé. Comprendre l'un facilite l'utilisation de l'autre.
- **Module 6** : gestion d'erreurs — un outil CLI doit gérer les erreurs proprement (fichier introuvable, argument invalide, timeout réseau) et les traduire en messages lisibles et codes de retour appropriés.
- **Module 7, chapitre 20** : signaux POSIX — un outil CLI doit gérer `SIGINT` (Ctrl+C) et `SIGPIPE` correctement.
- **Module 8, chapitre 24** : parsing JSON/YAML — les outils CLI lisent souvent des fichiers de configuration dans ces formats.

---

## Chapitres

### Chapitre 36 — Interfaces en Ligne de Commande Modernes

De la librairie de parsing au design complet d'un outil CLI professionnel. Ce chapitre couvre CLI11 comme librairie principale, argparse comme alternative, `fmt` pour la sortie formatée, la gestion des couleurs et du TTY, et l'architecture d'ensemble.

- **CLI11** : librairie header-only de parsing d'arguments. Installation (header-only ou via Conan/vcpkg), définition d'options (`app.add_option("-n,--name", name, "Description")`), flags booléens, sous-commandes (`app.add_subcommand("init", "Initialize project")`), validation des entrées (`CLI::ExistingFile`, `CLI::Range`, validators personnalisés), callbacks sur les options, génération automatique de `--help` avec description formatée.
- **argparse** : alternative légère à CLI11, API plus simple, adaptée aux outils avec peu d'options et sans sous-commandes.
- **`fmt`** : librairie de formatage dont `std::format` (C++20/C++23) est issu. Syntaxe Python-like (`fmt::print("Hello, {}!", name)`), formatage numérique, alignement, padding. Couleurs et styles via `fmt::color` — texte coloré, gras, souligné pour les messages d'erreur, les warnings, et la mise en évidence.
- **Gestion des couleurs et du TTY** : détection de terminal interactif (`isatty(STDOUT_FILENO)`), désactivation des escape codes ANSI quand la sortie est redirigée vers un fichier ou pipée vers un autre programme. Respect de la variable d'environnement `NO_COLOR` (convention https://no-color.org/).
- **Architecture d'un outil CLI professionnel** : structure en sous-commandes (à la `git commit`, `kubectl apply`), séparation du parsing (couche CLI) et de la logique métier (couche library), gestion des codes de retour, `--help` et `--version` cohérents, sortie machine-readable optionnelle (`--output json`), documentation et exemples d'utilisation.

---

## Points de vigilance

- **Codes de retour non conformes aux conventions Unix.** Un outil CLI qui retourne `0` en cas d'erreur ou un code arbitraire perd la composabilité avec le shell (`set -e`, `&&`, `||`, `$?`). Convention : `0` = succès, `1` = erreur générale, `2` = usage incorrect (mauvais arguments). Certains outils utilisent des codes spécifiques au-delà de `2` pour distinguer les types d'erreur (comme `grep` qui retourne `1` si aucune ligne ne matche). Définissez vos codes de retour, documentez-les dans `--help`, et ne retournez jamais `0` quand quelque chose a échoué.

- **TTY detection absente — ANSI escape codes dans un pipe.** Envoyer des escape codes ANSI (`\033[31m` pour le rouge) quand la sortie est pipée vers `less`, `grep`, ou un fichier produit des caractères parasites illisibles. Vérifiez `isatty(STDOUT_FILENO)` avant d'activer les couleurs, et respectez `NO_COLOR` et `FORCE_COLOR` comme variables d'environnement. CLI11 et `fmt` ne font pas cette vérification pour vous — c'est à votre code de la faire.

- **`SIGPIPE` non géré dans les outils qui écrivent sur stdout.** Quand un outil CLI est pipé vers `head` ou `less` et que le consommateur ferme le pipe, l'écriture suivante déclenche `SIGPIPE` qui tue le programme avec un code de retour `141`. C'est un comportement normal pour les outils simples, mais gênant si votre outil doit faire du cleanup (fermer des fichiers, libérer des locks). Solution : ignorer `SIGPIPE` (`signal(SIGPIPE, SIG_IGN)`) et gérer l'erreur `EPIPE` retournée par `write`/`std::cout`. Ou accepter le comportement par défaut si le cleanup n'est pas nécessaire — mais faites-le consciemment.

- **`--help` et documentation incohérents.** Un outil dont le `--help` ne correspond pas au comportement réel, ou qui n'a pas de `--help` du tout, perd la confiance de ses utilisateurs immédiatement. CLI11 génère le `--help` automatiquement à partir des descriptions passées à `add_option` et `add_subcommand` — profitez-en pour écrire des descriptions précises dès la définition des options. Si vous maintenez une man page ou un README séparé, vérifiez la cohérence avec `--help` à chaque release.

---

## Compétences acquises

À l'issue de ce module, vous savez :
- Construire un outil CLI complet avec CLI11 : options, flags, sous-commandes, validation, aide auto-générée.
- Formater la sortie avec `fmt` (couleurs, styles) en respectant la détection TTY et `NO_COLOR`.
- Structurer un outil CLI professionnel avec séparation parsing/logique, codes de retour Unix corrects, et sortie machine-readable optionnelle.
- Gérer `SIGPIPE` et les edge cases de la sortie pipée.
- Produire un outil que d'autres développeurs et opérateurs peuvent adopter sans friction.

---


⏭️ [Interfaces en Ligne de Commande Modernes](/36-interfaces-cli/README.md)
