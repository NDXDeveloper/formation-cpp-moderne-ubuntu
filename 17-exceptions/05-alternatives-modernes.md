🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 17.5 — Alternatives modernes : `std::expected` (C++23), codes d'erreur ⭐

## Quand préférer une approche sans exceptions

---

## Introduction

Les exceptions C++ sont un outil puissant, mais elles ne sont pas la réponse universelle à toute situation d'erreur. Dans certains contextes — boucles de parsing où chaque itération peut échouer, couches de validation où l'échec est aussi fréquent que le succès, systèmes embarqués où le coût du stack unwinding est prohibitif, ou simplement interfaces où l'on souhaite rendre le chemin d'erreur explicite dans le système de types — les alternatives sans exceptions produisent un code plus lisible, plus performant et plus prévisible.

C++ a longtemps souffert d'un déficit d'outils dans ce domaine. Le C offrait `errno` et les codes de retour, mais ces mécanismes sont fragiles et non typés. C++11 a apporté `std::error_code` et `std::system_error`, améliorant la situation pour les erreurs système. C++17 a introduit `std::optional`, qui exprime l'absence de valeur sans en donner la raison. Mais c'est **C++23** qui comble véritablement le vide avec `std::expected<T, E>` : un type qui combine la valeur de succès *et* l'information d'erreur dans un seul objet, rendant la gestion d'erreurs à la fois explicite, type-safe et composable.

Cette section explore l'ensemble de ces alternatives, des codes d'erreur classiques aux approches les plus modernes, en vous donnant les critères pour choisir le mécanisme adapté à chaque situation.

---

## Les limites des exceptions qui motivent les alternatives

Avant d'examiner les solutions, il est utile de cerner précisément les situations où les exceptions sont inadaptées. Il ne s'agit pas de les rejeter en bloc — elles restent le mécanisme privilégié pour les erreurs rares et graves — mais de reconnaître leurs faiblesses dans certains contextes.

### Erreurs fréquentes et prévisibles

Le modèle zero-cost des exceptions ne paie rien sur le chemin nominal, mais le coût du `throw` est élevé : typiquement des centaines de nanosecondes à plusieurs microsecondes pour le stack unwinding, le parcours des tables d'exception et l'allocation éventuelle de l'objet exception. Lorsqu'une erreur survient dans 10 %, 30 % ou 50 % des appels — ce qui est courant en parsing, en validation d'entrées, ou en recherche dans une structure de données — ce coût devient un goulot d'étranglement mesurable.

### Invisibilité dans la signature

En C++, contrairement à Java, une fonction ne déclare pas les exceptions qu'elle peut lever (en dehors de `noexcept` qui indique l'absence d'exceptions). L'appelant n'a aucun moyen, à la seule lecture de la signature, de savoir quelles erreurs une fonction peut produire. La gestion d'erreurs repose alors sur la documentation et la discipline — deux remparts fragiles.

### Contextes sans support d'exceptions

Certains environnements désactivent complètement les exceptions à la compilation (`-fno-exceptions`) : systèmes embarqués critiques, noyaux de systèmes d'exploitation, code GPU (CUDA/HIP), certains projets de jeux vidéo où la prévisibilité du temps d'exécution est primordiale. Dans ces contextes, les alternatives ne sont pas un luxe mais une nécessité.

### Composition difficile

Enchaîner plusieurs opérations faillibles avec des exceptions produit souvent soit un unique gros bloc `try/catch` qui mélange les erreurs de toutes les étapes, soit une cascade de blocs imbriqués qui obscurcit le flux logique. Les approches basées sur les types de retour se prêtent mieux à la composition séquentielle.

---

## Les codes d'erreur classiques : l'héritage du C

### `errno` et codes de retour entiers

Le modèle historique du C utilise la valeur de retour pour signaler l'échec (typiquement `-1` ou `NULL`) et la variable globale thread-local `errno` pour préciser la nature de l'erreur.

