🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 5.4 — Dangers : Memory leaks, dangling pointers, double free

## Le trio infernal

Les erreurs de gestion mémoire sont la première cause de bugs critiques, de failles de sécurité et de crashs inexplicables dans les programmes C++. Elles se classent en trois grandes catégories, chacune avec ses symptômes, ses causes et ses conséquences propres :

- **Memory leak** (fuite mémoire) : de la mémoire allouée n'est jamais libérée.
- **Dangling pointer** (pointeur pendant) : un pointeur référence de la mémoire qui a déjà été libérée.
- **Double free** (double libération) : un même bloc mémoire est libéré deux fois.

Ces trois catégories ne sont pas indépendantes — elles naissent souvent des mêmes faiblesses dans le code : propriété ambiguë, absence de RAII, gestion manuelle de `new`/`delete`. Comprendre chacune en profondeur est un prérequis pour apprécier pourquoi le C++ moderne s'est donné tant de mal pour les éliminer.

---

## Memory leaks : la mémoire qui ne revient jamais

### Le mécanisme

Une fuite mémoire se produit quand un bloc alloué sur le heap n'est jamais libéré, et que plus aucun pointeur du programme ne permet de le retrouver. La mémoire reste occupée jusqu'à la fin du processus, où le système d'exploitation la récupère.

Le cas le plus élémentaire :

```cpp
void fuite_simple() {
    int* p = new int(42);
    // ... aucun delete ...
}   // p est détruit (variable locale sur la stack), mais le bloc heap reste alloué
    // Plus personne ne connaît l'adresse → fuite irréversible
```

Quand la fonction retourne, la variable `p` (qui vit sur la stack) est détruite. Mais le bloc de 4 octets qu'elle pointait sur le heap est toujours là. Aucune variable du programme ne contient plus son adresse. Il est devenu un **orphelin** — impossible à libérer, impossible à réutiliser.

### Fuites par écrasement de pointeur

Un scénario fréquent est la réaffectation d'un pointeur sans libérer l'ancienne allocation :

```cpp
void fuite_ecrasement() {
    int* p = new int(10);     // allocation A
    p = new int(20);          // allocation B — l'adresse de A est perdue
    delete p;                 // libère B, mais A reste orpheline → fuite
}
```

Avant la seconde ligne, `p` était le seul chemin vers l'allocation A. En réaffectant `p`, ce chemin est détruit. L'allocation A est perdue à jamais.

### Fuites par retour anticipé

Les fonctions avec plusieurs chemins de retour sont particulièrement vulnérables :

```cpp
bool traiter_fichier(const std::string& chemin) {
    char* buffer = new char[4096];

    std::ifstream fichier(chemin);
    if (!fichier.is_open()) {
        return false;            // ❌ fuite — buffer n'est jamais libéré
    }

    if (fichier.peek() == std::ifstream::traits_type::eof()) {
        return false;            // ❌ fuite — même problème
    }

    // ... traitement ...

    delete[] buffer;
    return true;                 // ✅ libéré seulement sur ce chemin
}
```

Deux des trois chemins de retour oublient le `delete[]`. Ce type de bug est d'autant plus traître qu'il ne se manifeste que sur les chemins d'erreur — souvent les moins testés.

### Fuites par exception

Même sans retour anticipé explicite, une exception peut court-circuiter le `delete` :

```cpp
void traiter() {
    Connexion* conn = new Connexion("serveur", 8080);

    envoyer_requete(conn);     // si cette fonction lance une exception...

    delete conn;               // ... cette ligne n'est jamais atteinte → fuite
}
```

C'est le problème fondamental de la gestion manuelle face aux exceptions. Le mécanisme de déroulement de pile (*stack unwinding*) détruit les objets locaux, mais il ne peut pas deviner qu'un pointeur brut possède une ressource à libérer. C'est exactement le problème que le RAII (chapitre 6) et les smart pointers (chapitre 9) résolvent.

### Fuites dans les boucles : l'effet cumulatif

Une fuite isolée de quelques octets est souvent bénigne. Mais une fuite dans une boucle devient une hémorragie :

```cpp
void traiter_requetes(int nombre) {
    for (int i = 0; i < nombre; ++i) {
        char* buffer = new char[8192];   // 8 Ko par itération
        // ... traitement ...
        // Oubli de delete[] buffer → 8 Ko perdus à chaque itération
    }
    // Après 1 million d'itérations : ~8 Go de mémoire gaspillée
}
```

