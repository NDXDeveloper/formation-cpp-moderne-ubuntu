🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 33.4 Mocking avec Google Mock

## Le problème de l'isolation

Un test unitaire vérifie le comportement d'une **unité** de code — une fonction, une classe, un module — en isolation. Mais dans un projet réel, les unités collaborent : un service appelle une base de données, un contrôleur invoque un client HTTP, un processeur de commandes envoie des notifications. Tester ces unités directement contre leurs dépendances réelles pose trois problèmes fondamentaux.

**Lenteur.** Un appel réseau, une requête SQL ou une écriture disque transforme un test qui devrait durer quelques microsecondes en un test de plusieurs centaines de millisecondes. Multipliez par des centaines de tests et la suite devient trop lente pour être exécutée à chaque commit.

**Fragilité.** Le test dépend de l'état de l'infrastructure : la base de données est-elle disponible ? Le serveur distant répond-il ? Le fichier de configuration existe-t-il ? Un test qui échoue parce que le réseau est instable ne teste pas votre code — il teste votre réseau.

**Contrôle limité.** Comment tester le comportement de votre code quand la base de données renvoie une erreur de connexion ? Quand le client HTTP reçoit une réponse 503 ? Quand le disque est plein ? Reproduire ces conditions avec de vraies dépendances est difficile voire impossible.

Le **mocking** résout ces trois problèmes en remplaçant les dépendances réelles par des doublures de test — des objets qui simulent le comportement attendu et permettent de vérifier les interactions.

## Google Mock : principes

Google Mock (intégré à GTest depuis la fusion des projets) fournit un framework de mocking pour C++ basé sur les **méthodes virtuelles**. Le principe est le suivant :

1. Votre code de production dépend d'une **interface** (classe abstraite avec méthodes virtuelles pures).
2. En production, l'interface est implémentée par une classe concrète (le vrai client HTTP, le vrai accès base de données).
3. En test, l'interface est implémentée par un **mock** — une classe générée par Google Mock qui enregistre les appels reçus et retourne des valeurs configurées.

Ce modèle s'appuie sur le polymorphisme dynamique (section 7.3) et l'injection de dépendances (section 44.5). Le code de production ne sait pas s'il parle à une vraie implémentation ou à un mock — c'est exactement ce qui le rend testable.

## De l'interface au mock

### Définir l'interface

La première étape — et souvent la plus importante — est de s'assurer que la dépendance est derrière une interface. Prenons un service de notification :

```cpp
// include/mon_projet/notification_service.hpp
#pragma once
#include <string>

namespace mp {

class NotificationService {  
public:  
    virtual ~NotificationService() = default;

    virtual bool send_email(const std::string& to, 
                            const std::string& subject,
                            const std::string& body) = 0;

    virtual bool send_sms(const std::string& phone, 
                          const std::string& message) = 0;

    virtual int pending_count() const = 0;
};

} // namespace mp
```

Trois points importants. Le destructeur est `virtual` — indispensable pour que la destruction via un pointeur de base fonctionne correctement (section 7.2). Toutes les méthodes sont `virtual` et `= 0` — c'est une interface pure. Et la classe ne contient aucune donnée membre — elle ne définit qu'un contrat.

### Le code à tester

Le code de production reçoit l'interface par injection de dépendance, typiquement via le constructeur :

```cpp
// include/mon_projet/order_processor.hpp
#pragma once
#include "mon_projet/notification_service.hpp"
#include <memory>

namespace mp {

class OrderProcessor {  
public:  
    explicit OrderProcessor(std::shared_ptr<NotificationService> notifier);

    bool process_order(const std::string& order_id, 
                       const std::string& customer_email);

    bool cancel_order(const std::string& order_id,
                      const std::string& customer_email,
                      const std::string& reason);

private:
    std::shared_ptr<NotificationService> notifier_;
};

} // namespace mp
```

`OrderProcessor` ne sait pas si `notifier_` est un vrai service d'email ou un mock — il n'utilise que l'interface. Cette conception est la clé de la testabilité.

### Créer le mock

Google Mock fournit la macro `MOCK_METHOD` pour déclarer des méthodes mockées :

```cpp
// tests/mocks/mock_notification_service.hpp
#pragma once
#include <gmock/gmock.h>
#include "mon_projet/notification_service.hpp"

namespace mp::testing {

class MockNotificationService : public NotificationService {  
public:  
    MOCK_METHOD(bool, send_email,
                (const std::string& to, 
                 const std::string& subject,
                 const std::string& body),
                (override));

    MOCK_METHOD(bool, send_sms,
                (const std::string& phone, 
                 const std::string& message),
                (override));

    MOCK_METHOD(int, pending_count, (), (const, override));
};

} // namespace mp::testing
```

