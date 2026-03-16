🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 28.2 Variables, règles et patterns

> **Objectif** : Approfondir les mécanismes avancés de Make — fonctions de manipulation de texte, variables d'environnement, règles spéciales, techniques d'organisation — pour comprendre les Makefiles idiomatiques que vous rencontrerez dans les projets open source et les builds générés par CMake.

---

## Fonctions de manipulation de texte

Make embarque un ensemble de **fonctions** qui opèrent sur des chaînes de caractères et des listes de mots. Ces fonctions sont au cœur de tout Makefile non trivial.

### Substitution et transformation

```makefile
SRCS := main.cpp utils.cpp network.cpp

# Substitution de suffixe — deux syntaxes équivalentes
OBJS := $(SRCS:.cpp=.o)                          # main.o utils.o network.o  
OBJS := $(patsubst %.cpp,%.o,$(SRCS))            # Identique, plus explicite  

# Ajouter un préfixe à chaque mot
FULL_SRCS := $(addprefix src/,$(SRCS))            # src/main.cpp src/utils.cpp src/network.cpp

# Ajouter un suffixe
HEADERS := $(addsuffix .h,main utils network)     # main.h utils.h network.h

# Extraire le répertoire / le nom de fichier
DIR  := $(dir src/main.cpp)                       # src/  
FILE := $(notdir src/main.cpp)                    # main.cpp  
BASE := $(basename src/main.cpp)                  # src/main  
EXT  := $(suffix src/main.cpp)                    # .cpp  
```

`$(patsubst pattern,replacement,text)` est la fonction de substitution la plus flexible. Le `%` capture une portion arbitraire et la réinsère dans le remplacement :

```makefile
# Transformer les chemins sources en chemins objets dans un autre répertoire
OBJS := $(patsubst src/%.cpp,build/%.o,$(SRCS))
# src/main.cpp → build/main.o
# src/utils.cpp → build/utils.o
```

### Filtrage

```makefile
FILES := main.cpp utils.cpp readme.md config.h Makefile

# Garder uniquement les .cpp
CPP_FILES := $(filter %.cpp,$(FILES))             # main.cpp utils.cpp

# Exclure les .cpp
NON_CPP   := $(filter-out %.cpp,$(FILES))         # readme.md config.h Makefile
```

`$(filter ...)` et `$(filter-out ...)` sont essentiels pour séparer des listes mixtes — par exemple, extraire les sources C des sources C++ dans un projet multi-langage :

```makefile
ALL_SRCS := main.cpp driver.c utils.cpp wrapper.c  
CXX_SRCS := $(filter %.cpp,$(ALL_SRCS))  
C_SRCS   := $(filter %.c,$(ALL_SRCS))  
```

### Tri et déduplication

```makefile
LIBS := -lssl -lcrypto -lz -lssl  
UNIQUE_LIBS := $(sort $(LIBS))                    # -lcrypto -lssl -lz  
# sort trie ET déduplique
```

### Collecte de fichiers : `wildcard`

```makefile
# Tous les .cpp dans src/ (non récursif)
SRCS := $(wildcard src/*.cpp)

# Récursif — nécessite la fonction shell
SRCS := $(shell find src/ -name '*.cpp')
```

`$(wildcard ...)` est évalué par Make et ne descend pas dans les sous-répertoires. Pour un parcours récursif, `$(shell find ...)` est nécessaire mais introduit une dépendance au shell Unix.

---

## La fonction `$(shell ...)`

`$(shell ...)` exécute une commande shell et capture sa sortie standard. C'est le pont entre Make et le système :

```makefile
# Détecter le nombre de cœurs CPU
NPROCS := $(shell nproc)

# Date de build
BUILD_DATE := $(shell date -u +"%Y-%m-%d %H:%M UTC")

# Hash Git
GIT_HASH := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")

# Détecter si un programme est installé
CCACHE := $(shell command -v ccache 2>/dev/null)

# Compiler avec ccache si disponible
ifdef CCACHE
    CXX := ccache $(CXX)
endif
```

`$(shell ...)` est évalué **une seule fois** au moment du parsing du Makefile (pas à chaque utilisation de la variable), à condition d'utiliser l'affectation immédiate `:=`. Avec l'affectation récursive `=`, la commande serait exécutée à chaque expansion de la variable — un piège de performance.

