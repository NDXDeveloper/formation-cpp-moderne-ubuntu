🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 29.2 — Débogage via IDE (VS Code, CLion)

## Chapitre 29 : Débogage Avancé · Module 10

---

## Introduction

La section 29.1 vous a donné la maîtrise de GDB en ligne de commande. Vous savez naviguer, inspecter et poser des breakpoints conditionnels. Cette connaissance est fondamentale — et elle ne devient pas obsolète parce que vous utilisez un IDE.

En réalité, VS Code et CLion ne remplacent pas GDB. Ils l'enveloppent. Quand vous cliquez dans la marge pour poser un breakpoint dans VS Code, l'extension C/C++ envoie une commande `break` à GDB. Quand vous survolez une variable pour voir sa valeur, l'IDE envoie un `print`. Quand vous cliquez sur "Step Over", c'est un `next` qui part.

Comprendre cette relation change votre manière d'utiliser un IDE de débogage : vous ne dépendez plus de l'interface graphique. Quand l'IDE ne vous montre pas ce que vous cherchez, vous savez ouvrir la console GDB intégrée et taper la commande directement. L'IDE devient un accélérateur, pas une béquille.

Ce chapitre couvre la configuration et l'utilisation pratique du débogage dans les deux IDE les plus utilisés pour le C++ sous Linux : VS Code (gratuit, extensible, dominant dans l'écosystème DevOps) et CLion (payant, intégration native C++, dominant dans le développement pur).

---

## VS Code : Configuration et débogage C++

### Prérequis

Le débogage C++ dans VS Code repose sur deux extensions :

- **C/C++** (Microsoft, `ms-vscode.cpptools`) — fournit le débogueur, l'IntelliSense, et le formateur. C'est l'extension qui communique avec GDB ou LLDB en arrière-plan.
- **CMake Tools** (Microsoft, `ms-vscode.cmake-tools`) — détecte les projets CMake, gère les builds, et configure automatiquement le débogueur pour les targets CMake.

```bash
# Installation depuis le terminal (si VS Code est installé)
code --install-extension ms-vscode.cpptools  
code --install-extension ms-vscode.cmake-tools  
```

> 📎 *L'installation détaillée des extensions et la configuration d'IntelliSense sont couvertes en section 2.4.1.*

### Le fichier `launch.json`

La configuration du débogueur dans VS Code passe par le fichier `.vscode/launch.json`. C'est le fichier qui dit à VS Code quel binaire lancer, avec quel débogueur, et avec quels arguments.

Pour le créer : menu **Run → Add Configuration**, puis sélectionnez **C/C++: (gdb) Launch**. VS Code génère un squelette que vous adaptez :

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug config_parser",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/config_parser",
            "args": ["database.conf"],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "miDebuggerPath": "/usr/bin/gdb",
            "setupCommands": [
                {
                    "description": "Pretty-printing STL",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                },
                {
                    "description": "Ignorer sources STL dans step",
                    "text": "skip -gfi /usr/include/c++/*",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "build-debug"
        }
    ]
}
```

Les champs critiques :

| Champ | Rôle |
|---|---|
| `program` | Chemin vers le binaire compilé avec `-g -O0` |
| `args` | Arguments passés au programme (équivalent de `run <args>` dans GDB) |
| `MIMode` | Débogueur backend : `"gdb"` ou `"lldb"` |
| `miDebuggerPath` | Chemin vers l'exécutable GDB |
| `setupCommands` | Commandes GDB exécutées au démarrage (équivalent de `.gdbinit`) |
| `stopAtEntry` | `true` = s'arrêter à `main()` (équivalent de `start`) |
| `preLaunchTask` | Task de build à exécuter avant le lancement |

### Le champ `setupCommands` : votre `.gdbinit` dans VS Code

Le tableau `setupCommands` est l'endroit où vous injectez toute la configuration GDB que vous auriez normalement dans `~/.gdbinit`. Chaque entrée est une commande GDB envoyée au démarrage de la session :

```json
"setupCommands": [
    {
        "description": "Pretty-printing",
        "text": "-enable-pretty-printing",
        "ignoreFailures": true
    },
    {
        "description": "Print format",
        "text": "set print elements 200",
        "ignoreFailures": true
    },
    {
        "description": "Skip STL sources",
        "text": "skip -gfi /usr/include/c++/*",
        "ignoreFailures": true
    },
    {
        "description": "Skip libstdc++ sources",
        "text": "skip -gfi /usr/include/x86_64-linux-gnu/c++/*",
        "ignoreFailures": true
    }
]
```

Le `-enable-pretty-printing` est essentiel : sans lui, les conteneurs STL s'affichent dans le panneau de variables sous leur forme interne (`_M_impl`, `_M_start`, etc.) au lieu de leur contenu lisible.

### La task de build : `tasks.json`

Le champ `preLaunchTask` référence une task définie dans `.vscode/tasks.json`. Cette task recompile automatiquement le projet avant chaque session de débogage — vous êtes certain de déboguer le code actuel :

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "build-debug",
            "type": "shell",
            "command": "cmake",
            "args": [
                "--build", "${workspaceFolder}/build",
                "--config", "Debug",
                "--target", "config_parser"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": ["$gcc"]
        }
    ]
}
```