La syntaxe de `MOCK_METHOD` suit un schéma fixe :

```cpp
MOCK_METHOD(ReturnType, MethodName, (Parameters...), (Qualifiers...));
```

Le quatrième argument contient les qualificateurs de la méthode : `override`, `const`, `noexcept`, séparés par des virgules. `override` n'est pas techniquement obligatoire mais fortement recommandé — il garantit à la compilation que la signature du mock correspond bien à celle de l'interface. Si vous modifiez l'interface et oubliez de mettre à jour le mock, `override` produira une erreur de compilation au lieu d'un bug silencieux.

### Ancienne syntaxe (MOCK_METHODn)

Avant GTest 1.10, les mocks utilisaient des macros numérotées selon le nombre de paramètres :

```cpp
// ❌ Ancienne syntaxe — fonctionnelle mais obsolète
MOCK_METHOD3(send_email, bool(const std::string&, 
                               const std::string&, 
                               const std::string&));
```

Cette syntaxe fonctionne toujours mais n'est plus recommandée. La macro unifiée `MOCK_METHOD` est plus lisible et gère naturellement les qualificateurs `const`, `noexcept` et `override`. Si vous rencontrez l'ancienne syntaxe dans du code existant, la migration est mécanique.

## EXPECT_CALL : définir les attentes

Le cœur de Google Mock est la macro `EXPECT_CALL`, qui définit ce que le mock **s'attend à recevoir** comme appels. Une `EXPECT_CALL` se décompose en quatre parties :

```cpp
EXPECT_CALL(mock_object, method_name(matchers...))
    .Times(cardinality)         // Combien de fois
    .WillOnce(action)           // Que faire au 1er appel
    .WillRepeatedly(action);    // Que faire aux appels suivants
```

### Premier test avec EXPECT_CALL

```cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mon_projet/order_processor.hpp"
#include "mocks/mock_notification_service.hpp"

using namespace ::testing;

class OrderProcessorTest : public ::testing::Test {  
protected:  
    std::shared_ptr<mp::testing::MockNotificationService> mock_notifier =
        std::make_shared<mp::testing::MockNotificationService>();

    mp::OrderProcessor processor{mock_notifier};
};

TEST_F(OrderProcessorTest, SendsConfirmationEmailOnSuccess) {
    EXPECT_CALL(*mock_notifier, send_email(
            "alice@example.com",         // Argument exact attendu
            HasSubstr("Confirmation"),   // Matcher sur le sujet
            _                            // Wildcard : n'importe quel body
        ))
        .Times(1)
        .WillOnce(Return(true));         // Simule un envoi réussi

    bool result = processor.process_order("ORD-001", "alice@example.com");
    EXPECT_TRUE(result);
}
```

Décomposons chaque élément.

**`EXPECT_CALL(*mock_notifier, send_email(...))`** — Déclare qu'on s'attend à ce que `send_email` soit appelé sur le mock. Le déréférencement `*mock_notifier` est nécessaire car le mock est derrière un `shared_ptr`.

**Les arguments** — Chaque argument peut être une valeur exacte (`"alice@example.com"`) ou un matcher (`HasSubstr(...)`, `_`). C'est ici que les matchers de la section 33.3 prennent tout leur sens : ils permettent de vérifier les arguments passés à la dépendance sans être trop rigide.

**`.Times(1)`** — L'appel doit se produire exactement une fois. Si `send_email` est appelé zéro fois ou plus d'une fois, le test échoue.

**`.WillOnce(Return(true))`** — Quand l'appel se produit, retourner `true`. C'est l'**action** — elle détermine ce que le mock fait quand il est invoqué.

**Vérification automatique.** À la destruction du mock (fin du test), Google Mock vérifie automatiquement que toutes les `EXPECT_CALL` ont été satisfaites. Si `send_email` n'a jamais été appelé, le test échoue avec un message explicite. Cette vérification automatique est une différence fondamentale avec un simple *stub* (qui retourne des valeurs sans vérifier les appels).

## Matchers d'arguments

Les matchers d'arguments dans `EXPECT_CALL` utilisent exactement les mêmes matchers que `EXPECT_THAT` (section 33.3). Les plus courants dans le contexte du mocking :

