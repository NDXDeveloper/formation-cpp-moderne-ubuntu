🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 17.3 — Exceptions personnalisées

## Concevoir des classes d'exceptions adaptées à votre domaine métier

---

## Introduction

Les exceptions standard de la bibliothèque (`std::runtime_error`, `std::invalid_argument`, etc.) couvrent un large spectre d'erreurs génériques, mais elles atteignent rapidement leurs limites dans un projet réel. Lorsqu'un serveur HTTP reçoit une requête malformée, lorsqu'un parseur JSON rencontre un token inattendu à la ligne 42 colonne 17, ou lorsqu'un moteur de calcul détecte une matrice singulière, l'information portée par un simple `std::runtime_error("erreur")` est insuffisante pour permettre une récupération intelligente.

Les exceptions personnalisées répondent à ce besoin en vous permettant de :

- **typer finement les erreurs** pour que chaque bloc `catch` ne capture que ce qu'il sait traiter,
- **transporter de l'information contextuelle** (codes d'erreur, coordonnées dans un fichier, état du système au moment de l'échec),
- **structurer une hiérarchie d'erreurs** propre à votre domaine, reflétant la taxonomie naturelle des situations anormales de votre application.

Cette section vous guide dans la conception de classes d'exceptions robustes, depuis le cas simple jusqu'aux hiérarchies élaborées utilisées dans les projets professionnels.

---

## Hériter de `std::exception` : la règle d'or

Toute exception personnalisée en C++ doit hériter, directement ou indirectement, de `std::exception`. Cette convention n'est pas imposée par le langage — on peut techniquement lancer n'importe quel type, y compris un `int` ou une `std::string` — mais elle est **universellement attendue** dans le code professionnel, et ce pour trois raisons.

Premièrement, un bloc `catch (const std::exception& e)` constitue un filet de sécurité standard capable d'intercepter toute exception conforme à cette convention. Si votre exception n'hérite pas de `std::exception`, elle échappe à ce filet et ne peut être capturée que par un `catch (...)` qui ne donne accès à aucune information sur l'erreur.

Deuxièmement, la méthode virtuelle `what()` fournit une interface uniforme pour obtenir une description textuelle de l'erreur. Tous les outils de logging, tous les frameworks de test, et la majorité des bibliothèques tierces s'appuient sur cette interface.

Troisièmement, hériter de la hiérarchie standard permet à vos exceptions de s'intégrer naturellement dans les `catch` existants. Une exception qui hérite de `std::runtime_error` sera capturée aussi bien par un `catch (const VotreException&)` que par un `catch (const std::runtime_error&)` ou un `catch (const std::exception&)`.

### Ce qu'il ne faut jamais faire

```cpp
// ❌ Anti-pattern : lancer un type primitif
throw 42;  
throw "Erreur critique";  
throw std::string("Quelque chose a échoué");  
```

Ces formes sont syntaxiquement valides mais posent de nombreux problèmes : impossible d'appeler `what()`, impossible de distinguer les erreurs par type dans un `catch`, aucune information structurée, et risque de capture accidentelle par un `catch (int)` destiné à tout autre usage. Elles sont à proscrire sans exception.

---

## Première approche : hériter de `std::runtime_error`

