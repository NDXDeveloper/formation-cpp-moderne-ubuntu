# Chapitre 9 — Smart Pointers : Exemples compilables

## Compilation

Tous les exemples se compilent avec GCC 15 en C++23 :

```bash
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o nom_executable nom_fichier.cpp
```

**Flags utilisés :**

- `-std=c++23` : standard C++23 (requis pour `std::print`)
- `-Wall -Wextra -Wpedantic` : warnings maximum
- `-g` : symboles de débogage

**Aucune dépendance externe** : tous les exemples utilisent uniquement la bibliothèque standard et les headers POSIX standard.

---

## Liste des exemples

### 09_raii_vs_raw.cpp

- **Section :** 9.0 — Introduction
- **Fichier source :** README.md
- **Description :** RAII appliqué aux pointeurs — comparaison pointeurs bruts (fuite mémoire si exception) vs `unique_ptr` (libération automatique garantie).
- **Sortie attendue :** Démonstration des deux approches avec exception attrapée, aucune fuite mémoire.

### 09_unique_ptr_bases.cpp

- **Section :** 9.1 — std::unique_ptr : Possession exclusive
- **Fichier source :** 01-unique-ptr.md
- **Description :** Bases de `unique_ptr` — création, `std::move`, `sizeof` (8 octets comme un pointeur brut), polymorphisme (Animal/Chat/Chien), GrosBuffer (1 Mo), composition Voiture/Moteur.
- **Sortie attendue :**
  - `sizeof(int*) = sizeof(unique_ptr<int>) = 8`
  - Polymorphisme : "Miaou", "Wouf"
  - Composition : "Moteur démarre"

### 09_creation_utilisation.cpp

- **Section :** 9.1.1 — Création et utilisation
- **Fichier source :** 01.1-creation-utilisation.md
- **Description :** Création (`make_unique`, construction directe, `nullptr`), tableaux `T[]`, accès (`*`, `->`, `get()`), nullité, `reset()`, `release()`, interaction avec fonctions.
- **Sortie attendue :**
  - Création avec différents types (int, string, Point)
  - `get()` retourne la même adresse
  - `reset()` libère et remplace
  - `release()` rend le pointeur brut (libération manuelle)
  - Factory `creer_widget` + `afficher_widget`

### 09_transfert_propriete.cpp

- **Section :** 9.1.2 — Transfert de propriété avec std::move
- **Fichier source :** 01.2-transfert-propriete.md
- **Description :** Logger (transfert par valeur), factory pattern (Animal), conteneurs (vector, map avec extract), polymorphisme (Shape/Cercle/Rectangle avec calcul d'aires), état après move.
- **Sortie attendue :**
  - Logger : messages avant/après transfert
  - Factory : "Miaou", nullptr pour type inconnu
  - Vector : extraction du dernier élément "Coco"
  - Map : extraction avec `extract()`, taille réduite
  - Shapes : aire cercle(5) ≈ 78.54, aire totale ≈ 48.27
  - Après move : p nul, réaffectation possible

### 09_custom_deleters.cpp

- **Section :** 9.1.3 — Custom deleters
- **Fichier source :** 01.3-custom-deleters.md
- **Description :** Lambda, lambda C++20, pointeur de fonction, foncteur, comparaison des tailles, FileDescriptor RAII, malloc/free, factory générique (UniqueFile, UniquePipe).
- **Sortie attendue :**
  - Écriture/lecture de fichier avec chaque forme de deleter
  - Tailles : défaut = 8, ptr fonction = 16, lambda sans capture = 8, foncteur = 8
  - FileDescriptor : lecture via `open()/read()/close()`
  - malloc/free : "Alloué par malloc"
  - Factory : lecture de `/etc/hostname`, exécution de pipe

### 09_shared_weak_ptr.cpp

- **Section :** 9.2 — std::shared_ptr et std::weak_ptr
- **Fichier source :** 02-shared-weak-ptr.md
- **Description :** Possession partagée, compteur, sizeof (16 octets), cycle de références (memory leak démontré), arbre avec weak_ptr (destruction correcte), lock()/expired().
- **Sortie attendue :**
  - `sizeof(shared_ptr<int>) = 16`
  - Cycle : use_count = 2 pour les deux, aucun destructeur affiché (leak)
  - Arbre : "Destruction de Racine" et "Destruction de Feuille" affichés
  - lock() : accès à la valeur, puis "Ressource expirée" après reset

### 09_comptage_references.cpp

- **Section :** 9.2.1 — Comptage de références
- **Fichier source :** 02.1-comptage-references.md
- **Description :** use_count() à chaque étape, incrémentations (copie, lock), décrémentations (reset, nullptr, scope), opérations neutres (move, get, weak_ptr), cycle de vie complet d'un Sensor.
- **Sortie attendue :**
  - use_count varie de 1 à 5 (incrémentations)
  - move ne change pas le compteur
  - Sensor : "[+] créé" puis "[-] détruit" en fin de scope

### 09_cycles_weak_ptr.cpp

- **Section :** 9.2.2 — Cycles de références et std::weak_ptr
- **Fichier source :** 02.2-cycles-weak-ptr.md
- **Description :** Cycle simple (Personne, leak), résolution avec weak_ptr, cycle triangulaire (3 nœuds, leak), auto-référence (leak), API lock()/expired(), TreeNode avec enable_shared_from_this et chemin(), Worker avec weak_from_this.
- **Sortie attendue :**
  - Cycle : "[+] créé" mais jamais "[-] détruit" (leak intentionnel)
  - Sans cycle : destructeurs affichés correctement
  - TreeNode : "racine/fils", "racine/fils/petit-fils"
  - Worker : traitement puis destruction

### 09_make_unique_shared.cpp

- **Section :** 9.3 — std::make_unique et std::make_shared
- **Fichier source :** 03-make-unique-shared.md
- **Description :** Syntaxe de base (make_unique, make_shared, tableaux), lisibilité, symétrie unique/shared, constructeur privé (Singleton), initializer_list, compromis make_shared + weak_ptr.
- **Sortie attendue :**
  - Valeurs créées correctement (42, "Hello", vector de 10 zéros)
  - Singleton fonctionnel
  - initializer_list : "1 2 3 4 5"
  - Compromis : expired = true après reset

### 09_jamais_new_delete.cpp

- **Section :** 9.4 — Ne jamais utiliser new/delete dans du code moderne
- **Fichier source :** 04-jamais-new-delete.md
- **Description :** Alternatives complètes — stack vs heap, unique_ptr vs new, shared_ptr, vector vs new[], collection polymorphique, custom deleter FILE, Singleton, placement new, Engine avec propriété explicite (unique_ptr, shared_ptr, référence).
- **Sortie attendue :**
  - Widget créé/détruit automatiquement
  - shared_ptr : use_count de 1 à 2 et retour
  - Polymorphisme : "Miaou", "Wouf"
  - Placement new : construction/destruction manuelle
  - Engine : Renderer détruit, Logger survit (use_count = 1)