```cpp
// Valeur exacte
EXPECT_CALL(mock, method("exact_value"));

// Wildcard : n'importe quelle valeur
EXPECT_CALL(mock, method(_));

// Comparaisons
EXPECT_CALL(mock, method(Gt(0)));  
EXPECT_CALL(mock, method(Le(100)));  

// Chaînes
EXPECT_CALL(mock, method(HasSubstr("error")));  
EXPECT_CALL(mock, method(StartsWith("/api/")));  

// Compositions
EXPECT_CALL(mock, method(AllOf(Gt(0), Lt(100))));  
EXPECT_CALL(mock, method(AnyOf("active", "pending")));  

// Tous les arguments wildcards (raccourci)
EXPECT_CALL(mock, method(_, _, _));
```

La granularité des matchers est un choix de conception. Des matchers trop stricts (valeurs exactes partout) rendent les tests fragiles — la moindre modification du format d'un message casse le test. Des matchers trop lâches (wildcards partout) ne vérifient rien d'utile. Le bon équilibre consiste à vérifier les arguments qui représentent le **contrat** de l'interaction et à ignorer les détails d'implémentation.

## Cardinalité : Times()

`Times()` contrôle le nombre d'appels attendus :

```cpp
EXPECT_CALL(mock, method(_)).Times(1);           // Exactement 1 fois  
EXPECT_CALL(mock, method(_)).Times(3);           // Exactement 3 fois  
EXPECT_CALL(mock, method(_)).Times(0);           // Jamais appelé  
EXPECT_CALL(mock, method(_)).Times(AtLeast(1));  // Au moins 1 fois  
EXPECT_CALL(mock, method(_)).Times(AtMost(5));   // Au plus 5 fois  
EXPECT_CALL(mock, method(_)).Times(Between(2, 4)); // Entre 2 et 4 fois  
EXPECT_CALL(mock, method(_)).Times(AnyNumber()); // N'importe combien  
```

### Cardinalité implicite

Si vous omettez `Times()`, Google Mock infère la cardinalité :

- Avec `.WillOnce(...)` seul → `Times(1)`
- Avec `.WillOnce(...).WillOnce(...)` (deux WillOnce) → `Times(2)`
- Avec `.WillRepeatedly(...)` seul → `Times(AtLeast(0))`
- Avec `.WillOnce(...).WillRepeatedly(...)` → `Times(AtLeast(1))`

En pratique, l'inférence implicite est souvent suffisante. Un `Times()` explicite est préférable quand la cardinalité fait partie du comportement vérifié — par exemple, vérifier qu'un cache évite un second appel réseau.

## Actions : WillOnce et WillRepeatedly

Les actions déterminent ce que le mock fait quand il reçoit un appel. Les plus courantes :

### Return : retourner une valeur

```cpp
EXPECT_CALL(mock, get_status()).WillOnce(Return(200));  
EXPECT_CALL(mock, get_name()).WillRepeatedly(Return("Alice"));  
```

### ReturnRef : retourner une référence

```cpp
std::string cached_name = "Bob";  
EXPECT_CALL(mock, name()).WillRepeatedly(ReturnRef(cached_name));  
```

> ⚠️ L'objet référencé doit survivre à l'appel. Retourner une référence sur une variable locale du test est un comportement indéfini si le mock est invoqué après la fin du scope.

### ReturnArg : retourner un des arguments

```cpp
// Retourne le premier argument tel quel
EXPECT_CALL(mock, transform(_, _)).WillRepeatedly(ReturnArg<0>());
```

### DoAll : enchaîner plusieurs actions

```cpp
EXPECT_CALL(mock, save(_, _))
    .WillOnce(DoAll(
        SaveArg<1>(&captured_value),  // Capture le 2e argument
        Return(true)
    ));
```

`DoAll` exécute toutes les actions dans l'ordre et retourne le résultat de la dernière. C'est l'outil pour combiner des effets de bord (capture, logging) avec une valeur de retour.

### Invoke : appeler une fonction personnalisée

```cpp
EXPECT_CALL(mock, compute(_, _))
    .WillRepeatedly(Invoke([](int a, int b) { return a + b; }));
```

`Invoke` accepte n'importe quel callable — lambda, pointeur de fonction, `std::function`. C'est l'action la plus flexible, utilisée quand la logique du mock dépasse un simple `Return`.

### Throw : lever une exception

```cpp
EXPECT_CALL(mock, connect(_))
    .WillOnce(Throw(std::runtime_error("Connection refused")));
```

