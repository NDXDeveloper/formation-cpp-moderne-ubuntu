🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 9.4 Ne jamais utiliser new/delete dans du code moderne 🔥

## La règle

> **Dans du code C++ moderne, les appels directs à `new` et `delete` ne devraient jamais apparaître dans le code applicatif.**

Ce n'est pas une recommandation timide ni une préférence stylistique. C'est un consensus de l'industrie, formalisé dans les [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) (règles R.11 et R.12), défendu par les créateurs du langage et les comités de standardisation, et adopté par les équipes les plus exigeantes — de Google à Bloomberg, en passant par les projets open source majeurs.

La raison est simple : chaque `new` écrit manuellement est une promesse que le `delete` correspondant sera appelé exactement une fois, au bon moment, sur chaque chemin d'exécution possible. L'expérience de quarante ans de C et C++ montre que les humains ne tiennent pas cette promesse de manière fiable. Les smart pointers, eux, la tiennent systématiquement.

---

## Ce que signifie « code applicatif »

La distinction est importante. L'interdiction de `new`/`delete` s'applique au **code applicatif** — le code métier, les services, les outils, les composants que vous développez au quotidien.

Elle ne s'applique pas nécessairement au **code d'infrastructure bas niveau** : implémentation d'allocateurs personnalisés, de conteneurs, de smart pointers eux-mêmes, ou de couches d'abstraction au-dessus d'API C. Ce code-là manipule `new`/`delete` (ou `malloc`/`free`) par nécessité, mais il est encapsulé derrière une interface RAII que le code applicatif consomme sans jamais voir de pointeur brut possédant.

En d'autres termes : `new` et `delete` peuvent exister dans votre codebase, mais ils devraient être **confinés** dans des couches d'abstraction basses, jamais dispersés dans la logique métier.

---

## Les alternatives pour chaque situation

L'abandon de `new`/`delete` ne signifie pas l'abandon de l'allocation dynamique. Cela signifie que chaque allocation est **immédiatement encapsulée** dans un gestionnaire de durée de vie. Voici le guide de décision complet.

### L'objet peut vivre sur la stack ?

C'est toujours la première question à se poser. Si la durée de vie de l'objet correspond au scope d'une fonction ou d'un bloc, pas besoin d'allocation dynamique du tout :

```cpp
// ❌ Allocation dynamique inutile
auto config = std::make_unique<Config>("app.yaml");  
traiter(*config);  

// ✅ Variable locale — plus simple, plus rapide
Config config("app.yaml");  
traiter(config);  
```

Les variables locales sont allouées sur la stack, détruites automatiquement en sortie de scope, et ne coûtent aucun appel à l'allocateur. C'est la forme la plus efficace et la plus sûre de gestion mémoire.

### Un seul propriétaire ?

Si l'objet doit survivre au-delà du scope de création, ou si sa taille/son type n'est connu qu'à l'exécution, mais qu'un seul propriétaire existe à tout instant :

```cpp
// ❌ Ancien style
Widget* w = new Widget(42);
// ... 200 lignes plus tard ...
delete w;

// ✅ C++ moderne
auto w = std::make_unique<Widget>(42);
// Libéré automatiquement en sortie de scope
```

### Propriété partagée ?

Si plusieurs parties du code doivent co-posséder la ressource sans relation hiérarchique claire :

```cpp
// ❌ Ancien style — qui fait le delete ?
Widget* shared_w = new Widget(42);  
module_a->set_widget(shared_w);  
module_b->set_widget(shared_w);  
// delete shared_w; // Quand ? Qui ? Après A ou B ?

// ✅ C++ moderne
auto shared_w = std::make_shared<Widget>(42);  
module_a->set_widget(shared_w);  
module_b->set_widget(shared_w);  
// Libéré quand A et B n'en ont plus besoin
```

### Collection d'objets dynamiques ?

Utilisez un conteneur standard. Les conteneurs de la STL gèrent l'allocation et la libération de leurs éléments :

