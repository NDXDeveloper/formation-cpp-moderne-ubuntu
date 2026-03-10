🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 6.5 — La règle des 5 (Rule of Five) ⭐

## Chapitre 6 : Classes et Encapsulation

---

## Ce que vous allez apprendre

- Identifier les **cinq opérations spéciales** que le compilateur peut générer pour toute classe.  
- Comprendre pourquoi déclarer l'une d'entre elles impose de considérer les quatre autres.  
- Maîtriser la matrice complète de génération implicite du compilateur.  
- Connaître la **Règle du Zéro** (*Rule of Zero*) et savoir quand elle remplace la Règle des 5.  
- Utiliser `= default` et `= delete` pour exprimer vos intentions de manière explicite.

---

## Le problème : cinq opérations, un seul invariant

Tout au long de ce chapitre, nous avons construit `DynArray` pièce par pièce : constructeur par défaut, constructeur paramétré, constructeur de copie, constructeur de déplacement, destructeur. À chaque étape, nous avons signalé un danger résiduel — un opérateur d'affectation manquant, un *double free* en embuscade, une fuite mémoire sur un chemin oublié.

Ces dangers ont une cause commune : les cinq opérations spéciales d'une classe sont **interdépendantes**. Elles partagent la responsabilité de gérer la même ressource, et si l'une d'elles se comporte incorrectement, les autres ne peuvent pas compenser. C'est comme un contrat à cinq clauses : si vous rédigez soigneusement quatre clauses mais laissez la cinquième à son libellé par défaut, le contrat entier peut être invalide.

Les cinq opérations spéciales sont :

| # | Opération | Signature | Rôle |  
|---|-----------|-----------|------|  
| 1 | **Destructeur** | `~T()` | Libère les ressources à la fin de la vie de l'objet |  
| 2 | **Constructeur de copie** | `T(const T&)` | Crée un nouvel objet comme clone d'un existant |  
| 3 | **Opérateur d'affectation par copie** | `T& operator=(const T&)` | Remplace le contenu d'un objet existant par une copie |  
| 4 | **Constructeur de déplacement** | `T(T&&) noexcept` | Crée un nouvel objet en volant les ressources d'un autre |  
| 5 | **Opérateur d'affectation par déplacement** | `T& operator=(T&&) noexcept` | Remplace le contenu d'un objet existant en volant les ressources |

La **Règle des 5** stipule : *si vous devez déclarer explicitement l'une de ces cinq opérations, vous devez probablement les déclarer toutes les cinq.*

---

## Pourquoi ces cinq opérations sont liées

Le lien entre ces opérations est la **gestion de ressources**. Si votre classe possède une ressource brute (mémoire, fichier, connexion), alors :

- Le **destructeur** doit la libérer.  
- Le **constructeur de copie** doit la dupliquer (copie profonde).  
- L'**opérateur d'affectation par copie** doit libérer l'ancienne ressource et dupliquer la nouvelle.  
- Le **constructeur de déplacement** doit transférer la propriété.  
- L'**opérateur d'affectation par déplacement** doit libérer l'ancienne ressource et transférer la nouvelle.

Si vous écrivez un destructeur personnalisé, c'est que votre classe gère une ressource. Si elle gère une ressource, la copie et le déplacement implicites (qui font une copie superficielle membre à membre) sont presque certainement incorrects. Vous devez donc les redéfinir — ou les interdire.

Illustrons avec l'état actuel de `DynArray`. Nous avons un destructeur, un constructeur de copie et un constructeur de déplacement. Mais les **opérateurs d'affectation** sont encore implicites :

```cpp
DynArray a(3, 42);    // a = [42, 42, 42]  
DynArray b(5, 0);     // b = [0, 0, 0, 0, 0]  

b = a;                // Opérateur d'affectation par copie IMPLICITE
                      // → Copie superficielle : b.data_ = a.data_
                      // → L'ancien bloc de b fuit (5 int perdus)
                      // → a.data_ et b.data_ pointent vers le même bloc
                      // → Double free à la destruction
```

Le compilateur a généré un opérateur d'affectation par copie implicite qui copie chaque membre. Pour `data_`, il copie le pointeur — pas les données. C'est exactement le même bug que le constructeur de copie implicite (section 6.2.3), mais sur un objet **déjà construit**, ce qui ajoute un problème supplémentaire : la fuite de l'ancienne ressource.

---

## Construction vs affectation : la différence clé

Une confusion fréquente chez les débutants est la distinction entre **construction** et **affectation**. Le symbole `=` peut signifier l'un ou l'autre selon le contexte :

```cpp
DynArray a(3, 42);

DynArray b = a;        // CONSTRUCTION — b n'existe pas encore → constructeur de copie  
DynArray c(a);         // CONSTRUCTION — syntaxe alternative, même effet  

DynArray d(5, 0);  
d = a;                 // AFFECTATION — d existe déjà → opérateur d'affectation par copie  
```