```makefile
# ✅ Exécuté une seule fois
GIT_HASH := $(shell git rev-parse --short HEAD)

# ❌ Exécuté à chaque fois que $(GIT_HASH) apparaît
GIT_HASH = $(shell git rev-parse --short HEAD)
```

---

## Variables d'environnement et surcharge

Make intègre les **variables d'environnement** du shell comme variables Make de basse priorité. L'ordre de priorité (du plus fort au plus faible) est :

1. Variables passées en **ligne de commande** (`make CXX=clang++`)
2. Variables définies dans le **Makefile** (`CXX := g++`)
3. Variables d'**environnement** du shell (`export CXX=g++`)

L'opérateur `?=` s'insère entre les niveaux 2 et 3 — il ne définit la variable que si elle n'est définie ni en ligne de commande, ni dans le Makefile, ni dans l'environnement :

```makefile
# Valeur par défaut, surchargeable par l'environnement ou la CLI
CXX ?= g++
```

Ce mécanisme permet une configuration flexible :

```bash
# Utiliser le CXX du Makefile (g++ par défaut)
make

# Surcharger depuis la CLI
make CXX=clang++ CXXFLAGS="-std=c++23 -O3"

# Surcharger depuis l'environnement
export CXX=clang++  
make  
```

### Exporter vers les sous-processus

Par défaut, les variables Make ne sont **pas** transmises comme variables d'environnement aux commandes des recettes. Pour les exporter :

```makefile
export PATH := /opt/toolchain/bin:$(PATH)  
export LD_LIBRARY_PATH := /opt/libs/lib  

# Ou exporter toutes les variables (déconseillé — pollue l'environnement)
# export
```

---

## Règles de suffixe (legacy)

Avant les règles de pattern (`%.o: %.cpp`), Make utilisait des **règles de suffixe** :

```makefile
# Ancien style — règle de suffixe
.SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@
```

Cette syntaxe est obsolète. Les règles de pattern sont plus lisibles, plus flexibles (elles supportent les chemins avec répertoires), et sont le standard depuis GNU Make 3.80. Vous les rencontrerez encore dans de vieux Makefiles — sachez les reconnaître, mais ne les utilisez pas dans du code nouveau.

---

## Cibles et règles spéciales

Make définit plusieurs cibles et directives spéciales qui modifient son comportement.

### `.PHONY` — cibles factices

Couvert en section 28.1. Rappel : déclarez toutes les cibles qui ne correspondent pas à des fichiers :

```makefile
.PHONY: all clean test install help
```

### `.DEFAULT_GOAL` — changer la cible par défaut

```makefile
.DEFAULT_GOAL := test
# Maintenant 'make' sans argument lance 'test' au lieu de la première règle
```

### `.DELETE_ON_ERROR` — nettoyer en cas d'échec

```makefile
.DELETE_ON_ERROR:
```

Si une commande de recette échoue, Make supprime le fichier cible (potentiellement corrompu). Sans cette directive, un fichier partiellement écrit resterait sur le disque et serait considéré comme à jour au prochain `make`. C'est une bonne pratique à activer systématiquement.

### `.SECONDARY` et `.PRECIOUS` — préserver les intermédiaires

Par défaut, Make supprime les fichiers intermédiaires après le build. Pour les conserver :

```makefile
# Conserver tous les fichiers intermédiaires
.SECONDARY:

# Conserver des fichiers spécifiques
.PRECIOUS: %.o
```

---

## Organisation de Makefiles de taille moyenne

### Inclusion de fichiers

Un Makefile peut en inclure d'autres avec `include` :

```makefile
# Makefile principal
include config.mk        # Variables de configuration  
include rules.mk          # Règles communes  

# Inclusion optionnelle (pas d'erreur si le fichier n'existe pas)
-include local.mk         # Surcharges locales au développeur
```

Ce mécanisme est similaire à l'`include()` de CMake et permet de structurer un Makefile complexe en modules réutilisables.

### Makefile récursif vs non-récursif

Pour un projet avec des sous-répertoires, deux approches s'opposent.

