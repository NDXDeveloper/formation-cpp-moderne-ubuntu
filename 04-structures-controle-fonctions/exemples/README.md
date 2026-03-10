# Exemples — Chapitre 4 : Structures de Contrôle et Fonctions

Tous les exemples sont compilables avec `g++-15 -std=c++17 -Wall -Wextra` (sauf `03_4_par_pointeur.cpp` qui utilise `std::span` et nécessite `-std=c++20`).

---

## Liste des exemples

### 01_1_if_else.cpp
- **Section** : 4.1.1 — `if`, `else if`, `else` et `if constexpr`
- **Fichier source** : `01.1-if-else.md`
- **Description** : Conditionnelle de base (`if`/`else`), chaînage `else if` avec score/mention, `if constexpr` avec templates et traits de type, `if constexpr` avec `sizeof`
- **Sortie attendue** : `Fièvre détectée`, `Mention : Assez bien`, types identifiés (Entier/Flottant/Autre), `sizeof(int) == 4`
- **Comportement attendu** : `if constexpr` élimine les branches non pertinentes à la compilation ; `afficher(42)` n'instancie pas la branche `.size()`

### 01_2_switch.cpp
- **Section** : 4.1.2 — `switch` et `switch` avec initialisation (C++17)
- **Fichier source** : `01.2-switch.md`
- **Description** : Switch de base sur jours, `enum class` avec switch, fall-through démontré et documenté avec `[[fallthrough]]`, regroupement de cas (voyelles), switch avec initialisation C++17 (`std::stoi`), portée des variables avec accolades
- **Sortie attendue** : `Mercredi`, `Vert`, fall-through `deux/trois/autre`, initialisation cumulative `standard/minimale`, `Consonne (ou autre)`, `Not Found`, `case 1: val=42`
- **Comportement attendu** : Sans `break`, l'exécution tombe dans les cases suivants ; `[[fallthrough]]` supprime le warning ; switch avec initialisation limite la portée de la variable

### 01_3_range_based_for.cpp
- **Section** : 4.1.3 — Range-based `for` loop
- **Fichier source** : `01.3-range-based-for.md`
- **Description** : Boucles classiques (index, itérateurs) vs range-based for, trois formes de déclaration (valeur, `auto&`, `const auto&`), `std::map` avec structured bindings C++17, tableaux C natifs, `std::initializer_list`, itération sur `std::string`, type personnalisé `IntRange`
- **Sortie attendue** : Fruits affichés, scores modifiés `{77, 90, 96, 73}`, map alphabétique, `10 20 30 40 50`, `1 2 3 5 8 13`, `B o n j o u r`, `1 2 3 4 5`
- **Comportement attendu** : `auto` copie l'élément, `auto&` modifie en place, `const auto&` lecture sans copie ; `IntRange` fournit `begin()`/`end()` pour être compatible

### 02_declaration_definition.cpp
- **Section** : 4.2 — Déclaration et définition de fonctions
- **Fichier source** : `02-declaration-definition.md`
- **Description** : Anatomie d'une fonction, `void` et return anticipé, forward declaration, fibonacci itératif, test de primalité, `auto` return type C++14, trailing return type, `[[nodiscard]]`, `constexpr` et `inline`
- **Sortie attendue** : `Moyenne = 15`, `Fib(10) = 55`, `17 premier ? true`, `Bonjour, Alice`, `additionner(3, 4.5) = 7.5`, `checksum = 294`, `Aire cercle(r=5) ≈ 78.54`, `helper() = 42`
- **Comportement attendu** : La forward declaration permet d'appeler `carre` avant sa définition ; `[[nodiscard]]` émet un warning si le résultat est ignoré ; `constexpr` évalue à la compilation (`static_assert` passe)

### 03_passage_parametres.cpp
- **Section** : 4.3 — Passage de paramètres
- **Fichier source** : `03-passage-parametres.md`
- **Description** : Démonstration comparative des quatre modes de passage (valeur, référence, référence constante, pointeur) sur la même variable
- **Sortie attendue** : `a=0` (valeur, copie), `b=99` (référence), `c=0` (const ref), `d=99` (pointeur)
- **Comportement attendu** : Seuls le passage par référence et par pointeur modifient la variable originale

### 03_1_par_valeur.cpp
- **Section** : 4.3.1 — Passage par valeur
- **Fichier source** : `03.1-par-valeur.md`
- **Description** : Copie indépendante (`doubler`), sink parameter (`normaliser`), copy elision/RVO, slicing polymorphique (`Animal`/`Chien`), `const` sur paramètre par valeur
- **Sortie attendue** : `n = 5` après l'appel, `hello world` (normalisation), `Hello, World!` (RVO), `...` (slicing) puis `Woof!` (polymorphisme correct), `carre(7) = 49`
- **Comportement attendu** : Le slicing tronque l'objet dérivé ; le passage par `const Animal&` préserve le polymorphisme ; RVO évite la copie du retour

