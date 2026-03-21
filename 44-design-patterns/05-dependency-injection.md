🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 44.5 — Dependency Injection en C++

## Découplage et testabilité sans framework lourd

---

## Définition et intention

La Dependency Injection (DI) est un principe de conception où un objet **reçoit ses dépendances de l'extérieur** au lieu de les créer lui-même. C'est l'application directe du *Dependency Inversion Principle* (le "D" de SOLID) : un module de haut niveau ne dépend pas d'un module de bas niveau — les deux dépendent d'une abstraction.

En Java ou C#, la DI est synonyme de frameworks lourds (Spring, Autofac) avec des conteneurs d'injection, de l'introspection et du XML de configuration. En C++, la réalité est très différente. Le langage ne dispose pas de réflexion à l'exécution (avant C++26, cf. section 12.14.2), et la culture C++ privilégie le contrôle explicite et le zéro-overhead. La DI en C++ est donc **une pratique de conception**, pas un framework. Elle s'appuie sur les mécanismes natifs du langage : constructeurs, templates, concepts et — dans les cas les plus simples — le passage de paramètres.

---

## Le problème : dépendances câblées

Considérons un service qui envoie des notifications. Dans une implémentation naïve, les dépendances sont créées en interne :

```cpp
// ❌ Dépendances câblées — non testable, non configurable
class NotificationService {  
public:  
    void notify(std::string_view user_id, std::string_view message) {
        // Dépendance câblée vers une implémentation concrète
        auto db = PostgresDatabase("host=db.prod port=5432");
        auto user = db.find_user(user_id);

        // Dépendance câblée vers un service externe
        SmtpMailer mailer("smtp.prod.internal", 587);
        mailer.send(user.email, "Notification", message);

        // Dépendance câblée vers le logger global
        Logger::instance().log(
            std::format("Notified {} ({})", user_id, user.email)
        );
    }
};
```

Ce code est fonctionnel mais présente trois problèmes structurels.

**Non testable.** Un test unitaire de `notify()` déclenche une vraie connexion à PostgreSQL et envoie un vrai email. Impossible de tester la logique sans infrastructure réelle.

**Non configurable.** Changer de base de données (PostgreSQL → SQLite pour les tests, DynamoDB en production) ou de canal de notification (email → Slack → SMS) exige de modifier le code source de `NotificationService`.

**Couplage fort.** `NotificationService` connaît les types concrets `PostgresDatabase` et `SmtpMailer`, leurs constructeurs, leurs paramètres de connexion. Tout changement dans ces classes propage une modification ici.

---

## Les trois formes d'injection

La DI en C++ prend trois formes, par ordre de préférence.

### 1. Constructor Injection (recommandée)

Les dépendances sont passées au **constructeur**. L'objet est pleinement opérationnel dès sa construction — pas d'état intermédiaire invalide :

```cpp
class NotificationService {  
public:  
    NotificationService(
        std::shared_ptr<UserRepository> users,
        std::shared_ptr<Mailer>         mailer,
        std::shared_ptr<Logger>         logger
    )
        : users_(std::move(users))
        , mailer_(std::move(mailer))
        , logger_(std::move(logger))
    {}

    void notify(std::string_view user_id, std::string_view message) {
        auto user = users_->find(user_id);
        mailer_->send(user.email, "Notification", message);
        logger_->log(std::format("Notified {} ({})", user_id, user.email));
    }

private:
    std::shared_ptr<UserRepository> users_;
    std::shared_ptr<Mailer>         mailer_;
    std::shared_ptr<Logger>         logger_;
};
```

**Avantages** : les dépendances sont explicites dans la signature du constructeur. L'objet est immutable après construction (les dépendances ne changent plus). Le compilateur garantit que toutes les dépendances sont fournies.

### 2. Setter Injection (cas particuliers)

Les dépendances sont assignées après construction via des setters. Utile quand une dépendance est **optionnelle** ou **changeable** à l'exécution :

