🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 17.1 — Syntaxe : `try`, `catch`, `throw`

## Chapitre 17 — Exceptions et Gestion d'Erreurs · Module 6

---

## Vue d'ensemble

Le mécanisme d'exceptions de C++ repose sur trois mots-clés : `throw` pour signaler une erreur, `try` pour délimiter une zone de code surveillée, et `catch` pour intercepter et traiter l'erreur. Ces trois éléments forment un système de **transfert de contrôle non local** : lorsqu'une exception est levée, l'exécution quitte immédiatement la fonction courante — et potentiellement plusieurs fonctions parentes — jusqu'à atteindre un handler `catch` compatible.

Cette section couvre la syntaxe de ces trois mots-clés, le mécanisme de stack unwinding qui les sous-tend, les différentes formes de capture, et les bonnes pratiques pour structurer un code robuste face aux erreurs.

---

## `throw` : signaler une erreur

### Syntaxe de base

L'instruction `throw` lève (ou *lance*) une exception. Elle accepte n'importe quelle expression dont le type est copiable ou déplaçable — en pratique, on lance presque toujours un objet de type dérivé de `std::exception`.

```cpp
#include <stdexcept>
#include <string>

double diviser(double numerateur, double denominateur) {
    if (denominateur == 0.0) {
        throw std::invalid_argument("Division par zéro");
    }
    return numerateur / denominateur;
}
```

Au moment du `throw`, plusieurs choses se produisent dans cet ordre :

1. L'objet exception est **construit** et copié (ou déplacé) dans une zone mémoire spéciale gérée par le runtime — il ne réside pas sur la pile de la fonction qui lance.
2. L'exécution de la fonction courante est **interrompue immédiatement** : les instructions qui suivent le `throw` ne sont jamais exécutées.
3. Le mécanisme de **stack unwinding** démarre (détaillé plus bas).

### On peut lancer n'importe quel type

Techniquement, C++ permet de lancer un `int`, un `const char*`, un `std::string`, ou tout autre type. Le langage n'impose aucune contrainte sur le type lancé.

```cpp
// Légal, mais fortement déconseillé
throw 42;  
throw "erreur critique";  
throw std::string("oops");  
```

En pratique, **lancez toujours des objets dérivés de `std::exception`**. Les raisons sont multiples : un `catch(const std::exception&)` peut capturer uniformément toutes les erreurs de la bibliothèque standard et les vôtres, la méthode `what()` fournit un message lisible, et le code devient cohérent et prévisible pour toute l'équipe. Lancer un entier ou une chaîne brute rend le code difficile à maintenir et impossible à capturer de manière générique.

### `throw` sans opérande : relancer l'exception courante

Dans un bloc `catch`, l'instruction `throw;` (sans opérande) **relance l'exception en cours de traitement** sans la copier. C'est le mécanisme standard pour intercepter une exception, effectuer un traitement partiel (journalisation, nettoyage), puis la propager à l'appelant.

```cpp
void traiter_requete() {
    try {
        executer_requete_sql();
    } catch (const std::exception& e) {
        journaliser("Échec SQL : " + std::string(e.what()));
        throw;  // relance l'exception originale — y compris son type dynamique
    }
}
```

Un point subtil mais important : `throw;` préserve le **type dynamique** de l'exception. Si l'exception originale est de type `std::runtime_error` (dérivé de `std::exception`), c'est bien un `std::runtime_error` qui est relancé, même si le `catch` l'a capturée via une référence à `std::exception`. En revanche, écrire `throw e;` dans le même contexte provoquerait un **slicing** : l'objet serait copié en tant que `std::exception`, perdant toute information spécifique au type dérivé. Cette distinction est une source fréquente de bugs.

```cpp
catch (const std::exception& e) {
    throw;    // ✅ Relance le type dynamique original (ex: std::runtime_error)
    // throw e; // ❌ Slicing : relance une copie de type std::exception
}
```

---

## `try` / `catch` : intercepter une exception

### Structure syntaxique

Un bloc `try` délimite une zone de code dont les exceptions seront interceptées par un ou plusieurs blocs `catch` qui le suivent immédiatement.

