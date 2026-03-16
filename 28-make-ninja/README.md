🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 28. Makefile, Ninja et Build Automation

> **Niveau** : Avancé  
> **Partie** : IV — Tooling et Build Systems  
> **Module** : 9 — Build Systems et Gestion de Projet  
> **Durée estimée** : 4–5 heures  
> **Prérequis** : Chapitre 26 (CMake), en particulier les sections 26.5 (Ninja) et 26.1 (structure de projet)

---

## Objectifs du chapitre

À l'issue de ce chapitre, vous serez capable de :

- lire et comprendre un Makefile — sa syntaxe, ses variables, ses règles et ses patterns ;
- comprendre pourquoi Ninja est plus rapide que Make et comment fonctionne son format `build.ninja` ;
- situer Meson dans l'écosystème des build systems C++ et comprendre ses avantages ;
- choisir le bon outil selon le contexte : CMake + Ninja, Makefiles écrits à la main, ou Meson.

---

## Pourquoi ce chapitre après CMake ?

Le chapitre 26 a établi CMake comme le **méta-build system** standard de l'industrie C++. CMake génère les instructions de build pour un build system sous-jacent — et ce build system, dans la grande majorité des cas en 2026, est soit **Make** soit **Ninja**. Nous avons recommandé Ninja (section 26.5) pour ses performances supérieures, mais nous n'avons pas exploré le fonctionnement interne de ces outils, ni les cas où un Makefile écrit à la main ou un build system alternatif comme Meson est plus approprié.

Ce chapitre complète la compréhension du pipeline de build en descendant d'un niveau d'abstraction : de CMake (qui décrit *quoi* construire) aux build systems (qui décident *comment* et *dans quel ordre* construire).

---

## Les trois niveaux du pipeline de build

Pour situer ce chapitre dans le contexte global, voici les trois niveaux du pipeline de build C++ :

```
Niveau 3 : Méta-build system (CMake, Meson)
  │  Décrit le projet : cibles, dépendances, options
  │  Génère les instructions pour le niveau 2
  ▼
Niveau 2 : Build system (Make, Ninja)
  │  Exécute les instructions : compilation, linkage
  │  Gère la parallélisation et la détection de changements
  ▼
Niveau 1 : Outils de compilation (GCC, Clang, ld, ar)
  │  Compilent, assemblent et lient le code
  ▼
Binaires finaux
```

Les chapitres 26-27 ont couvert le niveau 3 (CMake) et son interaction avec le niveau 2. Ce chapitre couvre le **niveau 2** — les build systems eux-mêmes — et le cas particulier où le niveau 3 est absent (Makefiles écrits à la main) ou incarné par un outil différent de CMake (Meson).

---

## Make : l'ancêtre omniprésent

GNU Make est le build system le plus ancien et le plus répandu du monde Unix. Créé en 1976 (la version GNU date de 1988), il est installé par défaut sur pratiquement tout système Linux et fait partie de l'outillage de base de tout développeur C/C++.

Make lit un fichier `Makefile` qui décrit des **règles** : chaque règle associe une **cible** (un fichier à produire) à des **prérequis** (les fichiers dont elle dépend) et à une **recette** (les commandes à exécuter pour la produire). Make détermine quelles cibles doivent être reconstruites en comparant les timestamps des fichiers.

Même si vous utilisez CMake au quotidien (ce qui est recommandé), comprendre la syntaxe Make est précieux pour plusieurs raisons. De nombreux projets open source historiques utilisent encore des Makefiles écrits à la main. Les Makefiles générés par CMake (`cmake -G "Unix Makefiles"`) suivent la même syntaxe — savoir les lire aide au diagnostic. Et au-delà du C++, Make est utilisé comme outil d'automatisation généraliste (documentation, déploiement, tâches DevOps) dans de nombreux projets.

---

## Ninja : le spécialiste de la vitesse

Ninja, couvert en section 26.5 pour son intégration avec CMake, est un build system conçu pour être **généré par un outil de plus haut niveau**, pas écrit à la main. Son format `build.ninja` est volontairement minimaliste et optimisé pour le parsing rapide — pas pour la lisibilité humaine.

Ce chapitre explore le fonctionnement interne de Ninja : comment il représente le graphe de build, pourquoi son format est plus rapide à charger que les Makefiles, et comment lire un fichier `build.ninja` quand vous avez besoin de diagnostiquer un problème de build. L'objectif n'est pas d'écrire des fichiers Ninja à la main (personne ne fait ça), mais de comprendre ce que CMake génère pour vous.

