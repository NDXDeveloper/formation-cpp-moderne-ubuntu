🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 12.8 std::expected (C++23) : Gestion d'erreurs sans exceptions ⭐

## Un résultat ou une erreur — jamais les deux

La gestion d'erreurs est l'un des sujets les plus débattus en C++. Les exceptions, mécanisme officiel du langage depuis C++98, sont puissantes mais posent des problèmes réels dans certains contextes : surcoût en taille de binaire, latence imprévisible dans le chemin d'erreur, incompatibilité avec les environnements embarqués ou temps réel, et difficulté à raisonner sur les flux d'exécution quand n'importe quelle ligne peut lancer une exception.

Face à ces limitations, de nombreux projets — Google, LLVM, le noyau Linux (pour le C++ qu'il utilise), l'industrie du jeu vidéo — ont historiquement interdit ou limité les exceptions. Ces projets utilisent à la place des codes d'erreur (entiers), des `std::optional` (qui ne portent pas d'information sur la cause de l'erreur), ou des `std::variant<Result, Error>` (syntaxiquement lourd).

C++23 introduit `std::expected<T, E>` : un type qui contient soit une valeur de type `T` (le résultat attendu), soit une erreur de type `E`. C'est la formalisation standard d'un pattern que la communauté utilisait déjà via des bibliothèques tierces (`tl::expected`, `boost::outcome`), inspiré du `Result<T, E>` de Rust et de l'`Either<L, R>` de Haskell.

> 📎 *Cette section se concentre sur `std::expected` comme outil. Pour une discussion plus large sur les stratégies de gestion d'erreurs (exceptions vs codes d'erreur vs expected vs contrats), voir section 17.5.*

## Le problème : les trois approches historiques et leurs défauts

### Exceptions : puissantes mais à coût variable

```cpp
std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return content;
}

// Côté appelant — l'erreur est invisible dans la signature
try {
    auto content = read_file("/etc/config.yaml");
    process(content);
} catch (const std::exception& e) {
    log_error(e.what());
}
```

Les exceptions fonctionnent, mais la signature `std::string read_file(...)` ne dit rien sur le fait que la fonction peut échouer. L'appelant peut oublier le `try`/`catch` sans que le compilateur ne proteste. Et dans les chemins critiques en performance, le mécanisme de déroulement de pile (*stack unwinding*) lors d'une exception a un coût non négligeable et surtout imprévisible.

### Codes d'erreur : explicites mais fragiles

```cpp
enum class FileError { NotFound, PermissionDenied, IoError };

FileError read_file(const std::string& path, std::string& out_content) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return FileError::NotFound;
    }
    out_content.assign((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return FileError{}; // "pas d'erreur" — convention fragile
}

// Côté appelant — facile d'ignorer le code d'erreur
std::string content;
read_file("/etc/config.yaml", content);   // Code retour ignoré silencieusement
```

Les paramètres de sortie mélangent entrées et sorties dans la signature. Le code d'erreur de retour peut être ignoré sans avertissement. Et la distinction entre « pas d'erreur » et un code d'erreur valide repose sur une convention (valeur zéro, enum par défaut) qui n'est pas vérifiée par le compilateur.

### std::optional : trop muet

```cpp
std::optional<std::string> read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::nullopt;   // Échec... mais pourquoi ?
    }
    // ...
    return content;
}
```

`std::optional` exprime « il y a une valeur ou il n'y en a pas », mais ne porte aucune information sur la cause de l'absence. Quand la fonction échoue, l'appelant ne sait pas si le fichier n'existe pas, si les permissions sont insuffisantes, ou s'il y a eu une erreur d'I/O. Pour du code qui doit diagnostiquer et traiter différemment les cas d'erreur, `std::optional` est insuffisant.

## La solution : std::expected\<T, E\>

`std::expected<T, E>` contient soit une valeur de type `T`, soit une erreur de type `E`. Les deux états sont mutuellement exclusifs et le type actif est suivi automatiquement — comme un `std::variant<T, E>` spécialisé pour le cas résultat/erreur, avec une API dédiée :

```cpp
#include <expected>
#include <string>
#include <fstream>
#include <print>

enum class FileError {
    NotFound,
    PermissionDenied,
    IoError
};

std::expected<std::string, FileError> read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::unexpected(FileError::NotFound);
    }
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    if (file.bad()) {
        return std::unexpected(FileError::IoError);
    }
    return content;   // Retour implicite de la valeur
}
```

La signature est auto-documentée : `std::expected<std::string, FileError>` dit explicitement « cette fonction retourne un `string` ou une `FileError` ». L'appelant sait, rien qu'en lisant la signature, que la fonction peut échouer et connaît le type d'erreur possible.

