# Exemples — Chapitre 5 : Gestion de la Mémoire

Tous les exemples sont compilables avec `g++-15 -std=c++17 -Wall -Wextra` (sauf `07_bugs_memoire.cpp` et `08_stack_overflow_asan.cpp` qui nécessitent des options spéciales).

---

## Liste des exemples

### 01_diagramme_memoire.cpp
- **Section** : 5.1.1 — Diagramme mémoire d'un processus
- **Fichier source** : `01.1-diagramme-memoire.md`
- **Description** : Affiche les adresses des différentes zones mémoire (.text, .rodata, .data, .bss, heap, stack) pour visualiser l'organisation de l'espace d'adressage
- **Sortie attendue** : PID du processus, puis adresses croissantes : `.text` < `.rodata` < `.data` < `.bss` < `heap` << `stack`
- **Comportement attendu** : Les adresses changent à chaque exécution (ASLR) mais l'ordre relatif est constant. Le programme attend une touche Entrée pour permettre l'inspection avec `/proc/<PID>/maps` ou `pmap`

### 02_caracteristiques_stack.cpp
- **Section** : 5.1.2 — Caractéristiques de la Stack
- **Fichier source** : `01.2-caracteristiques-stack.md`
- **Description** : Démonstration du scope et des destructeurs automatiques, ordre de destruction LIFO garanti, et récursion sur un arbre binaire
- **Sortie attendue** : `Alice`, `Dupont`, `Alice` (scope) ; Construction A/B/C puis Destruction C/B/A (LIFO) ; `Somme de l'arbre : 15`
- **Comportement attendu** : Les destructeurs sont appelés dans l'ordre inverse de construction (LIFO) ; la récursion sur un arbre équilibré est sûre car la profondeur est logarithmique

### 03_caracteristiques_heap.cpp
- **Section** : 5.1.3 — Caractéristiques du Heap
- **Fichier source** : `01.3-caracteristiques-heap.md`
- **Description** : Fragmentation mémoire, durée de vie flexible des objets sur le heap, taille dynamique avec `std::vector`, et polymorphisme via `std::unique_ptr`
- **Sortie attendue** : Fragmentation illustrée, `Alice, 30 ans`, carrés `0 1 4 9 16 25 36 49 64 81`, `Chat : Miaou`, `Chien : Wouf`
- **Comportement attendu** : La fragmentation est invisible au programme mais réelle en interne ; `std::vector` gère automatiquement l'allocation heap ; `std::unique_ptr` gère la durée de vie des objets polymorphiques

### 04_allocation_dynamique.cpp
- **Section** : 5.2 — Allocation dynamique : new/delete, new[]/delete[]
- **Fichier source** : `02-allocation-dynamique.md`
- **Description** : Allocation et libération d'objets uniques et de tableaux, classe Connexion avec constructeur/destructeur, tableau de Capteurs, delete sur nullptr, allocation const, std::bad_alloc et nothrow
- **Sortie attendue** : `*q = 42`, ouverture/envoi/fermeture Connexion, températures `18.5 20.3 22.1 19.7 21`, 3 Capteurs construits puis détruits, delete nullptr sans effet, `*pc = 42`, `Allocation échouée : std::bad_alloc`
- **Comportement attendu** : `new` appelle le constructeur, `delete` appelle le destructeur puis libère la mémoire ; `delete` sur `nullptr` est sûr ; `new` lance `std::bad_alloc` en cas d'échec, `new(std::nothrow)` retourne `nullptr`

