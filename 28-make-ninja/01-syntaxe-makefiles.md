🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 28.1 Syntaxe de base des Makefiles

> **Objectif** : Comprendre les briques fondamentales d'un Makefile — règles, cibles, prérequis et recettes — pour être capable de lire un Makefile existant, de diagnostiquer un build Make, et d'écrire un Makefile simple pour les cas où CMake serait disproportionné.

---

## Le concept fondamental : la règle

Un Makefile est un ensemble de **règles**. Chaque règle décrit comment produire un fichier (la **cible**) à partir d'autres fichiers (les **prérequis**), en exécutant des commandes (la **recette**) :

```makefile
cible: prérequis1 prérequis2
	commande1
	commande2
```

La syntaxe est stricte sur un point crucial : **les lignes de commande doivent être indentées par une tabulation** (caractère `\t`), pas par des espaces. C'est la source d'erreur n°1 chez les débutants — de nombreux éditeurs convertissent les tabulations en espaces, ce qui casse silencieusement le Makefile. Make signale alors une erreur du type `*** missing separator. Stop.`

> 💡 **Astuce** : configurez votre éditeur pour utiliser de vraies tabulations dans les Makefiles. Dans VS Code, ajoutez à votre `settings.json` :
> ```json
> "[makefile]": {
>     "editor.insertSpaces": false,
>     "editor.tabSize": 4
> }
> ```

---

## Un premier Makefile complet

Commençons par un exemple concret qui compile un petit projet C++ :

```makefile
# Compilateur et flags
CXX      = g++  
CXXFLAGS = -std=c++23 -Wall -Wextra -O2  

# Cibles et sources
TARGET = my_app  
SRCS   = main.cpp utils.cpp network.cpp  
OBJS   = $(SRCS:.cpp=.o)  

# Règle par défaut — la première règle du fichier
all: $(TARGET)

# Règle de linkage : produire l'exécutable à partir des .o
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Règle de compilation : produire un .o à partir d'un .cpp
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Règle de nettoyage (cible phony — ne produit pas de fichier)
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
```

Ce Makefile contient tous les éléments fondamentaux. Décortiquons-les un par un.

---

## Règles explicites

Une **règle explicite** nomme une cible spécifique :

```makefile
my_app: main.o utils.o network.o
	g++ -std=c++23 -o my_app main.o utils.o network.o
```