```cpp
try {
    // Code susceptible de lever une exception
    auto resultat = diviser(10.0, 0.0);
    std::print("Résultat : {}\n", resultat);
}
catch (const std::invalid_argument& e) {
    std::print(stderr, "Argument invalide : {}\n", e.what());
}
catch (const std::exception& e) {
    std::print(stderr, "Erreur inattendue : {}\n", e.what());
}
```

Plusieurs règles gouvernent la résolution des `catch` :

- Les blocs `catch` sont évalués **dans l'ordre de déclaration**, de haut en bas.
- Le premier `catch` dont le type est compatible avec l'exception levée est sélectionné.
- La compatibilité suit les règles d'héritage : un `catch(const std::exception&)` capture toute exception dérivée de `std::exception`.
- **Une seule** clause `catch` est exécutée par exception.

### Ordre des clauses `catch` : du plus spécifique au plus général

L'ordre des blocs `catch` est crucial. Puisque le premier handler compatible est choisi, placer un type de base avant un type dérivé rend ce dernier **inatteignable**.

```cpp
// ❌ INCORRECT : le catch de base masque le catch dérivé
try {
    ouvrir_fichier("data.csv");
}
catch (const std::exception& e) {
    // Capture TOUTES les exceptions dérivées de std::exception
    // Le catch suivant ne sera jamais atteint
}
catch (const std::runtime_error& e) {
    // ⚠️ Code mort — jamais exécuté
}
```

Le compilateur (GCC, Clang) émet un avertissement dans ce cas, mais il ne s'agit pas d'une erreur de compilation. La bonne pratique est simple : **ordonnez toujours vos clauses `catch` du type le plus dérivé au type le plus général**.

```cpp
// ✅ CORRECT : du plus spécifique au plus général
try {
    ouvrir_fichier("data.csv");
}
catch (const std::ios_base::failure& e) {
    // Erreurs spécifiques aux flux I/O
    std::print(stderr, "Erreur I/O : {}\n", e.what());
}
catch (const std::runtime_error& e) {
    // Erreurs d'exécution générales
    std::print(stderr, "Erreur runtime : {}\n", e.what());
}
catch (const std::exception& e) {
    // Filet de sécurité pour toute exception standard
    std::print(stderr, "Erreur : {}\n", e.what());
}
```

### Capture par référence constante : la règle d'or

Capturez **toujours** les exceptions par référence constante (`const&`). Cette pratique combine trois avantages :

- **Pas de copie** : l'objet exception réside dans une zone mémoire spéciale du runtime ; la référence y accède directement.
- **Pas de slicing** : la référence préserve le type dynamique de l'exception, ce qui est indispensable pour accéder aux méthodes des classes dérivées et pour que `throw;` fonctionne correctement.
- **Sémantique de lecture** : `const` exprime que le handler inspecte l'exception sans la modifier, ce qui est le cas courant.

```cpp
catch (const std::runtime_error& e) {  // ✅ Référence constante
    std::print(stderr, "{}\n", e.what());
}

// catch (std::runtime_error e) {       // ❌ Copie + slicing potentiel
// catch (std::runtime_error* e) {      // ❌ Ne fonctionne que si throw &obj (dangereux)
```

La capture par référence non constante (`catch (std::exception& e)`) est parfois nécessaire si vous devez modifier l'objet exception avant de le relancer, mais ces cas sont rares.

### Le catch-all : `catch(...)`

La syntaxe `catch(...)` — avec des points de suspension — capture **toute exception**, quel que soit son type, y compris les types non dérivés de `std::exception` (entiers, chaînes brutes, exceptions provenant de bibliothèques C tierces).

```cpp
try {
    appeler_librairie_tierce();
}
catch (const std::exception& e) {
    std::print(stderr, "Exception standard : {}\n", e.what());
}
catch (...) {
    // Capture tout ce qui n'est pas std::exception
    std::print(stderr, "Exception inconnue capturée\n");
    throw;  // Relance — ne pas avaler silencieusement
}
```

