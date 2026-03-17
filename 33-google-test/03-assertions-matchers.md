🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 33.3 Assertions et matchers

## Vue d'ensemble

Les assertions sont le cœur de tout framework de test : elles expriment les attentes du développeur et produisent les verdicts pass/fail. Google Test propose une bibliothèque d'assertions à la fois riche et extensible, organisée autour de deux principes complémentaires.

Le premier est la distinction **EXPECT vs ASSERT**, introduite en section 33.2. Pour rappel : `EXPECT_*` enregistre l'échec mais poursuit l'exécution du test, tandis que `ASSERT_*` interrompt immédiatement le test en cours. Chaque assertion présentée dans cette section existe dans les deux variantes — il suffit de remplacer le préfixe. La règle reste la même : préférez `EXPECT_*` par défaut, réservez `ASSERT_*` aux vérifications dont dépend la validité du reste du test.

Le second est la distinction entre **assertions natives** (intégrées à GTest) et **matchers** (fournis par Google Mock). Les assertions natives couvrent les comparaisons courantes avec une syntaxe concise. Les matchers offrent un système composable et extensible qui permet d'exprimer des conditions arbitrairement complexes. Les deux systèmes se combinent via la macro `EXPECT_THAT` / `ASSERT_THAT`.

## Assertions booléennes

Les plus élémentaires — elles vérifient une condition vraie ou fausse :

```cpp
EXPECT_TRUE(connection.is_alive());  
EXPECT_FALSE(buffer.empty());  
```

Quand l'assertion échoue, le message par défaut est minimal (`Value of: connection.is_alive() — Actual: false — Expected: true`). Pour des diagnostics plus utiles, ajoutez un message personnalisé via `<<` :

```cpp
EXPECT_TRUE(connection.is_alive())
    << "La connexion au serveur " << server_addr << " a été perdue";
```

Ce pattern de message personnalisé fonctionne avec **toutes** les assertions GTest. Il est particulièrement précieux dans les tests paramétrés (section 33.2.3) et dans les boucles, où le contexte d'échec n'est pas évident sans aide.

## Assertions de comparaison

Ces assertions comparent deux valeurs et produisent un message détaillé indiquant les valeurs attendue et obtenue :

```cpp
// Égalité et inégalité
EXPECT_EQ(result, 42);          // result == 42  
EXPECT_NE(result, 0);           // result != 0  

// Comparaisons ordonnées
EXPECT_LT(latency_ms, 100);    // latency_ms < 100  
EXPECT_LE(retry_count, 3);     // retry_count <= 3  
EXPECT_GT(file_size, 0);       // file_size > 0  
EXPECT_GE(capacity, needed);   // capacity >= needed  
```

### Message d'échec typique

Quand `EXPECT_EQ(result, 42)` échoue, GTest produit :

```
Expected equality of these values:
  result
    Which is: 17
  42
```

Ce message montre les deux valeurs sans ambiguïté. C'est un avantage majeur sur un simple `EXPECT_TRUE(result == 42)` qui n'afficherait que `Value of: result == 42 — Actual: false` — obligeant le développeur à relancer le test sous debugger pour connaître la valeur de `result`.

### Ordre des arguments

GTest ne prescrit pas d'ordre strict entre valeur attendue et valeur obtenue — contrairement à JUnit ou pytest où la convention `(expected, actual)` est normalisée. Cependant, pour la cohérence, cette formation adopte la convention `EXPECT_EQ(actual, expected)` — la valeur calculée en premier, la valeur attendue en second. Choisissez une convention et tenez-vous-y dans tout le projet.

### Comparaison de pointeurs

`EXPECT_EQ` compare les valeurs pointées uniquement pour les types qui surchargent `operator==`. Pour les pointeurs bruts, il compare les **adresses** :

```cpp
int* p = &x;  
int* q = &x;  
EXPECT_EQ(p, q);      // ✅ Même adresse  

int a = 5, b = 5;  
EXPECT_EQ(&a, &b);    // ❌ Adresses différentes, même si valeurs égales  
```

Pour vérifier qu'un pointeur est null ou non :

```cpp
EXPECT_EQ(ptr, nullptr);     // Vérifie que ptr est null  
EXPECT_NE(ptr, nullptr);     // Vérifie que ptr n'est pas null  
```

