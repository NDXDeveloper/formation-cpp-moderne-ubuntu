🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 10.5 Return Value Optimization (RVO) et Copy Elision

## Introduction

Tout au long de ce chapitre, nous avons appris à écrire des constructeurs de déplacement performants et à utiliser `std::move` pour éviter les copies inutiles. Mais il existe quelque chose d'encore mieux qu'un déplacement : **ne rien faire du tout**.

La *copy elision* est une optimisation du compilateur qui **élimine purement et simplement** la construction d'objets intermédiaires — ni copie, ni déplacement. L'objet est construit directement à sa destination finale, comme si les étapes intermédiaires n'existaient pas.

C'est l'optimisation la plus puissante liée aux valeurs de retour en C++. Elle explique pourquoi retourner des objets **par valeur** — un pattern qui semblait coûteux avant C++11 — est non seulement correct mais souvent **optimal** en C++ moderne.

---

## Le concept : construire directement à destination

Sans copy elision, retourner un objet par valeur implique théoriquement plusieurs étapes :

```cpp
std::string creer_message() {
    std::string msg = "Bonjour le monde";  // 1. Construction de msg
    return msg;                             // 2. Copie/move de msg vers la valeur de retour
}                                           // 3. Destruction de msg

void exemple() {
    std::string resultat = creer_message(); // 4. Copie/move de la valeur de retour vers resultat
}                                           // 5. Destruction de la valeur de retour
```

Théoriquement : une construction, deux copies (ou déplacements), et deux destructions pour un seul objet utile. En pratique, avec la copy elision, le compilateur produit ceci :

```
1. Construction de resultat directement avec "Bonjour le monde"
   (aucune copie, aucun déplacement, aucun objet intermédiaire)
```

Le compilateur fusionne `msg`, la valeur de retour, et `resultat` en un **seul et même objet** en mémoire. Les constructeurs de copie et de déplacement ne sont jamais appelés — même pas comme étape conceptuelle.

---

## Les trois formes de copy elision

### 1. RVO — Return Value Optimization (prvalue)

Le RVO s'applique quand une fonction retourne un **objet temporaire** (une prvalue) directement dans le `return` :

```cpp
std::vector<int> generer() {
    return std::vector<int>{1, 2, 3, 4, 5};  // Temporaire retourné directement
}

auto v = generer();
// Le vector est construit UNE SEULE FOIS, directement dans v
// Aucun constructeur de copie ou de déplacement n'est invoqué
```

Le compilateur sait à l'avance que le temporaire dans le `return` est destiné à initialiser `v`. Il construit donc le vector directement dans l'espace mémoire de `v`, sans passer par un objet intermédiaire.

**Depuis C++17, le RVO sur les prvalues est obligatoire.** Ce n'est plus une optimisation optionnelle — le standard garantit que le constructeur de copie/déplacement n'est même pas nécessaire. Cela signifie que ce code compile même si le type est non-copiable et non-déplaçable :

```cpp
class NonCopiable {  
public:  
    NonCopiable() = default;
    NonCopiable(const NonCopiable&) = delete;
    NonCopiable(NonCopiable&&) = delete;
};

NonCopiable creer() {
    return NonCopiable{};  // ✅ Compile en C++17 — copy elision obligatoire
}

auto obj = creer();  // ✅ Aucun constructeur de copie/move n'est nécessaire
```

En C++14 et avant, ce code ne compilait pas : le compilateur devait vérifier que le constructeur de copie ou de déplacement était accessible, même s'il ne l'appelait pas.

### 2. NRVO — Named Return Value Optimization

Le NRVO est la variante où l'objet retourné est une **variable locale nommée** (une lvalue) :

```cpp
std::string construire_chemin(const std::string& base, const std::string& fichier) {
    std::string resultat = base;     // Variable locale nommée
    resultat += '/';
    resultat += fichier;
    return resultat;                  // NRVO — resultat est construit directement
}                                     //        dans l'espace de l'appelant

auto chemin = construire_chemin("/home", "data.txt");
// Avec NRVO : aucune copie, aucun déplacement
```

Le compilateur alloue `resultat` directement dans l'espace mémoire que l'appelant a réservé pour `chemin`. Quand la fonction retourne, il n'y a rien à transférer — l'objet est déjà à sa destination.