Le catch-all est utile dans deux contextes principaux : aux frontières d'un système (pour empêcher une exception de traverser une API C ou un callback) et dans les fonctions `main` ou les threads de plus haut niveau (pour garantir une terminaison propre). En dehors de ces cas, préférez toujours des captures typées qui vous donnent accès à l'information d'erreur.

> ⚠️ **Ne jamais avaler silencieusement une exception.** Un `catch(...)` vide — sans journalisation ni relance — masque les erreurs et rend le débogage quasi impossible. Si vous capturez une exception, vous devez soit la traiter, soit la journaliser et la relancer.

---

## Le stack unwinding en détail

Le stack unwinding (déroulement de pile) est le processus par lequel le runtime C++ remonte la pile d'appels lors d'une exception, en détruisant les objets locaux de chaque frame traversée. C'est le mécanisme qui relie les exceptions au RAII.

### Déroulement séquentiel

Considérons l'enchaînement d'appels suivant :

```cpp
#include <stdexcept>
#include <print>
#include <string>

struct Trace {
    std::string nom;
    Trace(std::string n) : nom(std::move(n)) {
        std::print("  Construction de {}\n", nom);
    }
    ~Trace() {
        std::print("  Destruction de {}\n", nom);
    }
};

void fonction_c() {
    Trace t3("t3 (fonction_c)");
    std::print("→ throw dans fonction_c\n");
    throw std::runtime_error("Erreur dans fonction_c");
    // Les instructions après throw ne sont jamais exécutées
}

void fonction_b() {
    Trace t2("t2 (fonction_b)");
    fonction_c();
    std::print("Après appel à fonction_c\n");  // jamais exécuté
}

void fonction_a() {
    Trace t1("t1 (fonction_a)");
    try {
        fonction_b();
    }
    catch (const std::runtime_error& e) {
        std::print("Exception capturée dans fonction_a : {}\n", e.what());
    }
}
```

L'appel à `fonction_a()` produit la sortie suivante :

```
  Construction de t1 (fonction_a)
  Construction de t2 (fonction_b)
  Construction de t3 (fonction_c)
→ throw dans fonction_c
  Destruction de t3 (fonction_c)
  Destruction de t2 (fonction_b)
Exception capturée dans fonction_a : Erreur dans fonction_c
  Destruction de t1 (fonction_a)
```

Le déroulement se lit clairement : après le `throw`, les destructeurs de `t3` puis `t2` sont appelés dans l'ordre inverse de construction — c'est le stack unwinding qui traverse `fonction_c` puis `fonction_b`. Le `catch` dans `fonction_a` arrête le déroulement. Enfin, `t1` est détruit normalement à la sortie du scope de `fonction_a`.

### Ce qui est détruit — et ce qui ne l'est pas

Le stack unwinding garantit la destruction de **tous les objets locaux dont la construction a été achevée** dans les frames traversées. Cela inclut les variables automatiques (sur la pile), les objets temporaires, et les sous-objets déjà construits d'un objet en cours de construction.

En revanche, le stack unwinding **ne libère pas** la mémoire allouée avec `new` si aucun destructeur n'en est responsable. C'est la raison fondamentale pour laquelle le chapitre 9 insiste sur la règle : **ne jamais utiliser `new`/`delete` directement dans du code moderne**. Les smart pointers (`std::unique_ptr`, `std::shared_ptr`) encapsulent l'allocation et leur destructeur libère la mémoire — le stack unwinding fait alors le reste.

```cpp
void fuite_memoire() {
    int* p = new int(42);      // allocation brute
    throw std::runtime_error("oops");
    delete p;                  // ❌ jamais exécuté → fuite mémoire
}

void pas_de_fuite() {
    auto p = std::make_unique<int>(42);  // RAII
    throw std::runtime_error("oops");
    // ✅ ~unique_ptr() libère la mémoire pendant le stack unwinding
}
```

### Exception pendant le stack unwinding : `std::terminate`

Que se passe-t-il si un destructeur lance une exception alors que le stack unwinding est déjà en cours (suite à une première exception) ? La réponse est brutale : `std::terminate()` est appelé, ce qui met fin immédiatement au programme sans autre forme de nettoyage.

