🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 6.3 — Destructeurs et le principe RAII

## Chapitre 6 : Classes et Encapsulation

---

## Ce que vous allez apprendre

- Comprendre le rôle du destructeur dans le cycle de vie d'un objet C++.  
- Maîtriser la syntaxe et les règles d'appel automatique du destructeur.  
- Saisir pourquoi le couple constructeur/destructeur est la clé de voûte de la gestion des ressources en C++.  
- Découvrir le principe **RAII** (*Resource Acquisition Is Initialization*), qui distingue fondamentalement le C++ des autres langages.  
- Mesurer l'impact du RAII sur la robustesse du code, en particulier face aux exceptions.

---

## Le problème : qui libère quoi, et quand ?

Tout programme non trivial acquiert des ressources au cours de son exécution : mémoire dynamique, fichiers ouverts, connexions réseau, verrous de synchronisation, handles système. Chacune de ces ressources a un coût — et si elle n'est pas libérée, le programme fuit.

En C, la gestion de ces ressources repose sur la discipline du développeur :

```c
// C — gestion manuelle des ressources
void process_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return;

    char* buffer = malloc(4096);
    if (!buffer) {
        fclose(f);          // Ne pas oublier de fermer le fichier
        return;
    }

    // ... traitement ...

    if (error_condition) {
        free(buffer);       // Ne pas oublier de libérer le buffer
        fclose(f);          // Ne pas oublier de fermer le fichier
        return;             // Chaque point de sortie doit tout nettoyer
    }

    // ... suite du traitement ...

    free(buffer);           // Encore une fois
    fclose(f);              // Encore une fois
}
```

Chaque point de sortie de la fonction doit libérer chaque ressource acquise, dans le bon ordre. Avec deux ressources, c'est déjà pénible. Avec cinq ressources et des conditions d'erreur imbriquées, c'est un champ de mines. Un seul chemin oublié, et le programme fuit.

Certains langages (Java, Python, Go) répondent à ce problème avec un **ramasse-miettes** (*garbage collector*) pour la mémoire, et des constructions spéciales (`try-with-resources`, `with`, `defer`) pour les autres ressources. Le C++ prend un chemin radicalement différent : il lie la durée de vie des ressources à la durée de vie des objets. C'est le principe RAII.

---

## Le destructeur : nettoyage automatique et garanti

Un destructeur est une **fonction membre spéciale** qui s'exécute automatiquement quand un objet atteint la fin de sa durée de vie. Il porte le même nom que la classe, précédé du caractère `~`, ne prend aucun paramètre, et n'a pas de type de retour :

```cpp
class Resource {  
public:  
    Resource() {
        std::cout << "Acquisition\n";
    }

    ~Resource() {
        std::cout << "Libération\n";
    }
};

void example() {
    Resource r;                   // "Acquisition"
    std::cout << "Utilisation\n";
}                                 // "Libération" — automatique, garanti

// Sortie :
// Acquisition
// Utilisation
// Libération
```

Le destructeur est appelé automatiquement par le compilateur. Vous n'avez jamais besoin de l'appeler manuellement — et en pratique, vous ne devriez jamais le faire. C'est une garantie du langage, pas une convention.

---

## Quand le destructeur est-il appelé ?

Le moment exact de l'appel dépend de la manière dont l'objet a été créé :

### Objets sur la pile (*stack*)

Le destructeur est appelé quand l'objet sort de sa **portée** (*scope*) — c'est-à-dire quand l'exécution atteint l'accolade fermante du bloc dans lequel l'objet a été déclaré :

```cpp
void process() {
    DynArray a(100);          // Construction de a

    if (some_condition) {
        DynArray b(50);       // Construction de b
        // ...
    }                         // Destruction de b — sort de portée

    // b n'existe plus ici
}                             // Destruction de a — sort de portée
```

Quand plusieurs objets sont détruits en fin de portée, l'ordre de destruction est l'**inverse** de l'ordre de construction. C'est logique : le dernier objet construit est le premier détruit, comme une pile d'assiettes.

```cpp
void order_demo() {
    DynArray x(1);    // Construction 1
    DynArray y(2);    // Construction 2
    DynArray z(3);    // Construction 3
}
// Destruction 3 (z), puis 2 (y), puis 1 (x)
```

### Objets sur le tas (*heap*)

Pour les objets alloués avec `new`, le destructeur est appelé par `delete` :

```cpp
DynArray* p = new DynArray(100);   // Construction
// ...
delete p;                           // Destruction — appelle ~DynArray()
```

Si vous oubliez `delete`, le destructeur n'est jamais appelé et la ressource fuit. C'est précisément la raison pour laquelle le C++ moderne recommande les smart pointers (chapitre 9) qui appellent `delete` automatiquement.

