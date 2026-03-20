# Exemples du Chapitre 37 — Dockerisation d'Applications C++

Projet C++ minimal avec deux Dockerfiles illustrant les techniques du chapitre.

## Prérequis

```bash
docker --version    # Docker 24+  
g++-15 --version    # GCC 15 (pour compilation locale)  
```

---

## Structure

```
exemples/
├── CMakeLists.txt
├── .dockerignore
├── include/myapp/app.hpp
├── src/main.cpp
├── src/app.cpp
├── Dockerfile.ubuntu        # Multi-stage Ubuntu (section 37.2)
├── Dockerfile.static        # Statique + scratch (section 37.5)
└── README.md
```

---

## Dockerfile.ubuntu (section 37.2)

| | |
|---|---|
| **Sections** | 37.2, 37.2.1, 37.2.2 |
| **Fichiers .md** | `02-multi-stage-builds.md`, `02.1-stage-compilation.md`, `02.2-stage-execution.md` |
| **Description** | Multi-stage : build sur Ubuntu 24.04, runtime Ubuntu 24.04 minimal, utilisateur non-root |

### Build et exécution

```bash
docker build -f Dockerfile.ubuntu -t ch37-ubuntu .  
docker run --rm ch37-ubuntu  
```

### Sortie attendue

```
Hello from C++ in Docker!
```

### Caractéristiques vérifiées

- **Taille** : ~78 MB (Ubuntu runtime + binaire)
- **Utilisateur** : non-root (appuser, UID 10001)
- **Multi-stage** : toolchain absente de l'image finale

---

## Dockerfile.static (section 37.5)

| | |
|---|---|
| **Section** | 37.5 |
| **Fichier .md** | `05-distroless.md` |
| **Description** | Build statique (`-static`) + image `scratch` — aucune dépendance runtime, image ultra-minimale |

### Build et exécution

```bash
docker build -f Dockerfile.static -t ch37-static .  
docker run --rm ch37-static  
```

### Sortie attendue

```
Hello from C++ in Docker!
```

### Caractéristiques vérifiées

- **Taille** : ~2.3 MB (binaire statique seul)
- **Image** : `scratch` — aucun shell, aucun gestionnaire de paquets
- **Linkage** : entièrement statique (`-static`)

---

## Compilation locale (sans Docker)

```bash
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-15  
cmake --build build  
./build/myapp
```

---

## Nettoyage

```bash
# Supprimer les images Docker
docker rmi ch37-ubuntu ch37-static 2>/dev/null

# Supprimer le build local
rm -rf build
```
