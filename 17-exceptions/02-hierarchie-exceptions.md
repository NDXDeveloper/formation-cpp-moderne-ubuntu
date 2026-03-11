🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 17.2 — Hiérarchie des exceptions standard (`std::exception`)

## Module 6 : Gestion des Erreurs et Robustesse · Niveau Intermédiaire

---

## Objectifs de la section

À l'issue de cette section, vous serez capable :

- de décrire l'arbre d'héritage complet des exceptions de la bibliothèque standard,
- de distinguer les familles `std::logic_error` et `std::runtime_error` et leur sémantique respective,
- de choisir la classe d'exception appropriée selon la nature de l'erreur rencontrée,
- de capturer les exceptions au bon niveau de granularité grâce au polymorphisme,
- d'identifier les exceptions levées implicitement par le langage et la bibliothèque standard.

---

## Pourquoi une hiérarchie ?

Le mécanisme `catch` de C++ repose sur la correspondance de types : un bloc `catch(const T&)` capture toute exception de type `T` ou de tout type dérivé de `T`. Ce comportement polymorphique n'a de sens que si les classes d'exceptions sont organisées dans une **hiérarchie d'héritage** cohérente.

La bibliothèque standard fournit une telle hiérarchie, enracinée dans `std::exception`. Elle offre deux avantages majeurs. D'abord, elle permet d'écrire un `catch` générique qui intercepte toutes les exceptions standard avec un seul bloc. Ensuite, elle impose une convention sémantique partagée : chaque branche de l'arbre correspond à une **catégorie d'erreur** reconnue par l'ensemble de l'écosystème C++.

Connaître cette hiérarchie est indispensable pour écrire des handlers efficaces, pour choisir la bonne classe de base lorsque vous créerez vos propres exceptions (section 17.3), et pour comprendre les exceptions que lèvent les conteneurs, les algorithmes et les flux de la STL.

---

## L'arbre complet des exceptions standard

Toutes les classes présentées ici sont déclarées dans l'en-tête `<stdexcept>`, à l'exception de `std::exception` elle-même (déclarée dans `<exception>`) et de quelques classes spécialisées issues d'autres en-têtes, signalées au cas par cas.

```
std::exception                              <exception>
├── std::logic_error                        <stdexcept>
│   ├── std::invalid_argument               <stdexcept>
│   ├── std::domain_error                   <stdexcept>
│   ├── std::length_error                   <stdexcept>
│   ├── std::out_of_range                   <stdexcept>
│   └── std::future_error                   <future>
├── std::runtime_error                      <stdexcept>
│   ├── std::range_error                    <stdexcept>
│   ├── std::overflow_error                 <stdexcept>
│   ├── std::underflow_error                <stdexcept>
│   ├── std::system_error                   <system_error>
│   │   └── std::filesystem::filesystem_error  <filesystem>
│   └── std::format_error                   <format>
├── std::bad_alloc                          <new>
│   └── std::bad_array_new_length           <new>
├── std::bad_cast                           <typeinfo>
├── std::bad_typeid                         <typeinfo>
├── std::bad_function_call                  <functional>
├── std::bad_weak_ptr                       <memory>
├── std::bad_variant_access                 <variant>
├── std::bad_optional_access                <optional>
└── std::bad_expected_access<E>             <expected>    (C++23)
```

L'arbre se divise en trois grandes familles : les erreurs logiques (`std::logic_error`), les erreurs d'exécution (`std::runtime_error`), et les exceptions autonomes directement rattachées à `std::exception`. Examinons chacune en détail.

---

## La racine : `std::exception`

La classe `std::exception`, définie dans `<exception>`, est la racine de toute la hiérarchie. Son interface est volontairement minimale :

```cpp
#include <exception>

class exception {  
public:  
    virtual ~exception() noexcept;
    virtual const char* what() const noexcept;
};
```

La méthode virtuelle `what()` retourne une chaîne C décrivant l'erreur. Comme elle est déclarée `noexcept`, elle ne doit jamais lever d'exception elle-même — une contrainte logique puisqu'on l'appelle précisément en contexte de gestion d'erreur.

Capturer `const std::exception&` constitue le filet de sécurité le plus large : grâce au polymorphisme, ce handler intercepte toute exception standard et toute exception utilisateur qui en dérive.