## Assertions sur les chaînes de caractères

GTest fournit des assertions dédiées aux chaînes C (`const char*`) qui gèrent correctement la comparaison par contenu plutôt que par adresse :

```cpp
const char* msg = get_error_message();

// Comparaison de contenu (sensible à la casse)
EXPECT_STREQ(msg, "File not found");  
EXPECT_STRNE(msg, "Success");  

// Comparaison insensible à la casse
EXPECT_STRCASEEQ(msg, "file NOT FOUND");  
EXPECT_STRCASENE(msg, "success");  // Échouerait si msg == "SUCCESS"  
```

Pour les `std::string`, ces assertions ne sont pas nécessaires — `EXPECT_EQ` appelle directement `operator==` de `std::string`, qui compare le contenu :

```cpp
std::string name = get_username();  
EXPECT_EQ(name, "Alice");  // Fonctionne correctement avec std::string  
```

Les assertions `STREQ` restent indispensables quand vous travaillez avec des API C, des buffers `char*` ou de l'interopérabilité (section 43.1) où les chaînes sont des pointeurs bruts.

## Assertions sur les flottants

La comparaison de nombres à virgule flottante est un piège classique. L'arithmétique IEEE 754 introduit des erreurs d'arrondi qui rendent la comparaison exacte (`EXPECT_EQ`) peu fiable :

```cpp
double result = 0.1 + 0.2;  
EXPECT_EQ(result, 0.3);    // ❌ Échoue probablement (0.30000000000000004 ≠ 0.3)  
```

GTest fournit trois assertions spécifiques qui gèrent ce problème de manière rigoureuse.

### EXPECT_FLOAT_EQ / EXPECT_DOUBLE_EQ

Ces assertions tolèrent une différence de **4 ULP** (*Units in the Last Place*) — la plus petite distance représentable entre deux flottants adjacents à cette magnitude. C'est une tolérance calibrée pour absorber les erreurs d'arrondi typiques d'une ou deux opérations arithmétiques :

```cpp
EXPECT_FLOAT_EQ(static_cast<float>(result), 0.3f);  
EXPECT_DOUBLE_EQ(result, 0.3);  
```

Utilisez `EXPECT_FLOAT_EQ` pour les `float` et `EXPECT_DOUBLE_EQ` pour les `double`. Mélanger les types (comparer un `float` avec `EXPECT_DOUBLE_EQ`) produit des résultats imprévisibles à cause des conversions implicites.

### EXPECT_NEAR

Quand les erreurs d'arrondi dépassent 4 ULP — typiquement après des chaînes de calculs, des fonctions trigonométriques ou des algorithmes itératifs — `EXPECT_NEAR` permet de spécifier une tolérance absolue explicite :

```cpp
double angle = compute_angle(sensor_data);  
EXPECT_NEAR(angle, 45.0, 0.01);  // Tolérance de ±0.01 degré  

double pi_approx = estimate_pi(1'000'000);  
EXPECT_NEAR(pi_approx, M_PI, 1e-5);  // Précision à 5 décimales  
```

Le troisième argument est la tolérance maximale absolue (`|actual - expected| ≤ tolerance`). En cas d'échec, le message indique les deux valeurs et l'écart constaté.

### Choisir la bonne assertion flottante

| Situation | Assertion | Justification |
|-----------|-----------|---------------|
| Résultat d'une opération simple (+, -, ×) | `EXPECT_DOUBLE_EQ` | Erreur d'arrondi ≤ 4 ULP |
| Chaîne de calculs, itérations | `EXPECT_NEAR` | Erreur cumulée, tolérance explicite |
| Calcul dont on connaît la précision | `EXPECT_NEAR` | Tolérance calibrée au domaine |
| Valeur exactement représentable (0.0, 0.5, puissances de 2) | `EXPECT_EQ` | Aucune erreur d'arrondi possible |

## Assertions sur les exceptions

Présentées brièvement en section 33.2.1, les assertions d'exception méritent un traitement complet.

```cpp
// Vérifie qu'une exception du type spécifié est levée
EXPECT_THROW(parser.parse("}{"), mp::ParseError);

// Vérifie qu'une exception (n'importe quel type) est levée
EXPECT_ANY_THROW(dangerousOperation());

// Vérifie qu'aucune exception n'est levée
EXPECT_NO_THROW(safeOperation());
```

