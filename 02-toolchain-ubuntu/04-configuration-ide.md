🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 2.4 — Configuration de l'IDE : VS Code, CLion, Vim/Neovim

> **Chapitre 2 · Mise en place de la Toolchain sur Ubuntu** · Niveau Débutant

---

## Introduction

Un compilateur, un build system et un debugger forment le socle technique d'un projet C++. Mais c'est l'**éditeur de code** — ou plus précisément l'environnement de développement intégré (IDE) — qui détermine votre confort quotidien. Un IDE bien configuré pour le C++ vous offre l'autocomplétion sémantique (qui comprend le type de chaque variable, même à travers des templates complexes), les diagnostics en temps réel (les erreurs apparaissent avant même de compiler), la navigation instantanée dans le code (aller à la définition, trouver toutes les références), le refactoring automatisé et le débogage intégré.

Sur Linux, trois familles d'éditeurs dominent le développement C++ :

- **Visual Studio Code (VS Code)** — éditeur léger, extensible, gratuit et open-source (Microsoft). C'est le choix le plus populaire en 2026 grâce à son écosystème d'extensions et son intégration CMake/clangd.
- **CLion** — IDE complet dédié au C/C++ (JetBrains). Payant mais puissant, avec un support natif de CMake, un debugger intégré et des refactorings avancés.
- **Vim/Neovim** — éditeurs de texte en terminal, ultra-rapides et infiniment configurables. Avec les bons plugins (LSP, DAP), ils offrent une expérience proche d'un IDE complet tout en restant dans le terminal.

Cette section vous guide dans la configuration de chaque option pour un workflow C++ professionnel sur Ubuntu.

---

## La pièce maîtresse : clangd (Language Server Protocol)

Avant de parler des éditeurs eux-mêmes, il faut comprendre la technologie qui les alimente pour le C++ : le **Language Server Protocol (LSP)**.

### Le problème historique

Traditionnellement, chaque éditeur devait implémenter lui-même l'analyse du code C++ pour fournir l'autocomplétion et les diagnostics. Le résultat était que chaque éditeur avait une qualité d'analyse différente, souvent médiocre, et que le travail était dupliqué entre les projets.

### La solution : LSP et clangd

Le Language Server Protocol, initié par Microsoft pour VS Code puis adopté par toute l'industrie, sépare les rôles : l'éditeur gère l'interface (affichage, raccourcis, thèmes), tandis qu'un **serveur de langage** externe fournit l'intelligence (autocomplétion, diagnostics, navigation). L'éditeur et le serveur communiquent via un protocole standardisé (LSP).

Pour le C++, le serveur de langage de référence est **clangd**, issu du projet LLVM. clangd utilise le même front-end d'analyse que le compilateur Clang, ce qui signifie que les diagnostics affichés dans votre éditeur sont exactement ceux que le compilateur produirait. C'est un avantage considérable par rapport aux analyseurs ad hoc.

### Installation de clangd

clangd est installé automatiquement si vous avez suivi l'installation de Clang à la section 2.1 (via le script `llvm.sh`). Sinon, installez-le séparément :

```bash
# Installation standalone
sudo apt install clangd

# Ou avec une version spécifique
sudo apt install clangd-20
```

Vérification :

```bash
clangd --version
# Output (exemple) :
# clangd version 20.x.x
```

### compile_commands.json : le fichier que clangd attend