Dans un serveur ou un daemon qui tourne en continu, ce type de fuite fait grimper la consommation mémoire de manière linéaire dans le temps, jusqu'à l'épuisement de la RAM et l'intervention de l'OOM Killer. C'est un des bugs les plus classiques des applications long-running.

### Symptômes d'une fuite mémoire

Les fuites ne provoquent pas de crash immédiat. Leurs symptômes sont insidieux et apparaissent progressivement : consommation mémoire qui augmente régulièrement sans raison apparente (visible dans `top`, `htop` ou `/proc/<PID>/status`), dégradation progressive des performances à mesure que le système commence à utiliser le swap, et finalement un crash par OOM Killer ou un `std::bad_alloc` quand la mémoire est épuisée.

---

## Dangling pointers : le fantôme en mémoire

### Le mécanisme

Un *dangling pointer* (pointeur pendant ou pointeur fantôme) est un pointeur qui contient l'adresse d'un bloc mémoire qui a **déjà été libéré**. Le pointeur semble valide — il contient une adresse numérique — mais la mémoire à cette adresse ne vous appartient plus.

```cpp
int* creer_et_detruire() {
    int* p = new int(42);
    delete p;
    return p;   // ❌ p est maintenant un dangling pointer
}

void utiliser() {
    int* fantome = creer_et_detruire();
    std::cout << *fantome << "\n";   // 💀 comportement indéfini
}
```

Après le `delete`, la mémoire est rendue à l'allocateur. Celui-ci peut la réattribuer à n'importe quel futur `new`. Déréférencer `fantome` peut alors :

- Afficher la valeur `42` (les anciens octets n'ont pas encore été écrasés — c'est le cas le plus trompeur).
- Afficher une valeur aléatoire (la mémoire a été réallouée et écrite par un autre code).
- Provoquer un `Segmentation fault` (la page mémoire a été rendue au système).
- Corrompre silencieusement les données d'un autre objet (le bloc a été réalloué et vous écrivez par-dessus).

Le scénario le plus dangereux est le dernier : une écriture via un dangling pointer modifie les données d'un objet totalement indépendant, provoquant des comportements erratiques bien loin du point de corruption. Ce type de bug peut prendre des jours à diagnostiquer.

### Dangling pointer par retour de référence locale

Un piège classique est de retourner une référence ou un pointeur vers une variable locale :

```cpp
int& obtenir_valeur() {
    int locale = 42;
    return locale;      // ❌ 'locale' est détruite au retour de la fonction
}

void utiliser() {
    int& ref = obtenir_valeur();
    std::cout << ref << "\n";   // 💀 comportement indéfini — ref pointe vers
                                 //    un stack frame qui n'existe plus
}
```

La variable `locale` vit sur la stack frame de `obtenir_valeur`. Quand la fonction retourne, ce frame est détruit. La référence retournée pointe vers une zone de stack qui sera écrasée par le prochain appel de fonction. Le compilateur émet généralement un avertissement pour ce cas (`-Wreturn-local-addr` avec GCC, activé par `-Wall`), mais uniquement dans les cas simples.

Le même problème se pose avec les pointeurs :

```cpp
int* obtenir_pointeur() {
    int locale = 42;
    return &locale;     // ❌ adresse d'une variable locale — dangling pointer
}
```

### Dangling pointer par invalidation d'itérateur

Un cas plus subtil survient avec les conteneurs de la STL. Certaines opérations **invalident** les itérateurs et les pointeurs vers les éléments :

```cpp
#include <vector>

std::vector<int> nombres = {1, 2, 3};  
int* ptr = &nombres[0];          // pointeur vers le premier élément  

nombres.push_back(4);            // peut provoquer une réallocation interne

std::cout << *ptr << "\n";       // 💀 comportement indéfini si réallocation
                                  //    ptr pointe vers l'ancien buffer (libéré)
```

Quand `push_back` déclenche une réallocation (parce que la capacité est insuffisante), le vector alloue un nouveau buffer, y copie les éléments, puis libère l'ancien. Tous les pointeurs, références et itérateurs vers l'ancien buffer deviennent des dangling pointers. Ce scénario est une source de bugs très fréquente, même chez les développeurs expérimentés. La section 13.1.3 détaille les règles d'invalidation pour chaque opération de `std::vector`.

### Dangling pointer dans les structures de données

Les dangling pointers sont particulièrement courants dans les structures qui partagent des pointeurs vers les mêmes objets :

```cpp
struct Equipe {
    std::string nom;
    Employe* responsable;   // pointeur brut vers un employé
};

void probleme() {
    Employe* alice = new Employe{"Alice"};
    Equipe equipe{"DevOps", alice};

    delete alice;           // alice est libérée

    // equipe.responsable est maintenant un dangling pointer
    std::cout << equipe.responsable->nom << "\n";  // 💀 comportement indéfini
}
```