```cpp
try {
    // ... code susceptible de lever une exception ...
} catch (const std::exception& e) {
    std::cerr << "Erreur : " << e.what() << '\n';
}
```

> ⚠️ **Capture par référence constante.** Capturez toujours les exceptions par `const&`, jamais par valeur. La capture par valeur provoque un *slicing* : si l'exception est d'un type dérivé, les données spécifiques au type dérivé sont perdues et `what()` peut retourner un message incorrect ou générique.

---

## Famille 1 : `std::logic_error` — les erreurs de logique du programme

`std::logic_error` et ses dérivées représentent des erreurs qui résultent d'une **violation de précondition** ou d'une **erreur de logique interne** au programme. En théorie, ces erreurs pourraient être évitées par une vérification préalable dans le code appelant. Elles signalent un défaut dans le raisonnement du programmeur plutôt qu'une condition externe imprévisible.

```cpp
#include <stdexcept>

// Constructeur commun à toutes les classes de cette famille
std::logic_error::logic_error(const std::string& message);  
std::logic_error::logic_error(const char* message);  
```

### `std::invalid_argument`

Signale qu'un argument passé à une fonction est invalide, indépendamment de l'état du programme. C'est l'une des exceptions les plus couramment utilisées.

```cpp
#include <stdexcept>
#include <string>

int parse_port(const std::string& str) {
    int port = std::stoi(str);  // peut lever std::invalid_argument si str n'est pas un nombre
    if (port < 0 || port > 65535) {
        throw std::invalid_argument("Port hors limites [0, 65535] : " + str);
    }
    return port;
}
```

La fonction `std::stoi` elle-même lève `std::invalid_argument` si la chaîne ne contient pas de représentation numérique valide. C'est un exemple d'exception levée directement par la bibliothèque standard.

### `std::domain_error`

Signale qu'une opération mathématique reçoit une valeur en dehors de son domaine de définition. En pratique, cette exception est rarement levée par la bibliothèque standard (les fonctions `<cmath>` utilisent `errno` ou retournent `NaN`), mais elle est utile pour les bibliothèques numériques utilisateur.

```cpp
#include <stdexcept>
#include <cmath>

double logarithme(double x) {
    if (x <= 0.0) {
        throw std::domain_error("logarithme() : argument non positif");
    }
    return std::log(x);
}
```

### `std::length_error`

Signale une tentative de créer un objet dont la taille dépasse la capacité maximale autorisée. C'est typiquement l'exception levée par les conteneurs de la STL lorsque la taille demandée excède `max_size()`.

```cpp
#include <vector>
#include <string>

void demonstration_length_error() {
    std::string s;
    // Tenter de redimensionner au-delà de la capacité maximale :
    s.resize(s.max_size() + 1);  // → lève std::length_error
}
```

### `std::out_of_range`

Signale un accès à un indice ou une clé hors des limites valides. C'est l'exception la plus fréquemment rencontrée au quotidien, levée par la méthode `at()` de la plupart des conteneurs et par `std::stoi` / `std::stol` lorsque la valeur convertie déborde du type cible.

```cpp
#include <vector>
#include <stdexcept>
#include <print>

void acceder(const std::vector<int>& v, std::size_t index) {
    try {
        int valeur = v.at(index);  // vérification de bornes → std::out_of_range
        std::print("v[{}] = {}\n", index, valeur);
    } catch (const std::out_of_range& e) {
        std::print(stderr, "Indice {} hors limites (taille : {})\n", index, v.size());
    }
}
```

> 💡 **`at()` vs `operator[]`** — La méthode `at()` effectue une vérification de bornes et lève `std::out_of_range` en cas de dépassement. L'opérateur `[]`, lui, ne vérifie rien : un accès hors limites via `[]` est un **comportement indéfini** (undefined behavior). Utilisez `at()` lorsque l'indice provient de données non fiables (entrée utilisateur, fichier de configuration). Préférez `[]` dans les boucles internes où la validité de l'indice est garantie par construction, pour éviter le coût de la vérification.

### `std::future_error`