```cpp
class Pipeline {  
public:  
    void set_compressor(std::unique_ptr<Compressor> comp) {
        compressor_ = std::move(comp);
    }

    void set_encryptor(std::unique_ptr<Encryptor> enc) {
        encryptor_ = std::move(enc);
    }

    void process(std::vector<std::byte>& data) {
        if (compressor_) compressor_->compress(data);
        if (encryptor_)  encryptor_->encrypt(data);
    }

private:
    std::unique_ptr<Compressor> compressor_;
    std::unique_ptr<Encryptor>  encryptor_;
};
```

**Inconvénient** : l'objet peut exister dans un état partiellement configuré. Il faut gérer les cas où une dépendance n'a pas été fournie (`nullptr` checks, valeurs par défaut).

### 3. Interface Injection (rare en C++)

L'objet implémente une interface d'injection. Le conteneur appelle cette interface pour fournir les dépendances. Ce style est courant en Java (annotations `@Inject`) mais rare en C++ car il ajoute de la complexité sans bénéfice clair dans un langage sans réflexion. Nous ne le couvrons pas ici.

---

## DI avec interfaces virtuelles

La forme la plus classique utilise des **classes abstraites** comme interfaces. Les types concrets en héritent, et le code client ne dépend que de l'abstraction :

```cpp
// ─── Interfaces (abstractions) ───

class UserRepository {  
public:  
    virtual ~UserRepository() = default;
    virtual User find(std::string_view id) = 0;
    virtual void save(const User& user) = 0;
};

class Mailer {  
public:  
    virtual ~Mailer() = default;
    virtual void send(std::string_view to,
                      std::string_view subject,
                      std::string_view body) = 0;
};
```

```cpp
// ─── Implémentations de production ───

class PostgresUserRepository : public UserRepository {
    pqxx::connection conn_;
public:
    explicit PostgresUserRepository(std::string_view connstr)
        : conn_(std::string(connstr)) {}

    User find(std::string_view id) override {
        // Query PostgreSQL...
    }

    void save(const User& user) override {
        // Insert/Update PostgreSQL...
    }
};

class SmtpMailer : public Mailer {
    std::string host_;
    int port_;
public:
    SmtpMailer(std::string host, int port)
        : host_(std::move(host)), port_(port) {}

    void send(std::string_view to,
              std::string_view subject,
              std::string_view body) override {
        // Envoi SMTP réel...
    }
};
```

```cpp
// ─── Assemblage dans main() ───

int main() {
    auto users  = std::make_shared<PostgresUserRepository>(
        "host=db.prod port=5432 dbname=app"
    );
    auto mailer = std::make_shared<SmtpMailer>("smtp.prod.internal", 587);
    auto logger = std::make_shared<FileLogger>("/var/log/app.log");

    auto service = NotificationService(users, mailer, logger);
    service.notify("user-42", "Votre commande est expédiée.");
}
```

### Testabilité : mocks et stubs

L'intérêt principal se révèle dans les tests. Les implémentations de production sont remplacées par des **mocks** :

```cpp
// ─── Mocks pour les tests ───

class MockUserRepository : public UserRepository {  
public:  
    User find(std::string_view id) override {
        return User{.id = std::string(id), .email = "test@example.com"};
    }

    void save(const User& user) override {
        saved_users.push_back(user);
    }

    std::vector<User> saved_users;
};

class MockMailer : public Mailer {  
public:  
    void send(std::string_view to,
              std::string_view subject,
              std::string_view body) override {
        sent_emails.push_back({
            std::string(to),
            std::string(subject),
            std::string(body)
        });
    }

    struct Email { std::string to, subject, body; };
    std::vector<Email> sent_emails;
};
```