### 05_arithmetique_pointeurs.cpp
- **Section** : 5.3 — Arithmétique des pointeurs et accès bas niveau
- **Fichier source** : `03-arithmetique-pointeurs.md`
- **Description** : Arithmétique de base (addition, soustraction, distance), équivalence pointeur-tableau, parcours idiomatique avec pointeur past-the-end, pointeurs vers structures (`->`), `void*`, `nullptr`, et les 4 formes de `const` sur les pointeurs
- **Sortie attendue** : `42` (valeur), `10 20 30 50` (arithmétique), `1.1 2.2 3.3` (double), `distance = 4`, `300` (4 formes d'accès), `5 10 15 20 25` (parcours), `3` (structures), `42` et `3.14` (void*), `p1 est nullptr`
- **Comportement attendu** : `p + n` avance de `n × sizeof(type)` octets ; `tab[i]` est équivalent à `*(tab + i)` ; `void*` nécessite un cast pour être déréférencé ; `const` peut s'appliquer au pointeur, à la donnée pointée, ou aux deux

### 06_dangers_memoire.cpp
- **Section** : 5.4 — Dangers : Memory leaks, dangling pointers, double free
- **Fichier source** : `04-dangers-memoire.md`
- **Description** : Illustration des patterns dangereux (fuite par écrasement, classe Buffer sans Rule of Five, invalidation d'itérateurs, propriété ambiguë) — code corrigé pour éviter les UB, les patterns problématiques sont documentés en commentaires
- **Sortie attendue** : Fuite corrigée (`A = 10`, `B = 20`), Buffer construit/détruit proprement, adresse de `nombres[0]` change après réallocation, `Responsable : Alice`
- **Comportement attendu** : La copie de Buffer est interdite (`= delete`) pour éviter le double free ; `push_back` peut invalider les pointeurs vers les éléments d'un vector ; la propriété ambiguë des pointeurs bruts est la source de dangling pointers

### 07_bugs_memoire.cpp
- **Section** : 5.5 / 5.5.1 / 5.5.2 — Programme cobaye pour Valgrind et ASan
- **Fichier source** : 05-outils-detection.md
- **Description** : Programme **volontairement bugué** contenant 4 erreurs mémoire intentionnelles : fuite mémoire (new sans delete), buffer overflow (écriture hors limites), use-after-free (lecture après delete), double free
- **Sortie attendue** : ⚠️ Comportement indéfini — ce programme est conçu pour être analysé par les outils, pas exécuté directement
- **Comportement attendu** : Valgrind détecte les 4 bugs (Invalid write, Invalid read, Invalid free, 400 bytes definitely lost) ; ASan détecte le premier bug rencontré et arrête le programme
- **Compilation spéciale** : voir ci-dessous

### 08_stack_overflow_asan.cpp
- **Section** : 5.5.2 — AddressSanitizer : Détection stack-buffer-overflow
- **Fichier source** : `05.2-addresssanitizer.md`
- **Description** : Programme avec un accès hors limites sur un tableau local (stack), conçu pour démontrer la détection par ASan des buffer overflows sur la stack — une capacité que Valgrind ne possède pas
- **Sortie attendue** : ASan signale `stack-buffer-overflow` avec identification de la variable `tableau` et de la ligne fautive
- **Comportement attendu** : ASan place des redzones autour des variables locales et détecte tout accès hors limites, y compris sur la stack
- **Compilation spéciale** : voir ci-dessous

---

## Compilation

```bash
# Compiler les exemples standards (C++17)
for f in 01_diagramme_memoire.cpp 02_caracteristiques_stack.cpp \
         03_caracteristiques_heap.cpp 04_allocation_dynamique.cpp \
         05_arithmetique_pointeurs.cpp 06_dangers_memoire.cpp; do
    echo "Compilation de $f..."
    g++-15 -std=c++17 -Wall -Wextra -o "${f%.cpp}" "$f" || echo "ECHEC: $f"
done

# Compiler le programme cobaye pour Valgrind (avec symboles de débogage)
g++-15 -std=c++17 -g -O0 -o 07_bugs_memoire 07_bugs_memoire.cpp

# Compiler le programme cobaye pour ASan
g++-15 -std=c++17 -g -O1 -fsanitize=address -o 08_stack_asan 08_stack_overflow_asan.cpp

# Compiler le programme cobaye avec ASan (alternative)
g++-15 -std=c++17 -g -O0 -fsanitize=address -o 07_bugs_asan 07_bugs_memoire.cpp
```

## Utilisation des outils de détection

```bash
# Analyser avec Valgrind
valgrind --leak-check=full ./07_bugs_memoire

# Analyser avec AddressSanitizer
./07_bugs_asan

# Détecter le stack-buffer-overflow avec ASan
./08_stack_asan
```

## Nettoyage

```bash
# Supprimer les binaires
rm -f 01_diagramme_memoire 02_caracteristiques_stack 03_caracteristiques_heap \
      04_allocation_dynamique 05_arithmetique_pointeurs 06_dangers_memoire \
      07_bugs_memoire 07_bugs_asan 08_stack_asan 08_stack_overflow_asan
```
