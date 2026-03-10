# 📘 Table des Matières - Maîtriser C++ Moderne sur Ubuntu
## Formation Complète Développeurs & DevOps — Débutant → Expert
### *Édition 2026 — Cloud Native & System Programming*


---

## **[PARTIE I : FONDATIONS](/partie-01-fondations.md)**

---

## **[Module 1 : L'Environnement de Développement sur Linux](/module-01-environnement-developpement-linux.md)** *(Niveau Débutant)*

### 1. [Introduction au C++ et à l'écosystème Linux](/01-introduction-cpp-linux/README.md)
- 1.1 [Histoire et évolution du C++ (C++98 → C++26)](/01-introduction-cpp-linux/01-histoire-evolution-cpp.md)
- 1.2 [Pourquoi C++ pour le DevOps et le System Programming](/01-introduction-cpp-linux/02-pourquoi-cpp-devops.md)
- 1.3 [Le cycle de compilation : Préprocesseur → Compilateur → Assembleur → Linker](/01-introduction-cpp-linux/03-cycle-compilation.md)
    - 1.3.1 [Le préprocesseur : #include, #define, macros](/01-introduction-cpp-linux/03.1-preprocesseur.md)
    - 1.3.2 [La compilation : Génération du code objet](/01-introduction-cpp-linux/03.2-compilation.md)
    - 1.3.3 [L'édition de liens : Résolution des symboles](/01-introduction-cpp-linux/03.3-edition-liens.md)
- 1.4 [Anatomie d'un exécutable ELF sur Linux](/01-introduction-cpp-linux/04-anatomie-elf.md)
    - 1.4.1 [Structure du format ELF (headers, sections, segments)](/01-introduction-cpp-linux/04.1-structure-elf.md)
    - 1.4.2 [Inspection avec readelf et objdump](/01-introduction-cpp-linux/04.2-inspection-elf.md)

### 2. [Mise en place de la Toolchain sur Ubuntu](/02-toolchain-ubuntu/README.md)
- 2.1 [Installation des compilateurs : GCC (g++) et LLVM (clang++)](/02-toolchain-ubuntu/01-installation-compilateurs.md)
    - 2.1.1 [Gestion des versions avec update-alternatives](/02-toolchain-ubuntu/01.1-gestion-versions.md)
    - 2.1.2 [Comparaison GCC vs Clang : Avantages et inconvénients](/02-toolchain-ubuntu/01.2-gcc-vs-clang.md)
    - **2.1.3 [État 2026 : GCC 15 et Clang 20 — Nouveautés et support C++26](/02-toolchain-ubuntu/01.3-etat-compilateurs-2026.md)** 🔥 
- 2.2 [Les outils essentiels : build-essential, gdb, make, ninja-build, cmake](/02-toolchain-ubuntu/02-outils-essentiels.md)
- **2.3 [Accélération de compilation : ccache (Compiler Cache)](/02-toolchain-ubuntu/03-ccache.md)** ⭐
    - 2.3.1 [Installation et configuration de ccache](/02-toolchain-ubuntu/03.1-installation-ccache.md)
    - 2.3.2 [Intégration avec CMake et Makefiles](/02-toolchain-ubuntu/03.2-integration-cmake-make.md)
    - 2.3.3 [Statistiques et monitoring du cache](/02-toolchain-ubuntu/03.3-statistiques-cache.md)
- 2.4 [Configuration de l'IDE : VS Code, CLion, Vim/Neovim](/02-toolchain-ubuntu/04-configuration-ide.md)
    - 2.4.1 [Extensions VS Code essentielles (C/C++, CMake Tools, clangd)](/02-toolchain-ubuntu/04.1-extensions-vscode.md)
    - 2.4.2 [Configuration du debugging intégré](/02-toolchain-ubuntu/04.2-configuration-debugging.md)
    - 2.4.3 [DevContainers : Environnement reproductible](/02-toolchain-ubuntu/04.3-devcontainers.md)
    - **2.4.4 [IA-assisted tooling : Copilot, Clangd AI et completion intelligente (2026)](/02-toolchain-ubuntu/04.4-ia-assisted-tooling.md)** ⭐ 
- 2.5 [Premier programme : Compilation manuelle et analyse](/02-toolchain-ubuntu/05-premier-programme.md)
    - 2.5.1 [Compilation étape par étape (g++ -E, -S, -c)](/02-toolchain-ubuntu/05.1-compilation-etapes.md)
    - 2.5.2 [Inspection des binaires (nm, objdump, ldd)](/02-toolchain-ubuntu/05.2-inspection-binaires.md)
    - 2.5.3 [Dépendances dynamiques et résolution](/02-toolchain-ubuntu/05.3-dependances-dynamiques.md)
- 2.6 [Options de compilation critiques](/02-toolchain-ubuntu/06-options-compilation.md)
    - 2.6.1 [Warnings : -Wall, -Wextra, -Wpedantic, -Werror](/02-toolchain-ubuntu/06.1-warnings.md)
    - 2.6.2 [Optimisation : -O0, -O2, -O3, -Os](/02-toolchain-ubuntu/06.2-optimisation.md)
    - 2.6.3 [Debug : -g, -ggdb3](/02-toolchain-ubuntu/06.3-debug.md)
    - 2.6.4 [Standard : -std=c++17, -std=c++20, -std=c++23, -std=c++26](/02-toolchain-ubuntu/06.4-standard.md)
- **2.7 [Introduction à std::print (C++23) : Le nouveau standard d'affichage](/02-toolchain-ubuntu/07-std-print.md)** ⭐
    - 2.7.1 [Syntaxe et comparaison avec std::cout et printf](/02-toolchain-ubuntu/07.1-syntaxe-comparaison.md)
    - 2.7.2 [Formatage type-safe et performant](/02-toolchain-ubuntu/07.2-formatage-type-safe.md)
    - 2.7.3 [État du support compilateur (GCC 15+, Clang 19+)](/02-toolchain-ubuntu/07.3-support-compilateur.md) 

> 💡 *La section 2.7 est une prise en main rapide de `std::print` pour les débutants. La couverture approfondie du système de formatage se trouve en **section 12.7** (std::print et std::format).*

---

## **[Module 2 : Les Fondamentaux du Langage](/module-02-fondamentaux-langage.md)** *(Niveau Débutant)*

### 3. [Types, Variables et Opérateurs](/03-types-variables-operateurs/README.md)
- 3.1 [Le typage statique fort et l'inférence de type](/03-types-variables-operateurs/01-typage-statique-inference.md)
    - 3.1.1 [auto : Déduction automatique du type](/03-types-variables-operateurs/01.1-auto.md)
    - 3.1.2 [decltype : Extraction du type d'une expression](/03-types-variables-operateurs/01.2-decltype.md)
    - 3.1.3 [Quand utiliser auto vs type explicite](/03-types-variables-operateurs/01.3-auto-vs-explicite.md)
- 3.2 [Types primitifs, tailles et représentation mémoire](/03-types-variables-operateurs/02-types-primitifs.md)
    - 3.2.1 [Entiers : int, long, short, int32_t, int64_t](/03-types-variables-operateurs/02.1-entiers.md)
    - 3.2.2 [Flottants : float, double, long double](/03-types-variables-operateurs/02.2-flottants.md)
    - 3.2.3 [sizeof et alignof : Taille et alignement](/03-types-variables-operateurs/02.3-sizeof-alignof.md)
- 3.3 [Conversion de types : Cast implicite vs explicite](/03-types-variables-operateurs/03-conversion-types.md)
    - 3.3.1 [static_cast : Conversions sûres](/03-types-variables-operateurs/03.1-static-cast.md)
    - 3.3.2 [reinterpret_cast : Réinterprétation mémoire](/03-types-variables-operateurs/03.2-reinterpret-cast.md)
    - 3.3.3 [const_cast : Manipulation de const](/03-types-variables-operateurs/03.3-const-cast.md)
    - 3.3.4 [dynamic_cast : Cast polymorphique](/03-types-variables-operateurs/03.4-dynamic-cast.md)
- 3.4 [Portée des variables (Scope) et durée de vie (Lifetime)](/03-types-variables-operateurs/04-portee-duree-vie.md)
- 3.5 [const, constexpr et consteval (C++20)](/03-types-variables-operateurs/05-const-constexpr-consteval.md)
    - 3.5.1 [const : Immutabilité à l'exécution](/03-types-variables-operateurs/05.1-const.md)
    - 3.5.2 [constexpr : Calcul à la compilation](/03-types-variables-operateurs/05.2-constexpr.md)
    - 3.5.3 [consteval : Fonctions obligatoirement compile-time](/03-types-variables-operateurs/05.3-consteval.md)

### 4. [Structures de Contrôle et Fonctions](/04-structures-controle-fonctions/README.md)
- 4.1 [Conditionnelles et boucles](/04-structures-controle-fonctions/01-conditionnelles-boucles.md)
    - 4.1.1 [if, else if, else et if constexpr](/04-structures-controle-fonctions/01.1-if-else.md)
    - 4.1.2 [switch et switch avec initialisation (C++17)](/04-structures-controle-fonctions/01.2-switch.md)
    - 4.1.3 [Range-based for loop](/04-structures-controle-fonctions/01.3-range-based-for.md)
- 4.2 [Déclaration et définition de fonctions](/04-structures-controle-fonctions/02-declaration-definition.md)
- 4.3 [Passage de paramètres](/04-structures-controle-fonctions/03-passage-parametres.md)
    - 4.3.1 [Par valeur](/04-structures-controle-fonctions/03.1-par-valeur.md)
    - 4.3.2 [Par référence (&)](/04-structures-controle-fonctions/03.2-par-reference.md)
    - 4.3.3 [Par référence constante (const &)](/04-structures-controle-fonctions/03.3-par-reference-const.md)
    - 4.3.4 [Par pointeur (*)](/04-structures-controle-fonctions/03.4-par-pointeur.md)
- 4.4 [Surcharge de fonctions (Function Overloading)](/04-structures-controle-fonctions/04-surcharge-fonctions.md)
- 4.5 [Valeurs par défaut et fonctions inline](/04-structures-controle-fonctions/05-valeurs-defaut-inline.md)

### 5. [Gestion de la Mémoire (Le cœur du sujet)](/05-gestion-memoire/README.md)
- 5.1 [Comprendre la Stack (Pile) vs le Heap (Tas)](/05-gestion-memoire/01-stack-vs-heap.md)
    - 5.1.1 [Diagramme mémoire d'un processus](/05-gestion-memoire/01.1-diagramme-memoire.md)
    - 5.1.2 [Caractéristiques de la Stack](/05-gestion-memoire/01.2-caracteristiques-stack.md)
    - 5.1.3 [Caractéristiques du Heap](/05-gestion-memoire/01.3-caracteristiques-heap.md)
- 5.2 [Allocation dynamique : new/delete, new[]/delete[]](/05-gestion-memoire/02-allocation-dynamique.md)
- 5.3 [Arithmétique des pointeurs et accès bas niveau](/05-gestion-memoire/03-arithmetique-pointeurs.md)
- 5.4 [Dangers : Memory leaks, dangling pointers, double free](/05-gestion-memoire/04-dangers-memoire.md)
- 5.5 [Outils de détection : Valgrind, AddressSanitizer](/05-gestion-memoire/05-outils-detection.md)
    - 5.5.1 [Valgrind : Installation et utilisation](/05-gestion-memoire/05.1-valgrind.md)
    - 5.5.2 [AddressSanitizer : Compilation avec -fsanitize=address](/05-gestion-memoire/05.2-addresssanitizer.md)

---

## **[Module 3 : Programmation Orientée Objet](/module-03-programmation-orientee-objet.md)** *(Niveau Débutant-Intermédiaire)*

### 6. [Classes et Encapsulation](/06-classes-encapsulation/README.md)
- 6.1 [Définition de classes : Membres et méthodes](/06-classes-encapsulation/01-definition-classes.md)
- 6.2 [Constructeurs](/06-classes-encapsulation/02-constructeurs.md)
    - 6.2.1 [Constructeur par défaut](/06-classes-encapsulation/02.1-constructeur-defaut.md)
    - 6.2.2 [Constructeur paramétré](/06-classes-encapsulation/02.2-constructeur-parametre.md)
    - 6.2.3 [Constructeur de copie](/06-classes-encapsulation/02.3-constructeur-copie.md)
    - 6.2.4 [Constructeur de déplacement](/06-classes-encapsulation/02.4-constructeur-deplacement.md)
    - 6.2.5 [Liste d'initialisation](/06-classes-encapsulation/02.5-liste-initialisation.md)
- 6.3 [Destructeurs et le principe RAII](/06-classes-encapsulation/03-destructeurs-raii.md)
    - 6.3.1 [Resource Acquisition Is Initialization](/06-classes-encapsulation/03.1-raii-concept.md)
    - 6.3.2 [Exemples pratiques de RAII](/06-classes-encapsulation/03.2-raii-exemples.md)
- 6.4 [Modificateurs d'accès : public, private, protected](/06-classes-encapsulation/04-modificateurs-acces.md)
- **6.5 [La règle des 5 (Rule of Five)](/06-classes-encapsulation/05-rule-of-five.md)** ⭐
    - 6.5.1 [Destructeur](/06-classes-encapsulation/05.1-destructeur.md)
    - 6.5.2 [Constructeur de copie](/06-classes-encapsulation/05.2-copy-constructor.md)
    - 6.5.3 [Opérateur d'affectation par copie](/06-classes-encapsulation/05.3-copy-assignment.md)
    - 6.5.4 [Constructeur de déplacement](/06-classes-encapsulation/05.4-move-constructor.md)
    - 6.5.5 [Opérateur d'affectation par déplacement](/06-classes-encapsulation/05.5-move-assignment.md)

### 7. [Héritage et Polymorphisme](/07-heritage-polymorphisme/README.md)
- 7.1 [Héritage simple et multiple](/07-heritage-polymorphisme/01-heritage-simple-multiple.md)
    - 7.1.1 [Héritage simple](/07-heritage-polymorphisme/01.1-heritage-simple.md)
    - 7.1.2 [Héritage multiple et problème du diamant](/07-heritage-polymorphisme/01.2-heritage-multiple.md)
    - 7.1.3 [Héritage virtuel](/07-heritage-polymorphisme/01.3-heritage-virtuel.md)
- 7.2 [Fonctions virtuelles et mécanisme de vtable](/07-heritage-polymorphisme/02-fonctions-virtuelles-vtable.md)
- 7.3 [Polymorphisme dynamique : virtual, override, final](/07-heritage-polymorphisme/03-polymorphisme-dynamique.md)
- 7.4 [Classes abstraites et interfaces pures](/07-heritage-polymorphisme/04-classes-abstraites.md)
- 7.5 [Coût du polymorphisme dynamique en performance](/07-heritage-polymorphisme/05-cout-polymorphisme.md)

### 8. [Surcharge d'Opérateurs et Conversions](/08-surcharge-operateurs/README.md)
- 8.1 [Surcharge des opérateurs arithmétiques et de comparaison](/08-surcharge-operateurs/01-operateurs-arithmetiques.md)
- 8.2 [Opérateurs d'affectation (= et +=, -=, etc.)](/08-surcharge-operateurs/02-operateurs-affectation.md)
- 8.3 [Opérateurs de conversion (conversion operators)](/08-surcharge-operateurs/03-operateurs-conversion.md)
- 8.4 [Opérateur d'appel de fonction (operator())](/08-surcharge-operateurs/04-operateur-appel.md)
- 8.5 [Opérateur spaceship <=> (C++20)](/08-surcharge-operateurs/05-operateur-spaceship.md)

---

## **[PARTIE II : C++ MODERNE](/partie-02-cpp-moderne.md)**

---

## **[Module 4 : C++ Moderne (C++11 → C++26)](/module-04-cpp-moderne.md)** *(Niveau Intermédiaire)*

### 9. [Smart Pointers : Gestion Automatique de la Mémoire](/09-smart-pointers/README.md) ⭐
- 9.1 [std::unique_ptr : Possession exclusive](/09-smart-pointers/01-unique-ptr.md)
    - 9.1.1 [Création et utilisation](/09-smart-pointers/01.1-creation-utilisation.md)
    - 9.1.2 [Transfert de propriété avec std::move](/09-smart-pointers/01.2-transfert-propriete.md)
    - 9.1.3 [Custom deleters](/09-smart-pointers/01.3-custom-deleters.md)
- 9.2 [std::shared_ptr et std::weak_ptr](/09-smart-pointers/02-shared-weak-ptr.md)
    - 9.2.1 [Comptage de références](/09-smart-pointers/02.1-comptage-references.md)
    - 9.2.2 [Cycles de références et std::weak_ptr](/09-smart-pointers/02.2-cycles-weak-ptr.md)
- 9.3 [std::make_unique et std::make_shared](/09-smart-pointers/03-make-unique-shared.md)
- **9.4 [Ne jamais utiliser new/delete dans du code moderne](/09-smart-pointers/04-jamais-new-delete.md)** 🔥

### 10. [Sémantique de Mouvement (Move Semantics)](/10-move-semantics/README.md) ⭐
- 10.1 [L-values vs R-values (&&)](/10-move-semantics/01-lvalues-rvalues.md)
- 10.2 [std::move : Transfert de propriété sans copie](/10-move-semantics/02-std-move.md)
- 10.3 [Move constructors et move assignment operators](/10-move-semantics/03-move-constructors.md)
- 10.4 [Perfect Forwarding avec std::forward](/10-move-semantics/04-perfect-forwarding.md)
- 10.5 [Return Value Optimization (RVO) et Copy Elision](/10-move-semantics/05-rvo-copy-elision.md)

### 11. [Programmation Fonctionnelle et Lambdas](/11-lambdas/README.md)
- 11.1 [Syntaxe des lambdas et types de captures](/11-lambdas/01-syntaxe-captures.md)
    - 11.1.1 [Capture par valeur [=]](/11-lambdas/01.1-capture-valeur.md)
    - 11.1.2 [Capture par référence [&]](/11-lambdas/01.2-capture-reference.md)
    - 11.1.3 [Capture de this](/11-lambdas/01.3-capture-this.md)
    - 11.1.4 [Captures mixtes et init captures](/11-lambdas/01.4-captures-mixtes.md)
- 11.2 [Lambdas génériques (C++14) et templated lambdas](/11-lambdas/02-lambdas-generiques.md)
- 11.3 [Utilisation avec les algorithmes STL](/11-lambdas/03-lambdas-stl.md)
- 11.4 [std::function et callable objects](/11-lambdas/04-std-function.md)

### 12. [Nouveautés C++17/C++20/C++23/C++26](/12-nouveautes-cpp17-26/README.md) ⭐
- 12.1 [Structured Bindings (C++17)](/12-nouveautes-cpp17-26/01-structured-bindings.md)
- 12.2 [std::optional, std::variant, std::any (C++17)](/12-nouveautes-cpp17-26/02-optional-variant-any.md)
- 12.3 [std::span (C++20) : Vue sur données contiguës](/12-nouveautes-cpp17-26/03-std-span.md)

> 📎 *Aperçu introductif. Pour la couverture détaillée et les cas d'usage, voir **section 13.5** (std::span).*

- 12.4 [Concepts (C++20) : Contraintes sur les templates](/12-nouveautes-cpp17-26/04-concepts.md)
- 12.5 [Ranges (C++20) : Pipelines fonctionnels](/12-nouveautes-cpp17-26/05-ranges.md)
- 12.6 [Coroutines (C++20) : Programmation asynchrone](/12-nouveautes-cpp17-26/06-coroutines.md)
- **12.7 [std::print et std::format (C++23) : Formatage moderne](/12-nouveautes-cpp17-26/07-std-print-format.md)** ⭐

> 💡 *Couverture approfondie du système de formatage. Pour la prise en main rapide, voir **section 2.7** (introduction à std::print).*

- **12.8 [std::expected (C++23) : Gestion d'erreurs sans exceptions](/12-nouveautes-cpp17-26/08-std-expected.md)** ⭐
- **12.9 [std::flat_map et std::flat_set (C++23) : Conteneurs ordonnés à mémoire contiguë](/12-nouveautes-cpp17-26/09-flat-containers.md)** ⭐
    - 12.9.1 [Avantages performance vs std::map](/12-nouveautes-cpp17-26/09.1-avantages-performance.md)
    - 12.9.2 [Cas d'usage et limites](/12-nouveautes-cpp17-26/09.2-cas-usage-limites.md)
- **12.10 [std::mdspan (C++23) : Vues multidimensionnelles](/12-nouveautes-cpp17-26/10-mdspan.md)** ⭐
- **12.11 [std::generator (C++23) : Coroutines simplifiées](/12-nouveautes-cpp17-26/11-generator.md)**
- **12.12 [std::stacktrace (C++23) : Traces d'exécution standard](/12-nouveautes-cpp17-26/12-stacktrace.md)**
- 12.13 [Modules (C++20) : Concept et état en 2026](/12-nouveautes-cpp17-26/13-modules.md) 
    - 12.13.1 [Avantages des modules sur les headers](/12-nouveautes-cpp17-26/13.1-avantages-modules.md)
    - 12.13.2 [État du support (GCC 15, Clang 20, MSVC)](/12-nouveautes-cpp17-26/13.2-etat-support.md) 
    - 12.13.3 [Maturité en 2026 : Ce qui fonctionne, ce qui reste limité](/12-nouveautes-cpp17-26/13.3-maturite-2026.md) 
- **12.14 [C++26 : Standard ratifié — Les grandes nouveautés](/12-nouveautes-cpp17-26/14-cpp26-overview.md)** 🔥 
    - 12.14.1 [Contrats (Contracts) : Préconditions, postconditions, assertions](/12-nouveautes-cpp17-26/14.1-contracts.md)
    - 12.14.2 [Réflexion statique (Static Reflection)](/12-nouveautes-cpp17-26/14.2-static-reflection.md)
    - 12.14.3 [Pattern Matching : inspect et is](/12-nouveautes-cpp17-26/14.3-pattern-matching.md)
    - **12.14.4 [std::execution (Senders/Receivers) : Asynchronisme standardisé — intégré C++26](/12-nouveautes-cpp17-26/14.4-std-execution.md)** 🔥 *(promu)*
        - 12.14.4.1 [Modèle Sender/Receiver : Concepts fondamentaux](/12-nouveautes-cpp17-26/14.4.1-sender-receiver.md) 
        - 12.14.4.2 [Remplacement de std::async et std::future](/12-nouveautes-cpp17-26/14.4.2-remplacement-async.md) 
        - 12.14.4.3 [Intégration avec les schedulers et thread pools](/12-nouveautes-cpp17-26/14.4.3-schedulers.md) 
    - 12.14.5 [Support compilateur C++26 : GCC 15 / Clang 20 — État mars 2026](/12-nouveautes-cpp17-26/14.5-etat-support-cpp26.md) 

---

## **[Module 5 : La Librairie Standard (STL)](/module-05-stl.md)** *(Niveau Intermédiaire)*

### 13. [Conteneurs Séquentiels](/13-conteneurs-sequentiels/README.md)
- 13.1 [std::vector : Le conteneur par défaut](/13-conteneurs-sequentiels/01-vector.md)
    - 13.1.1 [Fonctionnement interne et capacité](/13-conteneurs-sequentiels/01.1-fonctionnement-interne.md)
    - 13.1.2 [Méthodes essentielles](/13-conteneurs-sequentiels/01.2-methodes-essentielles.md)
    - 13.1.3 [Invalidation des itérateurs](/13-conteneurs-sequentiels/01.3-invalidation-iterateurs.md)
- 13.2 [std::array : Tableaux de taille fixe](/13-conteneurs-sequentiels/02-array.md)
- 13.3 [std::list et std::forward_list : Listes chaînées](/13-conteneurs-sequentiels/03-list-forward-list.md)
- 13.4 [std::deque : File double entrée](/13-conteneurs-sequentiels/04-deque.md)
- **13.5 [std::span : Vue sur données contiguës (zéro allocation)](/13-conteneurs-sequentiels/05-span.md)** ⭐
    - 13.5.1 [Span statique vs dynamique](/13-conteneurs-sequentiels/05.1-span-statique-dynamique.md)
    - 13.5.2 [Remplacement des paramètres pointeur+taille](/13-conteneurs-sequentiels/05.2-remplacement-pointeur-taille.md)
    - 13.5.3 [Interopérabilité avec vector, array et C arrays](/13-conteneurs-sequentiels/05.3-interoperabilite-conteneurs.md)
- 13.6 [Complexité algorithmique (Big O) et choix du conteneur](/13-conteneurs-sequentiels/06-complexite-big-o.md)

### 14. [Conteneurs Associatifs](/14-conteneurs-associatifs/README.md)
- 14.1 [std::map et std::multimap : Arbres binaires ordonnés](/14-conteneurs-associatifs/01-map-multimap.md)
- 14.2 [std::unordered_map : Tables de hachage](/14-conteneurs-associatifs/02-unordered-map.md)
    - 14.2.1 [Fonctionnement des hash tables](/14-conteneurs-associatifs/02.1-hash-tables.md)
    - 14.2.2 [Custom hash functions](/14-conteneurs-associatifs/02.2-custom-hash.md)
- 14.3 [std::set et std::unordered_set](/14-conteneurs-associatifs/03-set-unordered-set.md)
- **14.4 [std::flat_map et std::flat_set (C++23) : Alternatives cache-friendly](/14-conteneurs-associatifs/04-flat-map-flat-set.md)** ⭐
- 14.5 [Comparaison de performances : ordered vs unordered vs flat](/14-conteneurs-associatifs/05-comparaison-performances.md)

### 15. [Algorithmes de la STL](/15-algorithmes-stl/README.md)
- 15.1 [Recherche : std::find, std::binary_search](/15-algorithmes-stl/01-recherche.md)
- 15.2 [Tri : std::sort, std::stable_sort](/15-algorithmes-stl/02-tri.md)
- 15.3 [Transformation : std::transform, std::accumulate](/15-algorithmes-stl/03-transformation.md)
- 15.4 [Manipulation : std::copy, std::move, std::remove_if](/15-algorithmes-stl/04-manipulation.md)
- 15.5 [Itérateurs : input, output, forward, bidirectional, random_access](/15-algorithmes-stl/05-iterateurs.md)
- **15.6 [Ranges (C++20) : Simplification des algorithmes](/15-algorithmes-stl/06-ranges.md)** ⭐
    - 15.6.1 [Views et lazy evaluation](/15-algorithmes-stl/06.1-views-lazy.md)
    - 15.6.2 [Pipelines avec l'opérateur |](/15-algorithmes-stl/06.2-pipelines.md)
- **15.7 [Algorithmes parallèles : std::execution policies](/15-algorithmes-stl/07-algorithmes-paralleles.md)** ⭐
    - 15.7.1 [Politiques d'exécution : seq, par, par_unseq, unseq](/15-algorithmes-stl/07.1-politiques-execution.md)
    - 15.7.2 [Parallélisation de std::sort, std::transform, std::reduce](/15-algorithmes-stl/07.2-parallelisation-algorithmes.md)
    - 15.7.3 [Précautions et limitations pratiques](/15-algorithmes-stl/07.3-precautions-limitations.md)

### 16. [Templates et Métaprogrammation](/16-templates-metaprogrammation/README.md)
- 16.1 [Templates de fonctions](/16-templates-metaprogrammation/01-templates-fonctions.md)
- 16.2 [Templates de classes](/16-templates-metaprogrammation/02-templates-classes.md)
- 16.3 [Spécialisation partielle et totale](/16-templates-metaprogrammation/03-specialisation.md)
- 16.4 [SFINAE (Substitution Failure Is Not An Error)](/16-templates-metaprogrammation/04-sfinae.md)
- 16.5 [Variadic templates (C++11)](/16-templates-metaprogrammation/05-variadic-templates.md)
- **16.6 [Concepts (C++20) pour contraindre les templates](/16-templates-metaprogrammation/06-concepts.md)** ⭐
    - 16.6.1 [Syntaxe requires](/16-templates-metaprogrammation/06.1-requires.md)
    - 16.6.2 [Concepts standard de la STL](/16-templates-metaprogrammation/06.2-concepts-standard.md)
    - 16.6.3 [Création de concepts personnalisés](/16-templates-metaprogrammation/06.3-concepts-personnalises.md)
- 16.7 [Fold expressions (C++17)](/16-templates-metaprogrammation/07-fold-expressions.md)

---

## **[Module 6 : Gestion des Erreurs et Robustesse](/module-06-gestion-erreurs.md)** *(Niveau Intermédiaire)*

### 17. [Exceptions et Gestion d'Erreurs](/17-exceptions/README.md)
- 17.1 [Syntaxe : try, catch, throw](/17-exceptions/01-syntaxe-try-catch.md)
- 17.2 [Hiérarchie des exceptions standard (std::exception)](/17-exceptions/02-hierarchie-exceptions.md)
- 17.3 [Exceptions personnalisées](/17-exceptions/03-exceptions-personnalisees.md)
- 17.4 [noexcept et garanties d'exception](/17-exceptions/04-noexcept.md)
    - 17.4.1 [Spécifier noexcept](/17-exceptions/04.1-specifier-noexcept.md)
    - 17.4.2 [Impact sur les performances](/17-exceptions/04.2-impact-performances.md)
- **17.5 [Alternatives modernes : std::expected (C++23), codes d'erreur](/17-exceptions/05-alternatives-modernes.md)** ⭐
- 17.6 [Contrats (C++26) : Préconditions et postconditions — standard ratifié](/17-exceptions/06-contrats-cpp26.md) 

> 📎 *Introduction aux contrats dans le contexte de la gestion d'erreurs. Pour la couverture complète, voir **section 12.14.1** (Contrats C++26).*

### 18. [Assertions et Débogage Défensif](/18-assertions-debug/README.md)
- 18.1 [assert et static_assert](/18-assertions-debug/01-assert-static-assert.md)
- 18.2 [Compilation conditionnelle (#ifdef DEBUG)](/18-assertions-debug/02-compilation-conditionnelle.md)
- 18.3 [Logging et traces d'exécution](/18-assertions-debug/03-logging-traces.md)
- **18.4 [std::stacktrace (C++23) : Traces d'exécution standard](/18-assertions-debug/04-stacktrace.md)**

> 📎 *Intégration de `std::stacktrace` dans le débogage. Voir aussi **section 12.12** (std::stacktrace C++23).*

---

## **[PARTIE III : PROGRAMMATION SYSTÈME LINUX](/partie-03-programmation-systeme-linux.md)**

---

## **[Module 7 : Programmation Système sur Linux](/module-07-programmation-systeme.md)** *(Niveau Avancé)*

### 19. [Manipulation du Système de Fichiers](/19-systeme-fichiers/README.md)
- 19.1 [std::filesystem (C++17) : API moderne](/19-systeme-fichiers/01-std-filesystem.md)
    - 19.1.1 [Parcours de répertoires](/19-systeme-fichiers/01.1-parcours-repertoires.md)
    - 19.1.2 [Manipulation de chemins (path)](/19-systeme-fichiers/01.2-manipulation-chemins.md)
    - 19.1.3 [Opérations sur fichiers et répertoires](/19-systeme-fichiers/01.3-operations-fichiers.md)
- 19.2 [Appels système POSIX : open, read, write, close](/19-systeme-fichiers/02-appels-posix.md)
- 19.3 [Comparaison : API C++ vs API système](/19-systeme-fichiers/03-comparaison-api.md)
- 19.4 [Permissions, droits et gestion des erreurs](/19-systeme-fichiers/04-permissions-droits.md)

### 20. [Signaux POSIX](/20-signaux-posix/README.md)
- 20.1 [Comprendre les signaux Unix (SIGINT, SIGTERM, SIGSEGV)](/20-signaux-posix/01-signaux-unix.md)
- 20.2 [Installation de handlers (signal, sigaction)](/20-signaux-posix/02-handlers.md)
- 20.3 [Signaux et threads : Problématiques](/20-signaux-posix/03-signaux-threads.md)

### 21. [Threads et Programmation Concurrente](/21-threads-concurrence/README.md) ⭐
- 21.1 [std::thread : Création et gestion de threads](/21-threads-concurrence/01-std-thread.md)
- 21.2 [Synchronisation](/21-threads-concurrence/02-synchronisation.md)
    - 21.2.1 [std::mutex](/21-threads-concurrence/02.1-mutex.md)
    - 21.2.2 [std::lock_guard](/21-threads-concurrence/02.2-lock-guard.md)
    - 21.2.3 [std::unique_lock](/21-threads-concurrence/02.3-unique-lock.md)
    - 21.2.4 [std::scoped_lock (C++17)](/21-threads-concurrence/02.4-scoped-lock.md)
- 21.3 [Variables de condition : std::condition_variable](/21-threads-concurrence/03-condition-variable.md)
- 21.4 [Atomiques : std::atomic et memory ordering](/21-threads-concurrence/04-atomiques.md)
- 21.5 [std::async et std::future : Programmation asynchrone](/21-threads-concurrence/05-async-future.md)
- 21.6 [Thread-safety et data races](/21-threads-concurrence/06-thread-safety.md)
- **21.7 [std::jthread (C++20) : Threads auto-stoppables](/21-threads-concurrence/07-jthread.md)** ⭐
- **21.8 [Algorithmes parallèles appliqués à la concurrence](/21-threads-concurrence/08-algorithmes-paralleles.md)**

> 📎 *Mise en pratique des politiques d'exécution parallèle dans un contexte multi-thread. Voir **section 15.7** (algorithmes parallèles — std::execution policies).*

### 22. [Networking et Communication](/22-networking/README.md) ⭐
- 22.1 [Sockets TCP/UDP : API POSIX](/22-networking/01-sockets-posix.md)
    - 22.1.1 [Création de sockets](/22-networking/01.1-creation-sockets.md)
    - 22.1.2 [bind, listen, accept, connect](/22-networking/01.2-operations-sockets.md)
    - 22.1.3 [send, recv, sendto, recvfrom](/22-networking/01.3-envoi-reception.md)
- 22.2 [Client/Serveur basique en C++](/22-networking/02-client-serveur.md)
- 22.3 [Multiplexage I/O : select, poll, epoll](/22-networking/03-multiplexage-io.md)
- 22.4 [Librairies réseau modernes](/22-networking/04-librairies-reseau.md)
    - 22.4.1 [Standalone Asio : Networking sans Boost](/22-networking/04.1-standalone-asio.md)
    - 22.4.2 [Boost.Asio : Écosystème complet](/22-networking/04.2-boost-asio.md)
    - 22.4.3 [Quand choisir Asio standalone vs Boost.Asio](/22-networking/04.3-asio-vs-boost-asio.md)
- **22.5 [Clients HTTP : cpr (wrapper curl), cpp-httplib](/22-networking/05-clients-http.md)** ⭐
- **22.6 [gRPC et Protocol Buffers : RPC moderne haute performance](/22-networking/06-grpc-protobuf.md)** 🔥
    - 22.6.1 [Installation et configuration gRPC](/22-networking/06.1-installation-grpc.md)
    - 22.6.2 [Définition de services avec .proto](/22-networking/06.2-definition-proto.md)
    - 22.6.3 [Génération de code et implémentation](/22-networking/06.3-generation-code.md)
    - 22.6.4 [Streaming bidirectionnel](/22-networking/06.4-streaming.md)

### 23. [Processus et IPC (Inter-Process Communication)](/23-processus-ipc/README.md)
- 23.1 [fork, exec et gestion de processus](/23-processus-ipc/01-fork-exec.md)
- 23.2 [Pipes et communication inter-processus](/23-processus-ipc/02-pipes.md)
- 23.3 [Shared memory et mmap](/23-processus-ipc/03-shared-memory.md)
- 23.4 [Message queues POSIX](/23-processus-ipc/04-message-queues.md)

---

## **[Module 8 : Parsing et Formats de Données](/module-08-parsing-formats.md)** *(Niveau Avancé)* ⭐

### 24. [Sérialisation et Fichiers de Configuration](/24-serialisation-config/README.md) 🔥
- **24.1 [JSON : Lecture/Écriture avec nlohmann/json](/24-serialisation-config/01-json-nlohmann.md)** ⭐
    - 24.1.1 [Installation et intégration](/24-serialisation-config/01.1-installation.md)
    - 24.1.2 [Parsing de fichiers JSON](/24-serialisation-config/01.2-parsing.md)
    - 24.1.3 [Sérialisation d'objets C++](/24-serialisation-config/01.3-serialisation.md)
    - 24.1.4 [Gestion des erreurs de parsing](/24-serialisation-config/01.4-gestion-erreurs.md)
- **24.2 [YAML : Parsing avec yaml-cpp](/24-serialisation-config/02-yaml-cpp.md)** ⭐
    - 24.2.1 [Lecture de fichiers de configuration](/24-serialisation-config/02.1-lecture-config.md)
    - 24.2.2 [Écriture YAML](/24-serialisation-config/02.2-ecriture-yaml.md)
- 24.3 [TOML : Alternative moderne (toml++)](/24-serialisation-config/03-toml.md)
- 24.4 [XML : Parsing avec pugixml (legacy systems)](/24-serialisation-config/04-xml-pugixml.md)
- 24.5 [Bonnes pratiques : Validation de schémas](/24-serialisation-config/05-validation-schemas.md)

### 25. [Formats Binaires et Sérialisation Performante](/25-formats-binaires/README.md)
- **25.1 [Protocol Buffers (Protobuf) : Sérialisation Google](/25-formats-binaires/01-protobuf.md)** ⭐
    - 25.1.1 [Définition de messages .proto](/25-formats-binaires/01.1-definition-proto.md)
    - 25.1.2 [Génération de code C++](/25-formats-binaires/01.2-generation-code.md)
    - 25.1.3 [Sérialisation/Désérialisation](/25-formats-binaires/01.3-serialisation.md)
- 25.2 [FlatBuffers : Zéro-copy serialization](/25-formats-binaires/02-flatbuffers.md)
- 25.3 [MessagePack : JSON binaire compact](/25-formats-binaires/03-messagepack.md)
- 25.4 [Comparaison de performances et cas d'usage](/25-formats-binaires/04-comparaison-performances.md)

---

## **[PARTIE IV : TOOLING ET BUILD SYSTEMS](/partie-04-tooling-build-systems.md)**

---

## **[Module 9 : Build Systems et Gestion de Projet](/module-09-build-systems.md)** *(Niveau Avancé)*

### 26. [CMake : Le Standard de l'Industrie](/26-cmake/README.md) 🔥
- 26.1 [Structure d'un projet CMake moderne](/26-cmake/01-structure-projet.md)
- 26.2 [Écrire un CMakeLists.txt : targets, libraries, executables](/26-cmake/02-cmakelists.md)
    - 26.2.1 [add_executable et add_library](/26-cmake/02.1-add-executable-library.md)
    - 26.2.2 [target_link_libraries](/26-cmake/02.2-target-link-libraries.md)
    - 26.2.3 [target_include_directories](/26-cmake/02.3-target-include-directories.md)
    - 26.2.4 [PUBLIC, PRIVATE, INTERFACE](/26-cmake/02.4-public-private-interface.md)
- 26.3 [Gestion des dépendances et sous-répertoires](/26-cmake/03-dependances-sous-repertoires.md)
    - 26.3.1 [find_package](/26-cmake/03.1-find-package.md)
    - 26.3.2 [FetchContent](/26-cmake/03.2-fetchcontent.md)
    - 26.3.3 [add_subdirectory](/26-cmake/03.3-add-subdirectory.md)
- 26.4 [Configuration et génération de fichiers](/26-cmake/04-configuration-generation.md)
- **26.5 [Génération pour Ninja : cmake -G Ninja (recommandé)](/26-cmake/05-generation-ninja.md)** ⭐
- 26.6 [Toolchains et cross-compilation](/26-cmake/06-toolchains-cross-compilation.md)
- **26.7 [CMake 3.31+ : Nouveautés et meilleures pratiques 2026](/26-cmake/07-cmake-3-31-nouveautes.md)** ⭐ 

### 27. [Gestion des Dépendances](/27-gestion-dependances/README.md) ⭐
- 27.1 [Le problème des librairies en C++](/27-gestion-dependances/01-probleme-librairies.md)
- **27.2 [Conan 2.0 : Nouvelle API et conanfile.py](/27-gestion-dependances/02-conan-2.md)** 🔥
    - 27.2.1 [Installation et configuration](/27-gestion-dependances/02.1-installation-conan.md)
    - 27.2.2 [conanfile.py vs conanfile.txt](/27-gestion-dependances/02.2-conanfile.md)
    - 27.2.3 [Profils et settings](/27-gestion-dependances/02.3-profils-settings.md)
    - 27.2.4 [Intégration CMake](/27-gestion-dependances/02.4-integration-cmake.md)
- 27.3 [vcpkg : Alternative Microsoft](/27-gestion-dependances/03-vcpkg.md)
- 27.4 [Linkage statique (.a) vs dynamique (.so)](/27-gestion-dependances/04-linkage-statique-dynamique.md)
- 27.5 [Installation et distribution de librairies sur Linux](/27-gestion-dependances/05-distribution-linux.md)
- **27.6 [CMake Presets : Standardisation des configurations](/27-gestion-dependances/06-cmake-presets.md)** ⭐

### 28. [Makefile, Ninja et Build Automation](/28-make-ninja/README.md)
- 28.1 [Syntaxe de base des Makefiles](/28-make-ninja/01-syntaxe-makefiles.md)
- 28.2 [Variables, règles et patterns](/28-make-ninja/02-variables-regles.md)
- **28.3 [Ninja : Build system ultra-rapide](/28-make-ninja/03-ninja.md)** ⭐
    - 28.3.1 [Pourquoi Ninja est plus rapide](/28-make-ninja/03.1-pourquoi-rapide.md)
    - 28.3.2 [Fichiers build.ninja](/28-make-ninja/03.2-fichiers-ninja.md)
- 28.4 [Meson : Build system montant dans l'écosystème Linux](/28-make-ninja/04-meson.md)
    - 28.4.1 [Syntaxe déclarative et philosophie](/28-make-ninja/04.1-syntaxe-meson.md)
    - 28.4.2 [Projets notables utilisant Meson (GNOME, systemd, Mesa)](/28-make-ninja/04.2-projets-meson.md)
- 28.5 [Comparaison Make vs Ninja vs Meson : Performances et cas d'usage](/28-make-ninja/05-comparaison-build-systems.md)

---

## **[Module 10 : Débogage, Profiling et Qualité Code](/module-10-debogage-profiling.md)** *(Niveau Avancé)*

### 29. [Débogage Avancé](/29-debogage/README.md)
- 29.1 [GDB : Commandes essentielles et breakpoints](/29-debogage/01-gdb-commandes.md)
    - 29.1.1 [Navigation : run, step, next, continue](/29-debogage/01.1-navigation.md)
    - 29.1.2 [Inspection : print, display, watch](/29-debogage/01.2-inspection.md)
    - 29.1.3 [Breakpoints conditionnels](/29-debogage/01.3-breakpoints-conditionnels.md)
- 29.2 [Débogage via IDE (VS Code, CLion)](/29-debogage/02-debogage-ide.md)
- 29.3 [Core dumps et post-mortem debugging](/29-debogage/03-core-dumps.md)
- 29.4 [Sanitizers](/29-debogage/04-sanitizers.md)
    - 29.4.1 [AddressSanitizer (-fsanitize=address)](/29-debogage/04.1-addresssanitizer.md)
    - 29.4.2 [UndefinedBehaviorSanitizer (-fsanitize=undefined)](/29-debogage/04.2-ubsan.md)
    - 29.4.3 [ThreadSanitizer (-fsanitize=thread)](/29-debogage/04.3-threadsanitizer.md)
    - 29.4.4 [MemorySanitizer (-fsanitize=memory)](/29-debogage/04.4-memorysanitizer.md)
- **29.5 [std::stacktrace : Traces d'exécution intégrées au débogage](/29-debogage/05-stacktrace-debug.md)**

### 30. [Analyse Mémoire](/30-analyse-memoire/README.md)
- 30.1 [Valgrind : Détection de fuites et erreurs mémoire](/30-analyse-memoire/01-valgrind.md)
    - 30.1.1 [memcheck : Détection de fuites](/30-analyse-memoire/01.1-memcheck.md)
    - 30.1.2 [Lecture des rapports Valgrind](/30-analyse-memoire/01.2-lecture-rapports.md)
- 30.2 [Heap profiling avec Massif](/30-analyse-memoire/02-massif.md)
- 30.3 [Memory leaks : Détection et résolution](/30-analyse-memoire/03-memory-leaks.md)

### 31. [Profiling de Performance](/31-profiling/README.md)
- 31.1 [perf : Profiling CPU et sampling](/31-profiling/01-perf.md)
    - 31.1.1 [perf record et perf report](/31-profiling/01.1-perf-record-report.md)
    - 31.1.2 [perf stat : Compteurs matériels](/31-profiling/01.2-perf-stat.md)
- 31.2 [gprof : Profiling basé sur instrumentation](/31-profiling/02-gprof.md)
- 31.3 [Flamegraphs et visualisation](/31-profiling/03-flamegraphs.md)
- 31.4 [Hotspot et outils graphiques](/31-profiling/04-hotspot.md)

### 32. [Analyse Statique et Linting](/32-analyse-statique/README.md)
- 32.1 [clang-tidy : Analyse statique moderne](/32-analyse-statique/01-clang-tidy.md)
    - 32.1.1 [Configuration .clang-tidy](/32-analyse-statique/01.1-configuration.md)
    - 32.1.2 [Checks recommandés](/32-analyse-statique/01.2-checks-recommandes.md)
- 32.2 [cppcheck : Détection d'erreurs](/32-analyse-statique/02-cppcheck.md)
- 32.3 [clang-format 19 : Formatage automatique](/32-analyse-statique/03-clang-format.md) 
- 32.4 [Intégration dans le workflow de développement](/32-analyse-statique/04-integration-workflow.md)

---

## **[Module 11 : Tests et Qualité Logicielle](/module-11-tests-qualite.md)** *(Niveau Avancé)*

### 33. [Unit Testing avec Google Test](/33-google-test/README.md) ⭐
- 33.1 [Installation et configuration de GTest](/33-google-test/01-installation-gtest.md)
- 33.2 [Écriture de tests : TEST, TEST_F, fixtures](/33-google-test/02-ecriture-tests.md)
    - 33.2.1 [TEST : Tests simples](/33-google-test/02.1-test-simple.md)
    - 33.2.2 [TEST_F : Tests avec fixtures](/33-google-test/02.2-test-fixtures.md)
    - 33.2.3 [TEST_P : Tests paramétrés](/33-google-test/02.3-test-parametres.md)
- 33.3 [Assertions et matchers](/33-google-test/03-assertions-matchers.md)
- 33.4 [Mocking avec Google Mock](/33-google-test/04-google-mock.md)
- 33.5 [Test-Driven Development (TDD) en C++](/33-google-test/05-tdd.md)

### 34. [Couverture de Code](/34-couverture-code/README.md)
- 34.1 [gcov et lcov : Mesure de la couverture](/34-couverture-code/01-gcov-lcov.md)
- 34.2 [Génération de rapports HTML](/34-couverture-code/02-rapports-html.md)
- 34.3 [Intégration dans CMake](/34-couverture-code/03-integration-cmake.md)
- 34.4 [Objectifs de couverture](/34-couverture-code/04-objectifs-couverture.md)

### 35. [Benchmarking](/35-benchmarking/README.md)
- 35.1 [Google Benchmark : Micro-benchmarking](/35-benchmarking/01-google-benchmark.md)
- 35.2 [Mesure de performance fiable](/35-benchmarking/02-mesure-fiable.md)
- 35.3 [Interprétation des résultats](/35-benchmarking/03-interpretation-resultats.md)

---

## **[PARTIE V : DEVOPS ET CLOUD NATIVE](/partie-05-devops-cloud-native.md)**

---

## **[Module 12 : Création d'Outils CLI](/module-12-outils-cli.md)** *(Niveau Avancé)* ⭐

### 36. [Interfaces en Ligne de Commande Modernes](/36-interfaces-cli/README.md) 🔥
- **36.1 [CLI11 : Parsing d'arguments professionnel](/36-interfaces-cli/01-cli11.md)** ⭐
    - 36.1.1 [Installation et premiers pas](/36-interfaces-cli/01.1-installation.md)
    - 36.1.2 [Options, flags et sous-commandes](/36-interfaces-cli/01.2-options-flags.md)
    - 36.1.3 [Validation et callbacks](/36-interfaces-cli/01.3-validation-callbacks.md)
    - 36.1.4 [Génération d'aide automatique](/36-interfaces-cli/01.4-aide-automatique.md)
- 36.2 [argparse : Alternative légère](/36-interfaces-cli/02-argparse.md)
- **36.3 [fmt : Formatage avancé (pré-C++23)](/36-interfaces-cli/03-fmt.md)** ⭐
    - 36.3.1 [Syntaxe Python-like](/36-interfaces-cli/03.1-syntaxe-python.md)
    - 36.3.2 [Couleurs et styles](/36-interfaces-cli/03.2-couleurs-styles.md)
- 36.4 [Gestion des couleurs et du TTY](/36-interfaces-cli/04-couleurs-tty.md)
- 36.5 [Architecture d'un outil CLI professionnel (à la kubectl, git)](/36-interfaces-cli/05-architecture-cli.md)

---

## **[Module 13 : C++ dans une Approche DevOps](/module-13-devops.md)** *(Niveau Avancé)* 🔥

### 37. [Dockerisation d'Applications C++](/37-dockerisation/README.md)
- 37.1 [Création d'images Docker pour C++ (Ubuntu vs Alpine)](/37-dockerisation/01-images-docker.md)
- 37.2 [Multi-stage builds : Optimisation de la taille](/37-dockerisation/02-multi-stage-builds.md)
    - 37.2.1 [Stage de compilation](/37-dockerisation/02.1-stage-compilation.md)
    - 37.2.2 [Stage d'exécution minimal](/37-dockerisation/02.2-stage-execution.md)
- 37.3 [Gestion des librairies partagées dans les conteneurs](/37-dockerisation/03-librairies-partagees.md)
- 37.4 [Best practices : Sécurité et reproductibilité](/37-dockerisation/04-best-practices.md)
- **37.5 [Distroless images : Conteneurs minimaux](/37-dockerisation/05-distroless.md)** ⭐

### 38. [CI/CD pour C++](/38-cicd/README.md) ⭐
- 38.1 [Pipeline GitLab CI : Build → Test → Package](/38-cicd/01-gitlab-ci.md)
    - 38.1.1 [Structure .gitlab-ci.yml](/38-cicd/01.1-structure-gitlab-ci.md)
    - 38.1.2 [Jobs et stages](/38-cicd/01.2-jobs-stages.md)
- 38.2 [GitHub Actions : Workflows pour C++](/38-cicd/02-github-actions.md)
    - 38.2.1 [Structure workflow YAML](/38-cicd/02.1-structure-workflow.md)
    - 38.2.2 [Actions utiles pour C++](/38-cicd/02.2-actions-cpp.md)
- **38.3 [Accélération CI : ccache et sccache (cache distribué)](/38-cicd/03-acceleration-ci.md)** 🔥
    - 38.3.1 [Configuration ccache en CI](/38-cicd/03.1-ccache-ci.md)
    - 38.3.2 [sccache : Cache distribué cloud](/38-cicd/03.2-sccache.md)
- 38.4 [Automatisation : Tests, analyse statique, déploiement](/38-cicd/04-automatisation.md)
- 38.5 [Artifacts et gestion des releases](/38-cicd/05-artifacts-releases.md)
- 38.6 [Cross-compilation : ARM, RISC-V depuis x86_64](/38-cicd/06-cross-compilation.md)
- **38.7 [Matrix builds : Multi-compilateur, multi-version](/38-cicd/07-matrix-builds.md)** ⭐

### 39. [Packaging et Distribution](/39-packaging/README.md)
- 39.1 [Création de paquets DEB (Debian/Ubuntu)](/39-packaging/01-paquets-deb.md)
    - 39.1.1 [Structure d'un paquet DEB](/39-packaging/01.1-structure-deb.md)
    - 39.1.2 [Scripts de contrôle](/39-packaging/01.2-scripts-controle.md)
    - 39.1.3 [Construction avec dpkg-deb](/39-packaging/01.3-dpkg-deb.md)
- 39.2 [Création de paquets RPM (RedHat/CentOS)](/39-packaging/02-paquets-rpm.md)
- 39.3 [AppImage et distribution universelle](/39-packaging/03-appimage.md)
- 39.4 [Distribution via gestionnaires de paquets (apt, snap)](/39-packaging/04-distribution-apt-snap.md)

### 40. [Observabilité et Monitoring](/40-observabilite/README.md) ⭐
- **40.1 [Logging structuré : spdlog (recommandé) vs std::print](/40-observabilite/01-logging-spdlog.md)** 🔥
    - 40.1.1 [Installation et configuration spdlog](/40-observabilite/01.1-installation-spdlog.md)
    - 40.1.2 [Niveaux de log et sinks](/40-observabilite/01.2-niveaux-sinks.md)
    - 40.1.3 [Pattern de formatage](/40-observabilite/01.3-pattern-formatage.md)
- 40.2 [Métriques et instrumentation (Prometheus client)](/40-observabilite/02-prometheus.md)
- 40.3 [Tracing distribué (OpenTelemetry C++)](/40-observabilite/03-opentelemetry.md)
- 40.4 [Health checks et readiness probes](/40-observabilite/04-health-checks.md)
- **40.5 [Structured logging : JSON logs pour agrégation](/40-observabilite/05-json-logs.md)** ⭐

---

## **[PARTIE VI : SUJETS AVANCÉS](/partie-06-sujets-avances.md)**

---

## **[Module 14 : Optimisation de Performance](/module-14-optimisation.md)** *(Niveau Expert)*

### 41. [Optimisation CPU et Mémoire](/41-optimisation-cpu-memoire/README.md) ⭐
- 41.1 [Comprendre le cache CPU et la localité des données](/41-optimisation-cpu-memoire/01-cache-cpu.md)
    - 41.1.1 [Cache L1, L2, L3](/41-optimisation-cpu-memoire/01.1-niveaux-cache.md)
    - 41.1.2 [Cache lines et false sharing](/41-optimisation-cpu-memoire/01.2-cache-lines.md)
    - 41.1.3 [Data-oriented design](/41-optimisation-cpu-memoire/01.3-data-oriented.md)
- 41.2 [Branch prediction et optimisation des conditions](/41-optimisation-cpu-memoire/02-branch-prediction.md)
- 41.3 [SIMD et vectorisation (SSE, AVX)](/41-optimisation-cpu-memoire/03-simd-vectorisation.md)
    - 41.3.1 [Intrinsics](/41-optimisation-cpu-memoire/03.1-intrinsics.md)
    - 41.3.2 [Auto-vectorisation du compilateur](/41-optimisation-cpu-memoire/03.2-auto-vectorisation.md)
- 41.4 [Profile-Guided Optimization (PGO)](/41-optimisation-cpu-memoire/04-pgo.md)
- 41.5 [Link-Time Optimization (LTO)](/41-optimisation-cpu-memoire/05-lto.md)
- **41.6 [std::flat_map/flat_set et performance cache](/41-optimisation-cpu-memoire/06-flat-containers-perf.md)**

> 📎 *Application des conteneurs flat dans un contexte d'optimisation. Voir **section 14.4** (std::flat_map et std::flat_set C++23).*

### 42. [Programmation Bas Niveau](/42-programmation-bas-niveau/README.md)
- 42.1 [Inline assembly en C++](/42-programmation-bas-niveau/01-inline-assembly.md)
- 42.2 [Manipulation de bits et bitfields](/42-programmation-bas-niveau/02-manipulation-bits.md)
- 42.3 [Memory ordering et barrières mémoire](/42-programmation-bas-niveau/03-memory-ordering.md)
- 42.4 [Lock-free programming](/42-programmation-bas-niveau/04-lock-free.md)
    - 42.4.1 [Structures lock-free](/42-programmation-bas-niveau/04.1-structures-lock-free.md)
    - 42.4.2 [Compare-and-swap (CAS)](/42-programmation-bas-niveau/04.2-compare-and-swap.md)

---

## **[Module 15 : Interopérabilité](/module-15-interoperabilite.md)** *(Niveau Expert)*

### 43. [C++ et Autres Langages](/43-interoperabilite/README.md)
- 43.1 [C++ et C : extern "C" et ABI compatibility](/43-interoperabilite/01-cpp-et-c.md)
- **43.2 [Appeler du C++ depuis Python (pybind11)](/43-interoperabilite/02-pybind11.md)** ⭐
    - 43.2.1 [Installation et configuration](/43-interoperabilite/02.1-installation-pybind11.md)
    - 43.2.2 [Exposition de fonctions et classes](/43-interoperabilite/02.2-exposition.md)
    - 43.2.3 [Gestion des types et conversions](/43-interoperabilite/02.3-gestion-types.md)
    - 43.2.4 [nanobind : Alternative moderne et plus rapide](/43-interoperabilite/02.4-nanobind.md)
- **43.3 [C++ et Rust : FFI et interopérabilité](/43-interoperabilite/03-cpp-rust.md)** 🔥 *(étendu)*
    - 43.3.1 [extern "C" et liaison manuelle](/43-interoperabilite/03.1-extern-c-liaison.md)
    - **43.3.2 [cxx : Le bridge Rust↔C++ de référence](/43-interoperabilite/03.2-cxx-bridge.md)** ⭐
    - 43.3.3 [autocxx : Bindings automatiques C++ → Rust](/43-interoperabilite/03.3-autocxx.md) 
    - 43.3.4 [Stratégie de migration progressive : quand et comment introduire Rust](/43-interoperabilite/03.4-strategie-migration.md) 
    - 43.3.5 [Cas d'usage et retours d'expérience industrie 2025-2026](/43-interoperabilite/03.5-cas-usage-retours.md) 
- **43.4 [WebAssembly : Compiler du C++ pour le web (Emscripten)](/43-interoperabilite/04-webassembly.md)** ⭐
    - 43.4.1 [Installation Emscripten](/43-interoperabilite/04.1-installation-emscripten.md)
    - 43.4.2 [Compilation et intégration JavaScript](/43-interoperabilite/04.2-compilation-js.md)

---

## **[Module 16 : Patterns et Architecture](/module-16-patterns-architecture.md)** *(Niveau Expert)*

### 44. [Patterns de Conception en C++](/44-design-patterns/README.md)
- 44.1 [Singleton, Factory, Builder](/44-design-patterns/01-creational-patterns.md)
    - 44.1.1 [Singleton thread-safe](/44-design-patterns/01.1-singleton.md)
    - 44.1.2 [Factory et Abstract Factory](/44-design-patterns/01.2-factory.md)
    - 44.1.3 [Builder fluent](/44-design-patterns/01.3-builder.md)
- 44.2 [Observer, Strategy, Command](/44-design-patterns/02-behavioral-patterns.md)
- 44.3 [CRTP (Curiously Recurring Template Pattern)](/44-design-patterns/03-crtp.md)
- 44.4 [Type erasure et std::any](/44-design-patterns/04-type-erasure.md)
- 44.5 [Dependency Injection en C++](/44-design-patterns/05-dependency-injection.md)

### 45. [Sécurité en C++](/45-securite/README.md) ⭐
- 45.1 [Buffer overflows et protection](/45-securite/01-buffer-overflows.md)
- 45.2 [Integer overflows et underflows](/45-securite/02-integer-overflows.md)
- 45.3 [Use-after-free et temporal safety](/45-securite/03-use-after-free.md)
- 45.4 [Compilation avec protections](/45-securite/04-compilation-protections.md)
    - 45.4.1 [-fstack-protector](/45-securite/04.1-stack-protector.md)
    - 45.4.2 [-D_FORTIFY_SOURCE](/45-securite/04.2-fortify-source.md)
    - 45.4.3 [ASLR et PIE](/45-securite/04.3-aslr-pie.md)
- **45.5 [Fuzzing avec AFL++, LibFuzzer](/45-securite/05-fuzzing.md)** ⭐
    - 45.5.1 [Configuration AFL++](/45-securite/05.1-afl.md)
    - 45.5.2 [LibFuzzer et intégration](/45-securite/05.2-libfuzzer.md)
- **45.6 [Sécurité mémoire : Réponses concrètes du comité C++ en 2026](/45-securite/06-securite-memoire.md)** 🔥 
    - 45.6.1 [Contexte réglementaire 2026 : NSA, CISA, Union Européenne et Cyber Resilience Act](/45-securite/06.1-contexte-reglementaire.md) 
    - **45.6.2 [Safety Profiles : État de maturité et adoption en 2026](/45-securite/06.2-safety-profiles.md)** 🔥 
    - 45.6.3 [Hardening avec les sanitizers en production](/45-securite/06.3-hardening-sanitizers.md)
    - 45.6.4 [Stratégie de migration progressive et interopérabilité Rust](/45-securite/06.4-migration-interop-rust.md)
    - **45.6.5 [Bilan : C++ safe-by-default est-il atteignable ? (état 2026)](/45-securite/06.5-bilan-safe-cpp-2026.md)** 

---

## **[PARTIE VII : PROJET ET PROFESSIONNALISATION](/partie-07-projet-professionnalisation.md)**

---

## **[Module 17 : Architecture de Projet Professionnel](/module-17-architecture-projet.md)** *(Niveau Expert)*

### 46. [Organisation et Standards](/46-organisation-standards/README.md)
- 46.1 [Organisation des répertoires (src/, include/, tests/, docs/)](/46-organisation-standards/01-organisation-repertoires.md)
- 46.2 [Séparation .h/.cpp et compilation incrémentale](/46-organisation-standards/02-separation-h-cpp.md)
- 46.3 [Namespaces et éviter la pollution globale](/46-organisation-standards/03-namespaces.md)
- 46.4 [Documentation : Doxygen et commentaires](/46-organisation-standards/04-documentation-doxygen.md)
    - 46.4.1 [Syntaxe Doxygen](/46-organisation-standards/04.1-syntaxe-doxygen.md)
    - 46.4.2 [Génération de documentation](/46-organisation-standards/04.2-generation-doc.md)
- 46.5 [Standards de codage](/46-organisation-standards/05-standards-codage.md)
    - 46.5.1 [Google C++ Style Guide](/46-organisation-standards/05.1-google-style.md)
    - 46.5.2 [LLVM Style](/46-organisation-standards/05.2-llvm-style.md)
    - 46.5.3 [C++ Core Guidelines](/46-organisation-standards/05.3-core-guidelines.md)

### 47. [Collaboration et Maintenance](/47-collaboration/README.md) ⭐
- 47.1 [Git et workflows (GitFlow, trunk-based)](/47-collaboration/01-git-workflows.md)
- **47.2 [Pre-commit hooks : Automatisation de la qualité avant commit](/47-collaboration/02-pre-commit-hooks.md)** 🔥
    - 47.2.1 [Installation du framework pre-commit](/47-collaboration/02.1-installation-pre-commit.md)
    - 47.2.2 [Hooks essentiels pour C++](/47-collaboration/02.2-hooks-cpp.md)
- **47.3 [Configuration pre-commit : clang-format, clang-tidy, tests rapides](/47-collaboration/03-configuration-pre-commit.md)** 🔥
    - 47.3.1 [.pre-commit-config.yaml](/47-collaboration/03.1-pre-commit-config.md)
    - 47.3.2 [Intégration clang-format](/47-collaboration/03.2-integration-clang-format.md)
    - 47.3.3 [Intégration clang-tidy](/47-collaboration/03.3-integration-clang-tidy.md)
- 47.4 [Code reviews efficaces](/47-collaboration/04-code-reviews.md)
- 47.5 [Gestion de la dette technique](/47-collaboration/05-dette-technique.md)
- 47.6 [Semantic Versioning et changelogs](/47-collaboration/06-semantic-versioning.md)

### 48. [Ressources et Veille Technologique](/48-ressources/README.md)
- 48.1 [Livres de référence](/48-ressources/01-livres.md)
    - 48.1.1 [Effective C++ (Scott Meyers)](/48-ressources/01.1-effective-cpp.md)
    - 48.1.2 [C++ Concurrency in Action (Anthony Williams)](/48-ressources/01.2-concurrency-in-action.md)
    - 48.1.3 [A Tour of C++ (Bjarne Stroustrup)](/48-ressources/01.3-tour-of-cpp.md)
    - 48.1.4 [Embracing Modern C++ Safely (Lakos, Romeo, Khlebnikov, Meredith)](/48-ressources/01.4-embracing-modern-cpp.md)
- 48.2 [Conférences](/48-ressources/02-conferences.md)
    - 48.2.1 [CppCon](/48-ressources/02.1-cppcon.md)
    - 48.2.2 [Meeting C++](/48-ressources/02.2-meeting-cpp.md)
    - 48.2.3 [C++ Now](/48-ressources/02.3-cpp-now.md)
    - 48.2.4 [ACCU Conference](/48-ressources/02.4-accu.md)
- 48.3 [Standards et évolutions futures (C++26 ratifié, cap sur C++29)](/48-ressources/03-standards-futurs.md) 
    - 48.3.1 [Calendrier du comité ISO et processus de standardisation](/48-ressources/03.1-calendrier-iso.md)
    - 48.3.2 [Suivre les proposals (open-std.org, GitHub)](/48-ressources/03.2-suivre-proposals.md)
    - **48.3.3 [Premières proposals C++29 : Ce qui se prépare](/48-ressources/03.3-cpp29-preview.md)** 
- 48.4 [Communautés et forums](/48-ressources/04-communautes.md)
    - 48.4.1 [Stack Overflow](/48-ressources/04.1-stackoverflow.md)
    - 48.4.2 [Reddit r/cpp](/48-ressources/04.2-reddit-cpp.md)
    - 48.4.3 [Discord C++](/48-ressources/04.3-discord-cpp.md)
    - 48.4.4 [Compiler Explorer (godbolt.org)](/48-ressources/04.4-compiler-explorer.md)

---

## **[Conclusion](/conclusion/README.md)**

- [Récapitulatif des compétences acquises](/conclusion/01-recapitulatif.md)
- [Trajectoires professionnelles](/conclusion/02-trajectoires.md)
    - System Programming
    - Backend haute performance
    - Embedded / IoT
    - Finance quantitative
    - DevOps / SRE
    - Game Development / Moteurs 3D
- [Ressources pour aller plus loin](/conclusion/03-ressources.md)
- **[Checklist du développeur C++ Cloud Native](/conclusion/04-checklist-cloud-native.md)** ⭐

---

## **📊 Métriques de la Formation**

| Métrique | Valeur |
|----------|--------|
| **Parties** | 7 |
| **Modules** | 17 |
| **Chapitres** | 48 |
| **Sections** | ~305 |
| **Fichiers Markdown** | ~415 |
| **Durée estimée** | 120-170h |
| **Niveau couvert** | Débutant → Expert |
| **Standards C++** | C++11 → C++26 (ratifié) |
| **Compilateurs couverts** | GCC 15, Clang 20 |
| **Dernière mise à jour** | Mars 2026 |

---

## **🔥 Légende**

- ⭐ = Section importante / Best practice
- 🔥 = Section critique / Différenciateur professionnel
- 📎 = Renvoi vers une section liée (évite les doublons)
- 💡 = Note pédagogique


---

 