Cette règle signifie : « pour produire `my_app`, j'ai besoin de `main.o`, `utils.o` et `network.o`. Si l'un de ces fichiers est plus récent que `my_app` (ou si `my_app` n'existe pas), exécuter la commande de linkage. »

### L'algorithme de Make

Quand vous tapez `make my_app`, Make exécute l'algorithme suivant :

1. Chercher la règle pour `my_app`.
2. Vérifier chaque prérequis (`main.o`, `utils.o`, `network.o`).
3. Pour chaque prérequis, chercher s'il existe une règle pour le produire — si oui, l'évaluer récursivement.
4. Comparer les timestamps : si un prérequis est plus récent que la cible (ou si la cible n'existe pas), exécuter la recette.

C'est un algorithme de **parcours de graphe avec détection de péremption par timestamp**. Make construit un DAG (*Directed Acyclic Graph*) des dépendances et ne recompile que ce qui est nécessaire.

```
my_app
  ├── main.o
  │     └── main.cpp      (si main.cpp est plus récent que main.o → recompiler)
  ├── utils.o
  │     └── utils.cpp
  └── network.o
        └── network.cpp
```

### La règle par défaut

La **première règle** du Makefile est la règle par défaut — celle qui est exécutée quand vous tapez `make` sans argument. Par convention, on la nomme `all` :

```makefile
all: my_app
```

C'est une règle sans recette : elle dit simplement « pour construire `all`, il faut construire `my_app` ». Make résout ensuite `my_app` comme n'importe quelle autre cible. Si `all` doit produire plusieurs exécutables :

```makefile
all: my_app my_tool my_server
```

---

## Règles de pattern (implicites)

Les règles de pattern utilisent le caractère `%` comme wildcard pour décrire une famille de transformations :

```makefile
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
```

Cette règle dit : « pour n'importe quel fichier `.o`, si un fichier `.cpp` du même nom existe et est plus récent, exécuter la commande de compilation. » Le `%` capture le *stem* — la partie commune du nom. Pour `main.o`, `%` vaut `main`, et Make cherche `main.cpp` comme prérequis.

Les règles de pattern sont le mécanisme de généralisation de Make. Sans elles, il faudrait écrire une règle explicite par fichier source :

```makefile
# ❌ Fastidieux et non maintenable
main.o: main.cpp
	g++ -c main.cpp -o main.o
utils.o: utils.cpp
	g++ -c utils.cpp -o utils.o
network.o: network.cpp
	g++ -c network.cpp -o network.o
```

La règle de pattern les remplace toutes par une seule ligne.

---

## Variables

Make supporte des variables (parfois appelées macros) pour factoriser les valeurs répétées.

### Déclaration et utilisation

```makefile
CXX      = g++  
CXXFLAGS = -std=c++23 -Wall -Wextra -O2  
LDFLAGS  = -lssl -lcrypto  
TARGET   = my_app  
SRCS     = main.cpp utils.cpp network.cpp  
```

Les variables se référencent avec `$(NOM)` ou `${NOM}` :

```makefile
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS)
```

### Affectation simple (`=`) vs immédiate (`:=`)

Make distingue deux types d'affectation :

```makefile
# Affectation récursive (=) — évaluée à chaque utilisation
FOO = $(BAR)  
BAR = hello  
# $(FOO) vaut "hello" — BAR est évalué au moment de l'utilisation de FOO

# Affectation immédiate (:=) — évaluée une seule fois
FOO := $(BAR)  
BAR := hello  
# $(FOO) vaut "" — BAR n'était pas encore défini quand FOO a été évalué
```

L'affectation immédiate (`:=`) est plus prévisible et recommandée dans la plupart des cas. L'affectation récursive (`=`) est utile quand la valeur dépend de variables définies plus loin dans le fichier, mais peut causer des évaluations circulaires.

### Autres opérateurs d'affectation

```makefile
# Append (+=) — ajouter à une variable existante
CXXFLAGS += -Wpedantic

# Conditionnelle (?=) — ne définir que si la variable n'est pas déjà définie
CXX ?= g++
```

L'opérateur `?=` est utile pour permettre la surcharge depuis la ligne de commande :

```bash
# La variable définie en CLI a priorité sur ?= dans le Makefile
make CXX=clang++
```

### Variables automatiques

Make définit des **variables automatiques** dans le contexte de chaque règle. Elles évitent de répéter les noms de fichiers :

| Variable | Signification | Exemple (pour `main.o: main.cpp utils.h`) |
|:--------:|--------------|-------------------------------------------|
| `$@` | La cible | `main.o` |
| `$<` | Le premier prérequis | `main.cpp` |
| `$^` | Tous les prérequis (sans doublons) | `main.cpp utils.h` |
| `$?` | Les prérequis plus récents que la cible | (ceux qui ont changé) |
| `$*` | Le stem d'une règle de pattern | `main` (si `%.o: %.cpp`) |

Ces variables sont omniprésentes dans les Makefiles idiomatiques :

```makefile
# Sans variables automatiques — répétitif
my_app: main.o utils.o
	g++ -o my_app main.o utils.o

# Avec variables automatiques — concis et générique
my_app: main.o utils.o
	$(CXX) -o $@ $^

# Règle de pattern avec variables automatiques
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
```

---

## Cibles phony

Une cible **phony** (*factice*) est une cible qui ne correspond pas à un fichier. Elle représente une action plutôt qu'un artefact :

```makefile
.PHONY: clean all test install

clean:
	rm -f $(OBJS) $(TARGET)

test: $(TARGET)
	./$(TARGET) --run-tests

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/
```

La directive `.PHONY` indique à Make que ces noms ne sont pas des fichiers. Sans cette directive, si un fichier nommé `clean` existait dans le répertoire, Make considérerait la cible `clean` comme à jour (puisque le fichier existe et n'a pas de prérequis plus récent) et n'exécuterait pas la recette.

Les cibles phony courantes dans les projets C++ :

| Cible | Rôle |
|-------|------|
| `all` | Construire tous les artefacts principaux (règle par défaut) |
| `clean` | Supprimer les fichiers générés |
| `test` | Lancer les tests |
| `install` | Installer les binaires sur le système |
| `dist` | Créer une archive de distribution |
| `help` | Afficher l'aide |

---

## Gestion des dépendances de headers

Un piège classique des Makefiles C++ est la gestion des dépendances sur les headers. Si `main.cpp` inclut `utils.h` et que vous modifiez `utils.h`, Make ne recompile pas `main.o` — parce que la règle `%.o: %.cpp` ne mentionne que le fichier `.cpp` comme prérequis.

### Solution manuelle (fragile)

```makefile
main.o: main.cpp utils.h config.h  
utils.o: utils.cpp utils.h  
network.o: network.cpp network.h config.h  
```

Maintenir ces dépendances à la main est fastidieux et sujet aux erreurs — chaque `#include` ajouté ou retiré nécessite une mise à jour du Makefile.

### Solution automatique avec le compilateur

GCC et Clang peuvent générer automatiquement les dépendances de headers avec les flags `-MMD -MP` :

```makefile
CXXFLAGS := -std=c++23 -Wall -Wextra -O2 -MMD -MP  
SRCS     := main.cpp utils.cpp network.cpp  
OBJS     := $(SRCS:.cpp=.o)  
DEPS     := $(SRCS:.cpp=.d)  

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Inclure les fichiers de dépendances générés
-include $(DEPS)
```

Le flag `-MMD` dit au compilateur : « en plus de compiler, génère un fichier `.d` qui liste les headers dont ce `.cpp` dépend ». Le flag `-MP` ajoute des cibles phony pour chaque header, ce qui évite des erreurs quand un header est supprimé. Le `-include` (avec le tiret) inclut les fichiers `.d` s'ils existent et les ignore sinon (au premier build, ils n'existent pas encore).

Le fichier `main.d` généré ressemble à :

```makefile
main.o: main.cpp utils.h config.h /usr/include/c++/15/iostream
```

Make intègre ces dépendances automatiquement. Si `utils.h` change, `main.o` sera recompilé.

C'est exactement ce que CMake configure automatiquement quand vous utilisez `add_executable()` ou `add_library()`. Un des nombreux avantages de CMake sur un Makefile écrit à la main.

---

## Recettes multi-lignes et commandes silencieuses

### Préfixes de commande

Chaque ligne de commande dans une recette peut être préfixée par des caractères spéciaux :

```makefile
install: $(TARGET)
	@echo "Installation de $(TARGET)..."     # @ — ne pas afficher la commande
	@install -m 755 $(TARGET) /usr/local/bin/
	-rm -f /tmp/old_version                  # - — ignorer les erreurs
```

- `@` : supprime l'écho de la commande (Make affiche normalement chaque commande avant de l'exécuter).
- `-` : continue même si la commande échoue (par défaut, Make s'arrête à la première erreur).

### Continuité entre lignes

Chaque ligne de commande est exécutée dans un **shell séparé**. Cela signifie qu'un `cd` sur une ligne n'affecte pas la ligne suivante :

```makefile
# ❌ Ne fonctionne PAS — chaque ligne est un shell indépendant
build-subdir:
	cd subdir
	make            # Exécuté dans le répertoire original, pas dans subdir/

# ✅ Solution 1 : une seule ligne avec &&
build-subdir:
	cd subdir && make

# ✅ Solution 2 : continuation de ligne avec backslash
build-subdir:
	cd subdir && \
	make && \
	make install
```

---

## Exécution conditionnelle

Make supporte les directives conditionnelles pour adapter le Makefile selon le contexte :

```makefile
# Conditionnel sur une variable
DEBUG ?= 0

ifeq ($(DEBUG), 1)
    CXXFLAGS += -g -O0 -DDEBUG
else
    CXXFLAGS += -O2 -DNDEBUG
endif

# Conditionnel sur l'existence d'un programme
CCACHE := $(shell command -v ccache 2>/dev/null)  
ifdef CCACHE  
    CXX := ccache $(CXX)
endif
```

```bash
# Utilisation
make               # Build Release (DEBUG=0 par défaut)  
make DEBUG=1       # Build Debug  
```

La fonction `$(shell ...)` exécute une commande shell et capture sa sortie. C'est utile pour détecter des outils ou récupérer des informations système, mais rend le Makefile dépendant du shell disponible.

---

## Un Makefile réaliste pour un petit projet

Voici un Makefile complet et idiomatique pour un projet simple sans CMake — le genre de projet où un Makefile écrit à la main est raisonnable (un outil CLI avec quelques fichiers sources) :

```makefile
# ── Configuration ──────────────────────────────────────────────
CXX       := g++  
CXXFLAGS  := -std=c++23 -Wall -Wextra -Wpedantic -MMD -MP  
LDFLAGS   :=  
LDLIBS    := -lssl -lcrypto  

# Répertoires
SRCDIR    := src  
BUILDDIR  := build  
BINDIR    := build/bin  

# Sources et objets
SRCS      := $(wildcard $(SRCDIR)/*.cpp)  
OBJS      := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SRCS))  
DEPS      := $(OBJS:.o=.d)  

# Cible finale
TARGET    := $(BINDIR)/my_tool

# ── Règles ─────────────────────────────────────────────────────
.PHONY: all clean help

all: $(TARGET)

$(TARGET): $(OBJS) | $(BINDIR)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Créer les répertoires si nécessaire (order-only prerequisites)
$(BUILDDIR) $(BINDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILDDIR)

help:
	@echo "Cibles disponibles :"
	@echo "  all     - Compiler le projet (défaut)"
	@echo "  clean   - Supprimer les fichiers générés"
	@echo "  help    - Afficher cette aide"

# Inclure les dépendances de headers générées
-include $(DEPS)
```

### Éléments notables

**`$(wildcard ...)`** : collecte tous les fichiers `.cpp` du répertoire source. Contrairement à `file(GLOB)` en CMake, ce n'est pas un problème ici car Make réévalue le wildcard à chaque invocation — les nouveaux fichiers sont automatiquement détectés.

**`$(patsubst ...)`** : transforme les chemins de sources en chemins d'objets. `src/main.cpp` devient `build/main.o`.

**Order-only prerequisites** (`| $(BUILDDIR)`) : le `|` introduit des prérequis qui doivent exister mais dont le timestamp n'est pas vérifié. Le répertoire `build/` doit exister avant la compilation, mais sa date de modification ne doit pas déclencher une recompilation.

---

## Quand utiliser un Makefile écrit à la main vs CMake

| Situation | Recommandation |
|-----------|---------------|
| Projet C++ avec des bibliothèques, des tests, de la CI | **CMake** — la complexité justifie l'outil |
| Petit outil CLI (< 10 fichiers, 0-1 dépendances) | Makefile artisanal acceptable |
| Automatisation de tâches non-C++ (scripts, documentation) | **Make** — excellent outil d'automatisation |
| Projet qui doit être consommable par d'autres via `find_package` | **CMake** — Make ne fournit pas ce mécanisme |
| Projet multiplateforme (Linux + Windows + macOS) | **CMake** — Make n'est pas portable sur Windows |
| Intégration avec Conan ou vcpkg | **CMake** — les gestionnaires de paquets ciblent CMake |

En résumé, un Makefile écrit à la main est raisonnable pour les petits projets autonomes ou pour l'automatisation de tâches. Dès que le projet grandit, que des dépendances externes apparaissent, ou que la portabilité devient importante, CMake reprend l'avantage.

---

> **À suivre** : La section 28.2 approfondit les variables, les règles avancées et les patterns Make — les fonctions de manipulation de texte, les règles de suffixe, et les techniques d'organisation pour les Makefiles de taille moyenne.

⏭️ [Variables, règles et patterns](/28-make-ninja/02-variables-regles.md)
