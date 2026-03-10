🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 5.2 — Allocation dynamique : new/delete, new[]/delete[]

## Le contrat fondamental

L'allocation dynamique en C++ repose sur un contrat simple et impitoyable : **chaque `new` doit être équilibré par exactement un `delete`**, et **chaque `new[]` par exactement un `delete[]`**. Pas de `delete` → fuite mémoire. Deux `delete` → comportement indéfini. Mauvaise forme de `delete` → comportement indéfini. Il n'y a aucune tolérance, aucun filet de sécurité intégré au langage.

Ce contrat est la raison pour laquelle le C++ moderne déconseille fortement l'utilisation directe de `new` et `delete` (chapitre 9). Mais avant d'adopter les smart pointers, il faut comprendre ce qu'ils encapsulent. C'est l'objet de cette section.

> ⚠️ **Rappel pédagogique :** tout le code de cette section utilise `new` et `delete` bruts pour des raisons d'apprentissage. En code de production, préférez systématiquement `std::make_unique`, `std::make_shared` et les conteneurs STL.

---

## Allocation et libération d'un objet unique

### Syntaxe de base

L'opérateur `new` alloue de la mémoire sur le heap, construit un objet à cet emplacement, et retourne un pointeur vers l'objet :

```cpp
// Allocation + construction
int* p = new int;          // alloue un int non initialisé sur le heap  
int* q = new int(42);      // alloue un int initialisé à 42  
int* r = new int{42};      // idem, syntaxe d'initialisation uniforme (C++11)  

// Utilisation
std::cout << *q << "\n";   // affiche 42

// Libération + destruction
delete p;  
delete q;  
delete r;  
```

L'opérateur `delete` appelle le destructeur de l'objet (trivial pour un `int`, mais crucial pour des types complexes), puis libère la mémoire.

### Avec des types utilisateur

L'intérêt de `new` apparaît pleinement avec des objets dont le constructeur effectue un travail réel :

```cpp
#include <string>
#include <iostream>

class Connexion {  
public:  
    Connexion(const std::string& hote, int port)
        : hote_(hote), port_(port)
    {
        std::cout << "Connexion ouverte vers " << hote_ << ":" << port_ << "\n";
    }

    ~Connexion() {
        std::cout << "Connexion fermée (" << hote_ << ":" << port_ << ")\n";
    }

    void envoyer(const std::string& message) {
        std::cout << "Envoi: " << message << "\n";
    }

private:
    std::string hote_;
    int port_;
};

int main() {
    // new appelle le constructeur Connexion("serveur.local", 8080)
    Connexion* conn = new Connexion("serveur.local", 8080);

    conn->envoyer("ping");

    // delete appelle le destructeur ~Connexion(), puis libère la mémoire
    delete conn;

    return 0;
}
```

Sortie :

```
Connexion ouverte vers serveur.local:8080  
Envoi: ping  
Connexion fermée (serveur.local:8080)  
```

Le `delete` ne se contente pas de libérer des octets — il **appelle le destructeur** avant de rendre la mémoire à l'allocateur. C'est une distinction cruciale par rapport au `free()` du C, qui ne fait que libérer la mémoire brute sans destruction d'objet.

---

## Allocation et libération de tableaux

### Syntaxe avec new[] et delete[]

Pour allouer un tableau d'objets sur le heap, on utilise la forme tableau de `new`, notée `new[]`. La libération correspondante **doit** utiliser `delete[]` :

```cpp
// Allocation d'un tableau de 5 doubles
double* temperatures = new double[5];

// Initialisation manuelle
temperatures[0] = 18.5;  
temperatures[1] = 20.3;  
temperatures[2] = 22.1;  
temperatures[3] = 19.7;  
temperatures[4] = 21.0;  

// Utilisation
for (int i = 0; i < 5; ++i) {
    std::cout << temperatures[i] << " ";
}
std::cout << "\n";

// Libération — OBLIGATOIREMENT delete[], pas delete
delete[] temperatures;
```

Depuis C++11, on peut aussi initialiser un tableau alloué dynamiquement :

```cpp
int* valeurs = new int[4]{10, 20, 30, 40};  // initialisation à la déclaration  
int* zeros = new int[100]();                  // tous initialisés à 0  
// Attention : new int[100] sans () laisse les valeurs non initialisées

delete[] valeurs;  
delete[] zeros;  
```

### Avec des objets complexes

Quand `new[]` alloue un tableau d'objets, il appelle le **constructeur par défaut** pour chaque élément. Symétriquement, `delete[]` appelle le **destructeur** de chaque élément, dans l'ordre inverse de construction :

```cpp
#include <iostream>

class Capteur {  
public:  
    Capteur() { std::cout << "  Capteur construit\n"; }
    ~Capteur() { std::cout << "  Capteur détruit\n"; }
};

int main() {
    std::cout << "Allocation de 3 capteurs :\n";
    Capteur* capteurs = new Capteur[3];

    std::cout << "\nLibération :\n";
    delete[] capteurs;

    return 0;
}
```

