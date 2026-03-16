# Exemples du Chapitre 31 — Profiling de Performance

Ce répertoire contient un programme cible conçu pour être profilé avec les outils du chapitre 31 : `perf`, `gprof`, flamegraphs et Hotspot.

## Prérequis

```bash
g++-15 --version        # GCC 15  
perf version            # perf (linux-tools)  
gprof --version         # GNU profiler (inclus avec GCC)  
```

### Configuration de `perf` (si nécessaire)

Par défaut sur Ubuntu, `perf_event_paranoid` peut être trop restrictif :

```bash
# Vérifier
cat /proc/sys/kernel/perf_event_paranoid

# Autoriser perf pour l'utilisateur courant (temporaire)
sudo sysctl kernel.perf_event_paranoid=0

# Permanent
echo 'kernel.perf_event_paranoid=0' | sudo tee -a /etc/sysctl.d/99-perf.conf  
sudo sysctl --system  
```

---

## 01\_benchmark\_target.cpp

| | |
|---|---|
| **Sections** | 31.1, 31.2, 31.3, 31.4 |
| **Fichiers .md** | `01-perf.md`, `02-gprof.md`, `03-flamegraphs.md`, `04-hotspot.md` |
| **Description** | Programme avec trois hotspots identifiables : tri intensif (`tri_intensif`), copie de strings (`copie_strings`), et somme (`somme`). |

**Sortie attendue :**
```
Somme: 1408652220, Premier: -2147479173, Dernier: 2147483211
```

---

### Workflow perf (section 31.1)

```bash
# Compilation pour perf (optimisé + debug + frame pointers)
g++-15 -std=c++23 -O2 -g -fno-omit-frame-pointer -o bench_perf 01_benchmark_target.cpp

# Enregistrement du profil
perf record --call-graph fp -F 4000 ./bench_perf

# Analyse interactive
perf report

# Ou en mode texte
perf report --stdio --no-children | head -20

# Compteurs matériels (IPC, cache misses, branches)
perf stat ./bench_perf
```

**Comportement attendu :** `perf report` montre `tri_intensif` et `copie_strings` comme hotspots principaux. `perf stat` affiche les compteurs IPC, cache misses, etc.

---

### Workflow gprof (section 31.2)

```bash
# Compilation avec instrumentation
g++-15 -std=c++23 -O2 -pg -g -o bench_gprof 01_benchmark_target.cpp

# Exécution (génère gmon.out)
./bench_gprof

# Rapport flat profile
gprof bench_gprof gmon.out | head -20

# Rapport call graph
gprof bench_gprof gmon.out | less
```

**Comportement attendu :** `gprof` identifie `tri_intensif` et `copie_strings` avec le nombre exact d'appels (1 chacun) et le temps passé.

---

### Workflow flamegraph (section 31.3)

```bash
# Prérequis : cloner les scripts de Brendan Gregg
git clone https://github.com/brendangregg/FlameGraph.git /tmp/FlameGraph

# Enregistrer, exporter, générer
perf record --call-graph fp -F 4000 ./bench_perf  
perf script > profil_brut.txt  
cat profil_brut.txt | /tmp/FlameGraph/stackcollapse-perf.pl | /tmp/FlameGraph/flamegraph.pl > profil.svg  

# Ouvrir dans le navigateur
xdg-open profil.svg
```

**Comportement attendu :** Le flamegraph montre `tri_intensif` et `copie_strings` comme les « flammes » les plus larges.

---

### Workflow Hotspot (section 31.4)

```bash
# Prérequis
sudo apt install hotspot

# Enregistrer et ouvrir
perf record --call-graph fp -F 4000 ./bench_perf  
hotspot perf.data  
```

---

## Nettoyage

```bash
rm -f bench_perf bench_gprof gmon.out perf.data perf.data.old profil_brut.txt profil.svg
```