C'est l'outil idéal pour tester les chemins d'erreur : simuler un timeout, une déconnexion ou une réponse invalide sans avoir besoin de l'infrastructure réelle.

### Séquence d'actions

`WillOnce` et `WillRepeatedly` se combinent pour simuler des comportements qui évoluent au fil des appels :

```cpp
EXPECT_CALL(mock, connect(_))
    .WillOnce(Throw(std::runtime_error("Timeout")))   // 1er appel : échec
    .WillOnce(Throw(std::runtime_error("Timeout")))   // 2e appel : échec
    .WillOnce(Return(true));                           // 3e appel : succès
```

Ce pattern est parfait pour tester une logique de retry : vérifier que le code réessaie après un échec et finit par réussir.

## ON_CALL : comportement par défaut

`EXPECT_CALL` combine deux responsabilités : configurer le comportement **et** vérifier que l'appel a lieu. Parfois, vous voulez configurer un comportement sans imposer de contrainte sur les appels — typiquement pour les dépendances auxiliaires qui ne sont pas le sujet du test.

`ON_CALL` remplit ce rôle :

```cpp
TEST_F(OrderProcessorTest, CancelSendsEmailWithReason) {
    // ON_CALL : configure un comportement par défaut sans vérification
    ON_CALL(*mock_notifier, pending_count())
        .WillByDefault(Return(0));

    // EXPECT_CALL : c'est CE comportement qu'on vérifie
    EXPECT_CALL(*mock_notifier, send_email(
            "alice@example.com",
            HasSubstr("Annulation"),
            HasSubstr("rupture de stock")
        ))
        .WillOnce(Return(true));

    processor.cancel_order("ORD-001", "alice@example.com", "rupture de stock");
}
```

`ON_CALL` sur `pending_count()` dit : "si quelqu'un appelle cette méthode, retourne 0, mais je ne me soucie pas de savoir si elle est appelée ou combien de fois". Le test se concentre exclusivement sur l'interaction avec `send_email`.

La distinction `ON_CALL` / `EXPECT_CALL` est importante pour la maintenabilité. Un test qui pose des `EXPECT_CALL` sur chaque méthode du mock est fragile : la moindre modification du code de production (un appel de vérification supplémentaire, un log ajouté) casse le test. Réservez `EXPECT_CALL` aux interactions qui font partie du **contrat** du comportement testé, et utilisez `ON_CALL` pour tout le reste.

## Vérification de l'ordre des appels

Par défaut, Google Mock ne vérifie pas l'ordre des appels entre différentes `EXPECT_CALL`. Pour imposer un séquencement, utilisez `InSequence` :

```cpp
TEST_F(OrderProcessorTest, SendsEmailBeforeSms) {
    InSequence seq;  // Tout ce qui suit doit se produire dans l'ordre

    EXPECT_CALL(*mock_notifier, send_email(_, _, _))
        .WillOnce(Return(true));

    EXPECT_CALL(*mock_notifier, send_sms(_, _))
        .WillOnce(Return(true));

    processor.process_order("ORD-001", "alice@example.com");
}
```

Si `send_sms` est appelé avant `send_email`, le test échoue. `InSequence` est un objet dont la portée (scope) délimite la séquence — les `EXPECT_CALL` déclarées pendant sa durée de vie sont ordonnées.

### Séquences partielles

Pour des contraintes d'ordre plus fines entre des appels spécifiques sans imposer un ordre global :

```cpp
TEST_F(WorkflowTest, ValidateBeforeCommit) {
    Sequence validation_seq;

    EXPECT_CALL(mock_validator, validate(_))
        .InSequence(validation_seq)
        .WillOnce(Return(true));

    EXPECT_CALL(mock_db, commit(_))
        .InSequence(validation_seq)
        .WillOnce(Return(true));

    // Ces appels peuvent se produire n'importe quand
    EXPECT_CALL(mock_logger, log(_)).Times(AnyNumber());

    workflow.execute(data);
}
```

`validate` doit être appelé avant `commit`, mais `log` peut intervenir à n'importe quel moment. Les séquences nommées (`Sequence`) offrent un contrôle granulaire que `InSequence` global ne permet pas.

## Mocking de méthodes non-virtuelles

Google Mock repose fondamentalement sur le polymorphisme dynamique — les méthodes mockées doivent être `virtual`. C'est une contrainte qui reflète la conception du C++, mais qui pose problème quand le code existant n'utilise pas d'interfaces.

### Solution 1 : extraire une interface