### Subtilités de EXPECT_THROW

`EXPECT_THROW` vérifie le type exact de l'exception **ou un type dérivé**. C'est un `catch` polymorphique en interne. Si votre code lève une `mp::FileParseError` qui hérite de `mp::ParseError`, les deux assertions suivantes réussissent :

```cpp
EXPECT_THROW(parser.parse_file("bad.txt"), mp::FileParseError);  // Type exact  
EXPECT_THROW(parser.parse_file("bad.txt"), mp::ParseError);       // Type parent  
EXPECT_THROW(parser.parse_file("bad.txt"), std::exception);        // Ancêtre commun  
```

Plus le type spécifié est précis, plus le test est discriminant. Vérifier `std::exception` prouve seulement qu'une exception a été levée, pas que c'est la bonne.

### Inspecter le contenu d'une exception

Pour vérifier le message ou les attributs de l'exception, le pattern `try/catch` + `FAIL()` reste nécessaire (voir section 33.2.1). Cependant, avec les matchers Google Mock, il existe une alternative plus concise via `EXPECT_THAT` et le matcher `Throws` (disponible depuis GTest 1.11) :

```cpp
using ::testing::ThrowsMessage;  
using ::testing::HasSubstr;  

EXPECT_THAT(
    [&]() { parser.parse("}{"); },
    ThrowsMessage<mp::ParseError>(HasSubstr("unexpected token"))
);
```

Ce style sera détaillé dans la section matchers ci-dessous.

## Assertions d'échec explicites

Deux macros permettent de forcer un résultat de test sans condition :

```cpp
FAIL() << "Ce point ne devrait jamais être atteint";
// Fatal : arrête le test immédiatement

SUCCEED() << "Cas de test validé manuellement";
// Non fatal : marque un succès explicite
```

`FAIL()` est surtout utile dans les branches de contrôle qui ne devraient pas être empruntées :

```cpp
TEST(StateMachine, NeverReachesInvalidState) {
    auto state = machine.advance();
    switch (state) {
        case State::Idle:     EXPECT_TRUE(machine.can_start()); break;
        case State::Running:  EXPECT_TRUE(machine.can_stop());  break;
        case State::Stopped:  EXPECT_TRUE(machine.can_reset()); break;
        default:
            FAIL() << "État inattendu : " << static_cast<int>(state);
    }
}
```

## Les matchers Google Mock

Les assertions natives de GTest couvrent les cas courants, mais atteignent leurs limites quand les vérifications deviennent complexes : vérifier qu'une chaîne contient un sous-texte, qu'un conteneur contient certains éléments, ou qu'une valeur satisfait une combinaison de conditions. C'est là qu'interviennent les **matchers** de Google Mock.

Les matchers s'utilisent via la macro `EXPECT_THAT` (ou `ASSERT_THAT`) :

```cpp
#include <gmock/gmock.h>

EXPECT_THAT(value, matcher);
```

Le message d'échec décrit automatiquement ce qui était attendu et ce qui a été obtenu, en termes lisibles.

### Matchers sur les scalaires

```cpp
using namespace ::testing;

EXPECT_THAT(status_code, Eq(200));           // == 200 (équivalent EXPECT_EQ)  
EXPECT_THAT(retry_count, Ne(0));             // != 0  
EXPECT_THAT(latency, Lt(100));               // < 100  
EXPECT_THAT(latency, Le(100));               // <= 100  
EXPECT_THAT(score, Gt(0));                   // > 0  
EXPECT_THAT(score, Ge(passing_score));       // >= passing_score  
```

Ces matchers semblent redondants avec `EXPECT_EQ`, `EXPECT_LT`, etc. Leur intérêt apparaît quand ils sont **composés** avec les opérateurs logiques (voir plus bas) ou intégrés dans des matchers de conteneurs.

### Matchers sur les chaînes

