🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 19.1 — std::filesystem (C++17) : API moderne

## Module 7 — Programmation Système sur Linux *(Niveau Avancé)*

---

## Présentation générale

Avant C++17, le langage C++ ne disposait d'aucune abstraction standard pour interagir avec le système de fichiers. Les développeurs étaient contraints d'utiliser soit les appels système POSIX (portabilité limitée à Unix), soit des bibliothèques tierces comme **Boost.Filesystem**. Ce dernier a d'ailleurs servi de base directe à la proposition qui a abouti à l'intégration de `std::filesystem` dans le standard C++17.

`std::filesystem` fournit une API complète, portable et type-safe pour manipuler les fichiers, répertoires et chemins. Elle couvre les opérations courantes : parcours d'arborescences, création et suppression de fichiers et répertoires, copie, déplacement, renommage, lecture des métadonnées (taille, dates, permissions) et manipulation de chemins symboliques.

L'ensemble de l'API est accessible via un unique header :

```cpp
#include <filesystem>

// Alias conventionnel utilisé dans toute cette formation
namespace fs = std::filesystem;
```

---

## La classe centrale : `std::filesystem::path`

La pierre angulaire de toute l'API est la classe `fs::path`. Elle représente un chemin dans le système de fichiers — qu'il soit absolu, relatif, qu'il pointe vers un fichier existant ou non. Un `path` n'est pas un fichier : c'est une abstraction sur une chaîne de caractères structurée selon les conventions du système d'exploitation.

### Construction d'un path

Un `fs::path` se construit à partir de n'importe quel type de chaîne :

```cpp
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    // Depuis un littéral string
    fs::path p1 = "/home/user/projet/src/main.cpp";

    // Depuis une std::string
    std::string dir = "/var/log";
    fs::path p2(dir);

    // Depuis une string_view (C++17)
    std::string_view sv = "/etc/nginx/nginx.conf";
    fs::path p3(sv);

    // Path vide (valide, mais ne pointe vers rien)
    fs::path p4;

    std::println("p1 = {}", p1.string());
    std::println("p4 est vide : {}", p4.empty());
}
```

Un point important : construire un `fs::path` ne vérifie **jamais** l'existence du fichier ou répertoire sur le disque. Le path est un objet purement syntaxique tant que vous n'appelez pas une fonction qui interroge le filesystem.

### Décomposition d'un chemin

`fs::path` offre une série de méthodes pour extraire les différentes composantes d'un chemin. C'est l'un de ses atouts majeurs par rapport à la manipulation manuelle de chaînes de caractères :

```cpp
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    fs::path p = "/home/user/projet/build/app.tar.gz";

    std::println("Chemin complet  : {}", p.string());
    std::println("root_path()     : {}", p.root_path().string());
    std::println("root_name()     : {}", p.root_name().string());
    std::println("root_directory() : {}", p.root_directory().string());
    std::println("relative_path() : {}", p.relative_path().string());
    std::println("parent_path()   : {}", p.parent_path().string());
    std::println("filename()      : {}", p.filename().string());
    std::println("stem()          : {}", p.stem().string());
    std::println("extension()     : {}", p.extension().string());
}
```

Sortie sur Linux :

```
Chemin complet  : /home/user/projet/build/app.tar.gz  
root_path()     : /  
root_name()     :  
root_directory() : /  
relative_path() : home/user/projet/build/app.tar.gz  
parent_path()   : /home/user/projet/build  
filename()      : app.tar.gz  
stem()          : app.tar  
extension()     : .gz  
```

Notez que `root_name()` est vide sur Linux (il est utilisé sur Windows pour le préfixe de lecteur, par exemple `C:`). Notez également le comportement de `stem()` et `extension()` sur un fichier à double extension : `stem()` retourne `app.tar` et `extension()` retourne `.gz` (seule la dernière extension est considérée).

### Concaténation de chemins avec `/`

L'opérateur `/` est surchargé pour `fs::path`, ce qui permet de construire des chemins de manière expressive et lisible :

```cpp
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    fs::path base = "/home/user/projet";
    fs::path src = base / "src";
    fs::path main_file = src / "main.cpp";

    std::println("{}", main_file.string());
    // /home/user/projet/src/main.cpp

    // append() est l'équivalent méthode de l'opérateur /
    fs::path config = base;
    config /= "config";
    config /= "settings.yaml";
    std::println("{}", config.string());
    // /home/user/projet/config/settings.yaml
}
```

L'opérateur `/` gère automatiquement les séparateurs. Vous n'avez jamais besoin de vérifier si le chemin de gauche se termine par `/` ou non.