**Le NRVO n'est pas obligatoire.** Contrairement au RVO sur les prvalues (garanti en C++17), le NRVO reste une optimisation optionnelle que le compilateur est autorisé mais pas obligé d'appliquer. En pratique, GCC et Clang l'appliquent dans la grande majorité des cas, même en `-O0`.

### 3. Copy elision dans d'autres contextes

La copy elision peut aussi s'appliquer dans d'autres situations, moins courantes :

```cpp
// Initialisation directe depuis un temporaire
std::string s = std::string("Hello");  // Copy elision — un seul objet construit

// Passage de temporaire à une fonction par valeur
void traiter(std::string s);  
traiter(std::string("Hello"));         // Copy elision possible (C++17 garanti)  

// Lancer et attraper une exception (copy elision optionnelle)
throw std::runtime_error("erreur");    // Copy elision possible
```

---

## Quand le NRVO ne peut PAS s'appliquer

Le compilateur ne peut pas toujours fusionner la variable locale avec la destination. Voici les situations qui empêchent le NRVO.

### Plusieurs chemins de retour avec des variables différentes

```cpp
std::string choisir(bool condition) {
    std::string a = "Alpha";
    std::string b = "Beta";

    if (condition) {
        return a;  // Retourne a
    }
    return b;      // Retourne b
}
// ⚠️ NRVO impossible — le compilateur ne sait pas à l'avance
//    si c'est a ou b qui sera retourné. Il ne peut pas construire
//    les deux dans l'espace de l'appelant.
// → Déplacement implicite (pas de copie — voir section suivante)
```

Le compilateur doit choisir à la compilation quel objet allouer dans l'espace de l'appelant. Si deux variables candidates existent, il ne peut en choisir qu'une — et risque de se tromper. La plupart des compilateurs abandonnent le NRVO dans ce cas.

En revanche, si tous les chemins retournent **la même variable**, le NRVO s'applique :

```cpp
std::string construire(bool detaille) {
    std::string resultat = "Base";

    if (detaille) {
        resultat += " — détails complets";
    }
    return resultat;  // ✅ NRVO possible — un seul candidat (resultat)
}
```

### Retour d'un paramètre de la fonction

```cpp
std::string transformer(std::string input) {
    input += " transformé";
    return input;  // ⚠️ NRVO impossible pour les paramètres
                    //    input est alloué par l'appelant dans un espace différent
}
// → Déplacement implicite
```

Les paramètres de fonction sont alloués dans un espace mémoire dicté par la convention d'appel (ABI). Le compilateur ne peut pas les placer dans l'espace de retour.

### Retour d'un membre de l'objet

```cpp
class Builder {
    std::string resultat_;
public:
    std::string build() {
        return resultat_;  // ⚠️ NRVO impossible — c'est un membre, pas une locale
    }                       // → Copie (resultat_ doit rester intact dans l'objet)
};
```

Un membre de l'objet ne peut pas être fusionné avec la valeur de retour car l'objet continue d'exister après le retour — son membre doit rester valide. Ici, c'est une copie (pas un déplacement, sauf si vous écrivez `return std::move(resultat_)` — voir plus bas).

### Retour conditionnel d'un temporaire et d'une variable

```cpp
std::string obtenir(bool cache_valide) {
    std::string cache = lire_cache();

    if (cache_valide) {
        return cache;                     // Variable locale
    }
    return std::string("valeur par défaut");  // Temporaire
}
// Le mélange variable locale / temporaire complique le NRVO
// Comportement dépendant du compilateur
```

---

## Le déplacement implicite : le filet de sécurité

Quand le NRVO ne s'applique pas, le compilateur ne revient pas à la copie. Depuis C++11, le standard impose un **déplacement implicite** des variables locales et des paramètres dans les instructions `return` :

```cpp
std::string choisir(bool condition) {
    std::string a = "Alpha";
    std::string b = "Beta";

    if (condition) {
        return a;  // NRVO impossible, mais déplacement implicite
    }              // Équivalent à : return std::move(a);
    return b;      // Équivalent à : return std::move(b);
}
```