Sortie :

```
Allocation de 3 capteurs :
  Capteur construit
  Capteur construit
  Capteur construit

Libération :
  Capteur détruit
  Capteur détruit
  Capteur détruit
```

Si la classe n'a pas de constructeur par défaut accessible, `new[]` ne compilera pas — il faudra alors allouer les objets individuellement ou utiliser un `std::vector` qui accepte un constructeur paramétré.

---

## La règle absolue : correspondance new/delete

C'est la règle la plus critique de cette section. La forme de libération **doit correspondre exactement** à la forme d'allocation :

```
new       → delete        ✅  
new[]     → delete[]      ✅  
new       → delete[]      ❌ comportement indéfini  
new[]     → delete        ❌ comportement indéfini  
```

Pourquoi cette distinction ? Quand vous écrivez `new int[5]`, l'allocateur stocke quelque part le nombre d'éléments (typiquement dans les octets précédant le bloc retourné). `delete[]` utilise cette information pour savoir combien de destructeurs appeler. Si vous utilisez `delete` au lieu de `delete[]`, un seul destructeur est appelé, les quatre autres objets ne sont jamais détruits, et l'allocateur reçoit une adresse décalée qui corrompt ses structures internes.

Le résultat est un **comportement indéfini** — le programme peut sembler fonctionner correctement pendant des mois, puis planter mystérieusement un jour sous charge. Ce type de bug est parmi les plus difficiles à diagnostiquer.

```cpp
int* tab = new int[100];

delete tab;      // ❌ COMPORTEMENT INDÉFINI — devrait être delete[]  
delete[] tab;    // ✅ correct  
```

```cpp
int* val = new int(42);

delete[] val;    // ❌ COMPORTEMENT INDÉFINI — devrait être delete  
delete val;      // ✅ correct  
```

> **Astuce :** si vous avez du mal à retenir cette règle, c'est un excellent argument pour ne jamais utiliser `new`/`delete` directement. `std::vector` remplace `new[]`/`delete[]`, et `std::unique_ptr` remplace `new`/`delete`. Dans les deux cas, la correspondance est gérée automatiquement.

---

## delete sur nullptr : sûr et sans effet

Le standard C++ garantit que `delete` et `delete[]` appliqués à `nullptr` ne font **rien** — pas d'erreur, pas de crash, pas de comportement indéfini :

```cpp
int* p = nullptr;  
delete p;        // ✅ sûr — aucun effet  
delete[] p;      // ✅ sûr — aucun effet  
```

C'est une propriété utile qui simplifie le code de nettoyage. Un pattern courant dans le code legacy est d'assigner `nullptr` à un pointeur après `delete` pour éviter un double free accidentel :

```cpp
int* donnees = new int[1000];
// ... utilisation ...
delete[] donnees;  
donnees = nullptr;    // protection contre un éventuel double delete  

// Plus tard dans le code :
delete[] donnees;     // sans danger grâce au nullptr
```

Ce pattern est un palliatif, pas une solution. Il ne protège pas contre les copies du pointeur qui existent ailleurs dans le programme. Les smart pointers (chapitre 9) résolvent ce problème de manière bien plus robuste.

---

## Taille dynamique : le cas d'usage principal

Le scénario le plus courant pour `new[]` est l'allocation d'un tableau dont la taille est déterminée à l'exécution. C'est une situation que la stack ne peut pas gérer de manière portable en C++ :

```cpp
#include <iostream>

void traiter_donnees(int nombre_elements) {
    // La taille dépend d'un paramètre runtime
    double* buffer = new double[nombre_elements];

    // Remplissage
    for (int i = 0; i < nombre_elements; ++i) {
        buffer[i] = i * 1.5;
    }

    // Traitement
    double somme = 0.0;
    for (int i = 0; i < nombre_elements; ++i) {
        somme += buffer[i];
    }

    std::cout << "Somme : " << somme << "\n";

    // Libération obligatoire
    delete[] buffer;
}
```

Ce code fonctionne, mais il souffre d'un problème fondamental : si une exception est lancée entre le `new[]` et le `delete[]`, la mémoire n'est jamais libérée. Considérez cette variante :

```cpp
void traiter_donnees_fragile(int n) {
    double* buffer = new double[n];

    // Si cette fonction lance une exception...
    fonction_qui_peut_echouer();

    // ... cette ligne n'est jamais atteinte → fuite mémoire
    delete[] buffer;
}
```

C'est le problème de la **sécurité face aux exceptions** (*exception safety*). La solution idiomatique en C++ est d'utiliser un `std::vector`, dont le destructeur est automatiquement appelé même en cas d'exception, grâce au déroulement de pile (*stack unwinding*) :

```cpp
void traiter_donnees_robuste(int n) {
    std::vector<double> buffer(n);

    // Même si cette fonction lance une exception,
    // le destructeur de buffer est appelé → pas de fuite
    fonction_qui_peut_echouer();

    // Pas de delete nécessaire
}
```