### std::unexpected : construire une erreur

L'erreur est retournée en l'enveloppant dans `std::unexpected`. C'est le marqueur qui distingue un retour d'erreur d'un retour de valeur :

```cpp
return std::unexpected(FileError::NotFound);       // Retourne une erreur
return content;                                     // Retourne une valeur (implicite)
return std::expected<std::string, FileError>{content}; // Retourne une valeur (explicite)
```

`std::unexpected` est nécessaire parce que, sans lui, le compilateur ne saurait pas si `return some_value` est une valeur de type `T` ou une erreur de type `E` — surtout quand `T` et `E` sont le même type ou quand il y a des conversions implicites.

## Utilisation côté appelant

### Vérification et accès

```cpp
auto result = read_file("/etc/config.yaml");

// Vérification explicite (convertible en bool)
if (result) {
    std::print("Contenu ({} octets) : {:.50}\n", result->size(), *result);
} else {
    std::print("Erreur : {}\n", static_cast<int>(result.error()));
}
```

L'API reflète celle de `std::optional` :

```cpp
result.has_value();    // true si contient une valeur
*result;               // Accès à la valeur (UB si erreur)
result->size();        // Accès aux membres de la valeur
result.value();        // Accès à la valeur (lance bad_expected_access si erreur)
result.error();        // Accès à l'erreur (UB si valeur)
result.value_or(def);  // Retourne la valeur ou un défaut
```

### Pattern avec if-init (idiomatique)

```cpp
if (auto result = read_file(path); result) {
    process(*result);
} else {
    handle_error(result.error());
}
```

Ce pattern limite la portée de `result` au bloc `if`/`else` et sépare clairement le chemin de succès du chemin d'erreur.

### Propagation d'erreur manuelle

Un pattern fréquent est la propagation : une fonction appelle une autre qui retourne un `expected`, et en cas d'erreur, propage cette erreur à son propre appelant :

```cpp
std::expected<Config, FileError> load_config(const std::string& path) {
    auto content = read_file(path);
    if (!content) {
        return std::unexpected(content.error());   // Propagation
    }

    auto config = parse_config(*content);
    if (!config) {
        return std::unexpected(config.error());    // Propagation
    }

    return *config;
}
```

Ce pattern est fonctionnel mais verbeux quand il y a beaucoup d'étapes. C'est exactement ce que les opérations monadiques permettent de simplifier.

> 💡 **Comparaison avec Rust** — En Rust, l'opérateur `?` automatise cette propagation : `let content = read_file(path)?;` retourne automatiquement l'erreur si `read_file` échoue. C++ n'a pas d'équivalent syntaxique (bien que des propositions existent). Les opérations monadiques de `std::expected` sont l'alternative actuelle.

## Opérations monadiques : chaîner sans imbriquer

Comme `std::optional` (section 12.2), `std::expected` supporte trois opérations monadiques qui permettent de construire des pipelines de transformations faillibles sans cascades de `if` :

### and_then : chaîner des opérations faillibles

`and_then` applique une fonction qui retourne elle-même un `expected`. Si l'objet courant contient une erreur, la fonction n'est pas appelée et l'erreur est propagée automatiquement :

```cpp
#include <expected>
#include <string>
#include <charconv>

enum class ParseError { InvalidFormat, OutOfRange, FileNotFound };

std::expected<std::string, ParseError> read_file(const std::string& path) {
    // ... lecture du fichier ...
}

std::expected<int, ParseError> parse_int(const std::string& s) {
    int value{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec != std::errc{}) {
        return std::unexpected(ParseError::InvalidFormat);
    }
    return value;
}

std::expected<int, ParseError> validate_port(int port) {
    if (port < 1 || port > 65535) {
        return std::unexpected(ParseError::OutOfRange);
    }
    return port;
}

// Pipeline : lire → parser → valider
auto port = read_file("/etc/port.conf")
    .and_then(parse_int)
    .and_then(validate_port);

// port contient soit un int valide (1-65535), soit une ParseError
// Aucun if imbriqué !
```

Comparons avec la version sans opérations monadiques :

```cpp
// Sans monadiques : cascade de if
auto content = read_file("/etc/port.conf");
if (!content) return std::unexpected(content.error());
auto parsed = parse_int(*content);
if (!parsed) return std::unexpected(parsed.error());
auto validated = validate_port(*parsed);
if (!validated) return std::unexpected(validated.error());
// utiliser *validated
```

Le pipeline `and_then` élimine le boilerplate de vérification/propagation et rend le flux de données visible.

### transform : transformer la valeur (si présente)

