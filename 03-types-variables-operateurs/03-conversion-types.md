🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 3.3 — Conversion de types : Cast implicite vs explicite

> **Chapitre 3 — Types, Variables et Opérateurs** · Section 3 sur 5  
> Prérequis : [3.2 — Types primitifs, tailles et représentation mémoire](/03-types-variables-operateurs/02-types-primitifs.md)

---

## Introduction

Dans un langage à typage statique fort, les opérations entre types différents ne vont pas de soi. Que se passe-t-il quand on additionne un `int` et un `double` ? Quand on affecte un `double` à un `int` ? Quand on passe un pointeur vers une classe dérivée là où un pointeur vers la classe de base est attendu ?

Dans chacun de ces cas, une **conversion de type** intervient — le passage d'une valeur d'un type vers un autre. En C++, ces conversions prennent deux formes radicalement différentes par leur niveau de contrôle et de sécurité : les conversions **implicites**, décidées par le compilateur sans intervention du programmeur, et les conversions **explicites** (*casts*), demandées volontairement par le programmeur.

Comprendre la distinction entre ces deux mécanismes est essentiel pour écrire du code correct, sûr et maintenable. Cette section présente les principes généraux des conversions en C++, explique pourquoi les casts C-style sont dangereux, et introduit les quatre opérateurs de cast du C++ moderne qui les remplacent.

---

## Conversions implicites

Le compilateur C++ effectue automatiquement certaines conversions lorsque les types ne correspondent pas exactement. Ces conversions sont dites **implicites** parce qu'elles ne nécessitent aucune syntaxe particulière de la part du programmeur — elles se produisent « en coulisses ».

### Promotions numériques

Les promotions sont des conversions implicites qui élargissent un type vers un type plus grand sans perte d'information :

```cpp
short s = 10;  
int i = s;       // Promotion : short → int (aucune perte)  

int x = 42;  
double d = x;    // Promotion : int → double (aucune perte)  

float f = 1.5f;  
double d2 = f;   // Promotion : float → double (aucune perte)  
```

Ces conversions sont toujours sûres : le type cible est capable de représenter toutes les valeurs du type source. Le compilateur les effectue sans émettre le moindre avertissement.

### Conversions arithmétiques usuelles

Lorsque deux opérandes de types différents apparaissent dans une expression, le compilateur convertit l'opérande de type « inférieur » vers le type « supérieur » selon une hiérarchie définie (vue en section 3.2.1). C'est ce qui permet d'écrire des expressions mixtes :

```cpp
int count = 7;  
double average = count / 2.0; // count est converti en double avant la division  
                                // Résultat : 3.5 (division flottante)
```

Sans cette conversion implicite, il faudrait écrire `static_cast<double>(count) / 2.0` — ce qui serait correct mais inutilement verbeux.

### Conversions avec perte potentielle (narrowing)

Certaines conversions implicites sont autorisées par le compilateur mais comportent un **risque de perte d'information**. Ce sont les conversions dites *narrowing* :

```cpp
double pi = 3.14159;  
int truncated = pi;         // ⚠️ Perte de la partie fractionnaire → 3  

int64_t big = 5'000'000'000;  
int32_t small = big;        // ⚠️ Troncation des bits de poids fort  

int negative = -1;  
unsigned int u = negative;  // ⚠️ Réinterprétation du signe → 4'294'967'295  
```

Le compilateur émet un avertissement pour ces conversions (avec `-Wall -Wextra`), mais il les accepte. C'est l'un des héritages du C que le C++ moderne cherche à corriger — l'initialisation par accolades interdit ces conversions :

```cpp
int safe{3.14};    // ❌ Erreur de compilation : narrowing conversion  
int unsafe = 3.14; // ⚠️ Compile avec un warning seulement  
```

### Conversions booléennes

Tout type arithmétique ou pointeur peut être implicitement converti en `bool`. La règle est simple : zéro (ou `nullptr` pour les pointeurs) donne `false`, toute autre valeur donne `true` :

```cpp
int x = 42;  
if (x) { /* s'exécute : x != 0 → true */ }  

int* ptr = nullptr;  
if (ptr) { /* ne s'exécute pas : nullptr → false */ }  
```

La conversion inverse — de `bool` vers un type arithmétique — est également implicite : `true` devient 1 et `false` devient 0.

### Conversions de pointeurs dans la hiérarchie de classes

Un pointeur (ou une référence) vers une classe dérivée peut être implicitement converti vers un pointeur (ou une référence) vers sa classe de base. C'est le mécanisme fondamental du **polymorphisme** :

