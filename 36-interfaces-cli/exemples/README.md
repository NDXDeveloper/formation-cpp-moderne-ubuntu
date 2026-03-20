# Exemples du Chapitre 36 — Interfaces en Ligne de Commande Modernes

Projet CMake intégrant CLI11, argparse et fmt via FetchContent.

## Prérequis

```bash
g++-15 --version    # GCC 15  
cmake --version     # CMake 3.20+  
ninja --version     # Ninja  
```

## Compilation

```bash
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-15  
cmake --build build  
```

---

## Exemples

### 01\_cli11\_deptool.cpp (section 36.1)

| **Fichier .md** | `01-cli11.md` |
|---|---|
| **Description** | Sous-commandes install/list, validation Range, aide auto |

```bash
./build/deptool install boost --version 1.87 -j 8 -s
./build/deptool list --outdated
./build/deptool --help
```

### 02\_cli11\_httpcheck.cpp (section 36.1.1)

| **Fichier .md** | `01.1-installation.md` |
|---|---|
| **Description** | Options, flags exclusifs, formats json/text, headers |

```bash
./build/httpcheck https://example.com -v
./build/httpcheck https://example.com -f json
```

### 03\_cli11\_cbox.cpp (section 36.1.2)

| **Fichier .md** | `01.2-options-flags.md` |
|---|---|
| **Description** | Sous-commandes run/ps/stop, alias, trailing args |

```bash
./build/cbox run ubuntu -n test -d --rm
./build/cbox ps --all -f json
./build/cbox stop web1 web2 --force
```

### 04\_argparse\_wcount.cpp (section 36.2)

| **Fichier .md** | `02-argparse.md` |
|---|---|
| **Description** | Word count style Python — arguments positionnels, flags |

```bash
echo -e "hello world\nfoo bar" > /tmp/test.txt
./build/wcount /tmp/test.txt
./build/wcount /tmp/test.txt -l
```

### 05\_fmt\_couleurs.cpp (section 36.3)

| **Fichier .md** | `03-fmt.md`, `03.2-couleurs-styles.md` |
|---|---|
| **Description** | Couleurs et styles terminaux avec {fmt} |

```bash
./build/fmt_demo
```

### 06\_tty\_detection.cpp (section 36.4)

| **Fichier .md** | `04-couleurs-tty.md` |
|---|---|
| **Description** | Détection TTY avec isatty() |

```bash
./build/tty_detect           # depuis un terminal
echo "" | ./build/tty_detect  # via pipe
```

---

## Nettoyage

```bash
rm -rf build
```