```cpp
// ❌ Ancien style
int* tab = new int[1000];
// ... utilisation ...
delete[] tab;

// ✅ C++ moderne
std::vector<int> tab(1000, 0);
// Taille dynamique, mémoire gérée automatiquement
```

### Collection d'objets polymorphiques ?

Combinez un conteneur avec des smart pointers :

```cpp
// ❌ Ancien style — cauchemar de gestion mémoire
std::vector<Animal*> zoo;  
zoo.push_back(new Chat());  
zoo.push_back(new Chien());  
// ... qui fait le delete de chaque élément ?
for (auto* a : zoo) delete a;  // Fragile, oubliable

// ✅ C++ moderne
std::vector<std::unique_ptr<Animal>> zoo;  
zoo.push_back(std::make_unique<Chat>());  
zoo.push_back(std::make_unique<Chien>());  
// Tout est libéré automatiquement avec le vector
```

### Ressource non-C++ (fichier, handle, socket) ?

Utilisez un smart pointer avec custom deleter ou un wrapper RAII dédié :

```cpp
// ❌ Ancien style
FILE* f = fopen("data.txt", "r");
// ... traitement ...
fclose(f);  // Oublié si exception

// ✅ C++ moderne — custom deleter
auto f = std::unique_ptr<FILE, decltype(&fclose)>(
    fopen("data.txt", "r"), fclose
);

// ✅ Encore mieux — wrapper RAII dédié (section 9.1.3)
auto f = UniqueFile(fopen("data.txt", "r"));
```

### Tableau récapitulatif

| Besoin | Ancien style | C++ moderne |
|---|---|---|
| Objet à durée de vie locale | `T* p = new T; ... delete p;` | `T obj;` (stack) |
| Propriétaire unique, durée de vie étendue | `T* p = new T;` | `std::make_unique<T>()` |
| Propriété partagée | `T* p = new T;` (ambiguïté) | `std::make_shared<T>()` |
| Tableau dynamique | `T* p = new T[n]; ... delete[] p;` | `std::vector<T>` |
| Collection polymorphique | `vector<T*>` + `delete` manuel | `vector<unique_ptr<T>>` |
| Ressource C | `handle = api_open(); ... api_close(handle);` | `unique_ptr` + custom deleter |

---

## Les rares exceptions légitimes

La règle « pas de `new`/`delete` » admet quelques exceptions. Elles sont rares, bien identifiées, et devraient être **documentées** dans le code quand elles se présentent.

### 1. Custom deleters avec shared_ptr

`std::make_shared` ne supporte pas les custom deleters. La construction directe avec `new` est alors nécessaire :

```cpp
// Justifié : make_shared ne permet pas de custom deleter
auto conn = std::shared_ptr<DbConnection>(
    db_connect("localhost"),
    [](DbConnection* c) { db_disconnect(c); }
);
```

> 💡 Pour `unique_ptr`, vous pouvez éviter `new` en passant directement le pointeur retourné par l'API C ([section 9.1.3](/09-smart-pointers/01.3-custom-deleters.md)). Le `new` n'est nécessaire que si vous devez construire l'objet C++ vous-même.

### 2. Constructeurs privés ou protégés

Quand `make_unique`/`make_shared` ne peuvent pas accéder au constructeur :

```cpp
class Singleton {
    Singleton() = default;
public:
    static std::unique_ptr<Singleton> creer() {
        // Justifié : constructeur privé
        return std::unique_ptr<Singleton>(new Singleton());
    }
};
```

### 3. Placement new

`placement new` construit un objet à une adresse mémoire prédéterminée, sans allouer de mémoire. Il est utilisé dans les allocateurs, les conteneurs personnalisés, et les systèmes de *memory pooling* :

```cpp
alignas(Widget) char buffer[sizeof(Widget)];  
Widget* w = new (buffer) Widget(42);  // Placement new — pas d'allocation  
// ...
w->~Widget();  // Destruction explicite obligatoire
```

C'est du code d'infrastructure, pas du code applicatif. Si vous écrivez du `placement new` dans la logique métier, reconsidérez votre architecture.