```cpp
// ─── Test unitaire ───

void test_notify_sends_email() {
    auto mock_users  = std::make_shared<MockUserRepository>();
    auto mock_mailer = std::make_shared<MockMailer>();
    auto mock_logger = std::make_shared<NullLogger>();  // Logger silencieux

    auto service = NotificationService(mock_users, mock_mailer, mock_logger);

    service.notify("user-42", "Hello");

    // Vérifications
    assert(mock_mailer->sent_emails.size() == 1);
    assert(mock_mailer->sent_emails[0].to == "test@example.com");
    assert(mock_mailer->sent_emails[0].body == "Hello");
}
```

Le `NotificationService` est testé **en isolation totale** — pas de base de données, pas de serveur SMTP, pas de fichier de log. La logique métier est vérifiée indépendamment de l'infrastructure.

---

## DI avec templates et concepts : zéro overhead

L'approche par interfaces virtuelles introduit une indirection (vtable) et impose l'allocation heap (les objets polymorphiques sont typiquement manipulés via des smart pointers). Quand la performance est critique ou que les dépendances sont **connues à la compilation**, les templates éliminent ces coûts :

```cpp
template<typename TUsers, typename TMailer, typename TLogger>
    requires requires(TUsers u, std::string_view sv) {
        { u.find(sv) } -> std::convertible_to<User>;
    }
    && requires(TMailer m, std::string_view a, std::string_view b, std::string_view c) {
        m.send(a, b, c);
    }
    && requires(TLogger l, std::string s) {
        l.log(s);
    }
class NotificationService {  
public:  
    NotificationService(TUsers& users, TMailer& mailer, TLogger& logger)
        : users_(users), mailer_(mailer), logger_(logger) {}

    void notify(std::string_view user_id, std::string_view message) {
        auto user = users_.find(user_id);
        mailer_.send(user.email, "Notification", message);
        logger_.log(std::format("Notified {} ({})", user_id, user.email));
    }

private:
    TUsers&  users_;
    TMailer& mailer_;
    TLogger& logger_;
};
```

Les dépendances sont des **références directes** vers les objets concrets — aucune vtable, aucune allocation, aucune indirection. Le compilateur inline les appels si les implémentations sont visibles.

### CTAD pour simplifier l'instanciation

Grâce à la déduction de types du constructeur (CTAD, C++17), l'instanciation est propre :

```cpp
auto users  = PostgresUserRepository("host=db.prod port=5432");  
auto mailer = SmtpMailer("smtp.prod.internal", 587);  
auto logger = FileLogger("/var/log/app.log");  

// CTAD déduit les types template automatiquement
auto service = NotificationService(users, mailer, logger);  
service.notify("user-42", "Votre commande est expédiée.");  
```

### Concepts nommés pour une meilleure lisibilité

Les clauses `requires` inline deviennent vite illisibles avec de nombreuses dépendances. Des concepts nommés clarifient le contrat :

```cpp
template<typename T>  
concept UserRepositoryLike = requires(T repo, std::string_view id, const User& u) {  
    { repo.find(id) } -> std::convertible_to<User>;
    { repo.save(u) } -> std::same_as<void>;
};

template<typename T>  
concept MailerLike = requires(T m, std::string_view a, std::string_view b, std::string_view c) {  
    { m.send(a, b, c) } -> std::same_as<void>;
};

template<typename T>  
concept LoggerLike = requires(T l, std::string msg) {  
    { l.log(std::move(msg)) } -> std::same_as<void>;
};
```

```cpp
template<UserRepositoryLike TUsers, MailerLike TMailer, LoggerLike TLogger>  
class NotificationService {  
    // ... identique
};
```

Le contrat de chaque dépendance est **documenté par le concept**. Un type concret qui ne satisfait pas le concept produit un message d'erreur clair au site d'instanciation.

### Tests avec la DI template

Les mocks n'ont pas besoin d'hériter d'une interface — ils doivent simplement satisfaire le concept :