`transform` applique une fonction à la valeur contenue et enveloppe le résultat dans un `expected`. Si l'objet contient une erreur, elle est propagée sans appeler la fonction :

```cpp
auto port_str = read_file("/etc/port.conf")
    .and_then(parse_int)
    .and_then(validate_port)
    .transform([](int port) {
        return std::format(":{}", port);   // int → string, jamais faillible
    });
// port_str : std::expected<std::string, ParseError>
```

La différence clé avec `and_then` : la fonction passée à `transform` retourne une valeur brute (`T`), pas un `expected<T, E>`. C'est adapté aux transformations qui ne peuvent pas échouer.

### or_else : traiter ou remplacer l'erreur

`or_else` est le miroir de `and_then` : il est appelé uniquement quand l'objet contient une **erreur**. Il permet de récupérer d'une erreur, de la transformer, ou de fournir une valeur par défaut :

```cpp
// Fallback : si le fichier de config n'existe pas, utiliser un port par défaut
auto port = read_file("/etc/port.conf")
    .and_then(parse_int)
    .or_else([](ParseError e) -> std::expected<int, ParseError> {
        if (e == ParseError::FileNotFound) {
            return 8080;   // Valeur par défaut
        }
        return std::unexpected(e);   // Propager les autres erreurs
    });
```

### transform_error : convertir le type d'erreur

`transform_error` transforme l'erreur sans toucher à la valeur. C'est utile pour convertir entre différents types d'erreur lors de la traversée de couches d'abstraction :

```cpp
enum class AppError { ConfigError, NetworkError, DatabaseError };

auto result = read_file("/etc/config.yaml")
    .and_then(parse_config)
    .transform_error([](ParseError e) -> AppError {
        return AppError::ConfigError;   // Convertir ParseError → AppError
    });
// result : std::expected<Config, AppError>
```

## Design du type d'erreur

Le choix du type `E` dans `std::expected<T, E>` est une décision de design importante. Plusieurs approches coexistent.

### Enum class : simple et efficace

```cpp
enum class HttpError {
    Timeout,
    ConnectionRefused,
    DnsResolutionFailed,
    ServerError,
    InvalidResponse
};

std::expected<Response, HttpError> http_get(const std::string& url);
```

Les enums sont légers (un entier), exhaustifs dans un `switch`, et servent bien quand les cas d'erreur sont finis et connus. Leur limite : ils ne portent pas de contexte additionnel (message, détails).

### Struct d'erreur : riche et descriptive

```cpp
struct DbError {
    enum Code { ConnectionFailed, QueryFailed, Timeout, ConstraintViolation };

    Code code;
    std::string message;
    std::string query;          // La requête qui a échoué
    int native_error_code;      // Code natif du driver

    std::string to_string() const {
        return std::format("[DB-{}] {} (query: '{}')", 
                           static_cast<int>(code), message, query);
    }
};

std::expected<QueryResult, DbError> execute(const std::string& sql);
```

Les structs d'erreur portent un contexte riche — messages, codes natifs, traces — qui facilite le diagnostic. C'est l'approche recommandée pour les bibliothèques et les couches d'abstraction.

### std::error_code : interopérabilité avec le système

Pour interfacer avec les erreurs POSIX ou système, `std::error_code` (existant depuis C++11) est un choix naturel :

```cpp
std::expected<int, std::error_code> open_socket(const std::string& host, int port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return std::unexpected(std::error_code(errno, std::system_category()));
    }
    // ...
    return fd;
}
```

### std::string : pragmatique mais limité

Pour le prototypage ou les cas simples, une `std::string` comme type d'erreur fonctionne :

```cpp
std::expected<Config, std::string> parse_config(const std::string& input) {
    if (input.empty()) {
        return std::unexpected("Configuration vide"s);
    }
    // ...
}
```

Cette approche est facile mais empêche le traitement programmatique des erreurs (pas de `switch` sur un string). Elle est acceptable pour le prototypage mais à éviter dans du code de production où les erreurs doivent être traitées différemment selon leur nature.

## expected\<void, E\> : opérations sans résultat

Certaines opérations peuvent échouer mais ne retournent pas de valeur en cas de succès (écriture dans un fichier, envoi d'un message, suppression d'un enregistrement). `std::expected<void, E>` couvre ce cas :

```cpp
std::expected<void, FileError> write_file(const std::string& path,
                                           const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return std::unexpected(FileError::PermissionDenied);
    }
    file << content;
    if (file.bad()) {
        return std::unexpected(FileError::IoError);
    }
    return {};   // Succès — pas de valeur à retourner
}

// Utilisation
if (auto result = write_file("/tmp/output.txt", data); !result) {
    std::print("Erreur d'écriture : {}\n", static_cast<int>(result.error()));
}
```