```cpp
using namespace ::testing;

std::string msg = get_log_line();

EXPECT_THAT(msg, HasSubstr("ERROR"));           // Contient "ERROR"  
EXPECT_THAT(msg, StartsWith("[2026-"));          // Commence par "[2026-"  
EXPECT_THAT(msg, EndsWith("\n"));                // Se termine par un saut de ligne  
EXPECT_THAT(msg, MatchesRegex("\\[[0-9]{4}-[0-9]{2}-[0-9]{2}\\].*"));  // Regex complète  
EXPECT_THAT(msg, ContainsRegex("timeout after [0-9]+ms"));              // Regex partielle  

// Comparaison insensible à la casse
EXPECT_THAT(msg, StrCaseEq("error: file not found"));
```

> 💡 **Regex POSIX.** `ContainsRegex` et `MatchesRegex` utilisent les regex POSIX extended, pas ECMAScript. La classe `\d` n'existe pas en POSIX — utilisez `[0-9]` ou `[[:digit:]]` à la place. De même, `\w` se remplace par `[[:alnum:]_]`.

Les matchers de chaînes sont l'un des points forts de Google Mock. `HasSubstr` à lui seul remplace de nombreux patterns manuels et produit des messages d'échec bien plus utiles qu'un `EXPECT_TRUE(msg.find("ERROR") != std::string::npos)`.

### Matchers sur les conteneurs

C'est dans la vérification des conteneurs que les matchers révèlent toute leur puissance :

```cpp
using namespace ::testing;

std::vector<int> results = compute_primes(20);

// Vérification du contenu exact (ordre compris)
EXPECT_THAT(results, ElementsAre(2, 3, 5, 7, 11, 13, 17, 19));

// Contenu exact, sans contrainte d'ordre
EXPECT_THAT(results, UnorderedElementsAre(19, 17, 13, 11, 7, 5, 3, 2));

// Contient au moins ces éléments
EXPECT_THAT(results, IsSupersetOf({2, 7, 13}));

// Chaque élément satisfait un matcher
EXPECT_THAT(results, Each(Gt(0)));        // Tous positifs  
EXPECT_THAT(results, Each(Lt(20)));       // Tous < 20  

// Contient au moins un élément qui satisfait le matcher
EXPECT_THAT(results, Contains(7));  
EXPECT_THAT(results, Contains(Gt(15)));   // Au moins un > 15  

// Taille du conteneur
EXPECT_THAT(results, SizeIs(8));  
EXPECT_THAT(results, Not(IsEmpty()));  
```

#### ElementsAre vs UnorderedElementsAre

`ElementsAre` vérifie à la fois le contenu **et l'ordre**. C'est la forme la plus stricte. Chaque argument peut être une valeur littérale ou un matcher :

```cpp
EXPECT_THAT(names, ElementsAre(
    StartsWith("A"),
    "Bob",
    HasSubstr("lar")   // Matcher au lieu d'une valeur exacte
));
```

`UnorderedElementsAre` vérifie le contenu sans contrainte d'ordre — il cherche un appariement bijectif entre les éléments du conteneur et les matchers fournis. C'est l'outil idéal pour les résultats dont l'ordre n'est pas garanti (résultats de requêtes, parcours de maps non ordonnées).

### Matchers sur les paires et les maps

```cpp
using namespace ::testing;

std::map<std::string, int> scores = {{"Alice", 95}, {"Bob", 82}};

// Vérification d'une paire clé-valeur
EXPECT_THAT(scores, Contains(Pair("Alice", 95)));

// Clé présente, valeur satisfaisant un matcher
EXPECT_THAT(scores, Contains(Pair("Bob", Gt(80))));

// Toutes les valeurs > 70
EXPECT_THAT(scores, Each(Pair(_, Gt(70))));  // _ = Wildcard (n'importe quelle clé)

// Vérification de la clé seulement
EXPECT_THAT(scores, Contains(Key("Alice")));
```

Le wildcard `_` (underscore) est un matcher qui accepte n'importe quelle valeur. Combiné avec `Pair`, il permet de vérifier uniquement la clé ou uniquement la valeur.

### Matchers flottants

Les matchers offrent des alternatives composables aux assertions `EXPECT_DOUBLE_EQ` et `EXPECT_NEAR` :

