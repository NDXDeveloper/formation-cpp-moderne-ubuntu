🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 6.2 — Constructeurs

## Chapitre 6 : Classes et Encapsulation

---

## Ce que vous allez apprendre

- Comprendre le rôle fondamental des constructeurs dans le cycle de vie d'un objet.  
- Distinguer les cinq formes de constructeurs en C++ : par défaut, paramétré, de copie, de déplacement, et par liste d'initialisation.  
- Connaître les règles de génération automatique du compilateur et savoir quand il faut intervenir.  
- Utiliser les mots-clés `default`, `delete` et `explicit` pour contrôler le comportement des constructeurs.

---

## Le problème : un objet non initialisé est une bombe à retardement

En section 6.1, nous avons vu une première version de `DynArray` avec un constructeur qui alloue la mémoire et un destructeur qui la libère. Mais imaginons un instant qu'on puisse créer un `DynArray` sans passer par le constructeur — ou qu'on oublie d'initialiser un membre :

```cpp
// Scénario hypothétique — que se passe-t-il si data_ n'est pas initialisé ?
DynArray arr;          // data_ contient une adresse aléatoire  
arr[0] = 42;           // Écriture à une adresse mémoire inconnue  
// ... plus tard ...
// Le destructeur appelle delete[] sur un pointeur invalide → crash
```

Ce scénario n'est pas théorique. En C, c'est un bug classique : on déclare une structure, on oublie d'appeler la fonction d'initialisation, et le programme se retrouve avec des données corrompues. Le bug peut rester dormant pendant des mois avant de se manifester en production.

Le constructeur résout ce problème de façon définitive. En C++, **il est impossible de créer un objet sans exécuter l'un de ses constructeurs**. Le constructeur est la première chose qui s'exécute quand un objet naît — que ce soit sur la pile, sur le tas, dans un conteneur, ou comme membre d'une autre classe. C'est une garantie du langage, pas une convention.

---

## Qu'est-ce qu'un constructeur ?

Un constructeur est une **fonction membre spéciale** qui initialise un objet au moment de sa création. Il se distingue des méthodes ordinaires par trois caractéristiques :

- Il porte **le même nom que la classe**.  
- Il n'a **pas de type de retour** — pas même `void`.  
- Il est **appelé automatiquement** par le compilateur lors de la création d'un objet.

```cpp
class Timestamp {  
public:  
    Timestamp() {                          // Constructeur par défaut
        seconds_since_epoch_ = 0;
    }

    Timestamp(long seconds) {              // Constructeur paramétré
        seconds_since_epoch_ = seconds;
    }

private:
    long seconds_since_epoch_;
};

Timestamp t1;              // Appelle Timestamp()  
Timestamp t2(1709078400);  // Appelle Timestamp(long)  
Timestamp t3{1709078400};  // Idem, syntaxe d'initialisation uniforme (C++11)  
```

Le compilateur choisit le constructeur à appeler en fonction des arguments fournis — c'est la **surcharge** (overload resolution), le même mécanisme que pour les fonctions ordinaires vu en section 4.4.

---

## Les cinq formes de constructeurs

Le C++ définit cinq formes de constructeurs, chacune répondant à un besoin précis. Les trois premières concernent la **création** et la **copie** d'objets — des concepts familiers. Les deux dernières, liées à la **sémantique de déplacement**, sont apparues avec C++11 et seront approfondies au chapitre 10.

| Forme | Signature type | Rôle | Section |  
|-------|---------------|------|---------|  
| **Par défaut** | `T()` | Créer un objet sans argument | 6.2.1 |  
| **Paramétré** | `T(args...)` | Créer un objet avec des valeurs initiales | 6.2.2 |  
| **De copie** | `T(const T&)` | Créer un objet comme clone d'un autre | 6.2.3 |  
| **De déplacement** | `T(T&&)` | Créer un objet en *volant* les ressources d'un autre | 6.2.4 |  
| **Liste d'initialisation** | (mécanisme, pas une forme distincte) | Initialiser les membres avant le corps du constructeur | 6.2.5 |

