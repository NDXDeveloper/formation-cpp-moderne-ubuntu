# Exemples du Chapitre 30 — Analyse Mémoire

Ce répertoire contient les exemples pratiques du chapitre 30 — programmes intentionnellement buggés pour être analysés avec Valgrind (Memcheck et Massif).

## Prérequis

```bash
g++-15 --version       # GCC 15  
valgrind --version     # Valgrind 3.22+  
```

Si Valgrind n'est pas installé : `sudo apt install valgrind`

---

## Liste des exemples

### 01\_fuite\_simple.cpp

| | |
|---|---|
| **Section** | 30.1 |
| **Fichier .md** | `01-valgrind.md` |
| **Description** | Fuite mémoire simple — `new int[100]` sans `delete[]`. |

**Compilation et exécution :**
```bash
g++-15 -std=c++23 -g -O0 -o fuite_simple 01_fuite_simple.cpp  
valgrind --leak-check=full ./fuite_simple  
```

**Comportement attendu :** Valgrind détecte `definitely lost: 400 bytes in 1 blocks`.

---

### 02\_fuites\_multiples.cpp

| | |
|---|---|
| **Section** | 30.1.1 |
| **Fichier .md** | `01.1-memcheck.md` |
| **Description** | Fuites multiples — classe `Config` sans destructeur (fuite de `hostname`) + buffers `traiter_requete()` non libérés. |

**Compilation et exécution :**
```bash
g++-15 -std=c++23 -g -O0 -o fuites_multiples 02_fuites_multiples.cpp  
valgrind --leak-check=full --show-leak-kinds=all ./fuites_multiples  
```

**Comportement attendu :**
- `20 bytes in 1 blocks are definitely lost` (hostname)
- `3,072 bytes in 3 blocks are definitely lost` (3 buffers de 1024 octets)
- Total : `definitely lost: 3,092 bytes in 4 blocks`

---

### 03\_cycle\_shared\_ptr.cpp

| | |
|---|---|
| **Section** | 30.1.1 |
| **Fichier .md** | `01.1-memcheck.md` |
| **Description** | Fuite par cycle de `std::shared_ptr` — deux nœuds se référencent mutuellement. |

**Compilation et exécution :**
```bash
g++-15 -std=c++23 -g -O0 -o cycle_shared 03_cycle_shared_ptr.cpp  
valgrind --leak-check=full --show-leak-kinds=all ./cycle_shared  
```

**Comportement attendu :**
- `definitely lost: 64 bytes in 1 blocks`
- `indirectly lost: 64 bytes in 1 blocks`
- La solution est de remplacer un des `shared_ptr` par un `weak_ptr`

---

### 04\_erreurs\_memoire.cpp

| | |
|---|---|
| **Section** | 30.1 |
| **Fichier .md** | `01-valgrind.md` |
| **Description** | Quatre types d'erreurs Memcheck : use-after-free, valeur non initialisée, mismatch new/delete, double-free. Décommentez un test à la fois dans `main()`. |

**Compilation et exécution :**
```bash
g++-15 -std=c++23 -g -O0 -o erreurs_memoire 04_erreurs_memoire.cpp  
valgrind ./erreurs_memoire  
```

**Comportement attendu (use-after-free par défaut) :** Valgrind détecte `Invalid write of size 4`.

Pour tester les autres erreurs, modifiez `main()` pour appeler `test_uninit()`, `test_mismatch()` ou `test_double_free()`.

---

### 05\_massif\_demo.cpp

| | |
|---|---|
| **Section** | 30.2 |
| **Fichier .md** | `02-massif.md` |
| **Description** | Programme avec consommation mémoire croissante (cache non borné) pour démontrer le profiling avec Massif. |

**Compilation et exécution :**
```bash
g++-15 -std=c++23 -g -O0 -o massif_demo 05_massif_demo.cpp

# Profiling avec Massif
valgrind --tool=massif ./massif_demo

# Visualisation du profil
ms_print massif.out.*

# Visualisation graphique (optionnel)
# sudo apt install massif-visualizer
# massif-visualizer massif.out.*
```

**Sortie attendue :**
```
Phase 1 : 1000 insertions  
Phase 2 : 2000 insertions supplémentaires  
Cache final : 3000 entrées  
```

**Comportement attendu :** `ms_print` affiche un graphique ASCII montrant la croissance linéaire de la consommation mémoire (~830 KB au pic) avec l'arbre d'allocation identifiant `inserer_cache()` comme site dominant.

---

## Nettoyage

```bash
rm -f fuite_simple fuites_multiples cycle_shared erreurs_memoire massif_demo massif.out.*
```