---

## Ce que new fait réellement : les étapes internes

Pour bien comprendre `new`, il est utile de décomposer ce qu'il fait en interne. L'expression `new MonType(args...)` effectue deux opérations distinctes :

1. **Allocation de mémoire brute** — appel à `operator new(sizeof(MonType))`, qui appelle en interne `malloc()`. Si la mémoire est insuffisante, `std::bad_alloc` est lancée.
2. **Construction de l'objet** — appel du constructeur `MonType(args...)` à l'adresse allouée (*placement construction*).

Symétriquement, `delete ptr` effectue :

1. **Destruction de l'objet** — appel du destructeur `ptr->~MonType()`.
2. **Libération de la mémoire brute** — appel à `operator delete(ptr)`, qui appelle `free()`.

Cette décomposition en deux étapes est la raison pour laquelle `new` est plus qu'un simple `malloc` — et pourquoi mélanger `new`/`free` ou `malloc`/`delete` est un comportement indéfini :

```cpp
#include <cstdlib>

int* p1 = new int(42);  
free(p1);              // ❌ COMPORTEMENT INDÉFINI — le destructeur n'est pas appelé  
                       //    (trivial pour int, mais catastrophique pour des objets complexes)

int* p2 = (int*)malloc(sizeof(int));  
delete p2;             // ❌ COMPORTEMENT INDÉFINI — new n'a jamais été appelé  

int* p3 = new int(42);  
delete p3;             // ✅ correspondance correcte  
```

---

## Échec d'allocation : std::bad_alloc

Quand l'allocateur ne peut pas satisfaire une demande (mémoire épuisée ou très fragmentée), `new` lance l'exception `std::bad_alloc` :

```cpp
#include <iostream>
#include <new>       // std::bad_alloc

int main() {
    try {
        // Demande absurde : ~8 000 pétaoctets
        long long* p = new long long[1'000'000'000'000'000'000LL];
        delete[] p;
    }
    catch (const std::bad_alloc& e) {
        std::cerr << "Allocation échouée : " << e.what() << "\n";
    }
    return 0;
}
```

Il existe aussi une variante `nothrow` qui retourne `nullptr` au lieu de lancer une exception :

```cpp
#include <new>

int* p = new(std::nothrow) int[1'000'000'000];  
if (p == nullptr) {  
    std::cerr << "Allocation échouée\n";
    // Gestion de l'erreur sans exception
} else {
    // Utilisation
    delete[] p;
}
```

En pratique, sur un système Linux avec overcommit activé (voir section 5.1.3), `new` échoue rarement car le noyau accepte des réservations de mémoire virtuelle bien au-delà de la RAM disponible. Le vrai problème survient plus tard, quand le programme tente d'utiliser la mémoire allouée et que le noyau déclenche l'OOM Killer.

---

## Allocation dynamique et const

Un objet `const` peut être alloué dynamiquement. Le pointeur retourné est alors un pointeur vers `const`, ce qui empêche la modification de l'objet via ce pointeur :

```cpp
const int* p = new const int(42);

// *p = 10;       // ❌ erreur de compilation — l'objet est const
std::cout << *p;  // ✅ lecture autorisée

delete p;          // ✅ la libération reste possible (et obligatoire)
```

Notez que `delete` fonctionne sur un pointeur vers `const`. C'est logique : détruire un objet et modifier son contenu sont deux opérations conceptuellement différentes. On peut détruire un objet constant.

---

## Résumé des règles

Voici les règles à retenir de cette section, classées par ordre de criticité :

**Correspondance stricte.** `new` → `delete`, `new[]` → `delete[]`. Toute autre combinaison est un comportement indéfini. Ne mélangez jamais `new`/`free` ni `malloc`/`delete`.

**Un seul delete par allocation.** Chaque bloc alloué doit être libéré exactement une fois. Zéro fois = fuite mémoire. Deux fois = comportement indéfini (double free).

**Propriété claire.** À tout instant, il doit être évident quel composant du programme est responsable d'appeler `delete`. Si la propriété est ambiguë, les bugs sont inévitables.

**Exception safety.** Du code entre un `new` et le `delete` correspondant peut lancer une exception. Si c'est le cas, le `delete` n'est jamais atteint. C'est la motivation principale du RAII et des smart pointers.

**Préférez les alternatives.** `std::vector` remplace `new[]`/`delete[]` dans la quasi-totalité des cas. `std::unique_ptr` remplace `new`/`delete` pour la possession exclusive. `std::shared_ptr` gère la possession partagée. En C++ moderne, l'écriture directe de `new` et `delete` est réservée à des cas très spécifiques (allocateurs personnalisés, code très bas niveau, interfaçage avec des API C).

⏭️ [Arithmétique des pointeurs et accès bas niveau](/05-gestion-memoire/03-arithmetique-pointeurs.md)