> 💡 La liste d'initialisation n'est pas un type de constructeur à proprement parler — c'est un **mécanisme d'initialisation** utilisable dans n'importe quel constructeur. Nous la traitons en sous-section dédiée (6.2.5) car elle est souvent source de confusion chez les débutants et pourtant essentielle à maîtriser.

---

## Génération automatique par le compilateur

Le compilateur C++ peut **générer automatiquement** certains constructeurs si vous ne les écrivez pas. Cette génération implicite suit des règles précises qu'il faut connaître pour éviter les surprises.

**Règle fondamentale** : si vous ne déclarez aucun constructeur, le compilateur génère un constructeur par défaut implicite qui initialise chaque membre avec sa propre valeur par défaut (ou le laisse non initialisé pour les types primitifs sans *default member initializer*).

```cpp
class Simple {  
public:  
    // Aucun constructeur déclaré
    // → Le compilateur génère Simple() implicitement
private:
    int x_ = 0;           // Initialisé à 0 grâce au default member initializer
    std::string name_;     // Initialisé à "" (constructeur par défaut de std::string)
};

Simple s;   // OK — le constructeur implicite est généré
```

Mais dès que vous déclarez **un constructeur quelconque**, le compilateur **ne génère plus** le constructeur par défaut :

```cpp
class Rigid {  
public:  
    Rigid(int value) : value_(value) {}   // Constructeur paramétré déclaré
    // → Plus de constructeur par défaut implicite
private:
    int value_;
};

// Rigid r;       // ERREUR de compilation — pas de constructeur par défaut
Rigid r(42);      // OK
```

C'est logique : si vous avez pris la peine de définir un constructeur avec un argument obligatoire, c'est probablement que l'objet n'a pas de sens sans cet argument. Le compilateur respecte cette intention.

Si vous voulez **restaurer** le constructeur par défaut tout en gardant votre constructeur paramétré, C++11 fournit `= default` :

```cpp
class Flexible {  
public:  
    Flexible() = default;                    // Explicitement rétabli
    Flexible(int value) : value_(value) {}
private:
    int value_ = 0;   // Valeur par défaut utilisée par Flexible()
};

Flexible a;       // OK — utilise le constructeur par défaut  
Flexible b(42);   // OK — utilise le constructeur paramétré  
```

À l'inverse, `= delete` permet d'**interdire** un constructeur :

```cpp
class NonCopyable {  
public:  
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;   // Copie interdite
};

NonCopyable a;
// NonCopyable b = a;   // ERREUR — constructeur de copie supprimé
```

Le tableau suivant résume les règles de génération implicite. Nous y reviendrons en détail dans chaque sous-section, et la section 6.5 (Règle des 5) dressera le tableau complet :

| Vous déclarez… | Constructeur par défaut | Constructeur de copie | Constructeur de déplacement |  
|---|---|---|---|  
| Rien | ✅ Généré | ✅ Généré | ✅ Généré |  
| Un constructeur paramétré | ❌ Supprimé | ✅ Généré | ✅ Généré |  
| Un constructeur de copie | ❌ Supprimé | (celui que vous avez écrit) | ❌ Supprimé |  
| Un destructeur | ✅ Généré | ✅ Généré | ❌ Supprimé |

> ⚠️ Ce tableau est simplifié. Les interactions complètes entre les cinq opérations spéciales sont détaillées en section 6.5 (Règle des 5). Le point à retenir ici est que **déclarer une opération spéciale influence la génération des autres** — c'est pourquoi la Règle des 5 existe.

---

## `explicit` : bloquer les conversions implicites

Par défaut, un constructeur à un seul argument peut être utilisé comme **opérateur de conversion implicite**. Ce comportement est rarement souhaitable :

```cpp
class Distance {  
public:  
    Distance(double meters) : meters_(meters) {}
    double meters() const { return meters_; }
private:
    double meters_;
};

void print_distance(Distance d) {
    std::cout << d.meters() << " m\n";
}

print_distance(3.5);   // Compile ! Le double est implicitement converti en Distance
```