```cpp
using namespace ::testing;

EXPECT_THAT(result, DoubleEq(3.14));          // Tolérance 4 ULP  
EXPECT_THAT(result, FloatEq(3.14f));          // Tolérance 4 ULP (float)  
EXPECT_THAT(result, DoubleNear(3.14, 0.01));  // Tolérance absolue  
EXPECT_THAT(result, FloatNear(3.14f, 0.01f));  

// Composable avec des matchers de conteneurs
std::vector<double> measurements = get_sensor_data();  
EXPECT_THAT(measurements, Each(DoubleNear(25.0, 0.5)));  
```

L'intérêt des matchers flottants sur les assertions classiques apparaît dans la dernière ligne : vérifier que **chaque élément** d'un vecteur est proche d'une valeur cible, en une seule expression.

## Composition de matchers

Les matchers se composent avec des opérateurs logiques, ce qui permet d'exprimer des conditions complexes en une seule assertion lisible.

### Not : négation

```cpp
EXPECT_THAT(status, Not(Eq(0)));  
EXPECT_THAT(name, Not(IsEmpty()));  
EXPECT_THAT(results, Not(Contains(42)));  
```

### AllOf : conjonction (ET logique)

```cpp
EXPECT_THAT(password, AllOf(
    SizeIs(Ge(8)),             // Au moins 8 caractères
    HasSubstr("@"),            // Contient un @... (exemple simplifié)
    Not(HasSubstr(" "))        // Pas d'espaces
));
```

Quand `AllOf` échoue, le message indique **quel** sous-matcher a échoué, ce qui est bien plus informatif qu'un simple `EXPECT_TRUE(password.size() >= 8 && password.find("@") != npos && ...)`.

### AnyOf : disjonction (OU logique)

```cpp
EXPECT_THAT(response.status(), AnyOf(200, 201, 204));

EXPECT_THAT(error_msg, AnyOf(
    HasSubstr("timeout"),
    HasSubstr("connection refused"),
    HasSubstr("unreachable")
));
```

### Compositions imbriquées

Les matchers se composent à l'infini. Voici un exemple réaliste qui vérifie la structure d'une réponse HTTP parsée :

```cpp
using namespace ::testing;

auto response = client.get("/api/users");

EXPECT_THAT(response.headers(), Contains(Pair("Content-Type", HasSubstr("json"))));

EXPECT_THAT(response.json_body()["users"], AllOf(
    SizeIs(Gt(0)),
    Each(AllOf(
        Contains(Pair("id",   Not(IsEmpty()))),
        Contains(Pair("name", Not(IsEmpty()))),
        Contains(Pair("age",  Gt(0)))
    ))
));
```

Ce test unique vérifie que la réponse contient un header JSON, que le tableau `users` n'est pas vide, et que chaque utilisateur a un `id`, un `name` non vides et un `age` positif. L'expressivité des matchers composés remplace ici ce qui serait une dizaine de lignes d'assertions séparées.

## EXPECT_THAT vs assertions classiques

La question se pose naturellement : quand utiliser `EXPECT_THAT` avec un matcher plutôt qu'une assertion native ?

**Préférez les assertions classiques** quand la comparaison est simple et directe. `EXPECT_EQ(x, 42)` est plus concis et plus lisible que `EXPECT_THAT(x, Eq(42))`. Les assertions classiques restent le premier choix pour les cas élémentaires.

**Préférez EXPECT_THAT** dans les situations suivantes :

- Vérification de chaînes partielles (`HasSubstr`, `StartsWith`, `MatchesRegex`).
- Vérification de conteneurs (`ElementsAre`, `Contains`, `Each`, `SizeIs`).
- Conditions composées (`AllOf`, `AnyOf`, `Not`).
- Vérification de structures imbriquées (matchers de `Pair`, `Field`, `Property`).
- Tout cas où le message d'échec par défaut de `EXPECT_TRUE` serait insuffisant.

En pratique, les fichiers de test matures combinent naturellement les deux : assertions classiques pour les vérifications scalaires, matchers pour les vérifications structurelles.

## Créer un matcher personnalisé

Quand les matchers intégrés ne couvrent pas votre besoin, GTest offre la macro `MATCHER` pour en créer de nouveaux :

```cpp
// Matcher simple sans paramètre
MATCHER(IsPositive, "is a positive number") {
    return arg > 0;
    // 'arg' est la valeur testée, fournie automatiquement par la macro
}

EXPECT_THAT(balance, IsPositive());
```