### 4. Interfaçage avec des API C qui prennent la propriété

Quand une API C attend un pointeur brut et prend la responsabilité de la libération :

```cpp
// L'API C prend la propriété et appelle free() elle-même
extern "C" void register_handler(Handler* h);

auto h = std::make_unique<Handler>();  
register_handler(h.release());  // release() transfère la propriété à l'API C  
```

Ici, `new` n'est pas utilisé directement (c'est `make_unique` + `release()`), mais le principe est le même : la propriété quitte le monde des smart pointers pour entrer dans une gestion manuelle. C'est justifié quand l'API l'exige.

### 5. Code d'infrastructure et allocateurs

Les implémentations de conteneurs, d'allocateurs, de pools mémoire et de smart pointers eux-mêmes utilisent `new`/`delete` (ou plus souvent `::operator new`/`::operator delete` ou `malloc`/`free`). C'est leur raison d'être : fournir l'abstraction que le code applicatif consomme.

---

## Moderniser du code existant (legacy)

Dans la réalité, vous ne partez pas toujours d'une page blanche. Les codebases existantes contiennent souvent des centaines, voire des milliers d'occurrences de `new`/`delete`. La modernisation doit être **progressive et méthodique**.

### Stratégie de migration

**Étape 1 : identifier les zones à risque.** Utilisez `grep` ou `clang-tidy` pour localiser toutes les occurrences de `new` et `delete` dans le code applicatif :

```bash
# Recherche simple
grep -rn '\bnew\b' src/ --include='*.cpp' --include='*.h'  
grep -rn '\bdelete\b' src/ --include='*.cpp' --include='*.h'  

# Avec clang-tidy (plus précis, comprend le contexte C++)
clang-tidy -checks='cppcoreguidelines-owning-memory' src/*.cpp
```

**Étape 2 : prioriser.** Ne modernisez pas tout d'un coup. Commencez par les cas les plus dangereux :