Qui est "propriétaire" d'Alice ? Le code ne le dit pas clairement. C'est l'ambiguïté de propriété — le terreau fertile des dangling pointers. Les smart pointers résolvent ce problème en rendant la propriété explicite : `std::unique_ptr` pour la possession exclusive, `std::shared_ptr` pour la possession partagée.

---

## Double free : la corruption de l'allocateur

### Le mécanisme

Un *double free* se produit quand `delete` (ou `delete[]`) est appelé deux fois sur le même bloc mémoire :

```cpp
int* p = new int(42);  
delete p;       // ✅ première libération — correcte  
delete p;       // 💀 double free — comportement indéfini  
```

Au premier `delete`, l'allocateur marque le bloc comme libre et le remet dans sa liste de blocs disponibles. Au second `delete`, l'allocateur reçoit une adresse qu'il a déjà marquée comme libre. Selon l'implémentation, les conséquences peuvent être :

- **Crash immédiat** — l'allocateur de glibc détecte parfois le double free et termine le programme avec le message `free(): double free detected in tcache 2`.
- **Corruption silencieuse de l'allocateur** — les structures internes (listes chaînées de blocs libres) sont corrompues, provoquant des crashs ou des comportements erratiques lors d'allocations *ultérieures*, rendant le diagnostic extrêmement difficile.
- **Faille de sécurité** — un double free peut être exploité par un attaquant pour contrôler le contenu de la mémoire et exécuter du code arbitraire. C'est une classe de vulnérabilités bien connue (CWE-415).

### Double free par copie de pointeur

Le scénario le plus courant est l'existence de **deux pointeurs vers le même bloc**, chacun pensant être responsable de la libération :

```cpp
void double_free_copie() {
    int* original = new int(42);
    int* copie = original;         // deux pointeurs, même adresse

    delete original;               // ✅ première libération
    delete copie;                  // 💀 double free — même adresse
}
```

Ce problème se manifeste souvent dans les classes qui copient leurs pointeurs bruts sans implémenter correctement la Rule of Five (section 6.5) :

```cpp
class Buffer {  
public:  
    Buffer(int taille) : donnees_(new int[taille]), taille_(taille) {}
    ~Buffer() { delete[] donnees_; }

    // ⚠️ Pas de constructeur de copie ni d'opérateur d'affectation définis
    // Le compilateur génère des copies superficielles (shallow copy)

private:
    int* donnees_;
    int taille_;
};

void probleme() {
    Buffer a(100);
    Buffer b = a;       // copie superficielle : b.donnees_ == a.donnees_
}   // ~Buffer() appelé pour b → delete[] donnees_
    // ~Buffer() appelé pour a → delete[] donnees_ 💀 double free !
```