C'est pourquoi une règle absolue du C++ est que **les destructeurs ne doivent jamais lancer d'exceptions**. Depuis C++11, les destructeurs sont implicitement `noexcept` — si un destructeur lance malgré tout, `std::terminate` est appelé même en dehors d'un contexte de stack unwinding.

```cpp
struct Dangereux {
    ~Dangereux() noexcept(false) {  // ❌ Mauvaise idée
        throw std::runtime_error("Exception dans le destructeur");
    }
};

void catastrophe() {
    try {
        Dangereux d;
        throw std::logic_error("Première exception");
        // → stack unwinding → ~Dangereux() lance → deux exceptions actives
        // → std::terminate() → fin du programme
    }
    catch (...) {
        // Jamais atteint
    }
}
```

Si un destructeur doit effectuer une opération susceptible d'échouer (écriture de journal, flush d'un buffer), cette opération doit être encapsulée dans un `try`/`catch` interne qui absorbe l'erreur — typiquement en la journalisant sans la propager.

---

## `try` dans des contextes particuliers

### Function-try-block : protéger la liste d'initialisation

Un cas particulier souvent méconnu est le **function-try-block**, qui permet d'entourer l'intégralité d'une fonction — y compris la liste d'initialisation d'un constructeur — d'un bloc `try`. C'est le seul moyen de capturer une exception levée par l'initialisation d'un membre.

```cpp
#include <stdexcept>
#include <string>

class Connexion {  
public:  
    Connexion(const std::string& url) {
        if (url.empty()) throw std::invalid_argument("URL vide");
        // ... établir la connexion ...
    }
};

class Service {
    Connexion conn_;

public:
    Service(const std::string& url)
    try : conn_(url)  // ← le try englobe la liste d'initialisation
    {
        // Corps du constructeur
    }
    catch (const std::invalid_argument& e) {
        std::print(stderr, "Échec initialisation Service : {}\n", e.what());
        // Note : l'exception est automatiquement relancée à la sortie du catch
        // car l'objet Service ne peut pas exister dans un état valide
    }
};
```

Un point important : dans un function-try-block de constructeur, l'exception est **automatiquement relancée** à la fin du bloc `catch` si vous ne lancez pas explicitement une autre exception. C'est logique : si la liste d'initialisation échoue, l'objet ne peut pas être construit, et le code appelant doit en être informé.

### `try`/`catch` dans `main` : filet de sécurité global

Envelopper le contenu de `main` dans un `try`/`catch` est une pratique courante pour garantir qu'aucune exception ne termine le programme de manière incontrôlée (via `std::terminate`).

```cpp
int main() {
    try {
        Application app;
        return app.run();
    }
    catch (const std::exception& e) {
        std::print(stderr, "Erreur fatale : {}\n", e.what());
        return 1;
    }
    catch (...) {
        std::print(stderr, "Erreur fatale inconnue\n");
        return 2;
    }
}
```

Ce filet de sécurité vous permet de journaliser proprement l'erreur et de retourner un code de sortie approprié au système d'exploitation, plutôt que de laisser `std::terminate` produire un message cryptique.

---

## Bonnes pratiques et pièges courants

### Lancer des temporaires, pas des variables locales

Lancez toujours l'exception sous forme de temporaire anonyme. Ne déclarez pas une variable locale pour la lancer ensuite — cela ajoute une copie inutile et nuit à la lisibilité.

```cpp
// ✅ Temporaire — clair et direct
throw std::runtime_error("Connexion perdue");

// ❌ Variable locale inutile
std::runtime_error err("Connexion perdue");  
throw err;  // copie supplémentaire  
```

### Ne pas utiliser les exceptions pour le flux de contrôle