Dans la majorité des cas, hériter directement de `std::runtime_error` (ou de `std::logic_error` selon la nature de l'erreur) est le point de départ le plus pratique. Ces classes stockent déjà un message interne et implémentent `what()` pour vous.

```cpp
#include <stdexcept>
#include <string>

class FichierIntrouvableError : public std::runtime_error {  
public:  
    explicit FichierIntrouvableError(const std::string& chemin)
        : std::runtime_error("Fichier introuvable : " + chemin)
        , chemin_(chemin)
    {}

    const std::string& chemin() const noexcept { return chemin_; }

private:
    std::string chemin_;
};
```

L'utilisation est immédiate :

```cpp
#include <fstream>

std::string lire_fichier(const std::string& chemin) {
    std::ifstream f(chemin);
    if (!f.is_open()) {
        throw FichierIntrouvableError(chemin);
    }
    // ... lecture ...
    return contenu;
}

// Côté appelant :
try {
    auto config = lire_fichier("/etc/monapp/config.yaml");
} catch (const FichierIntrouvableError& e) {
    // Accès au chemin typé pour une récupération ciblée
    logger::error("Config manquante : {}", e.chemin());
    utiliser_config_par_defaut();
} catch (const std::exception& e) {
    // Filet de sécurité pour toute autre erreur
    logger::error("Erreur inattendue : {}", e.what());
}
```

Notez deux points importants dans cette conception :

- Le constructeur est marqué `explicit` pour éviter les conversions implicites depuis `std::string`.
- L'accesseur `chemin()` est marqué `noexcept` — un accesseur sur une exception ne devrait jamais lui-même lever une exception.

---

## Transporter de l'information contextuelle

Un simple message textuel est rarement suffisant pour une récupération programmatique. L'appelant a souvent besoin de données structurées pour décider de la marche à suivre : un code d'erreur numérique, des coordonnées dans un fichier, un identifiant de ressource, un horodatage.

### Exemple : erreur de parsing avec localisation

```cpp
#include <cstdint>
#include <stdexcept>
#include <string>

class ParseError : public std::runtime_error {  
public:  
    ParseError(const std::string& message,
               std::size_t ligne,
               std::size_t colonne,
               const std::string& fichier = "")
        : std::runtime_error(formater_message(message, ligne, colonne, fichier))
        , ligne_(ligne)
        , colonne_(colonne)
        , fichier_(fichier)
    {}

    std::size_t ligne()   const noexcept { return ligne_; }
    std::size_t colonne() const noexcept { return colonne_; }
    const std::string& fichier() const noexcept { return fichier_; }

private:
    std::size_t ligne_;
    std::size_t colonne_;
    std::string fichier_;

    static std::string formater_message(const std::string& msg,
                                        std::size_t ligne,
                                        std::size_t colonne,
                                        const std::string& fichier) {
        std::string resultat;
        if (!fichier.empty()) {
            resultat += fichier + ":";
        }
        resultat += std::to_string(ligne) + ":" + std::to_string(colonne);
        resultat += " — " + msg;
        return resultat;
    }
};
```

Le message retourné par `what()` reste lisible pour les humains (`config.yaml:12:5 — Clé dupliquée "port"`), tandis que les accesseurs typés permettent à l'appelant de réagir programmatiquement :

```cpp
try {
    auto config = parser_yaml("config.yaml");
} catch (const ParseError& e) {
    if (e.ligne() == 1 && e.colonne() == 1) {
        // Le fichier est probablement dans un mauvais encodage
        tenter_reparser_utf16(e.fichier());
    } else {
        afficher_erreur_contextuelle(e.fichier(), e.ligne(), e.colonne());
    }
}
```

### Exemple : erreur réseau avec code et tentative de retry

```cpp
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>

class NetworkError : public std::runtime_error {  
public:  
    enum class Code : std::uint16_t {
        timeout          = 1,
        connexion_refusee = 2,
        dns_non_resolu   = 3,
        tls_invalide     = 4
    };

    NetworkError(Code code,
                 const std::string& hote,
                 std::uint16_t port,
                 const std::string& detail = "")
        : std::runtime_error(formater(code, hote, port, detail))
        , code_(code)
        , hote_(hote)
        , port_(port)
    {}

    Code               code() const noexcept { return code_; }
    const std::string& hote() const noexcept { return hote_; }
    std::uint16_t      port() const noexcept { return port_; }

    /// Indique si l'erreur est potentiellement transitoire (retry possible)
    bool est_transitoire() const noexcept {
        return code_ == Code::timeout || code_ == Code::connexion_refusee;
    }

private:
    Code          code_;
    std::string   hote_;
    std::uint16_t port_;

    static std::string formater(Code code, const std::string& hote,
                                std::uint16_t port, const std::string& detail) {
        std::string msg = "Erreur réseau [" + std::to_string(static_cast<int>(code)) + "] "
                        + "vers " + hote + ":" + std::to_string(port);
        if (!detail.empty()) {
            msg += " — " + detail;
        }
        return msg;
    }
};
```

La méthode `est_transitoire()` illustre une bonne pratique : l'exception elle-même porte la logique permettant à l'appelant de décider s'il faut réessayer, sans que celui-ci ait à connaître le détail interne des codes d'erreur.

```cpp
void envoyer_avec_retry(const Requete& req, int max_tentatives = 3) {
    for (int tentative = 1; tentative <= max_tentatives; ++tentative) {
        try {
            client.envoyer(req);
            return; // succès
        } catch (const NetworkError& e) {
            if (!e.est_transitoire() || tentative == max_tentatives) {
                throw; // relancer si non récupérable ou dernière tentative
            }
            logger::warn("Tentative {}/{} échouée vers {}:{} — retry...",
                         tentative, max_tentatives, e.hote(), e.port());
            std::this_thread::sleep_for(std::chrono::seconds(tentative));
        }
    }
}
```

---

## Construire une hiérarchie d'exceptions

Dans un projet de taille significative, les exceptions s'organisent naturellement en une arborescence qui reflète la structure du domaine métier. Le principe est le même que pour la hiérarchie standard : une classe de base abstraite pour le domaine, puis des spécialisations pour chaque catégorie d'erreur.

### Architecture typique

```
std::runtime_error
 └── AppError                    ← base de toutes les erreurs de l'application
      ├── DatabaseError          ← erreurs liées à la base de données
      │    ├── ConnectionError
      │    ├── QueryError
      │    └── TransactionError
      ├── NetworkError           ← erreurs réseau
      │    ├── TimeoutError
      │    └── TlsError
      └── ValidationError        ← erreurs de validation métier
           ├── FormatError
           └── ConstraintError
```

### Implémentation de la classe de base

```cpp
#include <stdexcept>
#include <string>

class AppError : public std::runtime_error {  
public:  
    /// Code d'erreur applicatif unique, exploitable par les clients API
    enum class Domaine : int {
        general    = 0,
        database   = 1000,
        reseau     = 2000,
        validation = 3000
    };

    AppError(Domaine domaine, int code_detail, const std::string& message)
        : std::runtime_error(message)
        , domaine_(domaine)
        , code_detail_(code_detail)
    {}

    Domaine domaine()     const noexcept { return domaine_; }
    int     code_detail() const noexcept { return code_detail_; }

    /// Code unique combinant domaine et détail (ex: 2001 = timeout réseau)
    int code_complet() const noexcept {
        return static_cast<int>(domaine_) + code_detail_;
    }

private:
    Domaine domaine_;
    int     code_detail_;
};
```

### Spécialisations

```cpp
class DatabaseError : public AppError {  
public:  
    explicit DatabaseError(int code_detail, const std::string& message,
                           const std::string& requete = "")
        : AppError(Domaine::database, code_detail, message)
        , requete_(requete)
    {}

    const std::string& requete() const noexcept { return requete_; }

private:
    std::string requete_;
};

class ConnectionError : public DatabaseError {  
public:  
    explicit ConnectionError(const std::string& hote, const std::string& detail = "")
        : DatabaseError(1, "Connexion DB échouée vers " + hote +
                           (detail.empty() ? "" : " — " + detail))
        , hote_(hote)
    {}

    const std::string& hote() const noexcept { return hote_; }

private:
    std::string hote_;
};

class QueryError : public DatabaseError {  
public:  
    QueryError(const std::string& message, const std::string& requete)
        : DatabaseError(2, message, requete)
    {}
};
```

### Capture à différents niveaux de granularité

L'intérêt d'une hiérarchie se révèle au moment de la capture. Chaque couche de l'application peut intercepter les exceptions au niveau de précision qui lui convient :

```cpp
void traiter_requete_utilisateur() {
    try {
        auto resultat = service.executer_requete(/* ... */);
    }
    catch (const ConnectionError& e) {
        // Couche infrastructure : basculer vers la réplique
        logger::warn("DB primaire injoignable ({}), bascule réplique", e.hote());
        basculer_vers_replique();
    }
    catch (const DatabaseError& e) {
        // Couche service : erreur DB non récupérable localement
        logger::error("Erreur DB [{}] : {}", e.code_complet(), e.what());
        throw; // propager vers la couche supérieure
    }
    catch (const AppError& e) {
        // Couche API : toute erreur applicative → réponse d'erreur structurée
        repondre_erreur_json(e.code_complet(), e.what());
    }
}
```

---

## Surcharger `what()` : quand et comment

La classe `std::runtime_error` stocke le message passé au constructeur et retourne un `const char*` via `what()`. Dans la plupart des cas, construire le message final dans le constructeur (comme dans les exemples précédents) est la meilleure approche : c'est simple, et `what()` reste une simple lecture.

Il existe cependant des situations où vous souhaitez **générer le message à la demande** — par exemple parce qu'il dépend d'un état qui pourrait évoluer, ou parce que vous voulez éviter l'allocation d'une `std::string` dans le constructeur. Dans ce cas, vous devez hériter directement de `std::exception` et surcharger `what()` vous-même.

```cpp
#include <exception>
#include <cstring>
#include <cstdio>

class ErreurCode : public std::exception {  
public:  
    explicit ErreurCode(int code) noexcept
        : code_(code)
    {
        // Construire le message dans un buffer interne (pas d'allocation dynamique)
        std::snprintf(buffer_, sizeof(buffer_), "Erreur système (code %d)", code_);
    }

    const char* what() const noexcept override {
        return buffer_;
    }

    int code() const noexcept { return code_; }

private:
    int  code_;
    char buffer_[64];
};
```

Cette approche est utile dans les contextes où l'allocation mémoire pourrait elle-même échouer (situations de mémoire basse), mais elle est plus contraignante : vous devez gérer manuellement un buffer et vous assurer que `what()` ne lève jamais d'exception (elle est `noexcept`).

**Recommandation** : héritez de `std::runtime_error` par défaut. Ne descendez directement de `std::exception` avec un `what()` personnalisé que si vous avez une raison technique précise de le faire.

---

## Ré-emballage d'exceptions (exception wrapping)

Dans une architecture en couches, une erreur technique de bas niveau (erreur POSIX, exception d'une bibliothèque tierce) doit souvent être traduite en une exception du domaine applicatif. Cette technique, appelée **exception wrapping** ou **exception translation**, préserve l'information d'origine tout en présentant une interface cohérente à la couche supérieure.

### Avec `std::nested_exception`

C++11 a introduit `std::throw_with_nested` et `std::rethrow_if_nested` pour enchaîner les exceptions sans perdre la cause originale :

```cpp
#include <exception>
#include <stdexcept>

void acceder_base_de_donnees() {
    try {
        // Appel bas niveau qui lève une exception système
        connexion_posix(hote_, port_);
    } catch (const std::system_error& e) {
        // Ré-emballer dans une exception du domaine, en conservant l'originale
        std::throw_with_nested(
            ConnectionError(hote_, "Échec connexion POSIX")
        );
    }
}
```

L'appelant peut alors dérouler la chaîne d'exceptions imbriquées pour le diagnostic :

```cpp
void afficher_chaine_erreurs(const std::exception& e, int niveau = 0) {
    std::string indentation(niveau * 2, ' ');
    std::print("{}Causé par : {}\n", indentation, e.what());

    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception& cause) {
        afficher_chaine_erreurs(cause, niveau + 1);
    } catch (...) {
        std::print("{}Causé par : (exception inconnue)\n", indentation);
    }
}
```

Un appel typique produirait une sortie semblable à :

```
Causé par : Connexion DB échouée vers db.prod.local — Échec connexion POSIX
  Causé par : system_error: Connection refused [errno 111]
```

### Quand utiliser le wrapping

Le ré-emballage est particulièrement utile aux **frontières entre couches** :

- Entre une bibliothèque tierce et votre code applicatif — vous ne voulez pas exposer les types d'exceptions d'une bibliothèque que vos appelants ne connaissent pas.
- Entre le code réseau/système et la logique métier — les `std::system_error` ou `errno` n'ont pas de sens au niveau métier.
- Aux points d'entrée d'une API publique — vos utilisateurs ne devraient attraper que vos types d'exceptions documentés.

---

## Bonnes pratiques de conception

### Capturer par référence constante

Capturez toujours les exceptions par **référence constante** (`const&`). Capturer par valeur provoquerait un *slicing* : l'objet serait copié comme une instance de la classe de base, perdant toute information spécifique à la classe dérivée.

```cpp
// ✅ Correct : capture par référence constante
catch (const NetworkError& e) {
    // e.code(), e.hote(), e.port() sont tous accessibles
}

// ❌ Slicing : l'objet est copié comme un std::runtime_error
catch (std::runtime_error e) {
    // e est un std::runtime_error, pas un NetworkError
    // Toute information spécifique est perdue
}
```

### Ordonner les `catch` du plus spécifique au plus général

Les blocs `catch` sont évalués dans l'ordre. Un `catch (const std::exception&)` placé avant un `catch (const NetworkError&)` masquerait ce dernier, car `NetworkError` est un `std::exception`. Le compilateur n'émet pas toujours un avertissement dans ce cas — c'est à vous d'être vigilant.

```cpp
try {
    operation_reseau();
}
catch (const TimeoutError& e)   { /* le plus spécifique d'abord */ }  
catch (const NetworkError& e)   { /* catégorie intermédiaire    */ }  
catch (const AppError& e)       { /* base applicative           */ }  
catch (const std::exception& e) { /* filet de sécurité          */ }  
```

### Rendre les constructeurs et accesseurs `noexcept` quand c'est possible

Les accesseurs d'une exception ne devraient jamais lever d'exception eux-mêmes — ce serait absurde et dangereux. Marquez-les `noexcept`. Pour les constructeurs, c'est parfois impossible (construction d'une `std::string`), mais c'est un objectif à garder en tête, surtout pour les exceptions susceptibles d'être levées en situation de ressources limitées.

### Préférer la composition à l'héritage profond

Une hiérarchie de cinq niveaux ou plus devient difficile à maintenir. Si vous avez besoin de transporter des informations variées, préférez la composition : une seule classe d'exception avec un enum de catégorie et des champs optionnels est souvent plus pratique qu'un arbre d'héritage complexe.

```cpp
// Parfois plus pragmatique qu'une hiérarchie profonde
class ServiceError : public std::runtime_error {  
public:  
    enum class Categorie { database, reseau, validation, autorisation };

    ServiceError(Categorie cat, int code, const std::string& message,
                 std::source_location loc = std::source_location::current())
        : std::runtime_error(message)
        , categorie_(cat)
        , code_(code)
        , localisation_(loc)
    {}

    Categorie               categorie()     const noexcept { return categorie_; }
    int                     code()          const noexcept { return code_; }
    std::source_location    localisation()  const noexcept { return localisation_; }

private:
    Categorie            categorie_;
    int                  code_;
    std::source_location localisation_;
};
```

L'utilisation de `std::source_location` (C++20) permet de capturer automatiquement le fichier, la ligne et la fonction où l'exception a été créée — une information précieuse pour le diagnostic, sans avoir à écrire de macros.

### Ne pas utiliser les exceptions pour le flux de contrôle normal

Les exceptions sont conçues pour les situations **exceptionnelles**. Utiliser `throw` / `catch` comme un mécanisme de branchement dans une boucle de traitement, ou pour signaler une fin de fichier attendue, est un anti-pattern coûteux en performance et déroutant à la lecture. Pour les résultats attendus — valeur absente, validation échouée, fin de flux — préférez `std::optional`, `std::expected` ou un simple booléen.

---

## `std::runtime_error` vs `std::logic_error` : quelle base choisir ?

Le choix de la classe de base n'est pas anodin — il exprime une intention sémantique.

**`std::logic_error`** et ses dérivées (`std::invalid_argument`, `std::out_of_range`, `std::domain_error`) signalent des erreurs qui auraient pu être **évitées par le programmeur**. Elles correspondent à des violations de préconditions : passer un indice négatif, fournir un argument dans un format invalide alors que le format est documenté. En théorie, un programme parfaitement écrit ne devrait jamais les lever.

**`std::runtime_error`** et ses dérivées (`std::overflow_error`, `std::underflow_error`, `std::system_error`) signalent des erreurs qui **ne peuvent pas être prévenues** par le code seul : échec d'une opération réseau, disque plein, donnée externe corrompue.

En pratique, la grande majorité des exceptions personnalisées héritent de `std::runtime_error`, car les erreurs d'exécution sont de loin les plus fréquentes dans les applications réelles. Réservez `std::logic_error` aux bibliothèques dont l'API a des préconditions strictes que vous souhaitez vérifier à l'exécution en mode debug, en gardant à l'esprit que les contrats C++26 offriront un mécanisme plus adapté à cet usage.

---

## Récapitulatif

Les exceptions personnalisées sont un outil de conception à part entière, pas un simple détail d'implémentation. Une hiérarchie d'exceptions bien conçue rend le code de gestion d'erreurs plus lisible, plus ciblé et plus maintenable. Les principes clés à retenir :

- Hériter toujours de `std::exception`, de préférence via `std::runtime_error`.
- Transporter des données structurées (codes, identifiants, coordonnées) en plus du message textuel.
- Capturer par `const&`, du plus spécifique au plus général.
- Utiliser `std::throw_with_nested` pour préserver la chaîne de causalité aux frontières entre couches.
- Garder la hiérarchie aussi plate que possible — préférer la composition si la profondeur dépasse trois niveaux.
- Marquer tous les accesseurs `noexcept`.

> 📎 *La section suivante (17.4) explore `noexcept` et les garanties d'exception — un complément essentiel pour concevoir des exceptions qui s'intègrent correctement avec la sémantique de mouvement (chapitre 10) et les algorithmes de la STL (chapitre 15).*

⏭️ [noexcept et garanties d'exception](/17-exceptions/04-noexcept.md)