Le compilateur traite automatiquement la variable locale dans un `return` comme une rvalue, invoquant le constructeur de déplacement au lieu du constructeur de copie. Vous n'avez **pas besoin** d'écrire `std::move` — le compilateur le fait pour vous.

Les règles de déplacement implicite ont été progressivement élargies :

| Standard | Ce qui est implicitement déplacé dans un return |
|---|---|
| C++11 | Variables locales |
| C++14 | Variables locales |
| C++20 | Variables locales + paramètres de fonction |
| C++23 | Encore élargi — presque toutes les entités locales éligibles |

---

## Pourquoi return std::move(x) est presque toujours une erreur

C'est le point le plus contre-intuitif de toute la sémantique de mouvement, et le piège dans lequel tombent même les développeurs expérimentés.

### Le problème : std::move bloque le NRVO

```cpp
std::vector<int> generer() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    return std::move(v);  // ⚠️ PIRE que return v
}
```

Quand vous écrivez `return std::move(v)`, l'expression retournée n'est plus la variable locale `v` — c'est le résultat de `std::move(v)`, un xvalue. Le NRVO ne s'applique qu'aux variables locales nommées, pas aux expressions arbitraires. Le compilateur est donc **forcé** d'appeler le constructeur de déplacement.

Comparaison des scénarios :

```
return v;              → NRVO : AUCUNE opération (0 copie, 0 move)  
return std::move(v);   → Pas de NRVO : 1 déplacement (constructeur de move)  
```

Un déplacement de `std::vector` est certes rapide (copie de 3 pointeurs), mais il est strictement plus coûteux que zéro opération. `return std::move(v)` est donc une **pessimisation**.

### Quand return std::move est-il justifié ?

Il existe de rares cas où `return std::move` est correct, voire nécessaire :

**1. Retour d'un membre de l'objet que vous voulez abandonner :**

```cpp
class Builder {
    std::string resultat_;
public:
    // La méthode consomme le builder — move sémantiquement correct
    std::string build() && {  // Qualificateur de ref && → appelé sur rvalue
        return std::move(resultat_);  // ✅ Déplacement du membre
    }
};

auto texte = Builder().build();  // Le Builder est un temporaire
```