Définie dans `<future>`, cette exception est levée lorsqu'une opération sur un `std::future` ou un `std::promise` viole les règles d'utilisation : extraction multiple du résultat, promesse détruite sans valeur, etc. Elle transporte un `std::error_code` accessible via sa méthode `code()`.

```cpp
#include <future>
#include <print>

void demonstration_future_error() {
    std::promise<int> p;
    auto f = p.get_future();

    p.set_value(42);          // remplit la promesse
    int resultat = f.get();   // première extraction : OK (retourne 42)

    try {
        f.get();  // deuxième extraction → std::future_error
    } catch (const std::future_error& e) {
        std::print(stderr, "Future error : {} (code : {})\n",
                   e.what(), e.code().value());
    }
}
```

### La sémantique commune aux `logic_error`

Toutes les classes dérivées de `std::logic_error` partagent une intention commune : signaler une erreur que le programmeur **aurait pu éviter**. C'est pourquoi certains projets considèrent qu'une `logic_error` ne devrait jamais être capturée en production — elle devrait plutôt être traitée comme un bug à corriger dans le code source. En C++26, les **contrats** (section 17.6) offrent un mécanisme plus adapté pour exprimer ces préconditions, ce qui pourrait réduire progressivement l'usage des `logic_error` pour ce type de vérification.

---

## Famille 2 : `std::runtime_error` — les erreurs imprévisibles à l'exécution

`std::runtime_error` et ses dérivées représentent des erreurs que le programme ne peut pas raisonnablement prévenir à l'avance. Elles résultent de conditions externes : défaillance matérielle, état inattendu du système d'exploitation, données corrompues. C'est la famille d'exceptions la plus couramment utilisée comme base pour les exceptions personnalisées.

### `std::range_error`

Signale qu'un résultat de calcul ne peut pas être représenté dans le type cible pour des raisons de plage. En pratique, cette exception est peu utilisée par la bibliothèque standard elle-même.

### `std::overflow_error`

Signale un dépassement arithmétique positif. Levée par exemple par certaines implémentations de `std::bitset::to_ulong()` lorsque la valeur dépasse la capacité d'un `unsigned long`.

```cpp
#include <bitset>
#include <stdexcept>

void demonstration_overflow() {
    std::bitset<128> bits;
    bits.set();  // tous les bits à 1

    try {
        unsigned long val = bits.to_ulong();  // → std::overflow_error
    } catch (const std::overflow_error& e) {
        // La valeur 128 bits ne tient pas dans un unsigned long
    }
}
```

### `std::underflow_error`

Le pendant symétrique de `std::overflow_error`, signalant un dépassement arithmétique négatif. Cette exception est rarement levée dans la pratique par la bibliothèque standard.

### `std::system_error`

Définie dans `<system_error>`, cette exception représente une erreur provenant du système d'exploitation ou d'une bibliothèque système. Elle transporte un `std::error_code` qui combine un code numérique et une catégorie d'erreur, permettant une identification précise de la cause.

```cpp
#include <system_error>
#include <fstream>
#include <cerrno>
#include <print>

void ouvrir_ou_lancer(const std::string& chemin) {
    std::ifstream f(chemin);
    if (!f.is_open()) {
        throw std::system_error(
            std::make_error_code(std::errc::no_such_file_or_directory),
            "Échec d'ouverture de " + chemin
        );
    }
}

void appelant() {
    try {
        ouvrir_ou_lancer("/chemin/inexistant");
    } catch (const std::system_error& e) {
        std::print(stderr, "Erreur système : {} (code : {}, catégorie : {})\n",
                   e.what(), e.code().value(), e.code().category().name());
    }
}
```

`std::system_error` est particulièrement importante en programmation système Linux. Les opérations sur les threads (`std::thread`, `std::mutex`), les futures et les sockets POSIX wrappés en C++ peuvent toutes lever des `system_error`. Son sous-type `std::filesystem::filesystem_error` (défini dans `<filesystem>`) ajoute deux `std::filesystem::path` pour identifier les chemins impliqués dans l'erreur.