---

## Meson : le challenger moderne

Meson est un build system apparu en 2013, qui s'est imposé comme l'alternative la plus crédible à CMake pour les projets C/C++ et au-delà. Sa philosophie est radicalement différente : une syntaxe déclarative **non Turing-complète** (on ne peut pas écrire de boucles arbitraires ni de logique complexe), conçue pour être simple à lire, rapide à parser, et difficile à utiliser de manière incorrecte.

Meson génère exclusivement des fichiers Ninja — il n'a pas de backend Make, ce qui simplifie son architecture et garantit des builds rapides. Il est utilisé par des projets majeurs de l'écosystème Linux : GNOME (GTK, GLib), systemd, Mesa (pilotes GPU), PipeWire, et de nombreux autres. Il n'a pas atteint la masse critique de CMake en termes d'adoption globale et de support IDE, mais il est en croissance régulière et représente une alternative viable pour les projets qui ne sont pas contraints par l'écosystème CMake.

---

## Ce que ce chapitre ne couvre pas

Ce chapitre ne traite pas de Bazel (Google), Buck2 (Meta), ni des build systems orientés monorepo à très grande échelle. Ces outils résolvent des problèmes spécifiques aux dépôts contenant des millions de lignes de code et des milliers de cibles, avec des caches distribués et une exécution distante. Ils sont hors du périmètre de cette formation, mais valent la peine d'être étudiés si vous travaillez dans un contexte de monorepo d'entreprise.

---

## Plan du chapitre

| Section | Thème | Ce que vous apprendrez |
|---------|-------|----------------------|
| **28.1** | Syntaxe de base des Makefiles | Règles, cibles, prérequis, recettes, le fonctionnement fondamental de Make |
| **28.2** | Variables, règles et patterns | Variables automatiques (`$@`, `$<`, `$^`), règles de pattern (%.o: %.cpp), fonctions |
| **28.3** | Ninja : Build system ultra-rapide ⭐ | Pourquoi Ninja est plus rapide, format `build.ninja`, pools de jobs |
| **28.4** | Meson : Build system montant | Syntaxe déclarative, philosophie, projets notables, comparaison avec CMake |
| **28.5** | Comparaison Make vs Ninja vs Meson | Performances, cas d'usage, guide de décision |

Le chapitre suit une progression logique : Make d'abord (la base historique qu'il faut connaître), puis Ninja (le build system que vous utilisez au quotidien via CMake), et enfin Meson (l'alternative à CMake, pas à Ninja — une distinction importante).

---

## Prérequis techniques

```bash
# GNU Make (généralement pré-installé)
make --version
# GNU Make 4.3+

# Ninja (installé au chapitre 26)
ninja --version
# 1.11+

# Meson (pour la section 28.4)
sudo apt install meson  
meson --version  
# 1.3+
```

Si Meson n'est pas disponible via `apt` dans une version suffisamment récente, il s'installe aussi via pip :

```bash
pip3 install --user meson
```

---

## Une clarification terminologique

Les termes « build system » et « build tool » sont souvent utilisés de manière interchangeable, mais une distinction aide à la compréhension :

- Un **build tool** (Make, Ninja) exécute des commandes pour produire des fichiers à partir d'autres fichiers. Il gère la parallélisation et la détection de changements. Il ne connaît pas le C++ — il exécute des commandes arbitraires.

- Un **build system** (CMake, Meson) comprend les concepts du langage C++ (compilateurs, bibliothèques, standards, plateformes) et génère les instructions pour un build tool. Il abstrait les détails spécifiques au compilateur et à la plateforme.

Make est souvent utilisé comme build tool *et* comme build system artisanal (quand le Makefile est écrit à la main avec la logique de compilation). Ninja est exclusivement un build tool — il est toujours piloté par un build system de plus haut niveau. Meson est un build system qui génère pour Ninja. CMake est un build system (méta-build system) qui peut générer pour Make, Ninja, ou d'autres.

Ce chapitre couvre les **build tools** (Make, Ninja) et un **build system alternatif** (Meson).

---

> **À suivre** : La section 28.1 couvre la syntaxe de base des Makefiles — règles, cibles, prérequis et recettes — les briques fondamentales pour comprendre comment Make décide quoi compiler et dans quel ordre.

⏭️ [Syntaxe de base des Makefiles](/28-make-ninja/01-syntaxe-makefiles.md)