**Make récursif** : chaque sous-répertoire a son propre Makefile, et le Makefile racine invoque Make dans chaque sous-répertoire :

```makefile
# Approche récursive
.PHONY: all clean

all:
	$(MAKE) -C src
	$(MAKE) -C tests

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
```

`$(MAKE) -C dir` lance une nouvelle instance de Make dans le répertoire `dir`. La variable `$(MAKE)` (plutôt que `make` en dur) assure que la même version de Make est utilisée récursivement et que les options de la ligne de commande sont propagées.

L'inconvénient majeur du Make récursif est que **chaque invocation de Make est indépendante** — elle a son propre graphe de dépendances. Make ne peut pas paralléliser au-delà des frontières de sous-répertoires, et les dépendances inter-répertoires ne sont pas suivies. L'article fondateur « Recursive Make Considered Harmful » (Peter Miller, 1998) détaille ce problème.

**Make non-récursif** : un seul Makefile racine décrit l'ensemble du projet, en incluant des fragments depuis les sous-répertoires :

```makefile
# Makefile racine — non-récursif
SRCS :=  
OBJS :=  

include src/module.mk  
include tests/module.mk  

all: $(TARGET) $(TEST_TARGET)
```

```makefile
# src/module.mk
SRCS += src/core.cpp src/network.cpp  
TARGET := build/bin/my_app  

build/bin/my_app: $(patsubst src/%.cpp,build/obj/%.o,$(filter src/%,$(SRCS)))
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)
```

L'approche non-récursive donne à Make une vue globale du graphe de dépendances, permettant une parallélisation optimale et une détection de changements correcte. Elle est plus complexe à mettre en place mais plus performante.

En pratique, ce débat est largement résolu par CMake et Ninja : CMake génère un graphe de dépendances global (non-récursif) et Ninja l'exécute de manière optimale. Si vous utilisez CMake, vous bénéficiez automatiquement de l'approche non-récursive sans avoir à la configurer manuellement.

---

## Cibles utilitaires courantes

Les projets C++ qui utilisent des Makefiles artisanaux incluent souvent des cibles utilitaires au-delà de `all` et `clean` :

```makefile
.PHONY: all clean test format lint install help

# ── Build ──────────────────────────────────────────────────────
all: $(TARGET)

# ── Nettoyage ──────────────────────────────────────────────────
clean:
	rm -rf $(BUILDDIR)

# ── Tests ──────────────────────────────────────────────────────
test: $(TEST_TARGET)
	./$(TEST_TARGET) --gtest_color=yes

# ── Formatage automatique ─────────────────────────────────────
format:
	find src/ include/ -name '*.cpp' -o -name '*.h' | xargs clang-format -i

# ── Analyse statique ──────────────────────────────────────────
lint:
	clang-tidy src/*.cpp -- $(CXXFLAGS) -Iinclude/

# ── Installation ───────────────────────────────────────────────
PREFIX ?= /usr/local  
install: $(TARGET)  
	install -d $(PREFIX)/bin
	install -m 755 $(TARGET) $(PREFIX)/bin/

# ── Aide ───────────────────────────────────────────────────────
help:
	@echo "Utilisation : make [CIBLE]"
	@echo ""
	@echo "Cibles :"
	@echo "  all       Compiler le projet (défaut)"
	@echo "  clean     Supprimer les fichiers générés"
	@echo "  test      Lancer les tests"
	@echo "  format    Formater le code avec clang-format"
	@echo "  lint      Analyse statique avec clang-tidy"
	@echo "  install   Installer dans PREFIX (défaut: /usr/local)"
	@echo "  help      Afficher cette aide"
```

Ce pattern transforme Make en un **task runner** — un outil d'automatisation qui va au-delà de la compilation. Même dans un projet géré par CMake, il est courant de trouver un Makefile « wrapper » à la racine qui fournit des raccourcis :

```makefile
# Makefile wrapper pour un projet CMake
.PHONY: all clean test format

all:
	cmake --preset release
	cmake --build --preset release

test: all
	ctest --preset release

clean:
	rm -rf build*/

format:
	find src/ include/ -name '*.cpp' -o -name '*.h' | xargs clang-format -i
```