```cpp
struct MockUsers {
    User find(std::string_view id) {
        return User{.id = std::string(id), .email = "test@test.com"};
    }
    void save(const User&) { /* no-op */ }
};

struct MockMailer {
    struct Email { std::string to, subject, body; };
    std::vector<Email> sent;

    void send(std::string_view to, std::string_view subject, std::string_view body) {
        sent.push_back({std::string(to), std::string(subject), std::string(body)});
    }
};

struct NullLogger {
    void log(std::string) {}
};
```

```cpp
void test_notify() {
    MockUsers  users;
    MockMailer mailer;
    NullLogger logger;

    auto service = NotificationService(users, mailer, logger);
    service.notify("user-42", "Hello");

    assert(mailer.sent.size() == 1);
    assert(mailer.sent[0].to == "test@test.com");
}
```

Les mocks sont des structs minimales, sans héritage, sans virtual, sans overhead. Le concept garantit à la compilation qu'ils satisfont le contrat requis.

---

## DI avec `std::function` : le juste milieu

Entre les interfaces virtuelles (overhead runtime) et les templates (tout résolu à la compilation), `std::function` offre un **compromis pragmatique** : injection dynamique, sans hiérarchie de classes, avec un code simple :

```cpp
class NotificationService {  
public:  
    using FindUser  = std::function<User(std::string_view)>;
    using SendEmail = std::function<void(std::string_view, std::string_view,
                                         std::string_view)>;
    using Log       = std::function<void(std::string)>;

    NotificationService(FindUser find_user, SendEmail send_email, Log log)
        : find_user_(std::move(find_user))
        , send_email_(std::move(send_email))
        , log_(std::move(log))
    {}

    void notify(std::string_view user_id, std::string_view message) {
        auto user = find_user_(user_id);
        send_email_(user.email, "Notification", message);
        log_(std::format("Notified {} ({})", user_id, user.email));
    }

private:
    FindUser  find_user_;
    SendEmail send_email_;
    Log       log_;
};
```

```cpp
// Production
auto service = NotificationService(
    [&db](std::string_view id) { return db.find_user(id); },
    [&mailer](auto to, auto subj, auto body) { mailer.send(to, subj, body); },
    [&logger](std::string msg) { logger.log(std::move(msg)); }
);

// Test
auto service = NotificationService(
    [](std::string_view) { return User{.email = "test@test.com"}; },
    [&sent](auto to, auto, auto body) { sent.emplace_back(to, body); },
    [](std::string) {}  // NullLogger inline
);
```

Cette approche excelle quand les dépendances sont des **fonctions individuelles** plutôt que des objets avec plusieurs méthodes. Elle devient moins lisible quand une dépendance a une interface riche (cinq méthodes ou plus) — dans ce cas, une interface virtuelle ou un template est préférable.

---

## DI avec type erasure : le meilleur des deux mondes

Le type erasure (section 44.4) combine les avantages de la DI template (non-intrusif, pas d'héritage requis) avec ceux de la DI dynamique (sélection à l'exécution, collections hétérogènes) :

```cpp
class AnyUserRepository {  
public:  
    template<UserRepositoryLike T>
    AnyUserRepository(T repo)
        : pimpl_(std::make_unique<Model<T>>(std::move(repo))) {}

    AnyUserRepository(AnyUserRepository&&) noexcept = default;
    AnyUserRepository& operator=(AnyUserRepository&&) noexcept = default;

    User find(std::string_view id) { return pimpl_->find(id); }
    void save(const User& user)    { pimpl_->save(user); }

private:
    struct Concept {
        virtual ~Concept() = default;
        virtual User find(std::string_view id) = 0;
        virtual void save(const User& user) = 0;
    };

    template<typename T>
    struct Model final : Concept {
        T obj_;
        explicit Model(T obj) : obj_(std::move(obj)) {}
        User find(std::string_view id) override { return obj_.find(id); }
        void save(const User& user)    override { obj_.save(user); }
    };

    std::unique_ptr<Concept> pimpl_;
};
```