```cpp
#include <cstdio>
#include <cerrno>
#include <cstring>

void exemple_c_style() {
    FILE* f = std::fopen("/etc/shadow", "r");
    if (f == nullptr) {
        // errno contient le code d'erreur POSIX
        std::print(stderr, "Erreur : {} (errno={})\n", std::strerror(errno), errno);
        return;
    }
    // ... utilisation ...
    std::fclose(f);
}
```

Ce modèle souffre de faiblesses structurelles bien connues :

- **Rien n'oblige l'appelant à vérifier.** L'erreur peut être silencieusement ignorée, et le programme continue dans un état invalide.
- **La valeur d'erreur se mélange avec la valeur utile.** Une fonction qui retourne un `int` doit réserver une valeur sentinelle (souvent `-1`) pour signaler l'échec, ce qui réduit l'espace des valeurs légitimes.
- **`errno` est fragile.** Il peut être écrasé par n'importe quel appel système intermédiaire — il doit être lu immédiatement après l'appel qui l'a positionné.
- **Pas de typage.** Un `int` d'erreur n'exprime rien sur le domaine de l'erreur ; il faut consulter la documentation pour interpréter la valeur.

### `std::error_code` et `<system_error>` (C++11)

C++11 a introduit un cadre plus structuré avec `std::error_code` et `std::error_category`, regroupés dans l'en-tête `<system_error>`. Un `std::error_code` associe une valeur entière à une catégorie qui lui donne un sens, permettant de distinguer par exemple une erreur POSIX d'une erreur de votre propre domaine.

```cpp
#include <system_error>
#include <fstream>
#include <cerrno>

std::error_code ouvrir_fichier(const std::string& chemin, std::ifstream& flux) {
    flux.open(chemin);
    if (!flux.is_open()) {
        return std::error_code(errno, std::system_category());
    }
    return {}; // pas d'erreur : error_code par défaut = succès
}

// Utilisation :
std::ifstream f;  
if (auto ec = ouvrir_fichier("config.yaml", f); ec) {  
    std::print(stderr, "Erreur : {} ({})\n", ec.message(), ec.value());
    // ec.category().name() → "system"
}
```

Ce mécanisme est un progrès significatif : le code d'erreur est typé, portable, et extensible à des domaines personnalisés. Cependant, il conserve un défaut fondamental — **la valeur de retour et le résultat utile sont séparés**. La fonction ci-dessus utilise un paramètre de sortie (`std::ifstream&`) pour le résultat et la valeur de retour pour l'erreur. Ce pattern inverse la sémantique naturelle du retour de fonction et rend la composition malaisée.

### Créer une catégorie d'erreur personnalisée

Pour les projets qui utilisent `std::error_code` de manière systématique, il est possible de définir des catégories d'erreur propres à votre domaine :

```cpp
#include <system_error>

enum class AppErrc {
    ok                = 0,
    config_manquante  = 1,
    format_invalide   = 2,
    permission_refusee = 3
};

// Enregistrer AppErrc comme un type d'error_code
namespace std {
    template <>
    struct is_error_code_enum<AppErrc> : true_type {};
}

class AppErrorCategory : public std::error_category {  
public:  
    const char* name() const noexcept override { return "app"; }

    std::string message(int ev) const override {
        switch (static_cast<AppErrc>(ev)) {
            case AppErrc::ok:                 return "Succès";
            case AppErrc::config_manquante:   return "Fichier de configuration manquant";
            case AppErrc::format_invalide:    return "Format de données invalide";
            case AppErrc::permission_refusee: return "Permission refusée";
            default:                          return "Erreur inconnue";
        }
    }
};

inline const AppErrorCategory& app_category() {
    static AppErrorCategory instance;
    return instance;
}

inline std::error_code make_error_code(AppErrc e) {
    return {static_cast<int>(e), app_category()};
}
```

Ce mécanisme est solide et bien intégré à l'écosystème (Boost.Asio, Boost.Beast, de nombreuses bibliothèques réseau l'utilisent), mais la quantité de boilerplate est conséquente. C'est l'une des raisons qui ont motivé l'introduction de `std::expected`.

---

## `std::optional<T>` (C++17) : l'absence sans explication