- `new` sans `delete` correspondant dans la même fonction (fuites probables).
- `delete` dans un chemin d'exécution mais pas dans tous (fuites en cas d'exception).
- `new` dans des fonctions longues ou complexes (risque élevé d'oubli).
- Pointeurs bruts retournés par des fonctions (propriété ambiguë).

**Étape 3 : transformer progressivement.** Pour chaque occurrence, appliquez la transformation mécanique :

```cpp
// Avant
Widget* w = new Widget(args);
// ... utilisation de w ...
delete w;

// Après — remplacement direct
auto w = std::make_unique<Widget>(args);
// ... utilisation de *w ou w-> ...
// delete supprimé — la destruction est automatique
```

**Étape 4 : ajuster les interfaces.** Les fonctions qui retournent des `T*` possédants doivent progressivement retourner des `unique_ptr<T>`. Les fonctions qui reçoivent des `T*` possédants doivent recevoir des `unique_ptr<T>` par valeur :

```cpp
// Avant — propriété ambiguë
Widget* creer_widget();              // L'appelant doit-il delete ?  
void enregistrer(Widget* w);         // La fonction prend-elle la propriété ?  

// Après — propriété explicite dans le type
std::unique_ptr<Widget> creer_widget();          // Oui, l'appelant possède  
void enregistrer(std::unique_ptr<Widget> w);     // Oui, la fonction possède  
void utiliser(const Widget& w);                  // Non, simple observation  
```

**Étape 5 : valider.** Après chaque lot de modifications, exécutez les tests et Valgrind/AddressSanitizer pour vérifier qu'aucune régression mémoire n'a été introduite.

### Clang-tidy : automatiser la détection

`clang-tidy` propose plusieurs checks qui signalent l'utilisation de `new`/`delete` dans du code applicatif :

```yaml
# .clang-tidy
Checks: >
  cppcoreguidelines-owning-memory,
  cppcoreguidelines-no-malloc,
  modernize-make-unique,
  modernize-make-shared
```

- `modernize-make-unique` : signale les `unique_ptr(new T)` remplaçables par `make_unique`.
- `modernize-make-shared` : signale les `shared_ptr(new T)` remplaçables par `make_shared`.
- `cppcoreguidelines-owning-memory` : signale les pointeurs bruts possédants.
- `cppcoreguidelines-no-malloc` : signale les appels à `malloc`/`free`.

L'intégration de ces checks dans le pipeline CI ([section 38.4](/38-cicd/04-automatisation.md)) et les pre-commit hooks ([section 47.2](/47-collaboration/02-pre-commit-hooks.md)) garantit que les nouvelles occurrences de `new`/`delete` sont détectées avant d'atteindre la branche principale.

---

## Impact sur la lisibilité du code

L'élimination de `new`/`delete` a un effet profond sur la lisibilité qui dépasse la simple question de la sécurité mémoire.

### La propriété devient visible dans les types

Dans du code avec des pointeurs bruts, la propriété est un **contrat implicite** documenté (au mieux) dans un commentaire :

```cpp
// Ancien style — qui possède quoi ?
class Engine {
    Renderer* renderer_;       // Possédé ? Partagé ? Observé ?
    Logger* logger_;           // Idem
    Config* config_;           // Idem

    ~Engine() {
        delete renderer_;      // Ah, celui-ci est possédé
        // Pas de delete logger_ — observation ?
        // Pas de delete config_ — partagé ? Oublié ?
    }
};
```

Avec les smart pointers, la propriété est un **contrat explicite** encodé dans le système de types :

```cpp
// C++ moderne — chaque relation est documentée par le type
class Engine {
    std::unique_ptr<Renderer> renderer_;   // Possédé exclusivement
    std::shared_ptr<Logger> logger_;       // Partagé entre modules
    Config& config_;                       // Référence — pas de propriété

    // Pas de destructeur explicite nécessaire — tout est automatique
};
```

Un développeur qui lit cette déclaration comprend immédiatement l'architecture de propriété, sans lire le destructeur, les commentaires, ni le reste du code.

### Les fonctions expriment leur intention

```cpp
// ✅ "Je crée un objet et je vous en donne la propriété"
std::unique_ptr<Session> ouvrir_session(const Credentials& creds);

// ✅ "Je vous donne un accès partagé à cette ressource"
std::shared_ptr<Cache> obtenir_cache();

// ✅ "J'ai besoin d'utiliser votre objet, sans le posséder"
void traiter(const Session& session);

// ✅ "J'ai besoin de votre objet, mais il pourrait être nul"
void traiter_optionnel(const Session* session);
```

Chaque signature raconte une histoire de propriété complète. Le compilateur en garantit le respect.

---

## Résumé du chapitre 9

Ce chapitre a couvert les trois smart pointers du C++ moderne et la philosophie qui les sous-tend :

| Smart pointer | Possession | Copie | Surcoût | Cas d'usage principal |
|---|---|---|---|---|
| `unique_ptr` | Exclusive | Interdite (move) | Zéro | Choix par défaut — 95% des cas |
| `shared_ptr` | Partagée | Autorisée (atomique) | Bloc de contrôle + compteurs | Caches, graphes, multi-thread |
| `weak_ptr` | Aucune | Autorisée | Référence au bloc existant | Briser les cycles, observer |

La hiérarchie de décision à retenir :

```
Variable locale (stack)
  └─ suffisant ? → utiliser une variable locale
  └─ insuffisant ↓

std::unique_ptr
  └─ suffisant ? → utiliser make_unique
  └─ insuffisant ↓

std::shared_ptr + std::weak_ptr
  └─ utiliser make_shared
  └─ weak_ptr pour les références arrière et les observations
```

Et la règle qui synthétise tout :

> **Zéro `new`. Zéro `delete`. Zéro fuite.** Les smart pointers et les conteneurs de la STL gèrent la mémoire. Votre code gère la logique métier.

⏭️ [Sémantique de Mouvement (Move Semantics)](/10-move-semantics/README.md)