Ce code compile parce que le compilateur utilise `Distance(double)` pour convertir `3.5` en un objet `Distance`. C'est parfois pratique, mais souvent source de bugs subtils — imaginez un appel accidentel avec un entier qui n'a rien à voir avec une distance.

Le mot-clé `explicit` empêche cette conversion :

```cpp
class Distance {  
public:  
    explicit Distance(double meters) : meters_(meters) {}
    // ...
};

// print_distance(3.5);            // ERREUR — conversion implicite interdite
print_distance(Distance(3.5));     // OK — conversion explicite
```

**Règle pratique** : déclarez `explicit` tout constructeur pouvant être appelé avec un seul argument, sauf si la conversion implicite a une signification sémantique évidente. Les *C++ Core Guidelines* (règle C.46) sont claires : *"By default, declare single-argument constructors explicit."*

Depuis C++11, `explicit` s'applique aussi aux constructeurs à arguments multiples lorsque tous sauf le premier ont une valeur par défaut, et aux opérateurs de conversion. Depuis C++20, `explicit` peut même être conditionnel avec `explicit(condition)`, mais ces cas avancés dépassent le cadre de cette section.

---

## Syntaxes d'initialisation : un paysage à naviguer

Le C++ offre plusieurs syntaxes pour initialiser un objet. Cette diversité a des raisons historiques, mais elle peut dérouter. Voici les formes les plus courantes et quand les utiliser :

```cpp
class Point {  
public:  
    Point() : x_(0.0), y_(0.0) {}
    Point(double x, double y) : x_(x), y_(y) {}
private:
    double x_, y_;
};

// 1. Initialisation directe (parenthèses)
Point p1(3.0, 4.0);

// 2. Initialisation uniforme (accolades) — recommandée depuis C++11
Point p2{3.0, 4.0};

// 3. Initialisation par copie (signe =)
Point p3 = Point(3.0, 4.0);

// 4. Constructeur par défaut
Point p4;        // Appelle Point()  
Point p5{};      // Idem, plus explicite  
```

La **syntaxe à accolades** (`{}`) est généralement recommandée en C++ moderne. Elle présente un avantage de sécurité : elle interdit les **conversions implicites qui perdent de l'information** (*narrowing conversions*) :

```cpp
int value = 3.7;     // OK — tronqué silencieusement à 3
// int value{3.7};   // ERREUR — narrowing conversion interdite avec {}
```

Il y a cependant un piège à connaître : si la classe possède un constructeur acceptant un `std::initializer_list`, les accolades seront dirigées vers ce constructeur en priorité. C'est le cas de `std::vector` :

```cpp
std::vector<int> v1(5, 0);    // 5 éléments valant 0 → {0, 0, 0, 0, 0}  
std::vector<int> v2{5, 0};    // 2 éléments : 5 et 0   → {5, 0}  
```

Ce cas particulier concerne principalement les classes avec `std::initializer_list`. Pour nos propres classes sans ce constructeur, parenthèses et accolades se comportent de façon identique.

---

## Plan des sous-sections

Chaque sous-section qui suit détaille une forme de constructeur avec des exemples concrets, les pièges à éviter, et l'évolution progressive de notre classe `DynArray` :

| Sous-section | Thème | Ce que `DynArray` gagnera |  
|---|---|---|  
| **6.2.1** | Constructeur par défaut | Un état initial valide sans argument |  
| **6.2.2** | Constructeur paramétré | La création avec une taille donnée |  
| **6.2.3** | Constructeur de copie | La duplication correcte (plus de *double free*) |  
| **6.2.4** | Constructeur de déplacement | Le transfert de ressources sans copie |  
| **6.2.5** | Liste d'initialisation | L'initialisation efficace et correcte des membres |

---


⏭️ [Constructeur par défaut](/06-classes-encapsulation/02.1-constructeur-defaut.md)