Avec cette configuration, appuyer sur **F5** déclenche la chaîne complète : compilation → lancement → arrêt sur le premier breakpoint. Pas de commande manuelle.

### Utilisation quotidienne dans VS Code

**Poser un breakpoint** — cliquez dans la marge à gauche du numéro de ligne. Un point rouge apparaît. C'est l'équivalent de `break <fichier>:<ligne>` dans GDB.

**Breakpoint conditionnel** — clic droit sur un breakpoint existant → **Edit Breakpoint** → saisissez l'expression dans le champ **Expression**. Vous pouvez aussi choisir **Hit Count** (équivalent de `ignore`) ou **Log Message** (équivalent de `commands` avec `silent` + `printf` + `continue`).

**Lancer le débogage** — **F5** ou le bouton vert ▶ dans le panneau Run. Le programme démarre et s'arrête au premier breakpoint.

**Navigation** — la barre d'outils du débogueur propose six boutons :

| Bouton | Raccourci | Équivalent GDB |
|---|---|---|
| Continue | F5 | `continue` |
| Step Over | F10 | `next` |
| Step Into | F11 | `step` |
| Step Out | Shift+F11 | `finish` |
| Restart | Ctrl+Shift+F5 | `run` (relance) |
| Stop | Shift+F5 | `kill` + `quit` |

**Inspecter les variables** — le panneau **Variables** (à gauche) affiche automatiquement les variables locales et les arguments de la fonction courante. C'est `info locals` + `info args` combinés, avec un affichage arborescent pour les structures complexes. Vous pouvez déplier un `std::vector` pour voir chaque élément, déplier un objet pour voir ses membres.

**Hover** — survolez n'importe quelle variable dans l'éditeur avec la souris. Une infobulle affiche sa valeur courante. C'est un `print` instantané, sans quitter le code des yeux.

**Watch** — le panneau **Watch** permet d'ajouter des expressions à surveiller en permanence. C'est l'équivalent de `display` dans GDB. Ajoutez `entries.size()`, `line_num`, ou toute expression C++ valide. Les valeurs se mettent à jour à chaque arrêt.

**Call Stack** — le panneau **Call Stack** affiche la pile d'appels complète (`backtrace`). Cliquez sur un frame pour y naviguer — l'éditeur saute au code correspondant et le panneau Variables affiche les variables de ce frame. C'est l'équivalent de `frame <n>` mais instantané et visuel.

### La console de débogage : le pont vers GDB