Sans `std::move`, le membre serait copié (le compilateur ne peut pas déplacer implicitement un membre car l'objet pourrait encore être utilisé). Le `std::move` explicite est ici intentionnel.

**2. Retour d'un type différent nécessitant une conversion :**

```cpp
std::unique_ptr<Base> creer() {
    auto derived = std::make_unique<Derived>();
    return std::move(derived);  // ✅ Nécessaire pour la conversion Derived → Base
    // Note : certains compilateurs modernes gèrent aussi ce cas sans std::move
}
```

**3. Retour d'un paramètre (pré-C++20) :**

Avant C++20, les paramètres de fonction n'étaient pas implicitement déplacés dans le return. Le `std::move` explicite était alors justifié. Depuis C++20, ce n'est plus nécessaire dans la plupart des cas.

### L'arbre de décision

```
Retourner une variable locale ?
├── Oui → return x;                  (NRVO ou move implicite)
│         Ne PAS écrire std::move
│
├── Retourner un membre ?
│   ├── L'objet est encore nécessaire → return membre_;        (copie)
│   └── L'objet est abandonné        → return std::move(membre_);  (move)
│
├── Retourner un paramètre ?
│   ├── C++20+ → return param;       (move implicite)
│   └── C++17  → return std::move(param);  (move explicite)
│
└── Conversion de type nécessaire ?
    └── return std::move(x);         (move explicite pour conversion)
```

---

## Vérifier la copy elision en pratique

### Méthode 1 : instrumentation des constructeurs

Ajoutez des traces dans les constructeurs pour observer quelles opérations sont réellement invoquées :

```cpp
struct Trace {
    Trace()                    { std::print("[Trace] Construction\n"); }
    Trace(const Trace&)        { std::print("[Trace] COPIE\n"); }
    Trace(Trace&&) noexcept    { std::print("[Trace] MOVE\n"); }
    ~Trace()                   { std::print("[Trace] Destruction\n"); }
};

Trace creer() {
    Trace t;
    return t;
}

int main() {
    Trace resultat = creer();
}
```

Avec NRVO (comportement typique même en `-O0`) :

```
[Trace] Construction
[Trace] Destruction
```

Sans NRVO (forcer avec `-fno-elide-constructors` sur GCC pré-C++17) :

```
[Trace] Construction
[Trace] MOVE
[Trace] Destruction
[Trace] MOVE
[Trace] Destruction
[Trace] Destruction
```

### Méthode 2 : flags du compilateur

```bash
# GCC : désactiver la copy elision (impossible en C++17 pour les prvalues)
g++ -std=c++14 -fno-elide-constructors main.cpp

# Comparer le code assembleur
g++ -std=c++20 -O2 -S main.cpp -o avec_elision.s  
g++ -std=c++14 -O2 -fno-elide-constructors -S main.cpp -o sans_elision.s  
diff avec_elision.s sans_elision.s  
```

> 💡 En C++17 et au-delà, `-fno-elide-constructors` ne peut pas désactiver la copy elision **obligatoire** (prvalues). Il ne peut désactiver que le NRVO, qui reste optionnel.

### Méthode 3 : Compiler Explorer

[Compiler Explorer](https://godbolt.org) permet de visualiser le code assembleur généré. Comparez les versions avec et sans `std::move` dans le `return` pour constater que la version sans `std::move` produit un code plus court (NRVO appliqué).

---

## Implications pratiques : retourner par valeur sans crainte

La combinaison de la copy elision obligatoire (C++17), du NRVO (quasi systématique), et du déplacement implicite (filet de sécurité) a une conséquence majeure sur le style de code :

**Retournez par valeur.** Ne retournez pas par pointeur ou par référence pour « éviter les copies ». Les copies n'ont pas lieu.

```cpp
// ✅ C++ moderne — retour par valeur
std::vector<int> calculer() {
    std::vector<int> resultats;
    resultats.reserve(10000);
    for (int i = 0; i < 10000; ++i) {
        resultats.push_back(compute(i));
    }
    return resultats;  // NRVO → aucune copie
}

// ❌ Ancien style — retour par pointeur pour "éviter la copie"
std::unique_ptr<std::vector<int>> calculer_old() {
    auto resultats = std::make_unique<std::vector<int>>();
    // ... remplissage ...
    return resultats;  // Allocation supplémentaire inutile
}

// ❌ Ancien style — paramètre de sortie pour "éviter la copie"
void calculer_ancient(std::vector<int>& out) {
    out.clear();
    // ... remplissage ...
    // Moins lisible, pas plus performant
}
```

Le retour par valeur est plus lisible, plus simple, et grâce à la copy elision, tout aussi performant — voire plus, car il donne au compilateur la liberté maximale d'optimiser.

---

## Résumé du chapitre 10

Ce chapitre a couvert l'ensemble de la sémantique de mouvement, de la théorie à la pratique :

| Section | Concept clé |
|---|---|
| **10.1** | Lvalue (identité) vs rvalue (temporaire). Les références rvalue `T&&`. |
| **10.2** | `std::move` est un cast, pas un déplacement. Permission de voler les ressources. |
| **10.3** | Implémenter move constructor et move assignment. `noexcept` obligatoire. Règle du 0 / Règle des 5. |
| **10.4** | Perfect forwarding : forwarding references + `std::forward<T>` pour préserver la catégorie de valeur. |
| **10.5** | RVO/NRVO éliminent les copies et les moves. `return std::move(x)` est une pessimisation. |

La hiérarchie d'efficacité pour le transfert de données :

```
Copy elision (RVO/NRVO)     →  0 opération   →  Le mieux  
Déplacement (move)          →  O(1)           →  Très bien  
Copie profonde (copy)       →  O(n)           →  Dernier recours  
```

Et la règle finale qui synthétise les cinq sections :

> **Retournez par valeur, passez les sink arguments par valeur, laissez le compilateur optimiser. Intervenez avec `std::move` uniquement quand vous transférez intentionnellement une lvalue vers un nouveau propriétaire — et ne touchez plus l'objet source après.**

⏭️ [Programmation Fonctionnelle et Lambdas](/11-lambdas/README.md)