```cpp
class Animal { /* ... */ };  
class Dog : public Animal { /* ... */ };  

Dog rex;  
Animal* animal_ptr = &rex; // Conversion implicite : Dog* → Animal*  
Animal& animal_ref = rex;  // Conversion implicite : Dog& → Animal&  
```

Cette conversion est sûre car un `Dog` *est* un `Animal` (relation « est-un »). La conversion inverse (de base vers dérivée) n'est **pas** implicite — elle nécessite un cast explicite, car elle peut échouer. Nous verrons cela avec `dynamic_cast` en section 3.3.4.

---

## Le problème des casts C-style

Le C dispose d'une syntaxe unique pour toutes les conversions explicites : le **cast C-style**, qui consiste à placer le type cible entre parenthèses devant l'expression :

```cpp
double pi = 3.14159;  
int n = (int)pi;           // Cast C-style : supprime la partie fractionnaire  

void* raw = malloc(100);  
int* data = (int*)raw;     // Cast C-style : convertit void* en int*  
```

Cette syntaxe est toujours valide en C++, mais elle est **fortement déconseillée** pour plusieurs raisons :

**Elle est trop permissive.** Un cast C-style essaie successivement `const_cast`, `static_cast`, `static_cast` suivi de `const_cast`, `reinterpret_cast`, et `reinterpret_cast` suivi de `const_cast` — dans cet ordre — et utilise le premier qui fonctionne. Cela signifie qu'un cast C-style peut effectuer une réinterprétation mémoire dangereuse là où le programmeur pensait faire une simple conversion arithmétique, sans aucun avertissement.

**Elle est invisible dans le code.** La syntaxe `(int)x` se fond dans le code et est difficile à repérer lors d'une revue ou d'une recherche textuelle. Si un bug est lié à une conversion, identifier tous les casts dans une base de code est une opération laborieuse.

**Elle ne distingue pas les intentions.** Convertir un `double` en `int` (opération arithmétique courante) et convertir un `void*` en `int*` (opération dangereuse sur la mémoire brute) utilisent exactement la même syntaxe. Le lecteur du code ne peut pas savoir, en lisant un cast C-style, quel degré de risque il porte.

Le C++ offre en remplacement un système de casts **fonctionnels** — quatre variantes nommées, chacune avec une syntaxe distincte, un domaine d'application précis et un niveau de risque clairement identifié.

---

## Les quatre opérateurs de cast du C++

Le C++ définit quatre opérateurs de cast explicite. Chacun porte un nom qui exprime son intention et ses limites :

### `static_cast<T>(expr)`

C'est le cast le plus courant et le plus sûr. Il effectue des conversions vérifiables à la compilation : conversions numériques, conversions dans les hiérarchies de classes (vers le haut *et* vers le bas), conversions de `void*` vers un type pointeur, et conversions vers/depuis `enum`. Il refuse les conversions manifestement incohérentes.

```cpp
double pi = 3.14159;  
int n = static_cast<int>(pi); // Conversion numérique explicite  
```

### `reinterpret_cast<T>(expr)`

C'est le cast le plus dangereux. Il réinterprète les bits d'une valeur comme un autre type, sans aucune vérification de cohérence. Il est utilisé pour les conversions de pointeurs entre types non liés, l'interaction avec des API C bas niveau, et certains cas de programmation système.

```cpp
int x = 42;  
char* bytes = reinterpret_cast<char*>(&x); // Accès aux octets bruts de x  
```

### `const_cast<T>(expr)`

C'est le seul cast capable d'ajouter ou de retirer les qualificateurs `const` et `volatile`. Il est utilisé dans des situations très spécifiques, principalement pour interfacer du code C++ avec des API C qui ne respectent pas la const-correctness.

```cpp
const char* msg = "hello";  
char* mutable_msg = const_cast<char*>(msg); // Retire const  
// ⚠️ Modifier *mutable_msg est un comportement indéfini si l'objet original est vraiment const
```

### `dynamic_cast<T>(expr)`

C'est le seul cast qui effectue une vérification **à l'exécution**. Il est utilisé pour les conversions vers le bas (*downcast*) dans les hiérarchies polymorphiques (classes avec des fonctions virtuelles). Si la conversion est invalide, il retourne `nullptr` (pour les pointeurs) ou lève une exception `std::bad_cast` (pour les références).

```cpp
Animal* animal = get_animal();  
Dog* dog = dynamic_cast<Dog*>(animal); // nullptr si animal n'est pas un Dog  
```

---

## Pourquoi quatre casts au lieu d'un seul ?

