🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 24.6 — Expressions régulières en C++ : `std::regex` et alternatives performantes ⭐

> **Niveau** : Avancé  
> **Prérequis** : Chapitre 12 (Nouveautés C++17/C++20/C++23), chapitre 16 (Templates et métaprogrammation), notions de parsing (sections 24.1 à 24.5)  
> **Fichiers associés** : `24-serialisation-config/06-regex.md`

---

## Pourquoi une section dédiée aux expressions régulières ?

Les expressions régulières (regex) sont un outil incontournable pour toute personne travaillant avec du texte structuré ou semi-structuré. Que ce soit pour valider un format d'entrée, extraire des champs dans un fichier de log, parser une réponse HTTP ou nettoyer des données avant sérialisation, les regex interviennent à tous les niveaux du développement système et DevOps.

En C++, le sujet mérite une attention particulière parce que **l'implémentation standard (`std::regex`) est notoirement problématique en termes de performance**. C'est l'une des rares parties de la bibliothèque standard où la recommandation communautaire quasi unanime est : *évaluez sérieusement les alternatives avant de l'utiliser en production*.

Cette section couvre l'ensemble du paysage des regex en C++ moderne, de l'API standard aux alternatives compilées à la compilation ou optimisées pour le runtime.

---

## Le paysage des regex en C++ en 2026

Le développeur C++ dispose aujourd'hui de quatre grandes familles de solutions pour travailler avec les expressions régulières :

**`std::regex` (C++11)** — L'API standard, disponible partout, mais dont les performances sont souvent un ordre de grandeur en dessous des alternatives. Elle reste pertinente pour du prototypage rapide ou des cas où la vitesse d'exécution n'est pas un critère.

**CTRE — Compile-Time Regular Expressions (C++20)** — Une bibliothèque header-only qui évalue les expressions régulières à la compilation grâce aux templates et `constexpr`. Le résultat est du code machine spécialisé pour chaque pattern, avec des performances comparables à du parsing manuel.