```cpp
class NotificationService {  
public:  
    NotificationService(AnyUserRepository users, AnyMailer mailer, AnyLogger logger)
        : users_(std::move(users))
        , mailer_(std::move(mailer))
        , logger_(std::move(logger))
    {}

    void notify(std::string_view user_id, std::string_view message) {
        auto user = users_.find(user_id);
        mailer_.send(user.email, "Notification", message);
        logger_.log(std::format("Notified {} ({})", user_id, user.email));
    }

private:
    AnyUserRepository users_;
    AnyMailer         mailer_;
    AnyLogger         logger_;
};
```

L'interface de `NotificationService` est **non-template** (pas de paramètres template à propager partout), les types concrets n'ont **aucun héritage** à implémenter, et la sélection de l'implémentation se fait à **l'exécution** :

```cpp
// Production — PostgresUserRepository ne connaît pas AnyUserRepository
auto service = NotificationService(
    PostgresUserRepository("host=db.prod"),
    SmtpMailer("smtp.prod.internal", 587),
    FileLogger("/var/log/app.log")
);

// Test — MockUsers ne connaît pas AnyUserRepository non plus
auto service = NotificationService(
    MockUsers{},
    MockMailer{},
    NullLogger{}
);
```

---

## Le Composition Root : assembler le graphe de dépendances

Quel que soit le mécanisme d'injection (interfaces, templates, `std::function`, type erasure), les dépendances doivent être **assemblées quelque part**. Ce point d'assemblage est le *Composition Root* — typiquement `main()` ou une fonction d'initialisation dédiée.

### Structure recommandée

```cpp
// ─── composition.h ───
// Fonction d'assemblage séparée de main()

struct AppServices {
    std::shared_ptr<UserRepository>     users;
    std::shared_ptr<Mailer>             mailer;
    std::shared_ptr<Logger>             logger;
    std::unique_ptr<NotificationService> notifications;
    std::unique_ptr<OrderService>        orders;
    std::unique_ptr<HttpServer>          server;
};

AppServices compose_production();  
AppServices compose_testing();  
```

```cpp
// ─── composition.cpp ───

AppServices compose_production() {
    auto config = load_config("/etc/app/config.yaml");

    auto users  = std::make_shared<PostgresUserRepository>(config.db_connstr);
    auto mailer = std::make_shared<SmtpMailer>(config.smtp_host, config.smtp_port);
    auto logger = std::make_shared<SpdLogger>(config.log_level);

    auto notifications = std::make_unique<NotificationService>(users, mailer, logger);
    auto orders        = std::make_unique<OrderService>(users, logger);

    auto server = std::make_unique<HttpServer>(
        HttpServer::Builder()
            .port(config.port)
            .threads(config.threads)
            .build()
    );

    return {
        std::move(users),
        std::move(mailer),
        std::move(logger),
        std::move(notifications),
        std::move(orders),
        std::move(server)
    };
}

AppServices compose_testing() {
    auto users  = std::make_shared<InMemoryUserRepository>();
    auto mailer = std::make_shared<MockMailer>();
    auto logger = std::make_shared<NullLogger>();

    auto notifications = std::make_unique<NotificationService>(users, mailer, logger);
    auto orders        = std::make_unique<OrderService>(users, logger);
    auto server        = std::make_unique<HttpServer>(/* config test */);

    return { /* ... */ };
}
```

```cpp
// ─── main.cpp ───

int main(int argc, char* argv[]) {
    auto app = compose_production();
    app.server->start();
}
```

### Principes du Composition Root

**Centralisation.** Tout l'assemblage est en un seul endroit. Le reste du code ne crée jamais ses propres dépendances — il les reçoit.

**Séparation configuration/logique.** La logique métier (`NotificationService`) ne sait pas quelle base de données elle utilise. Le Composition Root est le seul point qui connaît les types concrets et les paramètres de connexion.

**Interchangeabilité.** `compose_production()` et `compose_testing()` assemblent le même graphe de services avec des implémentations différentes. Ajouter un `compose_staging()` ou `compose_docker()` est trivial.