Ce genre de Makefile ne remplace pas CMake — il offre une interface `make test`, `make format` que certains développeurs préfèrent aux commandes CMake plus longues.

---

## Parallélisation

Make supporte la parallélisation avec l'option `-j` :

```bash
make -j$(nproc)    # Utiliser tous les cœurs CPU  
make -j8           # Limiter à 8 jobs parallèles  
make -j1           # Séquentiel (utile pour le diagnostic)  
```

Sans `-j`, Make exécute les commandes séquentiellement. Contrairement à Ninja (qui parallélise par défaut), Make nécessite cette option explicitement.

La parallélisation révèle les **dépendances manquantes** dans le Makefile. Si deux règles peuvent s'exécuter en parallèle et qu'elles dépendent implicitement l'une de l'autre (sans que le Makefile ne l'exprime), le build peut échouer de manière intermittente avec `-j`. C'est un bug dans le Makefile, pas dans Make.

```makefile
# ❌ Dépendance implicite non déclarée — échoue parfois avec -j
generated.h:
	python3 gen.py > generated.h

main.o: main.cpp
	$(CXX) -c main.cpp -o main.o    # main.cpp inclut generated.h !

# ✅ Dépendance explicite — correct avec -j
main.o: main.cpp generated.h
	$(CXX) -c main.cpp -o main.o
```

---

## Lire les Makefiles générés par CMake

Quand vous utilisez CMake avec le générateur `Unix Makefiles`, CMake produit une hiérarchie de Makefiles dans le répertoire de build. Ces fichiers sont verbeux et ne sont pas destinés à être modifiés, mais savoir les parcourir aide au diagnostic :

```
build/
├── Makefile                      # Point d'entrée principal
├── CMakeFiles/
│   ├── Makefile2                 # Dispatcher pour les sous-répertoires
│   ├── my_project_core.dir/
│   │   ├── build.make           # Règles de compilation pour cette cible
│   │   ├── depend.make          # Dépendances de headers
│   │   └── flags.make           # Flags de compilation
│   └── ...
└── src/
    └── Makefile                  # Makefile du sous-répertoire
```

Pour voir les commandes exactes exécutées par Make (utile quand un flag semble incorrect) :

```bash
# Mode verbose
make VERBOSE=1

# Ou via CMake
cmake --build build -- VERBOSE=1
```

Avec Ninja, l'équivalent est `ninja -v`. Le diagnostic est généralement plus simple avec Ninja car le fichier `build.ninja` est un fichier plat unique, tandis que les Makefiles CMake sont distribués sur des dizaines de fichiers.

---

## Récapitulatif des fonctions Make

| Fonction | Syntaxe | Résultat |
|----------|---------|----------|
| Substitution | `$(patsubst %.cpp,%.o,$(SRCS))` | Remplace les extensions |
| Préfixe | `$(addprefix dir/,$(FILES))` | Ajoute un préfixe |
| Suffixe | `$(addsuffix .h,$(NAMES))` | Ajoute un suffixe |
| Filtre | `$(filter %.cpp,$(FILES))` | Garde les matchs |
| Exclure | `$(filter-out %.cpp,$(FILES))` | Retire les matchs |
| Tri | `$(sort $(LIST))` | Trie et déduplique |
| Répertoire | `$(dir path/file.cpp)` | `path/` |
| Nom de fichier | `$(notdir path/file.cpp)` | `file.cpp` |
| Base | `$(basename path/file.cpp)` | `path/file` |
| Extension | `$(suffix path/file.cpp)` | `.cpp` |
| Wildcard | `$(wildcard src/*.cpp)` | Liste les fichiers existants |
| Shell | `$(shell command)` | Capture la sortie |
| Mots | `$(words $(LIST))` | Nombre de mots |
| N-ième mot | `$(word 2,$(LIST))` | Deuxième mot |

---

> **À suivre** : La section 28.3 plonge dans Ninja — pourquoi il est plus rapide que Make, comment fonctionne le format `build.ninja`, et comment lire les fichiers générés par CMake pour le diagnostic.

⏭️ [Ninja : Build system ultra-rapide](/28-make-ninja/03-ninja.md)