### Objets temporaires

Les temporaires sont détruits à la fin de l'**expression complète** (*full expression*) qui les a créés :

```cpp
std::cout << DynArray(5).size() << "\n";
// DynArray(5) est construit, .size() retourne 5,
// puis le temporaire est détruit à la fin de l'expression (le ;)
```

### Membres d'une classe

Quand un objet est détruit, ses membres sont détruits **après** le corps du destructeur, dans l'ordre inverse de leur déclaration :

```cpp
class Composite {  
public:  
    ~Composite() {
        // Le corps s'exécute en premier
        std::cout << "~Composite body\n";
    }
    // Ensuite : ~second_, puis ~first_ (ordre inverse de déclaration)

private:
    DynArray first_{10};
    DynArray second_{20};
};
```

Ce mécanisme est récursif : le destructeur de chaque membre appelle à son tour les destructeurs de ses propres membres, et ainsi de suite. La destruction est complète et automatique, du sommet de la hiérarchie jusqu'aux types les plus primitifs.

---

## Destruction et exceptions : la garantie fondamentale

C'est ici que le destructeur révèle toute sa puissance. En C++, quand une exception est lancée, le **stack unwinding** (déroulement de la pile) se met en marche : tous les objets construits sur la pile entre le `throw` et le `catch` sont **automatiquement détruits**, dans l'ordre inverse de leur construction.

```cpp
void risky_operation() {
    DynArray data(1000);           // 1. Construction
    Connection conn("db.local");   // 2. Construction

    conn.execute("DROP TABLE ...");  // 3. Lance une exception !

    // --- Ce code n'est jamais atteint ---
    std::cout << "Done\n";
}
// Stack unwinding :
// → ~Connection() appelé automatiquement (2 détruit)
// → ~DynArray() appelé automatiquement (1 détruit)
// L'exception se propage au catch le plus proche
```

Comparez avec le code C du début de cette section : en C, chaque chemin d'erreur devait libérer manuellement chaque ressource. En C++, les destructeurs s'en chargent **quel que soit le chemin de sortie** — retour normal, `return` anticipé, ou exception. Aucune ressource ne fuit.

C'est la raison pour laquelle les destructeurs ne doivent **jamais lancer d'exception**. Si un destructeur lance pendant un stack unwinding déjà en cours, le programme appelle `std::terminate` et s'arrête immédiatement. Les destructeurs doivent être des opérations de nettoyage fiables et silencieuses.

---

## Le couple constructeur/destructeur : un contrat symétrique

Le constructeur et le destructeur forment un couple indissociable. Le constructeur **acquiert** les ressources et établit l'invariant de la classe. Le destructeur **libère** les ressources et démantèle l'objet proprement. Tout ce que le constructeur a acquis, le destructeur le rend :

```cpp
class FileHandle {  
public:  
    explicit FileHandle(const std::string& path)
        : fd_(::open(path.c_str(), O_RDONLY)) {     // Acquisition
        if (fd_ < 0) throw std::runtime_error("Cannot open: " + path);
    }

    ~FileHandle() {
        if (fd_ >= 0) ::close(fd_);                  // Libération
    }

    // ... méthodes de lecture ...

private:
    int fd_;
};
```

```cpp
class MutexLock {  
public:  
    explicit MutexLock(std::mutex& m) : mutex_(m) {
        mutex_.lock();                                // Acquisition
    }

    ~MutexLock() {
        mutex_.unlock();                              // Libération
    }

private:
    std::mutex& mutex_;
};
```

```cpp
class DynArray {  
public:  
    explicit DynArray(std::size_t size)
        : data_(new int[size]{}), size_(size) {}      // Acquisition

    ~DynArray() {
        delete[] data_;                                // Libération
    }

    // ...
private:
    int* data_;
    std::size_t size_;
};
```

Le pattern est toujours le même : le constructeur acquiert, le destructeur libère. La nature de la ressource change (mémoire, fichier, verrou), mais la structure reste identique. Ce pattern a un nom : **RAII** — *Resource Acquisition Is Initialization*.

---

## Qu'est-ce que le RAII ?

RAII est un principe de conception qui stipule que **l'acquisition d'une ressource doit se faire dans le constructeur d'un objet, et sa libération dans le destructeur**. La durée de vie de la ressource est ainsi liée à la durée de vie de l'objet — et comme le C++ garantit que les destructeurs sont appelés automatiquement, la libération est garantie.

