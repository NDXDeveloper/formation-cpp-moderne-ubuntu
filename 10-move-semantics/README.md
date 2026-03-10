🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 10. Sémantique de Mouvement (Move Semantics) ⭐

## Objectifs du chapitre

La sémantique de mouvement est l'une des innovations les plus importantes introduites par C++11. Elle a transformé la manière dont le langage gère le transfert de données, en ajoutant une distinction fondamentale : quand un objet n'est **plus nécessaire** après une opération, le C++ peut désormais **voler ses ressources** plutôt que les copier, puis détruire l'original devenu inutile.

Vous avez déjà croisé cette mécanique au chapitre précédent : `std::unique_ptr` interdit la copie mais autorise le déplacement via `std::move`, et c'est précisément la sémantique de mouvement qui rend cela possible. Ce chapitre vous donne les fondations théoriques et pratiques complètes du mécanisme.

À la fin de ce chapitre, vous serez capable de :

- Distinguer les **lvalues** et les **rvalues**, et comprendre pourquoi cette distinction est au cœur du mouvement.
- Comprendre ce que fait réellement `std::move` (et ce qu'il ne fait pas).
- Implémenter des **constructeurs de déplacement** et des **opérateurs d'affectation par déplacement** pour vos propres classes.
- Utiliser `std::forward` pour le **perfect forwarding** dans les fonctions templates.
- Comprendre les optimisations du compilateur (**RVO**, **NRVO**, **copy elision**) et leur interaction avec la sémantique de mouvement.

---

## Prérequis

Ce chapitre s'appuie sur plusieurs concepts que vous devez maîtriser :

- **Constructeurs et destructeurs** ([section 6.2](/06-classes-encapsulation/02-constructeurs.md)) — Vous devez comprendre les constructeurs de copie, les listes d'initialisation, et le cycle de vie des objets.
- **La Règle des 5** ([section 6.5](/06-classes-encapsulation/05-rule-of-five.md)) — Le mouvement est deux des cinq opérations spéciales. Vous devez savoir pourquoi elles forment un ensemble cohérent.
- **Pointeurs et références** ([sections 4.3](/04-structures-controle-fonctions/03-passage-parametres.md) et [5.3](/05-gestion-memoire/03-arithmetique-pointeurs.md)) — La sémantique de mouvement repose sur un nouveau type de référence (`&&`).
- **Smart pointers** ([chapitre 9](/09-smart-pointers/README.md)) — Vous avez déjà utilisé `std::move` avec `unique_ptr`. Ce chapitre explique le mécanisme sous-jacent.

---

## Le problème : le coût caché des copies

Pour comprendre pourquoi la sémantique de mouvement existe, il faut d'abord mesurer ce qu'elle remplace : la **copie profonde**.

Considérons une classe qui gère un buffer dynamique :

```cpp
class Buffer {
    size_t taille_;
    char* data_;

public:
    Buffer(size_t taille) : taille_(taille), data_(new char[taille]) {
        std::print("[Buffer] Allocation de {} octets\n", taille_);
    }

    // Constructeur de copie — copie profonde
    Buffer(const Buffer& other) : taille_(other.taille_), data_(new char[other.taille_]) {
        std::memcpy(data_, other.data_, taille_);
        std::print("[Buffer] Copie de {} octets\n", taille_);
    }

    ~Buffer() {
        delete[] data_;
        std::print("[Buffer] Libération de {} octets\n", taille_);
    }
};
```

Chaque copie alloue un nouveau bloc mémoire et duplique l'intégralité des données. Pour un buffer de 100 Mo, c'est 100 Mo alloués et 100 Mo copiés — à chaque fois.

Maintenant, observons ce qui se passe quand une fonction retourne un `Buffer` (dans un scénario sans optimisation du compilateur) :

```cpp
Buffer creer_buffer() {
    Buffer local(1024 * 1024);  // 1 Mo — allocation
    // ... remplissage ...
    return local;               // Copie vers la valeur de retour — 1 Mo copié
}                               // local détruit — 1 Mo libéré

void exemple() {
    Buffer b = creer_buffer();  // Copie vers b — 1 Mo copié (encore)
}                               // b détruit — 1 Mo libéré
```

Sans optimisation, ce code effectue **trois allocations** et **deux copies** d'un mégaoctet pour un seul buffer utile. L'objet `local` est créé, copié, puis immédiatement détruit. La copie intermédiaire est créée, copiée dans `b`, puis immédiatement détruite. Tout ce travail est gaspillé.

Le problème est que le compilateur (avant C++11) n'avait aucun moyen standardisé de dire : « cet objet va être détruit juste après, autant prendre directement ses ressources plutôt que les copier ».

---

## La solution : voler au lieu de copier

La sémantique de mouvement ajoute exactement cette capacité. Quand un objet est **temporaire** ou marqué comme **déplaçable**, au lieu de copier ses données, le nouvel objet peut **s'emparer de ses ressources internes** et laisser l'ancien dans un état vide mais valide :

```cpp
class Buffer {
    size_t taille_;
    char* data_;

public:
    // ... constructeur et constructeur de copie inchangés ...

    // Constructeur de déplacement — vole les ressources
    Buffer(Buffer&& other) noexcept
        : taille_(other.taille_), data_(other.data_)
    {
        other.taille_ = 0;
        other.data_ = nullptr;  // L'ancien objet est vidé
        std::print("[Buffer] Déplacement de {} octets (coût : ~0)\n", taille_);
    }

    ~Buffer() {
        delete[] data_;  // delete nullptr est un no-op — sûr
    }
};
```

Le constructeur de déplacement ne copie aucune donnée. Il copie deux valeurs scalaires (un `size_t` et un pointeur — 16 octets sur x86_64), puis met l'objet source à zéro. Pour un buffer de 100 Mo, le déplacement coûte exactement autant que pour un buffer de 16 octets : **quasiment rien**.

```
Copie profonde (ancien monde) :

  source                     destination
  ┌──────────┐              ┌──────────┐
  │ data_ ───┼──> [ABCDEF]  │ data_ ───┼──> [ABCDEF]  ← nouveau bloc, copie octet par octet
  │ taille_  │              │ taille_  │
  └──────────┘              └──────────┘
  (source inchangée)


Déplacement (C++11) :

  source                     destination
  ┌──────────┐              ┌──────────┐
  │ data_ ───┼──> nullptr   │ data_ ───┼──> [ABCDEF]  ← même bloc, juste le pointeur transféré
  │ taille_  │  0           │ taille_  │
  └──────────┘              └──────────┘
  (source vidée)
```

---

## L'idée clé : les catégories de valeurs

Pour que le compilateur sache **quand** il peut déplacer plutôt que copier, il a besoin d'un critère. Ce critère est la **catégorie de valeur** de l'expression :

- Une **lvalue** (« left value ») est une expression qui désigne un objet avec un emplacement mémoire persistant et un nom. Exemple : une variable, un élément d'un tableau, le résultat d'un déréférencement. L'objet pourrait être utilisé après l'opération — le copier est la seule option sûre.

- Une **rvalue** (« right value ») est une expression qui désigne un objet **temporaire** ou un objet dont le propriétaire a signalé qu'il n'en a plus besoin. Exemple : le résultat d'un appel de fonction, un littéral, ou le résultat de `std::move()`. L'objet va disparaître — ses ressources peuvent être volées sans risque.

```cpp
Buffer a(1024);              // a est une lvalue — elle a un nom, elle persiste

Buffer b = a;                // a est une lvalue → copie (on ne peut pas voler a)

Buffer c = Buffer(1024);     // Buffer(1024) est une rvalue → déplacement possible

Buffer d = std::move(a);     // std::move(a) convertit a en rvalue → déplacement
                              // a est maintenant dans un état vidé
```

`std::move` ne déplace rien. C'est un **cast** : il convertit une lvalue en rvalue, donnant la **permission** au compilateur (et aux constructeurs/opérateurs) de traiter l'objet comme déplaçable. Le déplacement réel est effectué par le constructeur de déplacement ou l'opérateur d'affectation par déplacement.

---

## Portée de la sémantique de mouvement

La sémantique de mouvement ne concerne pas uniquement les classes que vous écrivez. Elle imprègne tout le C++ moderne :

**La STL entière en bénéficie.** Les conteneurs (`std::vector`, `std::string`, `std::map`…) implémentent tous des constructeurs et opérateurs de déplacement. Un `std::vector` de 10 millions d'éléments se déplace en copiant trois pointeurs internes — pas en dupliquant 10 millions d'éléments.

```cpp
std::vector<int> v(10'000'000, 42);

// Copie : alloue 40 Mo, copie 10 millions d'entiers
std::vector<int> copie = v;

// Déplacement : copie 3 pointeurs (~24 octets), v devient vide
std::vector<int> deplace = std::move(v);
```

**Les smart pointers en dépendent.** `std::unique_ptr` est un type *move-only* — sa copie est supprimée, seul le déplacement existe. C'est la sémantique de mouvement qui rend possible le concept de possession exclusive transférable ([section 9.1.2](/09-smart-pointers/01.2-transfert-propriete.md)).

**Le retour de valeurs en profite.** Les fonctions qui retournent des objets lourds par valeur — un pattern qui était coûteux avant C++11 — sont désormais efficaces grâce au déplacement (et encore plus grâce au RVO, voir [section 10.5](/10-move-semantics/05-rvo-copy-elision.md)).

**Les conteneurs deviennent plus efficaces.** Quand un `std::vector` réalloue son buffer interne (parce que sa capacité est dépassée), il **déplace** ses éléments vers le nouveau buffer au lieu de les copier — à condition que le constructeur de déplacement soit `noexcept`.

---

## Le rôle crucial de noexcept

Un point revient sans cesse quand on parle de sémantique de mouvement : le mot-clé `noexcept`. Les constructeurs de déplacement et les opérateurs d'affectation par déplacement doivent être marqués `noexcept` chaque fois que c'est possible.

La raison est pratique : la STL utilise `std::move_if_noexcept` dans ses opérations internes. Si votre constructeur de déplacement n'est pas `noexcept`, les conteneurs **reviendront à la copie** par sécurité, annulant tout le bénéfice du mouvement :

```cpp
class Widget {  
public:  
    // ✅ noexcept → vector utilisera le déplacement lors des réallocations
    Widget(Widget&& other) noexcept;

    // ⚠️ Sans noexcept → vector copiera au lieu de déplacer
    // Widget(Widget&& other);
};
```

Ce sujet est approfondi en [section 10.3](/10-move-semantics/03-move-constructors.md).

---

## Vue d'ensemble des sections

Ce chapitre est organisé en cinq sections qui construisent progressivement la maîtrise de la sémantique de mouvement :

**[10.1 — L-values vs R-values (&&)](/10-move-semantics/01-lvalues-rvalues.md)**
Les catégories de valeurs en C++ : lvalue, rvalue, et les sous-catégories (prvalue, xvalue, glvalue). Les références rvalue (`T&&`) et leur rôle dans la surcharge de fonctions.

**[10.2 — std::move : Transfert de propriété sans copie](/10-move-semantics/02-std-move.md)**
Ce que `std::move` fait réellement (un cast, pas un déplacement), quand l'utiliser, et les pièges courants (utiliser un objet après move, move sur des objets `const`).

**[10.3 — Move constructors et move assignment operators](/10-move-semantics/03-move-constructors.md)**
Comment implémenter le constructeur de déplacement et l'opérateur d'affectation par déplacement pour vos propres classes. L'importance de `noexcept`. L'interaction avec la Règle des 5.

**[10.4 — Perfect Forwarding avec std::forward](/10-move-semantics/04-perfect-forwarding.md)**
Les *forwarding references* (`T&&` dans un contexte template), le problème de la perte de catégorie de valeur, et comment `std::forward` le résout. Indispensable pour écrire des fonctions template génériques comme `make_unique`.

**[10.5 — Return Value Optimization (RVO) et Copy Elision](/10-move-semantics/05-rvo-copy-elision.md)**
Les optimisations du compilateur qui éliminent les copies et les déplacements avant même qu'ils ne se produisent. NRVO, RVO, et la *mandatory copy elision* de C++17. Pourquoi `return std::move(x)` est presque toujours une erreur.

---

## Conventions utilisées dans ce chapitre

- **Standard minimum : C++17.** Les exemples compilent avec `-std=c++17`. Les comportements spécifiques à C++11/14 sont signalés quand ils diffèrent.
- **Compilateurs** : GCC 15 et Clang 20 sur Ubuntu, conformément à l'environnement de la formation.
- Les annotations `// lvalue` et `// rvalue` dans les exemples indiquent la catégorie de valeur d'une expression.
- Les annotations `// copie` et `// move` indiquent quelle opération est invoquée par le compilateur.

---

> **En résumé** — Avant C++11, le C++ n'avait qu'un seul mécanisme de transfert de données : la copie. La sémantique de mouvement ajoute un second mécanisme, drastiquement plus efficace, pour les situations où l'objet source n'est plus nécessaire. Comprendre quand et comment le compilateur choisit entre copie et déplacement est une compétence fondamentale du développeur C++ moderne.

⏭️ [L-values vs R-values (&&)](/10-move-semantics/01-lvalues-rvalues.md)