Avant d'aborder `std::expected`, il faut mentionner `std::optional`, qui est son prédécesseur conceptuel. Un `std::optional<T>` contient soit une valeur de type `T`, soit *rien*. Il est l'outil idéal lorsque l'absence de valeur est un résultat légitime et que l'appelant n'a pas besoin de savoir *pourquoi* la valeur est absente.

```cpp
#include <optional>
#include <string>
#include <map>

std::optional<std::string> trouver_env(const std::string& nom) {
    if (const char* val = std::getenv(nom.c_str())) {
        return std::string(val);
    }
    return std::nullopt; // absent, sans explication
}

// Utilisation :
if (auto home = trouver_env("HOME")) {
    std::print("Home : {}\n", *home);
} else {
    std::print("Variable HOME non définie\n");
}
```

`std::optional` brille pour les recherches (trouver un élément dans un conteneur), les conversions (parser un entier depuis une chaîne qui peut ne pas en être un), et les paramètres optionnels. Mais il ne convient pas dès que l'appelant a besoin de distinguer *plusieurs causes d'échec* — « le fichier n'existe pas » vs « le fichier existe mais est illisible » vs « le format est invalide ». C'est exactement le créneau de `std::expected`.

---

## `std::expected<T, E>` (C++23) : la synthèse moderne ⭐

### Le concept

`std::expected<T, E>` est un type discriminé (tagged union) qui contient :

- soit une **valeur** de type `T` — le cas de succès,
- soit une **erreur** de type `E` — le cas d'échec.