### Matcher avec paramètre

```cpp
MATCHER_P(IsBetween, low, high,
    "is between " + ::testing::PrintToString(low) +
    " and " + ::testing::PrintToString(high)) 
{
    return arg >= low && arg <= high;
}

EXPECT_THAT(temperature, IsBetween(36.0, 37.5));
```

> ⚠️ **Note syntaxique.** `MATCHER_P` prend un paramètre, `MATCHER_P2` en prend deux. La macro `MATCHER_P` ci-dessus reçoit `low` et `high` comme paramètres de template — le `high` est en réalité le deuxième, d'où l'utilisation de `MATCHER_P2` dans la vraie syntaxe GTest. L'exemple simplifié ci-dessus illustre le principe. Consultez la documentation GTest pour la syntaxe exacte avec `MATCHER_P2`.

### Matcher composable

Un matcher personnalisé se compose exactement comme les matchers intégrés :

```cpp
EXPECT_THAT(readings, Each(AllOf(IsPositive(), Lt(100.0))));  
EXPECT_THAT(patient.temperature(), AllOf(IsPositive(), IsBetween(35.0, 42.0)));  
```

Cette composabilité est le vrai pouvoir du système de matchers : chaque matcher est un composant réutilisable qui s'assemble avec les autres pour exprimer des invariants complexes de manière déclarative.

## Référence rapide

### Assertions classiques (les plus courantes)

| Assertion | Vérifie |
|-----------|---------|
| `EXPECT_TRUE(cond)` | `cond` est vrai |
| `EXPECT_FALSE(cond)` | `cond` est faux |
| `EXPECT_EQ(a, b)` | `a == b` |
| `EXPECT_NE(a, b)` | `a != b` |
| `EXPECT_LT(a, b)` | `a < b` |
| `EXPECT_LE(a, b)` | `a <= b` |
| `EXPECT_GT(a, b)` | `a > b` |
| `EXPECT_GE(a, b)` | `a >= b` |
| `EXPECT_FLOAT_EQ(a, b)` | `|a - b| ≤ 4 ULP` (float) |
| `EXPECT_DOUBLE_EQ(a, b)` | `|a - b| ≤ 4 ULP` (double) |
| `EXPECT_NEAR(a, b, tol)` | `|a - b| ≤ tol` |
| `EXPECT_STREQ(a, b)` | `strcmp(a, b) == 0` |
| `EXPECT_THROW(expr, type)` | `expr` lève une exception de type `type` |
| `EXPECT_NO_THROW(expr)` | `expr` ne lève aucune exception |

### Matchers Google Mock (les plus courants)

| Matcher | Vérifie |
|---------|---------|
| `Eq(v)`, `Ne(v)`, `Lt(v)`, `Le(v)`, `Gt(v)`, `Ge(v)` | Comparaisons scalaires |
| `IsNull()`, `NotNull()` | Pointeur null / non-null |
| `HasSubstr(s)` | Chaîne contient `s` |
| `StartsWith(s)`, `EndsWith(s)` | Préfixe / suffixe |
| `MatchesRegex(r)`, `ContainsRegex(r)` | Regex complète / partielle |
| `DoubleEq(v)`, `DoubleNear(v, tol)` | Flottants avec tolérance |
| `ElementsAre(...)` | Contenu et ordre exacts |
| `UnorderedElementsAre(...)` | Contenu exact, ordre libre |
| `Contains(m)` | Au moins un élément satisfait `m` |
| `Each(m)` | Tous les éléments satisfont `m` |
| `SizeIs(m)` | Taille du conteneur satisfait `m` |
| `IsEmpty()` | Conteneur vide |
| `Pair(k, v)` | Paire clé-valeur |
| `Key(k)` | Clé d'une paire |
| `AllOf(m1, m2, ...)` | Tous les matchers satisfaits (ET) |
| `AnyOf(m1, m2, ...)` | Au moins un matcher satisfait (OU) |
| `Not(m)` | Négation du matcher |
| `_` | Wildcard — accepte tout |

Toutes ces assertions et matchers remplacent le préfixe `EXPECT` par `ASSERT` pour obtenir la variante fatale.

---


⏭️ [Mocking avec Google Mock](/33-google-test/04-google-mock.md)