La multiplication des casts en C++ n'est pas un caprice de conception — c'est une stratégie délibérée de **séparation des responsabilités**. Chaque cast exprime une intention différente et porte un niveau de risque différent :

| Cast | Vérifié à la compilation | Vérifié à l'exécution | Niveau de risque |
|---|---|---|---|
| `static_cast` | Oui | Non | Faible |
| `dynamic_cast` | Oui | **Oui** | Faible (avec coût runtime) |
| `const_cast` | Oui | Non | Moyen |
| `reinterpret_cast` | Minimal | Non | **Élevé** |

Cette hiérarchie guide le développeur vers le cast le moins dangereux possible :

1. Essayez d'abord de vous en passer — les conversions implicites suffisent souvent.
2. Si un cast est nécessaire, utilisez `static_cast` — c'est le bon choix dans 90% des cas.
3. Si vous travaillez avec du polymorphisme et que vous avez besoin de vérifier le type à l'exécution, utilisez `dynamic_cast`.
4. Si vous interfacez avec une API C qui ne respecte pas `const`, utilisez `const_cast`.
5. N'utilisez `reinterpret_cast` qu'en dernier recours, pour de la manipulation mémoire brute.

Si vous trouvez un `reinterpret_cast` dans du code, il devrait attirer votre attention immédiatement : c'est une opération dangereuse qui mérite un commentaire expliquant pourquoi elle est nécessaire.

---

## Le cast fonctionnel (constructor-style)

Il existe une cinquième syntaxe de conversion, parfois appelée **cast fonctionnel** :

```cpp
double pi = 3.14159;  
int n = int(pi);        // Syntaxe fonctionnelle : équivalent à (int)pi  
```

Pour les types primitifs, cette syntaxe est équivalente au cast C-style et en partage les défauts. Elle est toutefois considérée comme acceptable dans un cas précis : l'initialisation explicite d'un type primitif là où l'intention est clairement une conversion arithmétique :

```cpp
auto ratio = double(numerator) / double(denominator);
```

Pour les types définis par l'utilisateur, la syntaxe fonctionnelle appelle le constructeur — c'est alors une **construction**, pas un cast :

```cpp
std::string s = std::string("hello"); // Appel de constructeur, pas un cast
```

En général, préférez `static_cast` au cast fonctionnel pour les conversions entre types primitifs : l'intention est plus explicite et la syntaxe se distingue clairement de la construction d'objets.

---

## Rechercher les casts dans le code

L'un des avantages pratiques des casts nommés C++ est leur facilité de recherche. Si vous soupçonnez qu'un bug est lié à une conversion, une simple recherche textuelle suffit :

```bash
grep -rn "static_cast"      src/  
grep -rn "reinterpret_cast" src/  
grep -rn "const_cast"       src/  
grep -rn "dynamic_cast"     src/  
```

La même recherche pour un cast C-style — `(int)`, `(double*)`, etc. — est pratiquement impossible sans un outil d'analyse syntaxique, car les parenthèses sont omniprésentes en C++. C'est un argument supplémentaire en faveur de l'abandon des casts C-style.

Les outils d'analyse statique comme `clang-tidy` (chapitre 32) détectent les casts C-style et proposent automatiquement leur remplacement par le cast nommé approprié (check `google-readability-casting` ou `cppcoreguidelines-pro-type-cstyle-cast`).

---

## Ce que nous allons couvrir

Les quatre sous-sections suivantes explorent chaque opérateur de cast en profondeur :

- **3.3.1 — `static_cast` : Conversions sûres.** Le cast par défaut du C++ moderne. Conversions numériques, conversions de pointeurs dans les hiérarchies de classes, conversions vers/depuis `enum`, et limites du mécanisme.

- **3.3.2 — `reinterpret_cast` : Réinterprétation mémoire.** Quand et pourquoi réinterpréter les bits bruts d'une valeur. Conversions de pointeurs entre types non liés, interaction avec les API C, règles d'aliasing strictes, et dangers.

- **3.3.3 — `const_cast` : Manipulation de `const`.** Retirer ou ajouter `const` : dans quels cas c'est légal, dans quels cas c'est un comportement indéfini, et comment l'éviter autant que possible.

- **3.3.4 — `dynamic_cast` : Cast polymorphique.** Vérification de type à l'exécution, fonctionnement avec les vtables, coût en performance, et alternatives (`static_cast` quand le type est garanti, `std::variant` pour le polymorphisme sans héritage).

---


⏭️ [static_cast : Conversions sûres](/03-types-variables-operateurs/03.1-static-cast.md)
