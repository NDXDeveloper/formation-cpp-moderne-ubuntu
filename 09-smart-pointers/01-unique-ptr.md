🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 9.1 std::unique_ptr : Possession exclusive

## Introduction

`std::unique_ptr` est le smart pointer que vous utiliserez le plus souvent en C++ moderne. Il incarne le modèle de possession le plus simple et le plus efficace : **une ressource n'a qu'un seul propriétaire à la fois**. Quand ce propriétaire est détruit, la ressource est automatiquement libérée.

Ce modèle correspond à l'écrasante majorité des situations réelles. Quand vous créez un objet sur le tas, c'est généralement parce qu'une seule entité dans votre programme est responsable de sa durée de vie. Les cas où plusieurs parties du code doivent réellement co-posséder une même ressource sont plus rares qu'on ne le pense — et pour ceux-là, `std::shared_ptr` existe ([section 9.2](/09-smart-pointers/02-shared-weak-ptr.md)).

```cpp
#include <memory>

{
    std::unique_ptr<int> p = std::make_unique<int>(42);
    // p possède exclusivement l'entier alloué sur le tas

    std::cout << *p << "\n";  // 42 — accès identique à un pointeur brut

}   // ✅ Destruction automatique : l'entier est libéré ici
```

---

## Pourquoi « unique » ?

Le nom `unique_ptr` exprime un **invariant fort** : à tout instant, **exactement un** `unique_ptr` pointe vers une ressource donnée. Cette unicité est garantie par le compilateur lui-même, via deux propriétés fondamentales :

**1. La copie est interdite.**

```cpp
std::unique_ptr<int> a = std::make_unique<int>(42);
std::unique_ptr<int> b = a;  // ❌ Erreur de compilation
```

Si la copie était autorisée, deux `unique_ptr` pointeraient vers la même ressource, et tous deux tenteraient de la libérer lors de leur destruction — un *double free*, exactement le bug que les smart pointers sont censés prévenir. Le compilateur refuse donc purement et simplement cette opération.

**2. Le déplacement (*move*) est autorisé.**

```cpp
std::unique_ptr<int> a = std::make_unique<int>(42);
std::unique_ptr<int> b = std::move(a);  // ✅ Transfert de propriété

// a est maintenant vide (nullptr)
// b possède la ressource
```

Le déplacement **transfère** la propriété d'un `unique_ptr` à un autre. L'ancien propriétaire devient nul. À tout instant, l'invariant est respecté : un seul `unique_ptr` possède la ressource. Ce mécanisme repose sur la sémantique de mouvement du C++11 ([chapitre 10](/10-move-semantics/README.md)).

Cette distinction copie/déplacement est ce qui rend `std::unique_ptr` fondamentalement supérieur à l'ancien `std::auto_ptr` du C++98, qui tentait de transférer la propriété lors de la copie — un comportement silencieux et source de bugs.

---

## Zéro coût à l'exécution

Un argument souvent avancé contre les abstractions est leur coût en performance. Pour `std::unique_ptr`, cet argument ne tient pas : il est conçu pour avoir **un surcoût nul** (*zero overhead*) par rapport à un pointeur brut.

Concrètement :

- `sizeof(std::unique_ptr<T>)` est égal à `sizeof(T*)` — un seul mot machine, typiquement 8 octets sur x86_64 — à condition d'utiliser le deleter par défaut.
- Le compilateur inline les appels au destructeur et aux opérateurs d'accès (`*`, `->`) de manière systématique. Le code machine généré est identique à celui d'un pointeur brut avec un `delete` manuel.
- Il n'y a pas de compteur de références, pas d'allocation supplémentaire, pas de synchronisation entre threads.

Vous pouvez vérifier cela simplement :

```cpp
#include <memory>
#include <iostream>

int main() {
    std::cout << "sizeof(int*)              = " << sizeof(int*) << "\n";
    std::cout << "sizeof(unique_ptr<int>)   = " << sizeof(std::unique_ptr<int>) << "\n";
    // Affiche la même valeur : 8 (sur x86_64)
}
```

Ce point est important : choisir `std::unique_ptr` plutôt qu'un pointeur brut est un gain de sécurité **sans compromis de performance**. Il n'y a aucune raison technique de préférer un pointeur brut avec `new`/`delete` dans du code applicatif.

> ⚠️ **Exception** : l'utilisation d'un *custom deleter* sous forme de lambda ou de foncteur avec état augmente la taille du `unique_ptr`, car le deleter est stocké dans l'objet lui-même. Ce cas est abordé en [section 9.1.3](/09-smart-pointers/01.3-custom-deleters.md).

---

## Analogie : la clé d'une chambre d'hôtel

Pour bien saisir la sémantique de `std::unique_ptr`, imaginez une clé de chambre d'hôtel — pas une carte magnétique duplicable, mais une clé physique unique :

- **Une seule clé existe** pour chaque chambre. Vous ne pouvez pas la photocopier (pas de copie).
- Vous pouvez **donner la clé à quelqu'un d'autre** (déplacement avec `std::move`), mais alors vous ne l'avez plus.
- Quand le dernier détenteur de la clé quitte l'hôtel (sort du scope), **la chambre est libérée** (le destructeur appelle `delete`).
- Si vous essayez d'ouvrir la porte sans clé (déréférencer un `unique_ptr` nul), vous avez un problème (comportement indéfini).