La règle est simple : si l'objet est **créé** par l'instruction, c'est une construction (le constructeur de copie ou de déplacement est appelé). Si l'objet **existe déjà** et reçoit une nouvelle valeur, c'est une affectation (l'opérateur `operator=` est appelé).

L'affectation est plus complexe que la construction parce qu'elle doit gérer **deux états** : l'état actuel de l'objet (qu'il faut nettoyer) et la nouvelle valeur (qu'il faut intégrer). Le constructeur ne gère qu'un seul état : le nouveau.

---

## La matrice de génération implicite

Le compilateur suit des règles précises pour décider quelles opérations il génère implicitement. Voici la matrice complète — le tableau de référence que nous avons annoncé en section 6.2 :

| Vous déclarez… | Destructeur | Copie ctor | Copie `=` | Move ctor | Move `=` |  
|---|---|---|---|---|---|  
| **Rien** | ✅ généré | ✅ généré | ✅ généré | ✅ généré | ✅ généré |  
| **Destructeur** | — | ✅ généré ⚠️ | ✅ généré ⚠️ | ❌ supprimé | ❌ supprimé |  
| **Copie ctor** | ✅ généré | — | ✅ généré ⚠️ | ❌ supprimé | ❌ supprimé |  
| **Copie `=`** | ✅ généré | ✅ généré ⚠️ | — | ❌ supprimé | ❌ supprimé |  
| **Move ctor** | ✅ généré | ❌ supprimé | ❌ supprimé | — | ❌ supprimé |  
| **Move `=`** | ✅ généré | ✅ généré ⚠️ | ✅ généré ⚠️ | ❌ supprimé | — |