```cpp
#include <filesystem>
#include <print>

void copier_fichier(const std::filesystem::path& src, const std::filesystem::path& dst) {
    try {
        std::filesystem::copy(src, dst);
    } catch (const std::filesystem::filesystem_error& e) {
        std::print(stderr, "Erreur FS : {}\n", e.what());
        std::print(stderr, "  path1 : {}\n", e.path1().string());
        std::print(stderr, "  path2 : {}\n", e.path2().string());
        std::print(stderr, "  code  : {}\n", e.code().message());
    }
}
```

### `std::format_error`

Ajoutée avec C++20 dans `<format>`, cette exception est levée lorsqu'une chaîne de format est invalide dans un contexte qui ne peut pas être vérifié à la compilation.

---

## Famille 3 : les exceptions autonomes

Plusieurs classes d'exceptions héritent directement de `std::exception` sans passer par `logic_error` ni `runtime_error`. Elles sont levées automatiquement par le langage ou par des composants spécifiques de la bibliothèque standard.

### `std::bad_alloc` — échec d'allocation mémoire

Levée par l'opérateur `new` lorsque le système ne peut pas satisfaire une demande d'allocation. C'est l'une des rares exceptions qu'on ne peut généralement pas prévenir par une vérification préalable. Sa classe dérivée `std::bad_array_new_length` signale spécifiquement qu'une taille invalide (négative ou excessive) a été passée à `new[]`.

```cpp
#include <new>
#include <print>

void allocation_massive() {
    try {
        // Tentative d'allocation de ~8 To de mémoire
        auto* p = new double[1'000'000'000'000ULL];
        delete[] p;
    } catch (const std::bad_alloc& e) {
        std::print(stderr, "Allocation échouée : {}\n", e.what());
    }
}
```

> 💡 En code moderne, les allocations manuelles via `new` devraient être rares (chapitre 9 — smart pointers). Cependant, `std::bad_alloc` reste pertinente : `std::vector::push_back`, `std::string::append` et toute opération qui alloue dynamiquement en interne peuvent la lever.

### `std::bad_cast` — échec de `dynamic_cast`

Levée lorsqu'un `dynamic_cast` vers un type référence échoue. Notez que `dynamic_cast` sur un pointeur retourne `nullptr` en cas d'échec plutôt que de lever une exception — seul le cast vers une **référence** lève `std::bad_cast`, puisqu'il n'existe pas de « référence nulle ».

```cpp
#include <typeinfo>

struct Base { virtual ~Base() = default; };  
struct Derivee : Base {};  
struct Autre : Base {};  

void cast_reference(Base& b) {
    try {
        Derivee& d = dynamic_cast<Derivee&>(b);  // → std::bad_cast si b n'est pas Derivee
    } catch (const std::bad_cast& e) {
        // b n'était pas du type Derivee
    }
}
```

### `std::bad_typeid`

Levée lorsque `typeid` est appliqué au déréférencement d'un pointeur polymorphique nul.

### `std::bad_function_call`

Levée par `std::function::operator()` lorsqu'on tente d'appeler un objet `std::function` qui n'encapsule aucun callable (état vide).

```cpp
#include <functional>

void appel_vide() {
    std::function<void()> fn;  // vide — aucun callable assigné

    try {
        fn();  // → std::bad_function_call
    } catch (const std::bad_function_call& e) {
        // fn n'a pas de cible
    }
}
```

### `std::bad_weak_ptr`

Levée par le constructeur de `std::shared_ptr` qui accepte un `std::weak_ptr`, lorsque la ressource pointée a déjà été détruite (le `weak_ptr` est expiré).

### `std::bad_variant_access` et `std::bad_optional_access`

Introduites respectivement par C++17, ces exceptions sont levées lorsque vous tentez d'accéder à la valeur d'un `std::variant` ou d'un `std::optional` via `std::get` / `.value()` alors que l'objet ne contient pas le type attendu (variant) ou est vide (optional).

```cpp
#include <optional>
#include <variant>

void acces_invalide() {
    std::optional<int> vide;
    // vide.value();  → std::bad_optional_access

    std::variant<int, std::string> v = 42;
    // std::get<std::string>(v);  → std::bad_variant_access
}
```

### `std::bad_expected_access<E>` (C++23)

Ajoutée avec `std::expected` en C++23, cette exception est levée par la méthode `.value()` lorsqu'un objet `std::expected<T, E>` contient une erreur plutôt qu'une valeur. Elle transporte l'erreur de type `E`, accessible via sa méthode `error()`.