La solution la plus propre et la plus recommandée est d'extraire une interface de la classe concrète :

```cpp
// Avant : classe concrète sans interface
class HttpClient {  
public:  
    std::string get(const std::string& url);
    std::string post(const std::string& url, const std::string& body);
};

// Après : interface + implémentation
class HttpClientInterface {  
public:  
    virtual ~HttpClientInterface() = default;
    virtual std::string get(const std::string& url) = 0;
    virtual std::string post(const std::string& url, 
                              const std::string& body) = 0;
};

class HttpClient : public HttpClientInterface {  
public:  
    std::string get(const std::string& url) override;
    std::string post(const std::string& url, 
                      const std::string& body) override;
};
```

Ce refactoring améliore la testabilité **et** la conception — le code de production dépend désormais d'une abstraction plutôt que d'une implémentation concrète (Dependency Inversion Principle, section 44.5).

### Solution 2 : template sur le type de dépendance

Quand l'extraction d'interface est impossible ou indésirable (coût du polymorphisme dynamique en hot path, code legacy), le mocking par template offre une alternative :

```cpp
// Le code de production est templatisé sur le type du client
template<typename HttpClient>  
class ApiService {  
public:  
    explicit ApiService(HttpClient& client) : client_(client) {}

    std::string fetch_user(int id) {
        return client_.get("/api/users/" + std::to_string(id));
    }

private:
    HttpClient& client_;
};
```

Le mock n'a pas besoin d'hériter d'une interface — il doit simplement fournir les mêmes méthodes :

```cpp
class MockHttpClient {  
public:  
    MOCK_METHOD(std::string, get, (const std::string& url));
    MOCK_METHOD(std::string, post, 
                (const std::string& url, const std::string& body));
};

TEST(ApiServiceTest, FetchUserCallsCorrectEndpoint) {
    MockHttpClient mock;
    ApiService<MockHttpClient> service{mock};

    EXPECT_CALL(mock, get("/api/users/42"))
        .WillOnce(Return(R"({"name":"Alice"})"));

    auto result = service.fetch_user(42);
    EXPECT_THAT(result, HasSubstr("Alice"));
}
```

Cette approche est connue sous le nom de *compile-time polymorphism* ou *duck typing* à la C++. Elle évite le coût de la vtable mais rend le code de production templated, ce qui augmente les temps de compilation et peut compliquer la lisibilité.

## Exemple intégré : tester un processeur de commandes

Rassemblons les concepts dans un test complet et réaliste qui vérifie le comportement de `OrderProcessor` dans plusieurs scénarios :

```cpp
using namespace ::testing;

class OrderProcessorTest : public ::testing::Test {  
protected:  
    std::shared_ptr<mp::testing::MockNotificationService> mock_notifier =
        std::make_shared<mp::testing::MockNotificationService>();

    mp::OrderProcessor processor{mock_notifier};
};

TEST_F(OrderProcessorTest, SuccessfulOrderSendsConfirmation) {
    EXPECT_CALL(*mock_notifier, send_email(
            "bob@example.com",
            HasSubstr("Confirmation"),
            AllOf(HasSubstr("ORD-042"), HasSubstr("Bob"))
        ))
        .WillOnce(Return(true));

    EXPECT_TRUE(processor.process_order("ORD-042", "bob@example.com"));
}

TEST_F(OrderProcessorTest, FailedEmailDoesNotCrashProcessing) {
    EXPECT_CALL(*mock_notifier, send_email(_, _, _))
        .WillOnce(Return(false));  // L'email échoue

    // Le processeur doit gérer l'échec gracieusement
    EXPECT_FALSE(processor.process_order("ORD-043", "bob@example.com"));
}

TEST_F(OrderProcessorTest, EmailServiceThrowingIsHandled) {
    EXPECT_CALL(*mock_notifier, send_email(_, _, _))
        .WillOnce(Throw(std::runtime_error("SMTP timeout")));

    // Le processeur ne doit pas propager l'exception
    EXPECT_NO_THROW(processor.process_order("ORD-044", "bob@example.com"));
}

TEST_F(OrderProcessorTest, CancellationIncludesReason) {
    EXPECT_CALL(*mock_notifier, send_email(
            _,
            HasSubstr("Annulation"),
            HasSubstr("article indisponible")
        ))
        .WillOnce(Return(true));

    processor.cancel_order("ORD-045", "bob@example.com", "article indisponible");
}
```