Le panneau **Debug Console** (accessible via **Ctrl+Shift+Y** ou l'onglet en bas) est un terminal interactif connecté au débogueur backend. Vous pouvez y taper des commandes GDB préfixées par `-exec` :

```
-exec print line_num
-exec info registers
-exec watch entries.size()
-exec set variable line_num = 42
-exec x/16xb &buffer
-exec call dump_entries(entries)
```

C'est votre échappatoire quand l'interface graphique ne suffit pas. Besoin d'un watchpoint hardware ? L'interface VS Code ne les propose pas nativement, mais `-exec watch variable` fonctionne. Besoin d'examiner la mémoire brute ? `-exec x/32xb ptr` est disponible. Besoin de sauvegarder vos breakpoints ? `-exec save breakpoints session.gdb`.

Tout ce que vous avez appris en section 29.1 reste accessible — l'IDE ajoute une couche visuelle, il ne remplace rien.

### Configuration multiple : debug, release, sanitizers

Un projet réel a souvent plusieurs configurations de débogage. Vous pouvez définir plusieurs entrées dans `launch.json` :

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug - Normal",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/debug/config_parser",
            "args": ["database.conf"],
            "MIMode": "gdb",
            "preLaunchTask": "build-debug"
        },
        {
            "name": "Debug - Large file",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/debug/config_parser",
            "args": ["large_config.conf", "--verbose"],
            "MIMode": "gdb",
            "preLaunchTask": "build-debug"
        },
        {
            "name": "Debug - Attach to PID",
            "type": "cppdbg",
            "request": "attach",
            "program": "${workspaceFolder}/build/debug/config_parser",
            "processId": "${command:pickProcess}",
            "MIMode": "gdb"
        }
    ]
}
```

La configuration **Attach to PID** utilise `"request": "attach"` et `"${command:pickProcess}"` — VS Code affiche un sélecteur de processus quand vous lancez cette configuration. C'est l'équivalent graphique de `gdb -p <PID>`.

Vous sélectionnez la configuration active via la liste déroulante dans le panneau Run avant d'appuyer sur F5.

---

## CLion : Débogage intégré natif

### Pourquoi CLion est différent

VS Code est un éditeur généraliste auquel on ajoute des extensions C++. CLion est un IDE conçu exclusivement pour C et C++. La conséquence sur le débogage est significative : CLion intègre nativement GDB et LLDB, détecte automatiquement les projets CMake, et configure le débogage sans fichier JSON à écrire manuellement.

Pour un développeur qui travaille exclusivement en C++, CLion offre une expérience de débogage plus fluide. Pour un DevOps qui utilise aussi Python, Go, Terraform et des scripts shell, VS Code est souvent le choix pragmatique — un seul éditeur pour tout.

### Configuration du débogueur

CLion détecte automatiquement les toolchains installées. La configuration se vérifie dans **Settings → Build, Execution, Deployment → Toolchains** :

```
Toolchain: Default
  CMake:    /usr/bin/cmake    (3.31+)
  C:        /usr/bin/gcc      (GCC 15)
  C++:      /usr/bin/g++      (GCC 15)
  Debugger: /usr/bin/gdb      (GDB 16+)
  Build:    /usr/bin/ninja    (Ninja 1.12)