Il combine les avantages des codes d'erreur (information sur la nature de l'échec, pas de stack unwinding, coût prévisible) avec ceux des exceptions (la valeur de retour porte le résultat, le chemin d'erreur ne peut pas être silencieusement ignoré si l'on tente d'accéder à la valeur).

```cpp
#include <expected>
#include <string>
#include <fstream>
#include <sstream>

enum class ConfigError {
    fichier_introuvable,
    permission_refusee,
    format_invalide,
    cle_manquante
};

std::expected<std::string, ConfigError> lire_config(const std::string& chemin) {
    std::ifstream f(chemin);
    if (!f.is_open()) {
        return std::unexpected(ConfigError::fichier_introuvable);
    }

    std::ostringstream ss;
    ss << f.rdbuf();
    std::string contenu = ss.str();

    if (contenu.empty()) {
        return std::unexpected(ConfigError::format_invalide);
    }

    return contenu; // succès : conversion implicite en expected<string, ConfigError>
}
```

### Construction et inspection

`std::expected` s'utilise avec une API cohérente et intuitive :

```cpp
auto resultat = lire_config("/etc/monapp/config.yaml");

// Test de présence
if (resultat.has_value()) {
    // ou simplement : if (resultat)
    std::print("Config chargée ({} octets)\n", resultat.value().size());
}

// Accès à la valeur (lève std::bad_expected_access si erreur)
std::string config = resultat.value();

// Accès à la valeur (comportement indéfini si erreur — plus rapide, pas de vérification)
std::string config = *resultat;

// Accès à l'erreur
if (!resultat) {
    ConfigError err = resultat.error();
    // ...
}
```

### Valeurs par défaut avec `value_or`

Comme `std::optional`, `std::expected` offre `value_or` pour fournir une valeur de repli sans branchement explicite :

```cpp
std::string config = lire_config("config.yaml")
                         .value_or("port: 8080\nlog_level: info");
```

### Typage riche des erreurs

Le paramètre `E` de `std::expected<T, E>` peut être n'importe quel type : un simple `enum class` comme ci-dessus, un `std::error_code`, une `std::string` descriptive, ou une classe d'erreur riche transportant du contexte. C'est cette flexibilité qui rend `std::expected` supérieur aux codes de retour entiers.

```cpp
struct ErreurValidation {
    std::string champ;
    std::string message;
    int         ligne = -1;
};

std::expected<Config, ErreurValidation> valider_config(const std::string& contenu) {
    // ...
    if (!contenu.contains("port")) {
        return std::unexpected(ErreurValidation{
            .champ = "port",
            .message = "Clé obligatoire manquante",
            .ligne = -1
        });
    }
    // ...
    return Config{/* ... */};
}
```

---

## Chaîner les opérations : l'interface monadique (C++23)

L'un des apports les plus significatifs de `std::expected` en C++23 est son interface monadique, composée de trois méthodes qui permettent de chaîner les opérations faillibles sans écrire de blocs `if/else` imbriqués. Ces méthodes, empruntées aux langages fonctionnels et déjà présentes sur `std::optional` depuis C++23, transforment la gestion d'erreurs séquentielle en un pipeline lisible.

### `and_then` : chaîner les opérations faillibles

`and_then` prend une fonction qui reçoit la valeur contenue (de type `T`) et retourne un nouvel `std::expected`. Si l'`expected` courant contient une erreur, la fonction n'est pas appelée et l'erreur est propagée telle quelle.

```cpp
enum class PipelineError {
    fichier_introuvable,
    format_invalide,
    port_hors_limites
};

std::expected<std::string, PipelineError> lire_fichier(const std::string& chemin) {
    std::ifstream f(chemin);
    if (!f.is_open()) return std::unexpected(PipelineError::fichier_introuvable);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::expected<Config, PipelineError> parser(const std::string& contenu) {
    // ... parsing YAML/JSON ...
    if (/* erreur */) return std::unexpected(PipelineError::format_invalide);
    return Config{/* ... */};
}

std::expected<Config, PipelineError> valider(Config cfg) {
    if (cfg.port < 1 || cfg.port > 65535) {
        return std::unexpected(PipelineError::port_hors_limites);
    }
    return cfg;
}

// Pipeline complet — chaque étape s'exécute seulement si la précédente a réussi
auto resultat = lire_fichier("config.yaml")
                    .and_then(parser)
                    .and_then(valider);
```

Ce pipeline est fonctionnellement équivalent à une cascade de `if/else`, mais il se lit de haut en bas comme une séquence d'étapes, sans imbrication. Si `lire_fichier` échoue, ni `parser` ni `valider` ne sont appelées — l'erreur est propagée automatiquement jusqu'au `resultat` final.

### `transform` : transformer la valeur sans risque d'échec

`transform` applique une fonction à la valeur contenue et encapsule le résultat dans un nouvel `expected`. Contrairement à `and_then`, la fonction passée à `transform` retourne directement une valeur (pas un `expected`), car elle est supposée ne pas pouvoir échouer.

```cpp
auto message = lire_fichier("config.yaml")
                   .and_then(parser)
                   .and_then(valider)
                   .transform([](const Config& cfg) {
                       return "Serveur démarré sur le port " + std::to_string(cfg.port);
                   });
// message est un std::expected<std::string, PipelineError>
```

### `or_else` : gérer ou transformer les erreurs

`or_else` est le symétrique de `and_then` : il n'est invoqué que si l'`expected` contient une erreur. Il permet de tenter une récupération, de transformer l'erreur, ou de loguer avant de propager.

```cpp
auto config = lire_fichier("config.yaml")
                  .or_else([](PipelineError err) -> std::expected<std::string, PipelineError> {
                      if (err == PipelineError::fichier_introuvable) {
                          logger::warn("Config absente, utilisation des défauts");
                          return "port: 8080\nlog_level: info"; // récupération
                      }
                      return std::unexpected(err); // propager les autres erreurs
                  })
                  .and_then(parser)
                  .and_then(valider);
```

### Comparaison visuelle : `if/else` vs pipeline monadique

Pour apprécier le gain en lisibilité, voici le même enchaînement écrit avec des tests explicites :

```cpp
// Style impératif — fonctionnel mais verbeux
auto fichier = lire_fichier("config.yaml");  
if (!fichier) {  
    if (fichier.error() == PipelineError::fichier_introuvable) {
        fichier = std::expected<std::string, PipelineError>("port: 8080\nlog_level: info");
    } else {
        return std::unexpected(fichier.error());
    }
}
auto parsed = parser(fichier.value());  
if (!parsed) {  
    return std::unexpected(parsed.error());
}
auto config = valider(parsed.value());  
if (!config) {  
    return std::unexpected(config.error());
}
// utiliser config.value()
```

```cpp
// Style monadique — même logique, flux linéaire
auto config = lire_fichier("config.yaml")
                  .or_else([](PipelineError err) -> std::expected<std::string, PipelineError> {
                      if (err == PipelineError::fichier_introuvable)
                          return "port: 8080\nlog_level: info";
                      return std::unexpected(err);
                  })
                  .and_then(parser)
                  .and_then(valider);
```

L'avantage du style monadique croît avec le nombre d'étapes : chaque nouvelle opération ajoute une ligne au pipeline au lieu d'un nouveau niveau d'imbrication.

---

## `std::expected<void, E>` : opérations sans valeur de retour

Certaines opérations peuvent échouer sans produire de valeur en cas de succès — écrire dans un fichier, envoyer un message réseau, appliquer une migration de schéma. `std::expected` gère ce cas via la spécialisation `std::expected<void, E>` :

```cpp
std::expected<void, NetworkError::Code> envoyer_heartbeat(const Connexion& conn) {
    if (!conn.est_active()) {
        return std::unexpected(NetworkError::Code::connexion_refusee);
    }

    int octets = conn.envoyer(paquet_heartbeat);
    if (octets <= 0) {
        return std::unexpected(NetworkError::Code::timeout);
    }

    return {}; // succès, pas de valeur
}

// Utilisation :
if (auto res = envoyer_heartbeat(conn); !res) {
    logger::error("Heartbeat échoué : code {}", static_cast<int>(res.error()));
}
```

Cette spécialisation remplace élégamment le pattern `bool` + paramètre de sortie ou le `std::error_code` retourné seul.

---

## Quand choisir quoi : arbre de décision

Le choix entre exceptions, `std::expected`, `std::optional` et codes d'erreur n'est pas dogmatique — il dépend du contexte. Voici un guide de décision pragmatique.

### Utilisez les exceptions quand :

- L'erreur est **rare** (< 1 % des appels) et grave — connexion DB perdue, fichier système corrompu, allocation mémoire échouée.
- La gestion de l'erreur se fait **loin** du point de détection — l'erreur doit traverser plusieurs couches d'appels.
- Vous avez besoin de la **garantie de non-ignorance** — une exception non capturée termine le programme, alors qu'un `std::expected` non inspecté compile sans avertissement.
- Vous travaillez dans un **constructeur** — les constructeurs n'ont pas de valeur de retour, les exceptions sont le seul mécanisme standard pour signaler un échec de construction.

### Utilisez `std::expected` quand :

- L'erreur est **fréquente** ou fait partie du flux normal — parsing, validation, recherche.
- L'appelant **immédiat** est responsable de la gestion — pas besoin de propagation à travers la pile.
- Vous voulez un **pipeline** d'opérations faillibles chaînées.
- Vous travaillez dans un environnement **sans exceptions** (`-fno-exceptions`).
- Vous souhaitez que la **signature** de la fonction documente explicitement les modes d'échec.

### Utilisez `std::optional` quand :

- L'absence de valeur est un résultat **légitime et auto-explicatif** — recherche dans un conteneur, variable d'environnement optionnelle.
- L'appelant n'a pas besoin de connaître la **raison** de l'absence.

### Utilisez `std::error_code` quand :

- Vous interfacez avec des **API système POSIX** ou des bibliothèques qui utilisent déjà ce mécanisme (Asio, Beast).
- Vous maintenez du code existant qui repose sur `<system_error>`.

---

## Combiner `std::expected` et exceptions

Dans un projet réel, `std::expected` et les exceptions coexistent naturellement. Un pattern fréquent consiste à utiliser `std::expected` dans les couches internes (parsing, validation, accès aux données) et à convertir en exception aux frontières — par exemple au point d'entrée d'un handler HTTP ou d'une commande CLI :

```cpp
// Couche interne : expected, pas d'exceptions
std::expected<Config, ConfigError> charger_config(const std::string& chemin);

// Frontière : conversion en exception pour les couches qui préfèrent ce modèle
Config charger_config_ou_throw(const std::string& chemin) {
    auto result = charger_config(chemin);
    if (!result) {
        throw ConfigException(result.error()); // conversion explicite
    }
    return std::move(result).value();
}
```

Le sens inverse est également utile — encapsuler un appel qui lève dans un `std::expected` :

```cpp
template <typename F, typename... Args>  
auto capturer_exception(F&& f, Args&&... args)  
    -> std::expected<std::invoke_result_t<F, Args...>, std::string>
{
    try {
        return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    } catch (const std::exception& e) {
        return std::unexpected(std::string(e.what()));
    }
}

// Utilisation :
auto resultat = capturer_exception([&] {
    return bibliotheque_tierce.analyser(donnees);
});
// resultat est un std::expected<Analyse, std::string>
```

Ce pattern de pont bidirectionnel permet à chaque couche de l'architecture d'utiliser le mécanisme qui lui convient le mieux, sans imposer un modèle unique à l'ensemble du projet.

---

## Coût et performances comparées

Le tableau suivant résume les caractéristiques de performance de chaque approche. Les ordres de grandeur sont indicatifs et dépendent du compilateur, de l'architecture et du contexte.

| Mécanisme | Coût chemin nominal | Coût chemin d'erreur | Taille binaire | Allocation |
|---|---|---|---|---|
| Exceptions | Quasi-nul | Élevé (µs, tables) | + tables unwind | Possible (exception object) |
| `std::expected` | Faible (test de flag) | Faible (idem) | Neutre | Aucune (stack) |
| `std::error_code` | Faible (test entier) | Faible (idem) | Neutre | Aucune |
| `std::optional` | Faible (test de flag) | Faible (idem) | Neutre | Aucune |

Le point clé : `std::expected` a un coût **constant et prévisible**, que le chemin soit nominal ou erroné. Les exceptions ont un coût quasi-nul en nominal mais élevé en erreur. Le choix optimal dépend donc directement du **ratio succès/échec** attendu.

---

## État du support compilateur (mars 2026)

`std::expected` est disponible à partir de :

| Compilateur | Version minimale | Flag requis |
|---|---|---|
| GCC | 12+ | `-std=c++23` |
| Clang | 16+ | `-std=c++23` |
| MSVC | 19.33+ (VS 2022 17.3) | `/std:c++latest` ou `/std:c++23` |

L'interface monadique (`and_then`, `transform`, `or_else`) requiert :

| Compilateur | Version minimale |
|---|---|
| GCC | 13+ |
| Clang | 17+ |
| MSVC | 19.36+ (VS 2022 17.6) |

Avec GCC 15 et Clang 20 (versions couvertes par cette formation), l'ensemble des fonctionnalités de `std::expected` est pleinement disponible.

---

## Récapitulatif

La gestion d'erreurs en C++ moderne n'est plus un choix binaire entre « exceptions ou codes de retour ». Le langage offre désormais un spectre d'outils complémentaires :

- **`std::optional`** pour l'absence sans explication.
- **`std::expected`** pour les résultats faillibles avec information d'erreur typée — c'est l'outil qui manquait à C++ depuis des décennies, et son interface monadique en fait un mécanisme de premier ordre pour les pipelines d'opérations.
- **`std::error_code`** pour l'interopérabilité avec les API système et les bibliothèques existantes.
- **Exceptions** pour les erreurs rares, graves, et qui doivent traverser les couches.

Le développeur C++ moderne choisit le mécanisme adapté à chaque situation plutôt que de s'enfermer dans une approche unique. L'essentiel est la **cohérence au sein de chaque couche** et la **clarté des conversions aux frontières**.

> 📎 *Pour la couverture complète de `std::expected` dans le contexte des nouveautés C++23, voir la **section 12.8** (std::expected). La section suivante (17.6) explore les **contrats C++26**, qui complètent le tableau en adressant les erreurs de programmation — préconditions violées, invariants brisés — avec un mécanisme intégré au langage.*

⏭️ [Contrats (C++26) : Préconditions et postconditions — standard ratifié](/17-exceptions/06-contrats-cpp26.md)