Pour analyser votre code, clangd a besoin de connaître les options de compilation de chaque fichier source (standard C++ utilisé, chemins d'inclusion, macros définies, etc.). Ces informations sont fournies par un fichier **`compile_commands.json`** — une base de données de compilation au format JSON, générée automatiquement par CMake.

Pour que CMake génère ce fichier :

```bash
cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Le fichier `build/compile_commands.json` est alors créé (ou mis à jour) à chaque configuration. clangd le cherche dans le répertoire de build ou à la racine du projet. La convention est de créer un lien symbolique à la racine :

```bash
ln -sf build/compile_commands.json compile_commands.json
```

Avec ce lien en place, clangd trouvera automatiquement la base de données de compilation quel que soit votre éditeur.

> 💡 **Astuce** — Ajoutez `CMAKE_EXPORT_COMPILE_COMMANDS=ON` dans votre CMake Preset de développement pour ne plus jamais l'oublier :  
>  
> ```json
> {
>     "name": "dev",
>     "cacheVariables": {
>         "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
>         "CMAKE_CXX_COMPILER_LAUNCHER": "ccache"
>     }
> }
> ```

### Configuration de clangd : le fichier `.clangd`

clangd peut être configuré via un fichier `.clangd` à la racine du projet :

```yaml
# .clangd — Configuration du serveur de langage
CompileFlags:
  Add:
    - "-std=c++23"
    - "-Wall"
    - "-Wextra"
  # Supprimer des warnings spécifiques qui polluent l'éditeur
  Remove:
    - "-W*"         # Puis réajouter ceux qu'on veut via Add

Diagnostics:
  # Activer les checks clang-tidy directement dans l'éditeur
  ClangTidy:
    Add:
      - modernize-*
      - bugprone-*
      - performance-*
    Remove:
      - modernize-use-trailing-return-type

InlayHints:
  # Afficher les types déduits, noms de paramètres, etc.
  Enabled: true
  ParameterNames: true
  DeducedTypes: true
```

Ce fichier est optionnel — clangd fonctionne très bien sans, en se basant uniquement sur `compile_commands.json`. Mais il permet d'affiner les diagnostics et d'activer des fonctionnalités comme les inlay hints (affichage des types déduits par `auto` directement dans le code) et l'intégration de clang-tidy.

---

## Visual Studio Code

### Pourquoi VS Code pour le C++

VS Code est l'éditeur le plus utilisé pour le développement C++ sur Linux en 2026. Ses atouts : gratuit, léger (comparé à un IDE complet), extensible via un marketplace riche, excellente intégration avec clangd et CMake, et une communauté massive qui produit documentation et extensions. Son principal défaut : il nécessite une configuration initiale via des extensions — contrairement à un IDE comme CLion qui fonctionne « out of the box ».

### Installation

```bash
# Méthode 1 : Via le paquet .deb officiel
wget -O vscode.deb "https://code.visualstudio.com/sha/download?build=stable&os=linux-deb-x64"  
sudo dpkg -i vscode.deb  
sudo apt install -f  # Résoudre les dépendances si nécessaire  

# Méthode 2 : Via snap
sudo snap install code --classic
```

### Extensions essentielles

L'écosystème C++ sur VS Code repose sur quelques extensions clé que nous détaillons en section 2.4.1. Voici un aperçu rapide pour une installation immédiate :

```bash
# Installer les extensions depuis le terminal
code --install-extension llvm-vs-code-extensions.vscode-clangd  
code --install-extension ms-vscode.cmake-tools  
code --install-extension ms-vscode.cpptools  
code --install-extension twxs.cmake  
```

**clangd** (`llvm-vs-code-extensions.vscode-clangd`) — le serveur de langage C++ qui fournit l'autocomplétion sémantique, les diagnostics et la navigation. C'est l'extension la plus importante.

**CMake Tools** (`ms-vscode.cmake-tools`) — intègre CMake dans VS Code : configuration, compilation, sélection du preset, choix de la cible — le tout depuis la barre de statut et la palette de commandes.

**C/C++** (`ms-vscode.cpptools`) — l'extension Microsoft historique. Utilisée principalement pour le **debugger** (elle intègre GDB/LLDB dans l'interface graphique de VS Code). Pour l'IntelliSense, nous préférons clangd.

**CMake Language Support** (`twxs.cmake`) — coloration syntaxique et autocomplétion pour les fichiers `CMakeLists.txt`.

> ⚠️ **Conflit clangd / IntelliSense** — L'extension C/C++ de Microsoft inclut son propre moteur IntelliSense qui entre en conflit avec clangd. Si vous utilisez les deux extensions, désactivez IntelliSense dans l'extension Microsoft en ajoutant cette ligne dans vos paramètres VS Code (`settings.json`) :  
>  
> ```json
> {
>     "C_Cpp.intelliSenseEngine": "disabled"
> }
> ```  
>  
> Cela conserve le support debugging de l'extension C/C++ tout en laissant clangd gérer l'analyse de code.

### Configuration minimale : `settings.json`

Voici une configuration de départ pour un workflow C++ productif :

```json
{
    // === clangd ===
    "clangd.path": "clangd",
    "clangd.arguments": [
        "--background-index",
        "--clang-tidy",
        "--header-insertion=iwyu",
        "--completion-style=detailed",
        "--function-arg-placeholders",
        "--fallback-style=Google"
    ],

    // === Désactiver IntelliSense Microsoft (utiliser clangd) ===
    "C_Cpp.intelliSenseEngine": "disabled",

    // === CMake Tools ===
    "cmake.generator": "Ninja",
    "cmake.configureOnOpen": true,
    "cmake.buildDirectory": "${workspaceFolder}/build",

    // === Éditeur : paramètres C++ ===
    "editor.formatOnSave": true,
    "editor.defaultFormatter": "llvm-vs-code-extensions.vscode-clangd",
    "[cpp]": {
        "editor.defaultFormatter": "llvm-vs-code-extensions.vscode-clangd"
    },

    // === Fichiers ===
    "files.associations": {
        "*.h": "cpp",
        "*.hpp": "cpp",
        "*.tpp": "cpp"
    }
}
```

Les arguments clangd méritent une explication :

- `--background-index` — indexe le projet en arrière-plan pour une navigation rapide dès l'ouverture.
- `--clang-tidy` — active l'analyse clang-tidy directement dans l'éditeur.
- `--header-insertion=iwyu` — propose l'ajout automatique des headers manquants (style « Include What You Use »).
- `--completion-style=detailed` — affiche les signatures complètes dans les suggestions d'autocomplétion.
- `--function-arg-placeholders` — insère des placeholders pour les arguments de fonction lors de la complétion.
- `--fallback-style=Google` — style de formatage utilisé quand aucun `.clang-format` n'est trouvé.

### Workflow typique dans VS Code

1. Ouvrez le dossier du projet (`File > Open Folder`).
2. CMake Tools détecte le `CMakeLists.txt` et propose de configurer le projet. Sélectionnez votre preset ou votre kit (compilateur).
3. clangd démarre, lit le `compile_commands.json`, et commence l'indexation en arrière-plan.
4. En quelques secondes, l'autocomplétion, les diagnostics et la navigation sont opérationnels.
5. Compilez via la palette de commandes (`Ctrl+Shift+P` → « CMake: Build ») ou via le raccourci `F7`.
6. Débuguez via `F5` (après configuration du `launch.json` — voir section 2.4.2).

---

## CLion

### Pourquoi CLion

CLion (JetBrains) est un IDE commercial dédié au C/C++. Son avantage principal est de fonctionner « out of the box » : il intègre nativement CMake, un debugger (GDB ou LLDB), l'analyse de code, le refactoring, les tests unitaires et le profiling. Aucune extension à installer ni à configurer — tout est cohérent et préconfiguré.

Son inconvénient est le prix (licence annuelle, gratuite pour les étudiants et les projets open source) et la consommation de ressources (Java-based, plus gourmand en RAM que VS Code).

### Installation

```bash
# Méthode 1 : Via Snap (recommandé)
sudo snap install clion --classic

# Méthode 2 : Via JetBrains Toolbox App
# Télécharger depuis https://www.jetbrains.com/toolbox-app/
```

### Configuration pour C++

Au premier lancement, CLion détecte automatiquement les compilateurs installés et CMake. Vérifiez et ajustez dans `File > Settings > Build, Execution, Deployment > Toolchains` :

- **Compiler** — sélectionnez `g++-15` ou `clang++-20` selon votre préférence.
- **CMake** — CLion utilise son CMake intégré par défaut, mais vous pouvez pointer vers le CMake système (`/usr/bin/cmake` ou `/usr/local/bin/cmake`).
- **Debugger** — GDB est le défaut ; LLDB est disponible comme alternative.
- **Build Tool** — sélectionnez Ninja pour des builds plus rapides.

Pour activer ccache dans CLion, ajoutez les variables CMake dans `Settings > Build, Execution, Deployment > CMake > CMake options` :

```
-DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache
```

### CLion et clangd

CLion utilise son propre moteur d'analyse de code par défaut (basé sur un parseur interne). Depuis CLion 2023.x, il est possible d'utiliser clangd comme moteur d'analyse (activable dans `Settings > Languages & Frameworks > C/C++ > Clangd`). L'analyse clangd est souvent plus précise et plus rapide sur les gros projets template-heavy.

### Forces de CLion

- **Refactoring avancé** — renommage intelligent, extraction de fonction/variable/constante, changement de signature, avec prévisualisation de tous les changements avant application.
- **Analyse de code intégrée** — détection de fuites mémoire, de boucles infinies potentielles, de variables non utilisées, avec corrections rapides (quick-fixes).
- **Debugger visuel** — affichage des variables, des conteneurs STL (visualisation du contenu d'un `std::vector` ou d'une `std::map`), évaluation d'expressions en temps réel.
- **Support natif des tests** — intégration de Google Test, Catch2, Boost.Test avec exécution et résultats directement dans l'IDE.
- **Profiling intégré** — profiling CPU et mémoire depuis l'IDE (via perf et Valgrind sur Linux).

---

## Vim / Neovim

### Pourquoi Vim/Neovim pour le C++

Vim et son successeur Neovim sont des éditeurs de texte en terminal, réputés pour leur rapidité et leur efficacité une fois les raccourcis clavier maîtrisés. Avec l'écosystème de plugins moderne de Neovim (LSP natif, Tree-sitter, DAP), il est possible d'obtenir une expérience de développement C++ comparable à celle d'un IDE graphique, tout en restant dans le terminal — un avantage considérable pour le travail en SSH, dans les conteneurs et sur les serveurs distants.

Neovim est généralement préféré à Vim classique en 2026 grâce à son support LSP natif (intégré depuis Neovim 0.5), son système de plugins en Lua (plus rapide et plus expressif que VimScript) et sa communauté de plugins très active.

### Installation de Neovim

```bash
# Version stable depuis les dépôts Ubuntu
sudo apt install neovim

# Ou version récente via PPA (recommandé)
sudo add-apt-repository ppa:neovim-ppa/unstable -y  
sudo apt update  
sudo apt install neovim  
```

### Configuration LSP pour C++

Neovim intègre un client LSP natif. La configuration la plus courante utilise le plugin **nvim-lspconfig** pour connecter Neovim à clangd :

```lua
-- ~/.config/nvim/init.lua (extrait)

-- Configuration de clangd via nvim-lspconfig
local lspconfig = require('lspconfig')

lspconfig.clangd.setup {
    cmd = {
        "clangd",
        "--background-index",
        "--clang-tidy",
        "--header-insertion=iwyu",
        "--completion-style=detailed",
        "--function-arg-placeholders",
        "--fallback-style=Google",
    },
    filetypes = { "c", "cpp", "objc", "objcpp", "cuda" },
    root_dir = lspconfig.util.root_pattern(
        "compile_commands.json",
        ".clangd",
        ".git"
    ),
}
```

### Plugins essentiels pour un workflow C++

L'écosystème Neovim s'organise typiquement autour d'un gestionnaire de plugins (comme **lazy.nvim**) et d'une sélection de plugins complémentaires :

- **nvim-lspconfig** — configuration simplifiée des serveurs LSP (dont clangd).
- **nvim-cmp** — autocomplétion avec support LSP, snippets et multiples sources.
- **nvim-dap** + **nvim-dap-ui** — intégration du Debug Adapter Protocol (DAP) pour le débogage pas-à-pas avec GDB, directement dans Neovim.
- **nvim-treesitter** — coloration syntaxique et manipulation structurelle du code basées sur l'arbre syntaxique (plus précis que les expressions régulières).
- **telescope.nvim** — recherche fuzzy pour les fichiers, les symboles, les grep et les diagnostics.
- **cmake-tools.nvim** — intégration CMake (configure, build, sélection de target) depuis Neovim.

### Distributions Neovim préconfigurées

Pour les développeurs qui ne veulent pas passer des heures à configurer Neovim manuellement, plusieurs distributions préconfigurées offrent un environnement de développement C++ fonctionnel immédiatement :

- **LazyVim** — distribution modulaire basée sur lazy.nvim, avec support LSP/DAP préconfiguré.
- **NvChad** — distribution orientée esthétique et performance, avec une interface de configuration accessible.
- **kickstart.nvim** — point de départ minimal et bien documenté, conçu pour être personnalisé.

Ces distributions incluent la configuration LSP, l'autocomplétion, le Tree-sitter et souvent le DAP. Il suffit d'installer clangd et de générer le `compile_commands.json` pour que l'analyse C++ fonctionne.

---

## Comparatif des trois options

| Critère | VS Code | CLion | Neovim |
|---|---|---|---|
| **Prix** | Gratuit | Payant (gratuit étudiants/OSS) | Gratuit |
| **Installation** | Simple | Simple | Simple (config plus longue) |
| **Configuration initiale** | Extensions à installer | Out of the box | Plugins + config Lua |
| **Moteur d'analyse C++** | clangd (extension) | Interne + clangd (option) | clangd (LSP natif) |
| **Debugging** | GDB via extension C++ | GDB/LLDB natif | GDB via nvim-dap |
| **Intégration CMake** | CMake Tools (extension) | Natif | cmake-tools.nvim |
| **Refactoring** | Basique (via clangd) | Avancé (natif) | Basique (via clangd) |
| **Consommation RAM** | ~300-600 Mo | ~1-3 Go | ~50-150 Mo |
| **Travail en SSH/terminal** | Possible (Remote-SSH) | Possible (Remote Dev) | Natif |
| **Courbe d'apprentissage** | Faible | Faible | Élevée (raccourcis Vim) |
| **Personnalisation** | Extensions (marketplace) | Limité (settings) | Illimitée (Lua) |

### Recommandation

**Si vous débutez en C++ et/ou en Linux** : commencez par **VS Code**. La courbe d'apprentissage est douce, la documentation abondante, et l'écosystème d'extensions couvre tous les besoins. C'est le choix que nous utiliserons principalement dans cette formation.

**Si vous êtes prêt à investir** (financièrement et en temps de prise en main) : **CLion** offre l'expérience la plus intégrée et les refactorings les plus puissants. C'est le choix de nombreuses équipes professionnelles.

**Si vous vivez dans le terminal** : **Neovim** avec clangd et nvim-dap offre une expérience de développement C++ qui n'a rien à envier aux IDE graphiques, avec une empreinte mémoire minimale et un fonctionnement natif en SSH.

Quel que soit votre choix, **clangd est le dénominateur commun** : les trois éditeurs l'utilisent (ou peuvent l'utiliser) comme moteur d'analyse C++. Maîtriser la configuration de clangd (le `compile_commands.json`, le fichier `.clangd`, les arguments en ligne de commande) vous servira indépendamment de l'éditeur.

---

## Vue d'ensemble des sous-sections

### [2.4.1 — Extensions VS Code essentielles (C/C++, CMake Tools, clangd)](/02-toolchain-ubuntu/04.1-extensions-vscode.md)

Configuration détaillée de chaque extension VS Code pour un workflow C++ optimal : résolution du conflit IntelliSense/clangd, configuration de CMake Tools avec les presets, personnalisation des diagnostics et du formatage.

### [2.4.2 — Configuration du debugging intégré](/02-toolchain-ubuntu/04.2-configuration-debugging.md)

Mise en place du débogage pas-à-pas dans chaque éditeur : configuration de `launch.json` (VS Code), Run/Debug configurations (CLion), nvim-dap (Neovim). Breakpoints conditionnels, watch expressions, inspection de la pile d'appels.

### [2.4.3 — DevContainers : environnement reproductible](/02-toolchain-ubuntu/04.3-devcontainers.md)

Utilisation des DevContainers VS Code pour encapsuler l'ensemble de la toolchain (compilateur, clangd, CMake, ccache) dans un conteneur Docker. L'objectif : un `git clone` suivi d'un « Reopen in Container » et l'environnement complet est prêt — identique pour chaque membre de l'équipe.

### [2.4.4 — IA-assisted tooling : Copilot, Clangd AI et complétion intelligente (2026)](/02-toolchain-ubuntu/04.4-ia-assisted-tooling.md) ⭐

Tour d'horizon des outils d'assistance par IA disponibles en 2026 pour le développement C++ : GitHub Copilot, complétion contextuelle, génération de tests, explication de code. Bonnes pratiques et limites pour un usage professionnel.

---

## Vérification de l'installation

Un script rapide pour vérifier que les prérequis IDE sont en place :

```bash
echo "=== Prérequis IDE pour C++ ==="

printf "%-20s : " "clangd"  
if command -v clangd &>/dev/null; then  
    clangd --version 2>&1 | head -1
else
    echo "NON INSTALLÉ ❌ (sudo apt install clangd)"
fi

printf "%-20s : " "VS Code"  
if command -v code &>/dev/null; then  
    code --version 2>&1 | head -1
else
    echo "Non installé (optionnel)"
fi

printf "%-20s : " "Neovim"  
if command -v nvim &>/dev/null; then  
    nvim --version 2>&1 | head -1
else
    echo "Non installé (optionnel)"
fi

printf "%-20s : " "compile_commands.json"  
if [ -f "compile_commands.json" ] || [ -f "build/compile_commands.json" ]; then  
    echo "TROUVÉ ✅"
else
    echo "NON TROUVÉ — Configurez CMake avec -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
fi
```

---

## Ce qu'il faut retenir

- **clangd est le moteur d'analyse C++ partagé** par VS Code, CLion et Neovim. Installez-le et maîtrisez sa configuration (`compile_commands.json`, fichier `.clangd`, arguments) — c'est un investissement qui porte ses fruits quel que soit l'éditeur.
- **`compile_commands.json`** est le fichier que clangd attend pour comprendre votre projet. Générez-le avec `cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON` et créez un lien symbolique à la racine du projet.
- **VS Code** est le choix le plus accessible : gratuit, extensible et bien documenté. Il nécessite quelques extensions (clangd, CMake Tools, C/C++) et une configuration initiale dans `settings.json`.
- **CLion** offre l'expérience la plus intégrée, avec un refactoring et un débogage de premier ordre. Idéal pour les équipes professionnelles.
- **Neovim** est le choix des développeurs qui vivent dans le terminal. Avec le LSP natif et nvim-dap, il offre une expérience C++ complète en quelques dizaines de Mo de RAM.
- **Quel que soit l'éditeur** : désactivez IntelliSense Microsoft si vous utilisez clangd, activez le formatage automatique via clang-format, et configurez le débogage intégré (section 2.4.2).

---


⏭️ [Extensions VS Code essentielles (C/C++, CMake Tools, clangd)](/02-toolchain-ubuntu/04.1-extensions-vscode.md)