**RE2 (Google)** — Un moteur runtime conçu pour garantir une exécution en temps linéaire par rapport à la taille de l'entrée, quels que soient le pattern et le texte. C'est le choix de référence quand les patterns sont dynamiques (fournis par l'utilisateur, lus depuis un fichier de configuration) et que la sécurité contre le *catastrophic backtracking* est requise.

**PCRE2** — L'implémentation de référence des expressions régulières compatibles Perl. Elle offre le jeu de fonctionnalités le plus riche (lookahead, lookbehind, backreferences, récursion) avec d'excellentes performances runtime grâce à son compilateur JIT.

Le tableau suivant résume les compromis fondamentaux :

| Critère | `std::regex` | CTRE | RE2 | PCRE2 |
|---|---|---|---|---|
| **Disponibilité** | Standard C++11 | Header-only (C++20) | Bibliothèque externe | Bibliothèque externe |
| **Pattern connu à la compilation** | Non exploité | ✅ Requis | Non requis | Non requis |
| **Pattern dynamique** | ✅ | ❌ | ✅ | ✅ |
| **Performance** | Faible | Excellente | Très bonne | Très bonne (JIT) |
| **Garantie temps linéaire** | ❌ | ✅ | ✅ | ❌ (backtracking) |
| **Fonctionnalités avancées** | ECMAScript subset | Sous-ensemble | Sous-ensemble (pas de backrefs) | Complet (Perl-compatible) |
| **Sécurité ReDoS** | ❌ Vulnérable | ✅ Immune | ✅ Immune | ❌ Vulnérable (atténuable) |

---

## Quand utiliser quoi : arbre de décision

Le choix de la bonne solution dépend de trois questions clés :

**1. Le pattern est-il connu à la compilation ?**
Si oui, CTRE est presque toujours le meilleur choix. Le compilateur génère du code machine optimisé spécifique au pattern, sans aucun coût runtime d'initialisation. C'est la solution idiomatique en C++ moderne quand le pattern est une constante du programme.

**2. Le pattern est-il fourni par l'utilisateur ou chargé dynamiquement ?**
Si oui, CTRE n'est pas applicable. Il faut choisir entre RE2 et PCRE2. RE2 garantit un temps d'exécution linéaire et protège contre les attaques ReDoS (*Regular Expression Denial of Service*), ce qui le rend préférable pour tout contexte exposé à des entrées non fiables (serveurs web, API, outils CLI acceptant des regex utilisateur). PCRE2 est préférable quand les fonctionnalités avancées (backreferences, lookaround, récursion) sont nécessaires et que les entrées sont contrôlées.

**3. La performance est-elle critique ?**
Si la réponse est non — par exemple un script de configuration lancé une fois, un outil de migration exécuté occasionnellement — `std::regex` peut suffire. Sa présence dans la bibliothèque standard élimine toute dépendance externe, ce qui a une valeur réelle dans certains contextes contraints.

---

## Le problème de performance de `std::regex` : contexte

Pour comprendre pourquoi les alternatives existent, il est utile de situer le problème. L'implémentation de `std::regex` dans les trois grandes bibliothèques standard (libstdc++, libc++, MSVC STL) repose historiquement sur un moteur à backtracking non optimisé. Cela signifie que pour certains patterns combinés à certaines entrées, le temps d'exécution peut croître de manière exponentielle.

Un exemple classique : matcher le pattern `(a+)+b` contre une chaîne composée uniquement de `a` (sans `b` final). Avec un moteur à backtracking naïf, chaque caractère supplémentaire double le nombre de chemins à explorer. Sur une entrée de 30 caractères, `std::regex` peut mettre plusieurs secondes là où RE2 ou CTRE répondent en quelques microsecondes.

Au-delà du backtracking catastrophique, `std::regex` souffre aussi d'un coût élevé de compilation du pattern à l'exécution et d'allocations mémoire fréquentes. Même sur des cas "normaux" sans explosion combinatoire, les benchmarks montrent régulièrement un facteur 10 à 50 de différence par rapport aux alternatives.

Ce constat n'est pas nouveau — il est documenté depuis l'introduction de `std::regex` en C++11. Malgré des améliorations incrémentales dans les implémentations, la situation en 2026 reste fondamentalement la même : `std::regex` n'est pas compétitif en performance.

---

## Vocabulaire et concepts communs

Avant de plonger dans chaque solution, voici les termes et concepts qui reviennent dans toutes les sous-sections :

**Moteur NFA vs DFA** — Les deux grandes familles d'implémentation. Un *NFA* (Nondeterministic Finite Automaton) explore les correspondances possibles par backtracking, ce qui permet les backreferences mais expose au risque d'explosion combinatoire. Un *DFA* (Deterministic Finite Automaton) construit un automate déterministe qui traite chaque caractère d'entrée exactement une fois, garantissant un temps linéaire mais interdisant certaines fonctionnalités.

**Backtracking catastrophique (ReDoS)** — Situation où un moteur NFA explore un nombre exponentiel de chemins pour un pattern ambigu sur une entrée non correspondante. C'est un vecteur d'attaque par déni de service quand les patterns ou les entrées proviennent de sources non fiables.

**Compilation du pattern** — La transformation d'une chaîne représentant une regex en une structure interne exploitable par le moteur. Cette étape peut être coûteuse (allocations, construction d'automate). En C++ standard, elle se fait à l'exécution via le constructeur de `std::regex`. Avec CTRE, elle se fait à la compilation.

**Groupes de capture** — Sous-expressions délimitées par des parenthèses qui permettent d'extraire des portions du texte correspondant. Toutes les solutions couvertes supportent les groupes de capture, mais avec des API et des performances différentes.

**Assertions (lookahead, lookbehind)** — Contraintes qui vérifient une condition à une position dans le texte sans consommer de caractères. Elles ne sont pas supportées par tous les moteurs (RE2 ne supporte pas le lookbehind, par exemple).

---

## Plan de la section

Les sous-sections suivantes détaillent chaque solution avec des exemples de code, des benchmarks comparatifs et des recommandations d'intégration dans un projet CMake :

- **24.6.1** — `std::regex` : API standard et limites de performance  
- **24.6.2** — CTRE : Compile-Time Regular Expressions (C++20) 🔥  
- **24.6.3** — RE2 et PCRE2 : Alternatives runtime performantes  
- **24.6.4** — Benchmarks et guide de choix

---

> 💡 **Convention utilisée dans cette section** : tous les exemples de code sont compilés avec GCC 15 ou Clang 20 en mode `-std=c++23` sauf mention contraire. Les benchmarks sont réalisés avec Google Benchmark (chapitre 35) sur un processeur AMD Zen 4, Ubuntu 24.04 LTS.

⏭️ [std::regex : API standard et limites de performance](/24-serialisation-config/06.1-std-regex.md)