```

CLion utilise GDB par défaut sous Linux. Vous pouvez basculer sur LLDB dans les paramètres, mais GDB est recommandé pour les raisons exposées en section 29.1 (meilleur support des pretty-printers, documentation plus abondante).

### CMake et profils de build

CLion gère les profils de build CMake dans **Settings → Build, Execution, Deployment → CMake**. Créez au minimum deux profils :

**Debug** — pour le débogage quotidien :
```
Build type: Debug  
CMake options: -DCMAKE_BUILD_TYPE=Debug  
```

**RelWithDebInfo** — pour le profiling (chapitre 31) et le débogage de problèmes qui ne se reproduisent pas en Debug :
```
Build type: RelWithDebInfo  
CMake options: -DCMAKE_BUILD_TYPE=RelWithDebInfo  
```

Le profil actif se sélectionne dans la barre d'outils en haut. CLion recompile automatiquement quand vous changez de profil.

### Utilisation quotidienne dans CLion

**Breakpoints** — identique à VS Code : cliquez dans la marge. CLion ajoute une fonctionnalité que VS Code n'a pas nativement : le **breakpoint dialog** (clic droit sur un breakpoint) qui expose directement la condition, le nombre de passages à ignorer, le log, et les actions automatiques — tout ce que `commands` fait dans GDB, dans une interface unifiée.

**Navigation** — les mêmes raccourcis standards :

| Action | Raccourci CLion | Équivalent GDB |
|---|---|---|
| Debug (lancer) | Shift+F9 | `run` |
| Step Over | F8 | `next` |
| Step Into | F7 | `step` |
| Smart Step Into | Shift+F7 | (sélection visuelle de la fonction cible) |
| Step Out | Shift+F8 | `finish` |
| Resume | F9 | `continue` |
| Run to Cursor | Alt+F9 | `until <ligne>` |

**Smart Step Into** est une fonctionnalité propre à CLion : quand une ligne contient plusieurs appels de fonction, CLion surligne chacun et vous laisse choisir dans lequel entrer. Sur une ligne comme :

```cpp
auto result = transform(parse(read_file(filename)));
```

Un `step` classique entre dans `read_file`. Smart Step Into vous laisse choisir `read_file`, `parse`, ou `transform` — directement avec la souris ou les flèches.

**Run to Cursor** (**Alt+F9**) exécute le programme jusqu'à la ligne où se trouve votre curseur. C'est l'équivalent de `until <ligne>` ou d'un `tbreak` + `continue`, mais sans quitter l'éditeur. Particulièrement utile pour sauter le reste d'une boucle.

**Evaluate Expression** (**Alt+F8**) ouvre une boîte de dialogue où vous pouvez taper n'importe quelle expression C++ et voir son résultat. C'est l'équivalent de `print <expr>` dans GDB, mais avec l'autocomplétion de CLion. Vous pouvez aussi modifier des variables directement dans le panneau Variables — double-cliquez sur une valeur et tapez la nouvelle.

### Le panneau Variables : inspection avancée

Le panneau Variables de CLion affiche les variables locales et les arguments avec un rendu arborescent similaire à VS Code. Deux fonctionnalités supplémentaires méritent d'être mentionnées :

**Inline Values** — CLion affiche les valeurs des variables directement dans l'éditeur, en gris à côté de chaque ligne de code. Vous voyez l'état du programme superposé au code source, sans déplacer votre regard vers un panneau latéral. Cette fonctionnalité est activée par défaut et se révèle remarquablement efficace pour suivre l'évolution des variables dans une boucle.

**Custom Type Renderers** — dans **Settings → Build, Execution, Deployment → Debugger → Data Views → C/C++ Type Renderers**, vous pouvez définir des renderers personnalisés pour vos types métier. Par exemple, pour afficher un `ConfigEntry` comme `"db_host=localhost (L2)"` au lieu de la structure complète, vous écrivez un renderer GDB Python. CLion le charge automatiquement.

### La console GDB intégrée

Comme VS Code, CLion donne accès à la console GDB brute pendant une session de débogage : onglet **GDB** ou **LLDB** dans le panneau de débogage en bas. La différence avec VS Code : pas besoin de préfixer les commandes par `-exec`. Vous tapez directement des commandes GDB :

```
(gdb) watch entries.size()
(gdb) info threads
(gdb) x/16xb &buffer[0]
(gdb) set variable line_num = 99
```

---

## Débogage distant (Remote Debugging)

Dans un contexte DevOps, le programme à déboguer ne tourne pas toujours sur votre machine de développement. Il peut tourner dans un conteneur Docker, sur un serveur distant, ou dans une VM de CI. GDB supporte le débogage distant via `gdbserver`, et les deux IDE s'y intègrent.

### Le principe

Sur la machine cible (serveur, conteneur), vous lancez `gdbserver` qui écoute sur un port réseau :

```bash
# Sur la machine distante
gdbserver :2345 ./config_parser database.conf
```

Sur votre machine de développement, GDB (ou l'IDE) se connecte à ce port et contrôle l'exécution à distance. Le binaire et les sources doivent être disponibles localement pour que le débogueur affiche le code source.

### Configuration VS Code pour le débogage distant

```json
{
    "name": "Debug - Remote",
    "type": "cppdbg",
    "request": "launch",
    "program": "${workspaceFolder}/build/debug/config_parser",
    "MIMode": "gdb",
    "miDebuggerServerAddress": "192.168.1.100:2345",
    "cwd": "${workspaceFolder}",
    "setupCommands": [
        {
            "text": "-enable-pretty-printing",
            "ignoreFailures": true
        }
    ]
}
```

Le champ `miDebuggerServerAddress` indique l'adresse et le port du `gdbserver` distant. Le champ `program` pointe vers le binaire local — il doit être identique à celui déployé sur la machine distante (même commit, mêmes flags de compilation).

### Configuration CLion pour le débogage distant

Dans CLion, créez une configuration **GDB Remote Debug** :

- **GDB Remote Debug** dans la liste des configurations Run/Debug
- **'target remote' args** : `192.168.1.100:2345`
- **Symbol file** : chemin vers le binaire local
- **Sysroot** : chemin vers les bibliothèques du système cible (si différent)

### Débogage dans un conteneur Docker

Le cas le plus fréquent en DevOps : déboguer un programme qui tourne dans un conteneur. Deux approches :

**Approche 1 — gdbserver dans le conteneur.** Ajoutez `gdbserver` à votre image Docker et exposez un port :

```dockerfile
FROM ubuntu:24.04 AS debug  
RUN apt-get update && apt-get install -y gdbserver  
COPY --from=builder /app/config_parser /app/config_parser  
EXPOSE 2345  
CMD ["gdbserver", ":2345", "/app/config_parser", "database.conf"]  
```

```bash
docker run --cap-add=SYS_PTRACE -p 2345:2345 myapp:debug
```

Le `--cap-add=SYS_PTRACE` est nécessaire — Docker bloque `ptrace` par défaut, et GDB en a besoin pour contrôler le processus.

Connectez-vous ensuite depuis VS Code ou CLion comme décrit ci-dessus.

**Approche 2 — DevContainers avec VS Code.** Si vous utilisez les DevContainers (section 2.4.3), le débogage fonctionne directement dans le conteneur sans `gdbserver`. VS Code exécute GDB à l'intérieur du conteneur, et tout est transparent :

```json
// .devcontainer/devcontainer.json
{
    "image": "mcr.microsoft.com/devcontainers/cpp:ubuntu",
    "capAdd": ["SYS_PTRACE"],
    "securityOpt": ["seccomp=unconfined"],
    "customizations": {
        "vscode": {
            "extensions": [
                "ms-vscode.cpptools",
                "ms-vscode.cmake-tools"
            ]
        }
    }
}
```

Le `seccomp=unconfined` et `SYS_PTRACE` sont indispensables pour que GDB fonctionne dans le conteneur. Une fois le DevContainer lancé, F5 débogue comme si vous étiez sur votre machine locale.

---

## Quand l'IDE ne suffit pas

L'interface graphique couvre 90 % des besoins. Les 10 % restants justifient de basculer dans la console GDB intégrée ou dans un terminal :

**Watchpoints hardware.** VS Code ne propose pas de bouton "watch for write" sur une variable. Dans la console : `-exec watch my_variable`.

**Breakpoints avec `commands`.** L'interface de log message de VS Code couvre les cas simples, mais pour un bloc de commandes complexe (affichage conditionnel, modification de variable, `continue`), la console GDB est plus flexible.

**Examen de la mémoire brute.** La commande `x` pour inspecter des octets à une adresse donnée n'a pas d'équivalent graphique dans VS Code (CLion propose un Memory View, mais il est limité).

**Catch events.** `catch throw`, `catch syscall` — aucun des deux IDE ne les expose dans l'interface. Console GDB uniquement.

**Reverse debugging.** GDB supporte l'exécution en arrière (`reverse-next`, `reverse-step`, `reverse-continue`). Aucun IDE ne l'expose nativement. C'est une fonctionnalité avancée qui permet de "rembobiner" l'exécution pour comprendre comment on est arrivé dans un état donné.

**Scripts GDB.** Pour des sessions de débogage automatisées (`source mon_script.gdb`), la console est le seul point d'accès.

La règle pragmatique : commencez dans l'IDE pour le confort visuel. Dès que vous sentez la friction d'une interface qui ne fait pas ce que vous voulez, passez dans la console GDB. Le coût de transition est nul puisque la console est intégrée.

---

## Comparaison VS Code vs CLion pour le débogage

| Critère | VS Code | CLion |
|---|---|---|
| Prix | Gratuit | Payant (licence JetBrains) |
| Configuration | Manuelle (`launch.json`) | Automatique (détection CMake) |
| Débogueur backend | GDB ou LLDB via extension | GDB ou LLDB natif |
| Smart Step Into | Non | Oui (sélection visuelle) |
| Inline Values | Via extension (variable) | Natif et fiable |
| Memory View | Non natif | Oui (basique) |
| Console GDB | Oui (préfixe `-exec`) | Oui (directe) |
| Run to Cursor | Oui (clic droit) | Oui (Alt+F9, plus fluide) |
| Custom Type Renderers | Via pretty-printers GDB | Interface dédiée + pretty-printers |
| Débogage distant | Oui (via `launch.json`) | Oui (configuration dédiée) |
| DevContainers | Natif, excellent | Support limité |
| Performance IDE | Léger (~300 Mo RAM) | Lourd (~1.5 Go RAM) |
| Polyglotte | Oui (extensions multiples) | C/C++ uniquement |

**Recommandation pour un DevOps/SRE** : VS Code. Vous déboguez du C++, mais aussi du Python, du Go, des Dockerfiles, du YAML. Un seul éditeur pour tout, avec un débogage C++ tout à fait capable.

**Recommandation pour un développeur C++ pur** : CLion. L'intégration native, le Smart Step Into, les inline values, et la détection automatique CMake réduisent la friction au strict minimum. L'investissement dans la licence se rentabilise en temps gagné.

**Recommandation universelle** : quel que soit l'IDE, maîtrisez GDB en ligne de commande (section 29.1). L'IDE peut changer, le débogueur reste.

---

> **À retenir** : un IDE de débogage est une interface graphique au-dessus de GDB. Il accélère le travail quotidien — breakpoints visuels, inspection par hover, navigation en un clic — mais il ne remplace pas la compréhension du débogueur sous-jacent. La console GDB intégrée est votre filet de sécurité : tout ce que l'IDE ne sait pas faire, GDB le peut.

⏭️ [Core dumps et post-mortem debugging](/29-debogage/03-core-dumps.md)