Cette analogie illustre aussi une subtilité : d'autres personnes peuvent **connaître le numéro de la chambre** (avoir un pointeur brut ou une référence vers la ressource) sans posséder la clé. Elles peuvent accéder à la chambre tant qu'elle existe, mais elles ne sont pas responsables de la libérer — et elles doivent s'assurer de ne plus y accéder une fois la chambre libérée.

---

## Quand utiliser std::unique_ptr

`std::unique_ptr` est le **choix par défaut** quand vous avez besoin d'une allocation dynamique. Voici les situations les plus courantes :

**Objets dont la taille ou le type n'est connu qu'à l'exécution.** C'est le cas classique du polymorphisme : vous stockez un pointeur vers une classe de base, et l'objet concret est déterminé au runtime.

```cpp
class Animal {
public:
    virtual ~Animal() = default;
    virtual void parler() const = 0;
};

class Chat : public Animal {
public:
    void parler() const override { std::print("Miaou\n"); }
};

class Chien : public Animal {
public:
    void parler() const override { std::print("Wouf\n"); }
};

// Le type concret dépend de l'entrée utilisateur
std::unique_ptr<Animal> creer_animal(const std::string& choix) {
    if (choix == "chat") return std::make_unique<Chat>();
    if (choix == "chien") return std::make_unique<Chien>();
    return nullptr;
}
```

**Objets volumineux que l'on ne veut pas copier.** Plutôt que de copier une structure de données lourde, on l'alloue sur le tas et on transfère la propriété.

```cpp
struct GrosBuffer {
    std::array<char, 1024 * 1024> data;  // 1 Mo
};

// Retourner un unique_ptr est gratuit (move semantics)
std::unique_ptr<GrosBuffer> allouer_buffer() {
    return std::make_unique<GrosBuffer>();
}
```

**Membres de classe avec durée de vie liée à l'objet parent.** Quand un objet contient une ressource qu'il possède exclusivement — un pattern extrêmement fréquent, souvent appelé *composition*.

```cpp
class Moteur { /* ... */ };

class Voiture {
    std::unique_ptr<Moteur> moteur_;  // La voiture possède son moteur
public:
    Voiture() : moteur_(std::make_unique<Moteur>()) {}
    // Pas besoin de destructeur explicite :
    // unique_ptr libère le moteur automatiquement
};
```

**Implémentation du pattern Pimpl** (*Pointer to Implementation*). Ce pattern permet de masquer les détails d'implémentation d'une classe dans le fichier `.cpp`, réduisant les dépendances de compilation. `std::unique_ptr` est le smart pointer naturel pour ce cas d'usage.

---

## Quand ne PAS utiliser std::unique_ptr

Aussi utile soit-il, `std::unique_ptr` n'est pas toujours la bonne réponse :

- **Si l'objet peut vivre sur la stack**, n'utilisez pas de smart pointer du tout. Une variable locale est plus rapide, plus simple, et sa durée de vie est parfaitement déterministe. Les smart pointers remplacent `new`/`delete`, pas les variables locales.
- **Si la possession doit être partagée** entre plusieurs parties du code qui n'ont pas de relation hiérarchique claire, `std::shared_ptr` est le bon choix ([section 9.2](/09-smart-pointers/02-shared-weak-ptr.md)).
- **Si vous avez besoin d'un pointeur non-possédant** (un observateur), utilisez un pointeur brut `T*` ou une référence `T&`. Un `unique_ptr` passé en paramètre signifie « je prends la propriété » — si ce n'est pas votre intention, ne passez pas un smart pointer.

---

## Vue d'ensemble des sous-sections

Ce module sur `std::unique_ptr` est découpé en trois sous-sections qui couvrent progressivement tous les aspects de son utilisation :

**[9.1.1 — Création et utilisation](/09-smart-pointers/01.1-creation-utilisation.md)**
Les bases : créer un `unique_ptr`, accéder à la ressource, vérifier la nullité, et les opérations courantes (`get()`, `reset()`, `release()`).

**[9.1.2 — Transfert de propriété avec std::move](/09-smart-pointers/01.2-transfert-propriete.md)**
Comment transférer la possession d'un `unique_ptr` à un autre — en passant en paramètre à une fonction, en retournant depuis une fonction, ou en stockant dans un conteneur.

**[9.1.3 — Custom deleters](/09-smart-pointers/01.3-custom-deleters.md)**
Pour les cas où la ressource n'est pas libérée par `delete` — fichiers (`fclose`), handles systèmes, ressources C — vous pouvez fournir une logique de destruction personnalisée.

---

## Résumé express

| Propriété | `std::unique_ptr` |
|---|---|
| **Possession** | Exclusive — un seul propriétaire |
| **Copie** | Interdite (erreur de compilation) |
| **Déplacement** | Autorisé (`std::move`) |
| **Surcoût mémoire** | Aucun (même taille qu'un pointeur brut) |
| **Surcoût CPU** | Aucun (calls inlinés) |
| **Thread-safety** | Non thread-safe par défaut (comme un pointeur brut) |
| **Header** | `<memory>` |
| **Disponible depuis** | C++11 |

> **Règle pratique** — Quand vous hésitez entre `unique_ptr` et `shared_ptr`, commencez toujours par `unique_ptr`. Vous pourrez toujours convertir un `unique_ptr` en `shared_ptr` plus tard si le besoin de possession partagée se confirme (la conversion inverse est impossible).

⏭️ [Création et utilisation](/09-smart-pointers/01.1-creation-utilisation.md)