Le compilateur génère automatiquement un constructeur de copie qui copie chaque membre — y compris le pointeur `donnees_`. Après la copie, `a.donnees_` et `b.donnees_` pointent vers le même bloc. Quand les deux objets sont détruits (en fin de scope, dans l'ordre inverse de construction), `delete[]` est appelé deux fois sur le même bloc.

C'est la motivation directe de la **Rule of Five** (section 6.5) : toute classe qui gère manuellement une ressource doit définir ou supprimer explicitement ses cinq opérations spéciales (destructeur, constructeur de copie, opérateur d'affectation par copie, constructeur de déplacement, opérateur d'affectation par déplacement).

### Double free par mélange de responsabilités

Un autre scénario fréquent est le partage de responsabilité entre un appelant et un appelé :

```cpp
void traiter(int* donnees) {
    // ... utilisation de donnees ...
    delete donnees;              // l'appelé libère
}

void orchestrer() {
    int* d = new int(42);
    traiter(d);                  // traiter() libère d
    delete d;                    // 💀 double free — d a déjà été libéré
}
```

Qui est responsable de la libération : `orchestrer` ou `traiter` ? Le code ne le dit pas clairement. Ce flou est la racine du problème. En C++ moderne, la solution est de rendre la propriété explicite via les types : passer un `std::unique_ptr<int>` par valeur (transfert de propriété) ou par référence (prêt temporaire), rendant les intentions impossibles à ignorer.

---

## Use-after-free : la combinaison mortelle

Le terme **use-after-free** désigne toute utilisation (lecture ou écriture) d'un bloc mémoire après sa libération. C'est en réalité un cas particulier du dangling pointer — on utilise un pointeur pendant. Ce terme est surtout employé dans le contexte de la **sécurité informatique**, car c'est l'une des vulnérabilités les plus exploitées.

Le scénario typique d'exploitation :

```cpp
// Étape 1 : allocation et libération
Objet* victime = new Objet();  
delete victime;                   // la mémoire est rendue à l'allocateur  

// Étape 2 : réallocation contrôlée par l'attaquant
// Si l'attaquant contrôle le contenu d'une allocation de même taille,
// celle-ci peut atterrir exactement à l'adresse de l'ancien objet
char* malveillant = new char[sizeof(Objet)];  
std::memcpy(malveillant, donnees_attaquant, sizeof(Objet));  

// Étape 3 : utilisation du dangling pointer
victime->methode_virtuelle();     // 💀 appelle du code contrôlé par l'attaquant
                                   // via la vtable corrompue
```

Ce schéma simplifié illustre pourquoi les use-after-free sont si dangereux : l'attaquant peut contrôler le contenu de la mémoire réallouée et rediriger les appels de fonctions virtuelles vers son propre code. C'est la raison pour laquelle les navigateurs web, les noyaux de systèmes d'exploitation et les logiciels critiques investissent massivement dans la prévention de ces bugs (chapitre 45).

---

## Le comportement indéfini : pourquoi c'est pire qu'un crash

Vous avez remarqué que la plupart des erreurs de cette section sont qualifiées de **comportement indéfini** (*undefined behavior*, souvent abrégé *UB*). Ce terme a une signification précise et effrayante en C++ : le standard ne prescrit **aucune** conséquence. Le programme peut :

- Planter immédiatement (le meilleur scénario — au moins vous le savez).
- Sembler fonctionner correctement pendant des mois, puis planter sous charge.
- Produire des résultats incorrects sans aucun signal d'erreur.
- Se comporter différemment en mode debug et en mode release (parce que l'optimiseur prend des décisions différentes en supposant l'absence d'UB).
- Être exploité par un attaquant pour exécuter du code arbitraire.

Le compilateur a le droit de supposer que le comportement indéfini **ne se produit jamais**. Cette hypothèse lui permet des optimisations agressives, mais elle signifie aussi que du code contenant un UB peut être transformé de manière surprenante par l'optimiseur. Un programme avec un UB ne fait pas ce que vous pensez qu'il fait — il fait ce que le compilateur décide de produire, sans aucune garantie.

C'est pourquoi la philosophie du C++ moderne est d'**éliminer structurellement** les possibilités de comportement indéfini, plutôt que de compter sur la vigilance du développeur :

```
Problème                          Solution C++ moderne
──────────────────────────────    ──────────────────────────────
Oubli de delete                → std::unique_ptr, std::shared_ptr (ch. 9)  
Dangling pointer               → RAII, smart pointers (ch. 6, 9)  
Double free                    → Rule of Five, = delete (ch. 6.5)  
Buffer overflow                → std::vector, std::span (ch. 13, 13.5)  
Copie involontaire de pointeur → Sémantique de mouvement (ch. 10)  
Propriété ambiguë              → Types de propriété explicites (ch. 9)  
```

---

## Résumé comparatif des trois dangers

```
              Memory Leak          Dangling Pointer       Double Free
              ─────────────        ──────────────────     ─────────────
Cause         delete oublié        Utilisation après      delete appelé
                                   delete                 deux fois

Symptôme      Consommation         Crash, corruption      Crash, corruption  
immédiat      mémoire croissante   silencieuse, ou        de l'allocateur  
                                   "ça marche" (piège)

Gravité       Dégradation          Comportement           Comportement
              progressive          indéfini               indéfini

Sécurité      Déni de service      Exploitation           Exploitation
              (OOM)                code arbitraire        code arbitraire

Détection     Valgrind, LeakSan    ASan, Valgrind         ASan, Valgrind
              (-fsanitize=leak)    (-fsanitize=address)   (-fsanitize=address)

Prévention    Smart pointers,      Smart pointers,        Rule of Five,  
C++ moderne   RAII, conteneurs     RAII, std::span        smart pointers  
```

La section suivante (5.5) présente les outils concrets — Valgrind et AddressSanitizer — qui permettent de détecter automatiquement ces trois catégories d'erreurs. Car aussi rigoureux que soit un développeur, l'œil humain ne suffit pas face à un codebase de plusieurs milliers de lignes : les outils sont indispensables.

⏭️ [Outils de détection : Valgrind, AddressSanitizer](/05-gestion-memoire/05-outils-detection.md)