---

## Capturer au bon niveau de granularité

La structure hiérarchique permet d'organiser les blocs `catch` du plus spécifique au plus général. Le runtime C++ parcourt les handlers dans l'ordre de déclaration et sélectionne le **premier** qui correspond au type de l'exception — il est donc essentiel de placer les types les plus dérivés en premier.

```cpp
#include <stdexcept>
#include <filesystem>
#include <print>

void traiter() {
    try {
        // ... opérations susceptibles de lever diverses exceptions ...
    }
    // 1. Le plus spécifique d'abord
    catch (const std::filesystem::filesystem_error& e) {
        std::print(stderr, "Erreur système de fichiers : {}\n  path1: {}\n",
                   e.what(), e.path1().string());
    }
    // 2. Famille intermédiaire
    catch (const std::system_error& e) {
        std::print(stderr, "Erreur système : {} (code {})\n",
                   e.what(), e.code().value());
    }
    // 3. Toute erreur d'exécution
    catch (const std::runtime_error& e) {
        std::print(stderr, "Erreur runtime : {}\n", e.what());
    }
    // 4. Toute erreur de logique
    catch (const std::logic_error& e) {
        std::print(stderr, "Erreur logique (bug probable) : {}\n", e.what());
    }
    // 5. Filet de sécurité — toute exception standard
    catch (const std::exception& e) {
        std::print(stderr, "Exception non catégorisée : {}\n", e.what());
    }
}
```

> ⚠️ **Ordre inversé = handler mort.** Si vous placez `catch (const std::exception&)` avant `catch (const std::out_of_range&)`, le second handler ne sera jamais atteint. GCC et Clang émettent un warning dans ce cas si vous compilez avec `-Wall`.

---

## Résumé décisionnel : quelle exception standard utiliser ?

Pour choisir la classe d'exception standard la plus adaptée, posez-vous les questions suivantes :

L'erreur provient-elle d'un argument invalide passé par l'appelant ? Si l'argument est fondamentalement incohérent (type incorrect, valeur absurde), utilisez `std::invalid_argument`. Si c'est un indice ou une clé hors limites d'une structure de données, utilisez `std::out_of_range`.

L'erreur est-elle un dépassement de capacité interne ? Si un conteneur ou un objet ne peut pas accueillir la taille demandée, `std::length_error` est appropriée.

L'erreur provient-elle d'un appel système ou d'une interaction avec l'OS ? Utilisez `std::system_error` avec un `std::error_code` qui identifie précisément la cause. Pour les opérations du système de fichiers, préférez `std::filesystem::filesystem_error`.

L'erreur est-elle une condition d'exécution imprévisible sans catégorie plus spécifique ? `std::runtime_error` est le choix par défaut pour les erreurs d'exécution qui n'entrent dans aucune sous-catégorie.

Aucune classe standard ne correspond à votre besoin ? Créez une exception personnalisée (section 17.3), en héritant de la classe standard la plus proche sémantiquement — généralement `std::runtime_error` pour les erreurs d'exécution ou `std::logic_error` pour les violations de précondition.

---

## Points clés à retenir

`std::exception` est la racine commune. La capturer par `const std::exception&` constitue le filet de sécurité universel pour toutes les exceptions standard.

`std::logic_error` regroupe les erreurs évitables par le programmeur : arguments invalides, indices hors limites, dépassements de capacité. En C++26, les contrats offriront souvent une meilleure alternative pour exprimer ces préconditions.

`std::runtime_error` regroupe les erreurs imprévisibles liées à l'environnement d'exécution. C'est la base recommandée pour la majorité des exceptions personnalisées.

Les exceptions autonomes (`bad_alloc`, `bad_cast`, `bad_variant_access`, etc.) sont levées automatiquement par le langage et la STL. Les connaître permet de ne pas être surpris par des exceptions inattendues en production.

L'ordre des blocs `catch` doit toujours aller du type le plus dérivé au type le plus général pour éviter les handlers inaccessibles.

---


⏭️ [Exceptions personnalisées](/17-exceptions/03-exceptions-personnalisees.md)