**Légende :** ✅ = généré implicitement. ❌ = non généré (l'utiliser provoque une erreur de compilation). ⚠️ = généré mais **déprécié** par le standard — ce comportement est conservé pour la compatibilité avec du code ancien, mais le comité C++ le déconseille et pourrait le supprimer dans une future version.

Les cases ⚠️ sont les plus dangereuses. Par exemple, si vous déclarez un destructeur, le compilateur génère encore le constructeur de copie et l'opérateur d'affectation par copie — mais cette génération est **dépréciée** depuis C++11. Le compilateur le fait par compatibilité avec du code C++98, pas parce que c'est correct. Dans la pratique, si vous avez un destructeur personnalisé, la copie implicite est presque toujours un bug.

### Les deux règles à retenir

Si la matrice est trop dense pour être mémorisée d'un coup, retenez ces deux règles :

**Règle 1** — Déclarer une opération de **déplacement** (constructeur ou affectation) supprime **toutes les opérations de copie** et l'autre opération de déplacement. Le compilateur considère que si vous avez personnalisé le déplacement, la copie par défaut est probablement incorrecte.

**Règle 2** — Déclarer un **destructeur** ou une opération de **copie** supprime les opérations de **déplacement**, mais conserve (de manière dépréciée) les opérations de copie. C'est le cas le plus piégeux : le code compile, mais le comportement est probablement faux.

---

## La Règle des 5 en pratique

La Règle des 5 se décline en trois scénarios concrets :

### Scénario 1 : Votre classe gère une ressource — définissez les cinq

C'est le cas de `DynArray`, `FileDescriptor`, `DatabaseConnection` — toute classe qui possède un pointeur brut, un handle système, ou une ressource acquise manuellement.

```cpp
class ResourceOwner {  
public:  
    ResourceOwner();                                          // Constructeur
    ~ResourceOwner();                                         // 1. Destructeur
    ResourceOwner(const ResourceOwner& other);                // 2. Copie ctor
    ResourceOwner& operator=(const ResourceOwner& other);     // 3. Copie =
    ResourceOwner(ResourceOwner&& other) noexcept;            // 4. Move ctor
    ResourceOwner& operator=(ResourceOwner&& other) noexcept; // 5. Move =
};
```

### Scénario 2 : Votre classe ne doit pas être copiée — supprimez la copie, définissez le déplacement

C'est le cas de `UniqueFile`, `std::unique_ptr`, `std::thread` — les ressources à propriété exclusive non duplicable.

```cpp
class MoveOnly {  
public:  
    MoveOnly();
    ~MoveOnly();
    MoveOnly(const MoveOnly&) = delete;                // Copie interdite
    MoveOnly& operator=(const MoveOnly&) = delete;     // Copie interdite
    MoveOnly(MoveOnly&& other) noexcept;               // Déplacement autorisé
    MoveOnly& operator=(MoveOnly&& other) noexcept;    // Déplacement autorisé
};
```

### Scénario 3 : Votre classe ne possède aucune ressource brute — ne déclarez rien

C'est le cas le plus fréquent en C++ moderne, et il a un nom : la **Règle du Zéro**.

```cpp
class ModernClass {
    // Aucune opération spéciale déclarée — le compilateur les génère toutes
    // et elles sont toutes correctes, parce que chaque membre sait se copier,
    // se déplacer et se détruire correctement.
private:
    std::string name_;
    std::vector<int> data_;
    std::unique_ptr<Config> config_;   // unique_ptr gère la mémoire
};
```

---

## La Règle du Zéro : l'objectif ultime

La Règle du Zéro dit : *une classe ne devrait pas avoir besoin de déclarer explicitement un destructeur, un constructeur de copie, un opérateur d'affectation par copie, un constructeur de déplacement, ou un opérateur d'affectation par déplacement.*

Comment y parvenir ? En construisant vos classes à partir de **membres qui gèrent eux-mêmes leurs ressources** : `std::string` au lieu de `char*`, `std::vector` au lieu de `new[]`/`delete[]`, `std::unique_ptr` au lieu de `new`/`delete`, `std::fstream` au lieu de `open()`/`close()`.

```cpp
// Règle des 5 — nécessaire parce que data_ est un pointeur brut
class OldStyleArray {  
public:  
    explicit OldStyleArray(std::size_t n) : data_(new int[n]{}), size_(n) {}
    ~OldStyleArray() { delete[] data_; }
    OldStyleArray(const OldStyleArray& o);
    OldStyleArray& operator=(const OldStyleArray& o);
    OldStyleArray(OldStyleArray&& o) noexcept;
    OldStyleArray& operator=(OldStyleArray&& o) noexcept;
private:
    int* data_;
    std::size_t size_;
};

// Règle du Zéro — vector gère tout
class ModernArray {  
public:  
    explicit ModernArray(std::size_t n) : data_(n) {}
    // Aucune opération spéciale — vector fait le travail
private:
    std::vector<int> data_;
};
```

Les deux classes offrent les mêmes fonctionnalités. La première exige six fonctions spéciales, chacune pouvant contenir un bug. La seconde n'en a aucune — et pourtant la copie, le déplacement et la destruction fonctionnent parfaitement parce que `std::vector` s'en charge.

La Règle du Zéro est l'**objectif** vers lequel tendre. La Règle des 5 est le **filet de sécurité** quand vous n'y parvenez pas — quand vous devez interfacer une API C, manipuler un handle système, ou implémenter un conteneur custom. Dans votre propre code métier, visez toujours la Règle du Zéro.

> 💡 Notre `DynArray` suit volontairement la Règle des 5 à des fins pédagogiques. En production, vous utiliseriez `std::vector<int>`, et votre classe suivrait la Règle du Zéro. L'objectif de ce chapitre est de comprendre *ce que `std::vector` fait sous le capot* pour que vous sachiez quand et comment intervenir.

---

## `= default` et `= delete` : exprimer l'intention

Le C++11 fournit deux mots-clés qui rendent la Règle des 5 explicite et lisible :

**`= default`** — demande au compilateur de générer l'implémentation par défaut. Utile pour documenter que vous avez *considéré* cette opération et que le comportement par défaut vous convient :

```cpp
class Explicit {  
public:  
    ~Explicit() = default;
    Explicit(const Explicit&) = default;
    Explicit& operator=(const Explicit&) = default;
    Explicit(Explicit&&) noexcept = default;
    Explicit& operator=(Explicit&&) noexcept = default;
};
```

**`= delete`** — supprime l'opération. Toute tentative de l'utiliser produit une erreur de compilation claire :

```cpp
class NonCopyable {  
public:  
    NonCopyable() = default;
    ~NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) noexcept = default;
    NonCopyable& operator=(NonCopyable&&) noexcept = default;
};
```

La bonne pratique est d'être **explicite sur les cinq opérations** dès que vous en déclarez une. Même si le compilateur génère certaines par défaut, écrire `= default` signale au lecteur : "j'ai réfléchi à cette opération, et le comportement par défaut est correct." C'est une forme de documentation vérifiée par le compilateur.

---

## Plan des sous-sections

Chaque sous-section qui suit détaille l'une des cinq opérations dans le contexte de `DynArray`. Les opérations déjà abordées (destructeur en 6.3, constructeur de copie en 6.2.3, constructeur de déplacement en 6.2.4) seront reprises sous l'angle de la Règle des 5 — en particulier pour vérifier leur cohérence mutuelle. Les deux opérations nouvelles (affectation par copie et affectation par déplacement) seront développées en détail.

| Sous-section | Opération | Statut dans `DynArray` |  
|---|---|---|  
| **6.5.1** | Destructeur | Déjà implémenté (6.3) — relecture |  
| **6.5.2** | Constructeur de copie | Déjà implémenté (6.2.3) — relecture |  
| **6.5.3** | Opérateur d'affectation par copie | **Nouveau** — à implémenter |  
| **6.5.4** | Constructeur de déplacement | Déjà implémenté (6.2.4) — relecture |  
| **6.5.5** | Opérateur d'affectation par déplacement | **Nouveau** — à implémenter |

À la fin de la section 6.5.5, `DynArray` sera une classe **complète et correcte**, conforme à la Règle des 5. Chaque opération spéciale sera en place, et l'invariant sera protégé quels que soient les chemins d'exécution.

---


⏭️ [Destructeur](/06-classes-encapsulation/05.1-destructeur.md)