Il existe aussi `concat()` (ou l'opérateur `+=`), qui concatène sans ajouter de séparateur. La distinction est importante :

```cpp
fs::path p = "/home/user/fichier";

// operator/ : ajoute un séparateur
auto a = p / ".bak";  
std::println("{}", a.string());  
// /home/user/fichier/.bak  ← un répertoire "fichier" avec un fichier ".bak"

// operator+= : concatène directement
auto b = p;  
b += ".bak";  
std::println("{}", b.string());  
// /home/user/fichier.bak  ← le résultat attendu pour ajouter une extension
```

### Normalisation avec `lexically_normal()`

Les chemins peuvent contenir des composantes redondantes (`.`, `..`, doubles séparateurs). La méthode `lexically_normal()` produit un chemin normalisé **sans accéder au disque** :

```cpp
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    fs::path p = "/home/user/./projet/../projet/src//main.cpp";
    std::println("Brut       : {}", p.string());
    std::println("Normalisé  : {}", p.lexically_normal().string());
    // Normalisé  : /home/user/projet/src/main.cpp
}
```

La famille des méthodes `lexically_*` effectue des transformations purement syntaxiques. Pour une résolution qui interroge réellement le filesystem (résolution de liens symboliques, par exemple), il faut utiliser `fs::canonical()` ou `fs::weakly_canonical()`, présentées plus loin.

### Chemins relatifs avec `lexically_relative()`

Cette méthode calcule un chemin relatif d'un path par rapport à un autre, sans accéder au disque :

```cpp
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    fs::path fichier = "/home/user/projet/src/main.cpp";
    fs::path base = "/home/user/projet";

    std::println("{}", fichier.lexically_relative(base).string());
    // src/main.cpp

    std::println("{}", base.lexically_relative(fichier).string());
    // ../..
}
```

---

## Interroger le filesystem

Au-delà de la manipulation syntaxique des chemins, `std::filesystem` fournit des **fonctions libres** (free functions) qui interrogent réellement le système de fichiers. Ces fonctions effectuent des appels système sous le capot.

### Vérifier l'existence et le type

```cpp
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    fs::path p = "/etc/hostname";

    // Existence
    std::println("Existe : {}", fs::exists(p));

    // Type
    std::println("Fichier régulier  : {}", fs::is_regular_file(p));
    std::println("Répertoire        : {}", fs::is_directory(p));
    std::println("Lien symbolique   : {}", fs::is_symlink(p));
    std::println("Vide              : {}", fs::is_empty(p));
}
```

Les fonctions de test les plus courantes sont `exists()`, `is_regular_file()`, `is_directory()`, `is_symlink()`, `is_empty()`, `is_block_file()`, `is_character_file()`, `is_fifo()` et `is_socket()`. Ces deux dernières sont particulièrement utiles sur Linux pour identifier les pipes nommés et les sockets Unix.

### Taille et métadonnées

```cpp
#include <filesystem>
#include <print>
#include <chrono>

namespace fs = std::filesystem;

int main() {
    fs::path p = "/etc/hostname";

    if (fs::exists(p) && fs::is_regular_file(p)) {
        // Taille en octets
        auto taille = fs::file_size(p);
        std::println("Taille : {} octets", taille);

        // Dernière modification
        auto ftime = fs::last_write_time(p);

        // Espace disque
        auto info = fs::space("/");
        std::println("Espace total     : {} Go", info.capacity / (1024*1024*1024));
        std::println("Espace libre     : {} Go", info.free / (1024*1024*1024));
        std::println("Espace dispo     : {} Go", info.available / (1024*1024*1024));
    }
}
```

`fs::space()` retourne une structure `fs::space_info` avec trois champs : `capacity`, `free` et `available`. La distinction entre `free` et `available` reflète la réserve de blocs typique des systèmes de fichiers Linux (par défaut, ext4 réserve 5 % de l'espace pour le super-utilisateur).

### Résolution canonique

```cpp
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    // canonical() résout les liens symboliques et normalise
    // Le chemin DOIT exister, sinon une exception est levée
    fs::path canon = fs::canonical("/usr/bin/python3");
    std::println("Canonical : {}", canon.string());

    // weakly_canonical() tolère que le chemin n'existe pas entièrement
    // Résout ce qui existe, normalise le reste syntaxiquement
    fs::path wcanon = fs::weakly_canonical("/home/user/projet/../nouveau/fichier.txt");
    std::println("Weakly canonical : {}", wcanon.string());

    // current_path() retourne le répertoire de travail courant
    std::println("CWD : {}", fs::current_path().string());
}
```

---

## Gestion des erreurs : deux approches systématiques

Un aspect fondamental de `std::filesystem` est sa gestion d'erreurs duale. La quasi-totalité des fonctions qui interagissent avec le disque existent en **deux surcharges** :

1. **Sans `std::error_code`** — lance une `fs::filesystem_error` en cas d'échec.
2. **Avec `std::error_code&`** — ne lance pas d'exception, écrit le code d'erreur dans le paramètre.

```cpp
#include <filesystem>
#include <print>
#include <system_error>

namespace fs = std::filesystem;

void approche_exceptions() {
    try {
        auto taille = fs::file_size("/chemin/inexistant");
        std::println("Taille : {}", taille);
    } catch (const fs::filesystem_error& e) {
        std::println("Erreur filesystem : {}", e.what());
        std::println("  path1 : {}", e.path1().string());
        std::println("  code  : {}", e.code().message());
    }
}

void approche_error_code() {
    std::error_code ec;
    auto taille = fs::file_size("/chemin/inexistant", ec);

    if (ec) {
        std::println("Erreur : {} (code {})", ec.message(), ec.value());
    } else {
        std::println("Taille : {}", taille);
    }
}

int main() {
    approche_exceptions();
    approche_error_code();
}
```

### Quelle approche choisir ?

Le choix entre les deux n'est pas une question de préférence personnelle. Il dépend du contexte :

**L'approche par exceptions** convient lorsque l'erreur est véritablement exceptionnelle — un fichier de configuration critique introuvable au démarrage, un répertoire de travail inaccessible. L'échec signifie que le programme ne peut pas continuer normalement. C'est aussi la forme la plus concise quand on enchaîne plusieurs opérations filesystem dans un même bloc `try`.

**L'approche par `error_code`** est préférable dans les boucles et les parcours (où un fichier inaccessible parmi des milliers ne doit pas interrompre le traitement), dans le code critique en performance (les exceptions ont un coût au lancement), et dans les contextes où les erreurs sont attendues et gérées localement (vérifier si un fichier optionnel existe avant de le lire).

Dans le code de cette formation, les deux approches seront utilisées selon le contexte. La règle générale est : si vous parcourez une arborescence ou testez des conditions, utilisez `error_code` ; si vous effectuez une opération qui *doit* réussir pour que le programme ait un sens, laissez l'exception se propager.

---

## Les opérations fondamentales

Voici un panorama des opérations les plus courantes. Chacune sera approfondie dans les sous-sections suivantes (19.1.1 à 19.1.3), mais cette vue d'ensemble permet de saisir l'étendue de l'API.

### Création

```cpp
namespace fs = std::filesystem;

// Créer un répertoire (un seul niveau)
fs::create_directory("/tmp/mon_projet");

// Créer une arborescence complète (équivalent mkdir -p)
fs::create_directories("/tmp/mon_projet/build/release");

// Pas de fonction create_file() — on utilise les flux C++ standard
// pour créer un fichier en l'ouvrant en écriture
#include <fstream>
std::ofstream{"/tmp/mon_projet/config.yaml"};
```

Un point qui surprend souvent : `std::filesystem` ne fournit pas de fonction pour **créer** un fichier vide ni pour **écrire** dans un fichier. La lecture et l'écriture du contenu restent du domaine de `<fstream>` (API C++ standard) ou des appels POSIX (section 19.2). `std::filesystem` gère l'**arborescence et les métadonnées**, pas le contenu.

### Copie, déplacement, suppression

```cpp
namespace fs = std::filesystem;

// Copier un fichier
fs::copy_file("source.txt", "dest.txt");

// Copier avec écrasement si le fichier destination existe
fs::copy_file("source.txt", "dest.txt", fs::copy_options::overwrite_existing);

// Copier un répertoire entier (récursif)
fs::copy("src_dir", "dst_dir", fs::copy_options::recursive);

// Renommer / déplacer
fs::rename("ancien_nom.txt", "nouveau_nom.txt");  
fs::rename("fichier.txt", "/tmp/archive/fichier.txt");  

// Supprimer un fichier ou un répertoire vide
fs::remove("/tmp/fichier_temp.txt");

// Supprimer un répertoire et tout son contenu (récursif)
auto nb = fs::remove_all("/tmp/mon_projet/build");  
std::println("{} éléments supprimés", nb);  
```

`fs::copy_options` est un type enum bitmask qui permet de contrôler finement le comportement de la copie : `skip_existing`, `overwrite_existing`, `update_existing`, `recursive`, `copy_symlinks`, `directories_only`, entre autres. Ces options se combinent avec l'opérateur `|`.

### Liens symboliques et statut

```cpp
namespace fs = std::filesystem;

// Créer un lien symbolique
fs::create_symlink("/usr/bin/python3.12", "/tmp/python");

// Créer un lien symbolique vers un répertoire
fs::create_directory_symlink("/var/log", "/tmp/logs_link");

// Lire la cible d'un lien symbolique
fs::path cible = fs::read_symlink("/tmp/python");

// Obtenir le statut (suit les liens symboliques)
fs::file_status st = fs::status("/tmp/python");  
std::println("Type : fichier régulier = {}", st.type() == fs::file_type::regular);  

// Obtenir le statut sans suivre les liens (symlink_status)
fs::file_status lst = fs::symlink_status("/tmp/python");  
std::println("Type : lien symbolique = {}", lst.type() == fs::file_type::symlink);  
```

La distinction entre `fs::status()` et `fs::symlink_status()` est l'équivalent C++ de la distinction entre `stat()` et `lstat()` en POSIX. Elle est essentielle lorsque vous parcourez des arborescences contenant des liens symboliques.

---

## Récapitulatif de l'API

| Catégorie | Fonctions principales |
|---|---|
| **Chemins** | `path`, `operator/`, `lexically_normal()`, `lexically_relative()` |
| **Tests** | `exists()`, `is_regular_file()`, `is_directory()`, `is_symlink()`, `is_empty()` |
| **Métadonnées** | `file_size()`, `last_write_time()`, `space()`, `status()` |
| **Création** | `create_directory()`, `create_directories()`, `create_symlink()` |
| **Copie/Déplacement** | `copy()`, `copy_file()`, `rename()` |
| **Suppression** | `remove()`, `remove_all()` |
| **Résolution** | `canonical()`, `weakly_canonical()`, `current_path()`, `read_symlink()` |
| **Parcours** | `directory_iterator`, `recursive_directory_iterator` *(section 19.1.1)* |
| **Permissions** | `permissions()`, `status()` *(section 19.4)* |

---

## Performance et considérations système

Chaque fonction de `std::filesystem` qui interroge le disque effectue un ou plusieurs **appels système** sous le capot. Par exemple, `fs::exists()` appelle `stat()`, `fs::file_size()` appelle aussi `stat()`, et `fs::is_directory()` fait de même. Si vous enchaînez ces vérifications sur le même fichier, vous effectuez plusieurs appels système redondants :

```cpp
// ❌ Trois appels stat() pour le même fichier
if (fs::exists(p) && fs::is_regular_file(p) && !fs::is_empty(p)) {
    auto taille = fs::file_size(p);  // Encore un stat()
}

// ✅ Un seul appel stat(), puis interrogation du résultat
std::error_code ec;  
auto st = fs::status(p, ec);  
if (!ec && fs::is_regular_file(st)) {  
    auto taille = fs::file_size(p);  // Inévitable : file_size n'accepte pas un status
}
```

Ce n'est pas une micro-optimisation théorique : lors du parcours de milliers de fichiers (logs, artefacts de build, répertoires de cache), réduire les appels système a un impact mesurable. Nous reviendrons sur ces aspects en section 19.1.1 lors de l'utilisation de `directory_entry`, qui met en cache les résultats de `stat()`.

---

## Ce qui vient ensuite

Les sous-sections suivantes approfondissent les trois grands cas d'usage de `std::filesystem` :

- **19.1.1 — Parcours de répertoires** : `directory_iterator`, `recursive_directory_iterator`, filtrage et patterns.
- **19.1.2 — Manipulation de chemins** : techniques avancées de construction, normalisation et résolution.
- **19.1.3 — Opérations sur fichiers et répertoires** : copie, déplacement, suppression, fichiers temporaires et gestion atomique.

> 💡 **Note** — `std::filesystem` est l'un des meilleurs exemples de la philosophie du C++ moderne : offrir des abstractions de haut niveau sans sacrifier la possibilité de descendre au niveau système quand c'est nécessaire. L'API C++17 couvre 90 % des besoins courants. Pour les 10 % restants (I/O non-bloquantes, `inotify`, `sendfile`, contrôle fin des permissions ACL…), les appels POSIX de la section 19.2 prennent le relais.

⏭️ [Parcours de répertoires](/19-systeme-fichiers/01.1-parcours-repertoires.md)