---

## Comparaison des approches d'injection

| Approche | Overhead | Testabilité | Intrusion | Complexité | Cas d'usage |
|---|---|---|---|---|---|
| **Interfaces virtuelles** | vtable + heap | ✅ Mocks via héritage | Oui (héritage) | Faible | DI classique, code applicatif |
| **Templates + concepts** | Zéro | ✅ Mocks structurels | Non | Moyenne (templates) | Bibliothèques, code perf-critique |
| **`std::function`** | Indirection | ✅ Lambdas | Non | Faible | Dépendances fonctionnelles simples |
| **Type erasure** | Indirection + heap | ✅ Mocks structurels | Non | Élevée (boilerplate) | Interface non-template + types ouverts |

### Guide de décision rapide

**Le choix par défaut** pour un projet applicatif est l'injection par **interfaces virtuelles** + `std::shared_ptr`. C'est simple, bien compris par les équipes, compatible avec Google Mock (section 33.4), et le coût de la vtable est négligeable dans du code applicatif.

**Si la dépendance est une seule fonction** (callback, stratégie, policy), `std::function` est plus léger qu'une interface complète.

**Si la performance est critique** (boucle chaude, bibliothèque haute fréquence), les **templates + concepts** éliminent toute indirection. Le coût est une propagation des paramètres template dans le code client.

**Si les types concrets ne doivent pas hériter** (bibliothèques tierces, types valeurs) et que la résolution doit être dynamique, le **type erasure** est la solution.

---

## Anti-patterns

### Injection de tout

Injecter une dépendance qui n'a aucune raison d'être substituée est du surengineering. Un `std::vector` ou un `std::string` ne sont pas des dépendances injectables — ce sont des détails d'implémentation. N'injecter que les collaborateurs à **frontière d'effet de bord** : base de données, réseau, système de fichiers, horloge, générateur aléatoire.

```cpp
// ❌ Over-injection — std::allocator n'a pas besoin d'être injecté
class Service {
    std::shared_ptr<Allocator> alloc_;
    std::shared_ptr<StringFormatter> fmt_;  // Un std::format suffit
    // ...
};

// ✅ Injecter uniquement les frontières d'effet de bord
class Service {
    std::shared_ptr<Database> db_;      // I/O externe
    std::shared_ptr<HttpClient> http_;  // I/O externe
    std::shared_ptr<Clock> clock_;      // Temps testable
};
```

### Conteneur de services comme God Object

Un conteneur qui expose **tous** les services de l'application crée un couplage implicite massif. Chaque composant peut accéder à n'importe quel service, ce qui rend les dépendances opaques :

```cpp
// ❌ Service Locator — dépendances cachées
class ServiceContainer {
    std::unordered_map<std::string, std::any> services_;
public:
    template<typename T>
    T& get(const std::string& name) {
        return std::any_cast<T&>(services_.at(name));
    }
};

class OrderService {
    ServiceContainer& container_;
public:
    void process() {
        // Quelles sont les vraies dépendances ? Impossible à savoir
        // sans lire tout le corps de la méthode
        auto& db = container_.get<Database>("db");
        auto& mailer = container_.get<Mailer>("mailer");
        auto& cache = container_.get<Cache>("cache");
    }
};
```

```cpp
// ✅ Injection explicite — dépendances visibles dans le constructeur
class OrderService {  
public:  
    OrderService(
        std::shared_ptr<Database> db,
        std::shared_ptr<Mailer> mailer
    ) : db_(std::move(db)), mailer_(std::move(mailer)) {}
};
```

Le Service Locator (anti-pattern) rend les dépendances **implicites**. L'injection par constructeur les rend **explicites**. La signature du constructeur est la documentation vivante des dépendances d'un composant.

### Graphe circulaire de dépendances