Le `return {};` retourne un `expected` vide dans l'état « succès ». C'est plus expressif qu'un `bool` ou un code d'erreur brut, car le type d'erreur est explicite et le compilateur peut vérifier que toutes les erreurs sont traitées.

## expected vs les alternatives : grille de choix

Chaque mécanisme de gestion d'erreurs a sa place. Voici une grille de décision :

**Utiliser des exceptions quand :**
- L'erreur est exceptionnelle (ne devrait pas arriver en fonctionnement normal).
- Le code appelant est distant du point d'erreur (plusieurs niveaux de pile entre les deux).
- Le projet n'a pas de contraintes temps réel ou de taille de binaire.
- La construction d'un objet échoue (constructeur — pas de valeur de retour possible).

**Utiliser `std::expected` quand :**
- L'erreur est un résultat attendu et fréquent (fichier introuvable, parsing invalide, timeout réseau).
- L'appelant immédiat doit traiter l'erreur (pas de propagation à travers de nombreuses couches).
- La performance dans le chemin d'erreur est importante (pas de stack unwinding).
- Les exceptions sont indisponibles ou interdites dans le projet.
- On veut que la signature de la fonction documente explicitement les cas d'échec.

**Utiliser `std::optional` quand :**
- Il n'y a qu'un seul mode d'échec et la cause n'a pas besoin d'être communiquée.
- La sémantique est « présent ou absent » plutôt que « succès ou erreur ».

**Utiliser des codes d'erreur (enum/int) quand :**
- L'interopérabilité avec du code C est nécessaire.
- L'API doit être consommable depuis d'autres langages (FFI).

En pratique, un projet mature combine souvent plusieurs approches : `std::expected` pour les erreurs prévisibles dans la logique métier, des exceptions pour les situations véritablement exceptionnelles, et `std::optional` pour les lookups et les valeurs facultatives.

## Performance

`std::expected<T, E>` a des caractéristiques de performance prévisibles et favorables.

**Aucune allocation heap.** La valeur ou l'erreur est stockée en place, dans le même espace mémoire (comme `std::variant`). La taille est `max(sizeof(T), sizeof(E))` plus un discriminant.

**Coût constant dans le chemin d'erreur.** Contrairement aux exceptions, le coût de retourner une erreur est identique au coût de retourner une valeur — c'est un retour de fonction ordinaire. Pas de stack unwinding, pas de recherche de handler dans la pile d'appels.

**Pas de surcoût dans le chemin de succès.** Quand il n'y a pas d'erreur, le code est aussi performant qu'un retour de valeur classique. Le discriminant est vérifié lors de l'accès, mais le compilateur optimise souvent cette vérification quand le test a déjà été effectué.

**Zéro surcoût en taille de binaire.** Les exceptions nécessitent des tables d'exception (`.eh_frame`) qui augmentent la taille du binaire de 10 à 30 % selon les projets. `std::expected` n'ajoute aucune métadonnée au binaire.

## Bonnes pratiques

**Utiliser `std::expected` pour les erreurs prévisibles et traitables.** Si l'appelant immédiat doit prendre une décision en fonction du type d'erreur, `expected` est le bon outil. Si l'erreur doit simplement remonter jusqu'à un handler global, les exceptions peuvent être plus adaptées.

**Concevoir des types d'erreur riches.** Un `enum class` avec des variantes descriptives est le minimum. Pour les bibliothèques, ajouter un message et un contexte dans une struct rend le diagnostic bien plus facile.

**Exploiter les opérations monadiques pour les pipelines.** La séquence `and_then` → `transform` → `or_else` élimine les cascades de `if` et rend le flux de données lisible. C'est le style idiomatique recommandé en C++23.

**Ne pas utiliser expected pour les invariants de programmation.** Un index hors bornes dans un tableau, une précondition violée — ces situations sont des bugs, pas des erreurs attendues. Elles relèvent des assertions (section 18.1) ou des contrats (section 12.14.1), pas de `std::expected`.

**Combiner `expected` et `std::print` pour des messages d'erreur propres.** Rendre le type d'erreur formattable (section 12.7) permet d'utiliser `std::print("Erreur : {}\n", result.error())` directement.

**Penser à `expected<void, E>` pour les opérations sans résultat.** C'est plus expressif qu'un `bool` de retour et porte l'information d'erreur typée.

---

>  
> 📎 [17.5 Alternatives modernes : std::expected, codes d'erreur](/17-exceptions/05-alternatives-modernes.md)

⏭️ [std::flat_map et std::flat_set (C++23) : Conteneurs ordonnés à mémoire contiguë](/12-nouveautes-cpp17-26/09-flat-containers.md)