### 03_2_par_reference.cpp
- **Section** : 4.3.2 — Passage par référence (`&`)
- **Fichier source** : `03.2-par-reference.md`
- **Description** : Alias vers l'original, incrémentation, modification de conteneur, swap, retour de valeurs multiples (ancien style et structured bindings C++17), retour de référence sûr (élément de vecteur, variable statique)
- **Sortie attendue** : `n = 10`, `visites = 3`, `4 éléments`, `x=2 y=1`, `3 + 0.14` (deux fois), `vec[0] = 99`, `count = 3`
- **Comportement attendu** : La modification via référence affecte directement l'original ; le compteur `static` persiste entre les appels

### 03_3_par_reference_const.cpp
- **Section** : 4.3.3 — Passage par référence constante (`const &`)
- **Fichier source** : `03.3-par-reference-const.md`
- **Description** : Lecture sans copie, acceptation des temporaires (lvalue, littéral, rvalue, expression), `operator<<` pour `Point`, fonctions utilitaires (`contient`, `distance`)
- **Sortie attendue** : `Bonjour`, longueurs des chaînes, `Point : (3, 4)`, `contient = true`, `distance = 5`
- **Comportement attendu** : `const &` accepte les temporaires grâce à la prolongation de durée de vie ; aucune copie n'est effectuée

### 03_4_par_pointeur.cpp
- **Section** : 4.3.4 — Passage par pointeur (`*`)
- **Fichier source** : `03.4-par-pointeur.md`
- **Description** : Déréférencement, paramètre optionnel (`nullptr`), tableaux/buffers avec pointeur et `std::span` (C++20), arbre binaire avec parcours infixe, `nullptr` vs surcharge, vérification de `nullptr`, `std::unique_ptr`, pointeur `const` (4 formes)
- **Sortie attendue** : `n = 10`, enregistrement avec/sans commentaire, carrés `0 1 4 9 16`, arbre `1 2 3 4 5`, `f(int)` puis `f(int*)`, timeout/erreur, `42`, pointeurs const
- **Comportement attendu** : `nullptr` appelle `f(int*)` sans ambiguïté ; `std::span` encapsule pointeur et taille ; le parcours infixe s'arrête sur `nullptr`
- **Compilation** : Nécessite `-std=c++20` pour `std::span`

### 04_surcharge_fonctions.cpp
- **Section** : 4.4 — Surcharge de fonctions (Function Overloading)
- **Fichier source** : `04-surcharge-fonctions.md`
- **Description** : Surcharge de base (`afficher`), résolution avec promotions (`char→int`, `float→double`), conversion utilisateur (`Distance`), `explicit`, `nullptr` vs `0`, portée et masquage avec `using`, promotion `bool`
- **Sortie attendue** : Types identifiés, promotions correctes (`char→int`, `float→double`), `3.14 m` (conversion utilisateur), `f(int)` pour `0` et `f(int*)` pour `nullptr`, portée résolue avec `using`, `f(bool)` pour `true`
- **Comportement attendu** : La promotion est préférée à la conversion standard ; `explicit` interdit la conversion implicite ; `using` rend les surcharges de la portée externe visibles

### 05_valeurs_defaut_inline.cpp
- **Section** : 4.5 — Valeurs par défaut et fonctions `inline`
- **Fichier source** : `05-valeurs-defaut-inline.md`
- **Description** : Paramètres par défaut (`connecter`, `formater_date`, `puissance`), `inline`, `constexpr` (implicitement inline), variables `inline constexpr` C++17, macros vs fonctions inline, méthode de classe implicitement inline
- **Sortie attendue** : URLs avec ports/SSL, dates formatées, `puissance(5)=25`, `puissance(2,10)=1024`, `carre(7)=49`, `5!=120`, `MAX_CONNECTIONS=100`, `Aire=20`
- **Comportement attendu** : Les défauts sont appliqués de droite à gauche ; `constexpr` est évalué à la compilation (`static_assert` passe) ; `inline` permet la définition dans les en-têtes

---

## Compilation

```bash
# Compiler un exemple (C++17)
g++-15 -std=c++17 -Wall -Wextra -o nom_exemple nom_exemple.cpp

# Compiler 03_4_par_pointeur (C++20 requis pour std::span)
g++-15 -std=c++20 -Wall -Wextra -o 03_4_par_pointeur 03_4_par_pointeur.cpp

# Compiler tous les exemples
for f in *.cpp; do
    std=""
    if echo "$f" | grep -q "03_4"; then std="-std=c++20"; else std="-std=c++17"; fi
    echo "Compilation de $f..."
    g++-15 $std -Wall -Wextra -o "${f%.cpp}" "$f" || echo "ECHEC: $f"
done
```

## Nettoyage

```bash
# Supprimer les binaires
rm -f 01_1_if_else 01_2_switch 01_3_range_based_for 02_declaration_definition \
      03_passage_parametres 03_1_par_valeur 03_2_par_reference \
      03_3_par_reference_const 03_4_par_pointeur 04_surcharge_fonctions \
      05_valeurs_defaut_inline
```