Les exceptions sont conçues pour les **situations exceptionnelles**, pas pour les branchements ordinaires. Utiliser `throw`/`catch` comme substitut à un `if`/`else` ou comme mécanisme de sortie de boucle est un anti-pattern qui dégrade les performances (le coût du lancement d'exception est élevé) et obscurcit la logique du programme.

```cpp
// ❌ Anti-pattern : exception comme flux de contrôle
try {
    while (true) {
        auto valeur = file.lire_suivant();  // lance si fin de fichier
        traiter(valeur);
    }
}
catch (const FinDeFichier&) {
    // "normal" — la boucle est finie
}

// ✅ Approche correcte : tester la condition normalement
while (auto valeur = file.lire_suivant()) {  // retourne std::optional
    traiter(*valeur);
}
```

### Capturer au bon niveau d'abstraction

Un piège fréquent chez les débutants consiste à placer des `try`/`catch` autour de chaque appel de fonction. Cela produit un code verbeux et fragile. La bonne pratique est de capturer les exceptions **au niveau où vous avez suffisamment de contexte pour prendre une décision pertinente** : réessayer l'opération, afficher un message à l'utilisateur, renvoyer une réponse d'erreur HTTP, ou journaliser et terminer proprement.

```cpp
// ❌ Trop granulaire — chaque appel est enveloppé
void traiter_commande(const Commande& cmd) {
    try { valider(cmd); }
    catch (...) { /* ... */ }

    try { enregistrer(cmd); }
    catch (...) { /* ... */ }

    try { notifier(cmd); }
    catch (...) { /* ... */ }
}

// ✅ Capture au niveau approprié — là où la décision de récupération a du sens
void traiter_commande(const Commande& cmd) {
    valider(cmd);       // propage si invalide
    enregistrer(cmd);   // propage si échec base de données
    notifier(cmd);      // propage si échec réseau
}

// L'appelant décide quoi faire en cas d'échec
void boucle_traitement(std::queue<Commande>& file) {
    while (!file.empty()) {
        try {
            traiter_commande(file.front());
            file.pop();
        }
        catch (const std::exception& e) {
            journaliser_erreur(e, file.front());
            file.pop();  // passe à la commande suivante
        }
    }
}
```

### Ne jamais laisser une exception traverser une frontière C

Les callbacks C (signaux POSIX, callbacks de bibliothèques C comme SQLite ou libcurl) ne comprennent pas les exceptions C++. Si une exception traverse une frontière `extern "C"`, le comportement est **indéfini** — en pratique, le programme crashe sans stack unwinding.

```cpp
// Callback enregistré auprès d'une bibliothèque C
extern "C" void mon_callback(void* data) {
    try {
        auto* obj = static_cast<MonObjet*>(data);
        obj->traiter();  // peut lancer
    }
    catch (const std::exception& e) {
        // Journaliser, stocker l'erreur, mais ne pas propager
        std::print(stderr, "Erreur dans callback : {}\n", e.what());
    }
    catch (...) {
        std::print(stderr, "Erreur inconnue dans callback\n");
    }
}
```

---

## Résumé

Le mécanisme d'exceptions de C++ repose sur une interaction étroite entre `throw`, `try`/`catch`, et le stack unwinding. Les règles essentielles à retenir sont les suivantes :

- **`throw`** lève une exception ; préférez toujours un type dérivé de `std::exception`.
- **`throw;`** (sans opérande) relance l'exception en cours sans slicing — ne confondez pas avec `throw e;`.
- **`catch`** intercepte par type ; ordonnez les clauses du plus dérivé au plus général.
- **Capturez par `const&`** systématiquement pour éviter copies et slicing.
- **Le stack unwinding** détruit les objets locaux dans l'ordre inverse de construction — c'est la raison pour laquelle RAII et smart pointers sont indispensables.
- **Les destructeurs ne doivent jamais lancer** d'exception : deux exceptions simultanées provoquent `std::terminate`.
- **Capturez au bon niveau** d'abstraction, là où vous pouvez prendre une décision de récupération pertinente.
- **Ne traversez jamais** une frontière C avec une exception non capturée.

---

> 📎 *La section suivante (17.2) présente la hiérarchie des exceptions standard de la bibliothèque C++, pour vous aider à choisir le type d'exception approprié à chaque situation.*

⏭️ [Hiérarchie des exceptions standard (std::exception)](/17-exceptions/02-hierarchie-exceptions.md)
