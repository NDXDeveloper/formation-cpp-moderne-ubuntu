🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 10.2 std::move : Transfert de propriété sans copie

## Ce que std::move fait réellement

`std::move` est probablement la fonction la plus mal nommée de toute la bibliothèque standard. Son nom suggère qu'elle déplace quelque chose. **Elle ne déplace rien.** C'est un **cast** — une conversion de type — et rien d'autre.

`std::move` prend une expression (généralement une lvalue) et la convertit en **rvalue reference** (`T&&`). Cette conversion donne la **permission** au code récepteur (constructeur de déplacement, opérateur d'affectation par déplacement) de traiter l'objet comme déplaçable. Le déplacement réel — le vol de ressources — est effectué par le constructeur ou l'opérateur, pas par `std::move` lui-même.

### L'implémentation

`std::move` est d'une simplicité désarmante. Voici une version simplifiée, fonctionnellement équivalente :

```cpp
// Version simplifiée (l'implémentation réelle gère plus de cas)
template <typename T>  
constexpr std::remove_reference_t<T>&& move(T&& arg) noexcept {  
    return static_cast<std::remove_reference_t<T>&&>(arg);
}
```

C'est un `static_cast` vers `T&&`. Rien de plus. Pas d'allocation, pas de copie, pas de modification de l'objet. Au moment de la compilation, `std::move` est intégralement éliminé — il ne génère **aucune instruction machine**. C'est une information pour le compilateur, pas une opération à l'exécution.

### Le modèle mental correct

Pensez à `std::move` comme à un **panneau « à donner »** que vous collez sur un objet. Le panneau ne déplace pas l'objet. Il annonce que le propriétaire actuel n'en a plus besoin et que quiconque le veut peut prendre ses composants internes. Si personne ne se manifeste (aucun constructeur de déplacement n'est appelé), l'objet reste intact.

```cpp
std::string s = "Hello";

// Coller le panneau "à donner"
std::string&& ref = std::move(s);
// s est toujours "Hello" — personne n'a encore pris les ressources

// Quelqu'un se manifeste — le constructeur de déplacement
std::string t = std::move(s);
// MAINTENANT s est vidé — le constructeur de déplacement a volé le buffer
```

---

## Quand utiliser std::move

`std::move` doit être utilisé quand **deux conditions** sont réunies simultanément :

1. Vous avez une **lvalue** dont vous voulez transférer les ressources.
2. Vous **n'utiliserez plus** cette lvalue après le transfert (ou seulement pour la détruire, la réaffecter, ou la tester).

### Transfert vers une fonction qui prend la propriété

```cpp
void enregistrer(std::unique_ptr<Config> config);

auto config = std::make_unique<Config>("prod.yaml");
// config est une lvalue — on ne peut pas la passer par valeur directement
enregistrer(std::move(config));  // ✅ Transfert de propriété
// config est nul — ne plus l'utiliser
```

### Transfert vers un membre de classe

Un pattern extrêmement fréquent : un constructeur qui prend la propriété d'un argument pour le stocker en membre :

```cpp
class Service {
    std::string nom_;
    std::unique_ptr<Logger> logger_;

public:
    Service(std::string nom, std::unique_ptr<Logger> logger)
        : nom_(std::move(nom))          // ✅ nom est copié dans le paramètre,
                                         //    puis déplacé dans le membre
        , logger_(std::move(logger))     // ✅ logger est déplacé dans le membre
    {}
};
```

Ici, `nom` est passé **par valeur** (une copie a lieu à l'appel), puis déplacé dans le membre. Ce pattern « pass by value + move » est idiomatique quand la fonction doit de toute façon stocker une copie de l'argument.

### Transfert dans un conteneur

```cpp
std::vector<std::string> noms;  
std::string s = "un nom très long qui dépasse le small string optimization";  

noms.push_back(s);              // Copie de s dans le vector  
noms.push_back(std::move(s));   // Déplacement — s est vidé, pas de copie  
```

### Retour explicite depuis une variable locale (rarement nécessaire)

Dans la grande majorité des cas, le compilateur déplace (ou élide) automatiquement les variables locales retournées par une fonction. L'usage explicite de `std::move` dans un `return` est **rarement nécessaire et souvent contre-productif** :

```cpp
std::string construire() {
    std::string resultat = "Hello";
    return resultat;          // ✅ NRVO ou déplacement implicite
    // return std::move(resultat);  // ⚠️ Empêche le NRVO — PIRE
}
```

Ce point important est traité en profondeur en [section 10.5](/10-move-semantics/05-rvo-copy-elision.md).

---

## Quand NE PAS utiliser std::move

L'enthousiasme des développeurs qui découvrent `std::move` les pousse souvent à en mettre partout. C'est une erreur. Un `std::move` mal placé est au mieux inutile, au pire nuisible.

### Ne pas move un objet qu'on va encore utiliser

C'est l'erreur la plus dangereuse. Après un `std::move`, l'objet source est dans un **état valide mais indéterminé**. Le lire, le déréférencer ou appeler des méthodes qui supposent un état non-vide est un bug :

```cpp
std::vector<int> donnees = {1, 2, 3, 4, 5};  
auto sauvegarde = std::move(donnees);  

// ❌ donnees est dans un état indéterminé
std::print("Taille : {}\n", donnees.size());  // Peut afficher 0, peut planter  
donnees[0] = 42;                               // 💥 Comportement indéfini potentiel  

for (auto& x : donnees) { /* ... */ }          // ❌ Résultat imprévisible
```

Les seules opérations sûres après un `std::move` sont :

- **Destruction** (sortie de scope) — toujours sûr.
- **Réaffectation** — `donnees = {10, 20, 30};` remet l'objet dans un état connu.
- **Opérations sans préconditions** — `donnees.size()` est techniquement sûr pour `std::vector` (retourne 0 ou la taille résiduelle), mais s'y fier est fragile.

### Ne pas move des objets const

`std::move` sur un objet `const` produit un `const T&&`. Aucun constructeur de déplacement n'accepte un `const T&&` (il ne pourrait pas vider l'objet source). Le compilateur sélectionne silencieusement le constructeur de **copie** — vous payez le coût d'une copie en croyant avoir un déplacement :

```cpp
const std::string s = "Hello";  
std::string t = std::move(s);  // ⚠️ COPIE, pas déplacement !  
                                 // std::move(s) → const string&&
                                 // Pas de match avec string(string&&)
                                 // Fallback vers string(const string&) → copie
```

Le code compile, ne produit aucun warning par défaut, et fait exactement le contraire de ce que le développeur attend. C'est l'un des pièges les plus sournois du C++.

> 💡 Activez le warning `-Wpessimizing-move` (GCC, Clang) pour que le compilateur signale les `std::move` sur des objets `const` ou dans des contextes où le move est contre-productif.

### Ne pas move dans un return (sauf cas rares)

Le compilateur applique automatiquement le déplacement (ou mieux, le NRVO) sur les variables locales retournées. Ajouter `std::move` **empêche le NRVO** :

```cpp
std::vector<int> generer() {
    std::vector<int> v = {1, 2, 3};
    
    return v;              // ✅ NRVO : aucune copie, aucun déplacement
    // return std::move(v); // ⚠️ Empêche NRVO, force un déplacement
}
```

La seule situation où `return std::move(x)` est justifié est quand la variable retournée n'est **pas éligible au NRVO** — par exemple quand elle est un paramètre de la fonction ou un membre :

```cpp
std::string transformer(std::string input) {
    input += " transformé";
    return std::move(input);  // ✅ Justifié : input est un paramètre,
                               //    pas une variable locale → pas de NRVO
                               // Note : depuis C++11, le move implicite s'applique
                               //        aussi aux paramètres dans la plupart des cas
}
```

> En pratique, les compilateurs modernes (GCC 15, Clang 20) appliquent le déplacement implicite même sur les paramètres de fonction. Le `std::move` explicite dans le `return` est de moins en moins nécessaire. Voir [section 10.5](/10-move-semantics/05-rvo-copy-elision.md).

### Ne pas move des types primitifs

Déplacer un `int`, un `double`, un `bool` ou un pointeur brut est identique à les copier — ce sont des types scalaires sans ressources internes à transférer. `std::move` est inutile et ajoute du bruit :

```cpp
int x = 42;  
int y = std::move(x);  // ⚠️ Inutile — copie un int, comme sans std::move  
                         // x vaut toujours 42
```

### Ne pas move des objets small-buffer-optimized

Certains types utilisent une optimisation interne (SSO pour `std::string`, SBO pour `std::function`) où les petites données sont stockées directement dans l'objet, sans allocation sur le tas. Pour ces objets, le déplacement est une copie déguisée — il n'y a pas de pointeur interne à voler :

```cpp
std::string court = "Hi";           // 2 caractères — SSO actif  
std::string t = std::move(court);   // "Déplacement" mais en réalité une copie  
                                     // des ~24 octets du buffer interne SSO
```

Le déplacement reste correct (l'objet source est mis dans un état vide), mais le gain de performance est nul pour les petites chaînes. Ce n'est pas une raison d'éviter `std::move` — c'est une raison de ne pas s'attendre à un gain miraculeux dans tous les cas.

---

## std::move et la STL

La bibliothèque standard est intégralement conçue pour tirer parti de `std::move`. Voici les interactions les plus courantes.

### Conteneurs

Tous les conteneurs de la STL sont déplaçables. Le déplacement d'un conteneur transfère ses ressources internes (buffer, nœuds, buckets) en temps constant :

```cpp
std::vector<int> v1(1'000'000, 42);  
std::vector<int> v2 = std::move(v1);  
// v2 possède le million d'entiers
// v1 est vide (size() == 0, capacity() == 0)

std::map<std::string, int> m1 = {{"a", 1}, {"b", 2}};  
std::map<std::string, int> m2 = std::move(m1);  
// m2 possède l'arbre de m1
// m1 est vide
```

### std::swap optimisé

`std::swap` utilise la sémantique de mouvement depuis C++11. Échanger deux objets lourds est désormais trois déplacements (pas trois copies) :

```cpp
std::vector<int> a(1'000'000);  
std::vector<int> b(2'000'000);  

std::swap(a, b);
// Équivalent à :
// auto temp = std::move(a);   // 1 déplacement
// a = std::move(b);           // 1 déplacement
// b = std::move(temp);        // 1 déplacement
// Total : 3 transferts de pointeurs, 0 copie de données
```

### Algorithmes de la STL

Plusieurs algorithmes de la STL utilisent le déplacement : `std::move` (l'algorithme, pas le cast), `std::move_backward`, et les algorithmes de partition/tri qui réarrangent les éléments :

```cpp
std::vector<std::string> source = {"alpha", "beta", "gamma"};  
std::vector<std::string> destination(3);  

// L'algorithme std::move déplace chaque élément
std::move(source.begin(), source.end(), destination.begin());
// destination = {"alpha", "beta", "gamma"}
// source = {"", "", ""}  (chaînes vidées)
```

> ⚠️ Ne confondez pas `std::move` le cast (`<utility>`) et `std::move` l'algorithme (`<algorithm>`). Le cast prend un objet unique, l'algorithme prend une range d'itérateurs.

### Insertion avec déplacement

Les conteneurs offrent des méthodes optimisées pour l'insertion par déplacement :

```cpp
std::vector<std::string> vec;

std::string s = "une chaîne longue pour éviter le SSO";

vec.push_back(s);              // Copie — s est une lvalue  
vec.push_back(std::move(s));   // Déplacement — s est vidé  

// emplace_back construit directement dans le vector
vec.emplace_back("construction in-place");  // Ni copie ni déplacement
```

`emplace_back` est souvent encore mieux que `push_back` + `std::move`, car il construit l'objet directement à sa destination finale, éliminant même le déplacement. Cependant, `push_back(std::move(x))` reste utile quand l'objet existe déjà et doit être transféré.

---

## L'état après déplacement : le contrat

Le standard C++ garantit qu'un objet après déplacement est dans un **état valide mais non spécifié**. Cela signifie :

- L'objet **peut être détruit** — son destructeur ne crashera pas.
- L'objet **peut être réaffecté** — l'opérateur `=` fonctionne.
- L'état interne est **indéterminé** — vous ne savez pas ce que contient l'objet.

### Ce que les types de la STL garantissent

Les types de la bibliothèque standard offrent une garantie plus forte que le minimum : après déplacement, l'objet est dans un état « par défaut » ou « vide ». Ce n'est pas imposé par le standard pour les types utilisateur, mais c'est une convention largement suivie :

| Type | État après move |
|---|---|
| `std::string` | Chaîne vide (`""`) — garanti en pratique |
| `std::vector<T>` | Vecteur vide (`size() == 0`) — garanti en pratique |
| `std::unique_ptr<T>` | `nullptr` — **garanti par le standard** |
| `std::shared_ptr<T>` | `nullptr` — **garanti par le standard** |
| `std::optional<T>` | `nullopt` — **garanti par le standard** |
| Types scalaires (`int`, `double`) | **Inchangés** — le move est une copie |

### Bonnes pratiques pour vos types

Quand vous implémentez un constructeur de déplacement pour vos propres classes, laissez l'objet source dans un état **destructible et réaffectable**. L'idéal est un état « vide » cohérent avec la sémantique de votre classe :

```cpp
class Connection {
    int socket_fd_;
    std::string host_;

public:
    Connection(Connection&& other) noexcept
        : socket_fd_(other.socket_fd_)
        , host_(std::move(other.host_))
    {
        other.socket_fd_ = -1;  // ✅ État "pas de connexion"
        // host_ de other est déjà vidé par std::move
    }

    ~Connection() {
        if (socket_fd_ >= 0) {
            ::close(socket_fd_);
        }
        // Si socket_fd_ == -1, le destructeur ne fait rien — sûr
    }
};
```

---

## Erreurs fréquentes et diagnostics

### Erreur n°1 : utiliser après move

```cpp
std::vector<int> data = {1, 2, 3};  
process(std::move(data));  

// ❌ Bug silencieux
std::print("Taille : {}\n", data.size());
```

**Diagnostic** : activez `-Wuse-after-move` (Clang) ou utilisez clang-tidy avec le check `bugprone-use-after-move`.

```bash
$ clang-tidy --checks='bugprone-use-after-move' main.cpp
```

### Erreur n°2 : move sur const (copie silencieuse)

```cpp
void enregistrer(const std::string& nom) {
    noms_.push_back(std::move(nom));  // ⚠️ COPIE — nom est const
}
```

**Diagnostic** : `-Wpessimizing-move` (GCC/Clang) détecte certains cas. Pour une couverture complète, utilisez `clang-tidy` avec `performance-move-const-arg`.

### Erreur n°3 : move dans un return (NRVO bloqué)

```cpp
std::string build() {
    std::string s = "result";
    return std::move(s);  // ⚠️ Bloque le NRVO
}
```

**Diagnostic** : `-Wpessimizing-move` (GCC) signale spécifiquement ce cas.

### Erreur n°4 : move inutile sur une rvalue

```cpp
std::string s = std::move(std::string("Hello"));  // ⚠️ Inutile
// std::string("Hello") est déjà une rvalue
```

**Diagnostic** : `-Wredundant-move` (GCC/Clang).

### Flags de compilation recommandés

Pour détecter les problèmes liés à `std::move` lors de la compilation :

```bash
g++ -std=c++20 -Wall -Wextra -Wpessimizing-move -Wredundant-move ...  
clang++ -std=c++20 -Wall -Wextra -Wpessimizing-move -Wredundant-move ...  
```

---

## Résumé

| Concept | Détail |
|---|---|
| **Ce que fait std::move** | Un `static_cast<T&&>` — conversion en rvalue reference |
| **Ce qu'il ne fait pas** | Il ne déplace rien, ne modifie rien, ne génère aucune instruction |
| **Quand l'utiliser** | Lvalue dont on n'a plus besoin, à transférer vers un récepteur |
| **Quand l'éviter** | Objets `const`, return de variables locales, types primitifs, objets encore nécessaires |
| **État après move** | Valide mais non spécifié — destructible et réaffectable |
| **Move sur const** | Silencieusement ignoré → copie. Piège majeur. |
| **Move dans return** | Presque toujours contre-productif (bloque NRVO) |
| **Diagnostics** | `-Wpessimizing-move`, `-Wredundant-move`, `bugprone-use-after-move` |

> **Règle pratique** — `std::move` est un outil de précision, pas un accélérateur universel. Utilisez-le uniquement quand vous transférez intentionnellement la propriété d'une lvalue, et ne touchez plus l'objet source après. Dans le doute, ne mettez pas de `std::move` — le compilateur est souvent plus intelligent que vous pour décider quand déplacer.

⏭️ [Move constructors et move assignment operators](/10-move-semantics/03-move-constructors.md)