Chaque test vérifie un aspect distinct du contrat entre `OrderProcessor` et `NotificationService` : l'envoi nominal, la gestion d'un échec de retour, la gestion d'une exception, et le contenu du message d'annulation. Le mock permet de simuler chaque scénario en une ligne d'action, sans aucune infrastructure réseau.

## Nice Mocks, Strict Mocks et Naggy Mocks

Par défaut, Google Mock est **naggy** : si une méthode du mock est appelée sans `EXPECT_CALL` correspondante, le framework émet un **avertissement** (warning) mais ne fait pas échouer le test. Ce comportement par défaut est un compromis qui peut être ajusté.

### NiceMock : silencieux

```cpp
auto mock = std::make_shared<NiceMock<mp::testing::MockNotificationService>>();
```

`NiceMock` supprime les avertissements pour les appels inattendus. Le mock retourne silencieusement la valeur par défaut du type de retour (`false` pour `bool`, `0` pour `int`, `""` pour `std::string`). C'est utile quand le mock a beaucoup de méthodes mais que le test ne s'intéresse qu'à une ou deux d'entre elles.

### StrictMock : intolérant

```cpp
auto mock = std::make_shared<StrictMock<mp::testing::MockNotificationService>>();
```

`StrictMock` fait **échouer le test** pour tout appel inattendu — c'est-à-dire tout appel sans `EXPECT_CALL` correspondante. C'est le mode le plus rigoureux, utile pour vérifier qu'aucune interaction parasite ne se produit.

Le risque de `StrictMock` est la fragilité : ajouter un appel de log dans le code de production casse tous les tests qui utilisent un `StrictMock` sur cette interface. Réservez-le aux cas où le nombre exact d'interactions fait partie du contrat.

### Recommandation

Pour la plupart des cas, le mock par défaut (naggy) ou `NiceMock` offre le meilleur rapport signal/bruit. Combinez-le avec des `EXPECT_CALL` ciblées sur les interactions importantes et des `ON_CALL` pour les comportements par défaut. N'utilisez `StrictMock` que quand l'absence d'appels parasites est une propriété que vous voulez explicitement vérifier.

## Bonnes pratiques du mocking

### Ne mockez que ce que vous possédez

Mocker directement une API tierce (Boost.Asio, libcurl, la STL) est fragile et crée un couplage fort avec les détails d'implémentation de cette bibliothèque. Créez plutôt une couche d'abstraction fine (un *wrapper* ou *adapter*) autour de la bibliothèque, et mockez cette abstraction :

```cpp
// ❌ Mocker directement une API tierce
class MockCurlHandle { ... };  // Couplé aux internals de curl

// ✅ Abstraire puis mocker
class HttpClientInterface { ... };       // Votre abstraction  
class CurlHttpClient : public HttpClientInterface { ... };  // Production  
class MockHttpClient : public HttpClientInterface { ... };   // Test  
```

### Gardez les mocks simples

Un mock dont les `EXPECT_CALL` s'étalent sur vingt lignes avec des matchers imbriqués, des séquences et des actions complexes est un signal d'alarme. Il indique souvent que le code testé fait trop de choses ou que le test vérifie des détails d'implémentation plutôt qu'un comportement. Un bon test mock tient en quelques `EXPECT_CALL` claires.

### Préférez les stubs aux mocks quand c'est suffisant

Si vous n'avez pas besoin de vérifier qu'une méthode a été appelée mais simplement de fournir un comportement, un stub (via `ON_CALL` ou une implémentation manuelle simple) est plus léger et moins fragile qu'un mock avec `EXPECT_CALL`. La distinction conceptuelle est la suivante :

- **Stub** — fournit des réponses prédéfinies. Ne vérifie pas les appels. Rend le test possible.
- **Mock** — fournit des réponses prédéfinies **et** vérifie les interactions. Rend le test assertif sur les collaborations.

Utilisez un mock quand l'interaction elle-même est le comportement que vous testez ("le processeur doit envoyer un email"). Utilisez un stub quand la dépendance est un détail de support ("le repository retourne des données, mais je teste la logique de calcul").

### Attention à la sur-spécification

Le piège le plus courant du mocking est de vérifier **comment** le code fait les choses plutôt que **ce qu'il** fait. Un test qui vérifie l'ordre exact de trois appels internes, les arguments précis de chaque appel et le nombre exact d'invocations est un test qui casse à chaque refactoring — même quand le comportement observable reste correct. Testez le contrat, pas l'implémentation.

---


⏭️ [Test-Driven Development (TDD) en C++](/33-google-test/05-tdd.md)