Si A dépend de B et B dépend de A, c'est un signal de mauvaise conception. La DI ne résout pas les cycles — elle les rend visibles. La solution est de **casser le cycle** en extrayant l'interface partagée dans un troisième composant, ou en utilisant un `std::weak_ptr` si une dépendance bidirectionnelle est véritablement inévitable.

### Trop de paramètres au constructeur

Si un constructeur reçoit six, sept, huit dépendances, la classe a probablement trop de responsabilités. C'est un signal pour la découper en composants plus petits et plus focalisés. Règle empirique : au-delà de **quatre dépendances injectées**, questionner le périmètre de la classe.

---

## Injection de l'horloge : un cas d'école

L'horloge système (`std::chrono::system_clock::now()`) est une dépendance implicite fréquente et particulièrement gênante en test : les résultats changent à chaque exécution. L'injection d'une abstraction d'horloge est un cas d'école de la DI bien appliquée :

```cpp
template<typename T>  
concept ClockLike = requires(T c) {  
    { c.now() } -> std::convertible_to<std::chrono::system_clock::time_point>;
};

// Horloge réelle
struct SystemClock {
    auto now() const { return std::chrono::system_clock::now(); }
};

// Horloge de test — temps contrôlé manuellement
struct FakeClock {
    std::chrono::system_clock::time_point current;

    auto now() const { return current; }

    void advance(std::chrono::seconds delta) {
        current += delta;
    }
};
```

```cpp
template<ClockLike TClock = SystemClock>  
class TokenValidator {  
    TClock clock_;
    std::chrono::seconds ttl_;
public:
    explicit TokenValidator(std::chrono::seconds ttl, TClock clock = {})
        : clock_(std::move(clock)), ttl_(ttl) {}

    bool is_valid(const Token& token) const {
        auto age = clock_.now() - token.issued_at;
        return age < ttl_;
    }
};
```

```cpp
// Production — utilise l'horloge système par défaut
auto validator = TokenValidator(std::chrono::hours(24));

// Test — temps contrôlé, résultats déterministes
void test_token_expiration() {
    FakeClock clock{.current = std::chrono::system_clock::now()};
    auto validator = TokenValidator(std::chrono::seconds(60), clock);

    auto token = Token{.issued_at = clock.now()};
    assert(validator.is_valid(token));    // Juste créé → valide

    clock.advance(std::chrono::seconds(61));
    assert(!validator.is_valid(token));   // 61s plus tard → expiré
}
```

Le paramètre template a une **valeur par défaut** (`SystemClock`) : le code de production n'a pas besoin de spécifier l'horloge. Seuls les tests passent une `FakeClock`. Le coût runtime en production est **zéro** — `SystemClock::now()` est inliné.

---

## Points clés à retenir

- La DI en C++ est une **pratique de conception**, pas un framework. Elle s'appuie sur les mécanismes natifs : constructeurs, templates, concepts, `std::function`.  
- **L'injection par constructeur** est le choix par défaut. Les dépendances sont explicites, l'objet est immutable après construction, le compilateur garantit la complétude.  
- **Interfaces virtuelles + `shared_ptr`** sont la forme la plus simple pour du code applicatif. **Templates + concepts** offrent le zéro-overhead pour le code performance-critique. **`std::function`** convient aux dépendances fonctionnelles simples. **Le type erasure** est non-intrusif et dynamique.  
- Le **Composition Root** (`main()` ou fonction dédiée) est le seul endroit qui connaît les types concrets et assemble le graphe de dépendances. Le reste du code ne dépend que d'abstractions.  
- N'injecter que les collaborateurs aux **frontières d'effet de bord** (I/O, réseau, horloge, aléatoire). Ne pas injecter les détails d'implémentation.  
- Le **Service Locator** (conteneur de services global) est un anti-pattern : il rend les dépendances implicites. L'injection par constructeur les rend explicites.  
- Au-delà de **quatre dépendances** injectées, questionner le périmètre de la classe — c'est probablement un signal de responsabilités excessives.

⏭️ [Sécurité en C++](/45-securite/README.md)