Le nom est mal choisi (l'accent est mis sur l'*acquisition*, alors que l'idée forte est la *libération automatique*), mais le concept est le plus important du C++ moderne. Bjarne Stroustrup, le créateur du C++, a suggéré le nom alternatif **CADRE** (*Constructor Acquires, Destructor Releases*) ou **SBRM** (*Scope-Bound Resource Management*), mais RAII est resté dans l'usage courant.

Les sous-sections suivantes détailleront le concept (6.3.1) et montreront des exemples pratiques dans différents domaines (6.3.2).

---

## Pourquoi le RAII change tout

Pour mesurer l'impact du RAII, reprenons l'exemple C du début de cette section et réécrivons-le en C++ idiomatique :

```cpp
// C++ avec RAII — les destructeurs gèrent tout
void process_file(const std::string& path) {
    FileHandle file(path);               // Ouvre le fichier (ou lance une exception)
    std::vector<char> buffer(4096);      // Alloue le buffer (RAII via vector)

    // ... traitement ...

    if (error_condition) {
        return;    // file et buffer sont détruits automatiquement
    }

    // ... suite du traitement ...

}   // file et buffer sont détruits automatiquement
```

Aucun `fclose` explicite, aucun `free` explicite, aucun risque d'oublier un nettoyage sur un chemin d'erreur. Le code est plus court, plus lisible, et **garanti sans fuite** quels que soient les chemins de sortie — y compris les exceptions.

C'est pour cette raison que le C++ moderne n'a pas besoin de ramasse-miettes. Le RAII offre un nettoyage **déterministe** (on sait exactement quand la ressource est libérée) et **universel** (il fonctionne pour toute ressource, pas seulement la mémoire). Un garbage collector ne gère que la mémoire, avec un timing imprévisible. Le RAII gère la mémoire, les fichiers, les connexions, les verrous — tout — au moment précis où l'objet sort de portée.

---

## Génération implicite et `= default`

Comme pour les constructeurs, le compilateur génère un destructeur implicite si vous n'en déclarez pas. Ce destructeur implicite appelle les destructeurs de chaque membre dans l'ordre inverse de déclaration. Pour la grande majorité des classes, c'est exactement ce qu'il faut :

```cpp
class UserProfile {
    std::string name_;
    std::string email_;
    std::vector<std::string> tags_;
    // Destructeur implicite → ~string(), ~string(), ~vector() appelés automatiquement
};
```

Vous n'avez besoin d'écrire un destructeur explicite que si votre classe gère **directement** une ressource brute — un pointeur obtenu par `new`, un descripteur de fichier, un handle système. Si tous vos membres sont des types standard ou des types RAII, le destructeur implicite suffit.

Comme pour les constructeurs, `= default` rend l'intention explicite :

```cpp
class Widget {  
public:  
    ~Widget() = default;   // "J'ai réfléchi, et le destructeur implicite me convient"
};
```

---

## Quand écrire un destructeur est un signal d'alarme

Écrire un destructeur déclenche la **Règle des 5** (section 6.5). Si vous avez besoin d'un destructeur personnalisé, c'est que votre classe gère une ressource brute — et dans ce cas, la copie et le déplacement implicites sont presque certainement incorrects. Vous devez alors considérer les quatre autres opérations spéciales.

C'est aussi un signal pour se poser la question : **cette ressource ne pourrait-elle pas être encapsulée dans un type RAII existant ?** Dans la grande majorité des cas, la réponse est oui :

| Ressource brute | Wrapper RAII standard |  
|---|---|  
| `new T` / `delete` | `std::unique_ptr<T>` |  
| `new T[]` / `delete[]` | `std::unique_ptr<T[]>` ou `std::vector<T>` |  
| `new T` partagé | `std::shared_ptr<T>` |  
| `fopen` / `fclose` | `std::fstream` |  
| `lock` / `unlock` | `std::lock_guard`, `std::scoped_lock` |

Si vous pouvez remplacer votre ressource brute par un wrapper standard, **faites-le**. Votre classe n'aura plus besoin de destructeur personnalisé, et la Règle des 5 disparaît d'elle-même. C'est la **Règle du Zéro** (*Rule of Zero*) : la meilleure classe est celle qui n'a besoin d'aucune opération spéciale.

> 💡 La Règle du Zéro sera abordée en section 6.5 aux côtés de la Règle des 5. Le chapitre 9 (Smart Pointers) montrera comment éliminer les `new`/`delete` explicites de votre code.

---

## Plan des sous-sections

| Sous-section | Thème | Ce que vous apprendrez |  
|---|---|---|  
| **6.3.1** | Resource Acquisition Is Initialization | Le principe RAII formalisé, ses garanties, et pourquoi il élimine des catégories entières de bugs |  
| **6.3.2** | Exemples pratiques de RAII | Wrappers RAII pour la mémoire, les fichiers, les verrous, les timers — le pattern appliqué à des ressources variées |

---


⏭️ [Resource Acquisition Is Initialization](/06-classes-encapsulation/03.1-raii-concept.md)
