🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 30.3 — Memory leaks : Détection et résolution

## Introduction

Les sections précédentes ont présenté les outils — Memcheck pour détecter les fuites (section 30.1) et Massif pour profiler la consommation mémoire dans le temps (section 30.2). Cette section aborde le problème des fuites mémoire sous l'angle méthodologique : comment les traquer dans un projet réel, comment les corriger durablement, et surtout comment les prévenir à la source.

Une fuite mémoire (*memory leak*) se produit lorsqu'un programme alloue de la mémoire sur le heap et perd la capacité de la libérer. Le bloc reste alloué jusqu'à la fin du processus, où le système d'exploitation le récupère. Pour un programme de courte durée — un outil CLI qui s'exécute en quelques secondes — les conséquences sont négligeables. Pour un service de longue durée — un serveur web, un daemon système, un microservice conteneurisé — les conséquences sont potentiellement catastrophiques : consommation mémoire croissante, dégradation progressive des performances, OOM kill par le noyau, redémarrages en cascade.

La difficulté des fuites mémoire tient à leur nature insidieuse. Contrairement à un segfault qui interrompt immédiatement le programme, une fuite peut s'accumuler pendant des heures ou des jours avant de se manifester. Et contrairement à un bug logique qui produit un résultat incorrect visible, une fuite n'altère pas le comportement fonctionnel du programme — du moins pas immédiatement.

---

## Taxonomie des fuites mémoire

Toutes les fuites ne se ressemblent pas. Les identifier par catégorie permet de choisir la bonne stratégie de correction.

### Fuite par perte de référence

C'est la fuite classique : le dernier pointeur vers un bloc alloué sort de la portée ou est écrasé sans que le bloc ait été libéré.

```cpp
void traiter() {
    auto* donnees = new Donnees();
    // ... traitement ...
    // donnees sort de la portée sans delete
}
```

Memcheck classe ces blocs comme *definitely lost*. C'est la catégorie la plus simple à détecter et à corriger.

### Fuite par chemin d'exécution non couvert

L'allocation est correctement libérée sur le chemin nominal, mais un chemin d'erreur — `return` anticipé, exception levée, condition de bord — contourne la libération.

```cpp
Resultat* analyser(const std::string& fichier) {
    FILE* f = fopen(fichier.c_str(), "r");
    if (!f) return nullptr;

    auto* resultat = new Resultat();

    if (!resultat->charger(f)) {
        fclose(f);
        return nullptr;  // FUITE : resultat n'est pas libéré
    }

    fclose(f);
    return resultat;
}
```

Ces fuites sont particulièrement traîtres car elles ne se manifestent que lorsque le chemin d'erreur est emprunté — ce qui peut être rare en tests mais fréquent en production.

### Fuite par accumulation sans borne

Le programme maintient une structure de données qui ne cesse de croître : un cache sans politique d'éviction, un historique sans limite, un pool de connexions qui ne libère jamais les connexions inactives.

```cpp
class JournalEvenements {
    std::vector<std::string> entrees_;
public:
    void ajouter(const std::string& msg) {
        entrees_.push_back(msg);  // Croissance illimitée
    }
};
```

Techniquement, ce n'est pas une fuite au sens de Memcheck — chaque bloc est accessible via `entrees_`. Mais l'effet en production est identique : une consommation mémoire qui croît sans fin. Massif (section 30.2) est l'outil adapté pour diagnostiquer ce type de problème.

### Fuite par cycle de références

Deux objets (ou plus) se référencent mutuellement via des `std::shared_ptr`, empêchant les compteurs de références d'atteindre zéro.

```cpp
struct A {
    std::shared_ptr<B> b_ptr;
};
struct B {
    std::shared_ptr<A> a_ptr;
};

void creer_cycle() {
    auto a = std::make_shared<A>();
    auto b = std::make_shared<B>();
    a->b_ptr = b;
    b->a_ptr = a;
    // Compteurs : a=2, b=2
    // Après sortie de portée : a=1, b=1 → jamais libérés
}
```

Memcheck classe ces blocs comme *definitely lost* et *indirectly lost* (ou parfois *possibly lost* selon la version). La solution est de briser le cycle en remplaçant l'une des références par un `std::weak_ptr` (section 9.2.2).

### Fuite de ressources non mémoire

Les fuites ne concernent pas uniquement la mémoire. Les descripteurs de fichiers, les sockets, les handles de threads, les connexions à des bases de données sont autant de ressources finies qui doivent être libérées. Un descripteur de fichier non fermé ne consomme que quelques octets de mémoire noyau, mais le système impose une limite par processus (typiquement 1 024 sur Linux). Épuiser cette limite bloque toutes les ouvertures de fichiers et de sockets.

```cpp
void lire_config(const std::string& chemin) {
    int fd = open(chemin.c_str(), O_RDONLY);
    if (fd < 0) return;

    char buffer[256];
    read(fd, buffer, sizeof(buffer));
    // Oubli : close(fd) manquant
}
```

Valgrind détecte ces fuites avec l'option `--track-fds=yes` (section 30.1.1). La solution RAII (chapitre 6.3) s'applique directement : encapsuler la ressource dans un objet dont le destructeur appelle la fonction de libération appropriée.

---

## Méthodologie de détection dans un projet réel

### Étape 1 : Confirmer la fuite

Avant de lancer un outil d'analyse, confirmez que le problème est bien une fuite mémoire et non une consommation légitime élevée. Les indicateurs suivants orientent le diagnostic :

**Consommation croissante dans le temps.** Si la mémoire résidente (RSS) augmente de façon monotone pendant l'exécution, c'est le signe le plus fiable d'une fuite ou d'une accumulation sans borne. Surveillez avec `top`, `htop` ou un monitoring Prometheus/Grafana.

```bash
# Surveiller la RSS d'un processus en continu
while true; do
    ps -o rss= -p $(pgrep mon_service) | awk '{print strftime("%H:%M:%S"), $1/1024 " MB"}'
    sleep 5
done
```

**Consommation stable mais trop élevée.** Si la mémoire est haute mais ne croît plus, le problème est probablement un pic d'allocation ou une structure surdimensionnée, pas une fuite. Massif est l'outil adapté.

**OOM kill en production.** Le fichier `/var/log/syslog` ou `dmesg` contient les traces des processus tués par le noyau pour libérer de la mémoire. Si votre service apparaît régulièrement dans ces logs, une fuite est probable.

### Étape 2 : Reproduire avec un cas minimal

L'analyse sous Valgrind ralentit le programme de 10 à 20×. Exécuter un service complet avec sa charge de production sous Valgrind est rarement praticable. L'objectif est de construire un scénario de test réduit qui déclenche la fuite en un temps raisonnable.

Stratégies de réduction :

- **Accélérer le cycle.** Si la fuite se produit toutes les N requêtes, réduisez N en simplifiant la charge utile ou en désactivant les traitements non pertinents.
- **Isoler le sous-système.** Si le profiling (Massif, ou monitoring en production) pointe vers un module spécifique, écrivez un harnais de test qui exerce uniquement ce module.
- **Amplifier le symptôme.** Si la fuite est de quelques octets par opération, exécutez des milliers d'opérations en boucle pour que la fuite soit visible dans le rapport Valgrind.

### Étape 3 : Analyser avec Memcheck

Exécutez le cas de reproduction sous Memcheck :

```bash
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
    --error-exitcode=1 ./test_reproduction
```

Appliquez la méthodologie de lecture décrite en section 30.1.2 :

1. Commencez par le LEAK SUMMARY.
2. Examinez les blocs *definitely lost* en priorité.
3. Identifiez la première frame de votre code dans chaque pile d'appels.
4. Regroupez les fuites qui partagent la même fonction d'allocation.

### Étape 4 : Profiler avec Massif si nécessaire

Si Memcheck ne révèle aucune fuite (*definitely lost* à zéro) mais que la consommation mémoire croît, le problème est une accumulation sans borne — pas une fuite au sens strict. Passez à Massif :

```bash
valgrind --tool=massif --time-unit=ms --detailed-freq=1 ./test_reproduction  
ms_print massif.out.*  
```

L'arbre d'allocation du dernier snapshot identifie la structure de données responsable de l'accumulation.

### Étape 5 : Corriger et vérifier

Appliquez la correction (voir la section suivante), recompilez, et relancez Memcheck :

```bash
valgrind --leak-check=full --error-exitcode=1 --quiet ./test_reproduction
```

Le mode `--quiet` combiné à `--error-exitcode=1` transforme Valgrind en un test pass/fail. Si le code de sortie est 0, la fuite est résolue. Ajoutez ce test à votre suite CI pour prévenir les régressions.

---

## Patterns de correction

### Pattern 1 : Remplacer `new`/`delete` par un smart pointer

C'est la correction la plus fréquente et la plus robuste. Chaque `new` dans du code applicatif devrait être un signal d'alarme (section 9.4). Le remplacement par un smart pointer élimine structurellement la possibilité de fuite.

**Avant (fuite sur le chemin d'erreur) :**

```cpp
Connexion* creer_connexion(const Config& cfg) {
    auto* conn = new Connexion(cfg.host, cfg.port);

    if (!conn->connecter()) {
        // FUITE : conn n'est pas libéré
        return nullptr;
    }

    return conn;
}
```

**Après (correction avec `std::unique_ptr`) :**

```cpp
std::unique_ptr<Connexion> creer_connexion(const Config& cfg) {
    auto conn = std::make_unique<Connexion>(cfg.host, cfg.port);

    if (!conn->connecter()) {
        return nullptr;  // conn est automatiquement libéré
    }

    return conn;  // Transfert de propriété à l'appelant
}
```

Le smart pointer libère la mémoire à la sortie de la portée, quel que soit le chemin emprunté — retour normal, retour anticipé ou exception. L'appelant reçoit un `std::unique_ptr` qui rend la sémantique de propriété explicite.

### Pattern 2 : Appliquer RAII aux ressources non mémoire

Les descripteurs de fichiers, sockets et handles divers bénéficient du même traitement : encapsuler la ressource dans un objet dont le destructeur appelle la fonction de libération.

**Avant :**

```cpp
void envoyer_rapport(const std::string& dest) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    // ... configuration et connexion ...

    if (erreur_connexion) {
        return;  // FUITE : sock n'est pas fermé
    }

    // ... envoi ...
    close(sock);
}
```

**Après (wrapper RAII) :**

```cpp
class SocketGuard {
    int fd_;
public:
    explicit SocketGuard(int fd) : fd_(fd) {}
    ~SocketGuard() { if (fd_ >= 0) close(fd_); }

    int get() const { return fd_; }

    // Non copiable, déplaçable
    SocketGuard(const SocketGuard&) = delete;
    SocketGuard& operator=(const SocketGuard&) = delete;
    SocketGuard(SocketGuard&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    SocketGuard& operator=(SocketGuard&& other) noexcept {
        if (this != &other) { if (fd_ >= 0) close(fd_); fd_ = other.fd_; other.fd_ = -1; }
        return *this;
    }
};

void envoyer_rapport(const std::string& dest) {
    SocketGuard sock(socket(AF_INET, SOCK_STREAM, 0));

    if (erreur_connexion) {
        return;  // sock est automatiquement fermé
    }

    // ... envoi avec sock.get() ...
}   // sock est automatiquement fermé
```

Ce pattern s'applique à toute ressource : `std::unique_ptr` avec un *custom deleter* (section 9.1.3) est une alternative plus concise lorsque la sémantique de propriété est simple.

### Pattern 3 : Briser les cycles de `shared_ptr`

Lorsque Memcheck signale des blocs *definitely lost* ou *possibly lost* dont les piles d'appels impliquent `std::make_shared`, examinez le graphe de propriété de vos objets. Si deux objets se référencent mutuellement, l'un des deux liens doit être un `std::weak_ptr`.

La règle de décision est simple : dans une relation parent-enfant, le parent possède l'enfant (via `shared_ptr`) et l'enfant référence le parent (via `weak_ptr`). Dans un graphe arbitraire, identifiez un arbre couvrant et convertissez les arcs restants en `weak_ptr`.

**Avant (cycle) :**

```cpp
struct Employe {
    std::string nom;
    std::shared_ptr<Equipe> equipe;  // Référence vers le parent
};

struct Equipe {
    std::string nom;
    std::vector<std::shared_ptr<Employe>> membres;  // Référence vers les enfants
};
```

**Après (cycle brisé) :**

```cpp
struct Employe {
    std::string nom;
    std::weak_ptr<Equipe> equipe;  // weak_ptr : ne participe pas au comptage
};

struct Equipe {
    std::string nom;
    std::vector<std::shared_ptr<Employe>> membres;
};
```

### Pattern 4 : Borner les structures à croissance illimitée

Pour les caches, historiques et buffers qui accumulent des données, la correction n'est pas une libération manquante mais l'ajout d'une politique de limitation.

**Cache LRU à taille bornée :**

```cpp
#include <list>
#include <unordered_map>
#include <string>

template<typename K, typename V>  
class CacheLRU {  
    size_t capacite_;
    std::list<std::pair<K, V>> items_;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> index_;

public:
    explicit CacheLRU(size_t capacite) : capacite_(capacite) {}

    void inserer(const K& cle, const V& valeur) {
        auto it = index_.find(cle);
        if (it != index_.end()) {
            items_.erase(it->second);
            index_.erase(it);
        }

        items_.push_front({cle, valeur});
        index_[cle] = items_.begin();

        // Éviction si la capacité est dépassée
        while (items_.size() > capacite_) {
            index_.erase(items_.back().first);
            items_.pop_back();
        }
    }
};
```

La capacité maximale est définie à la construction. Chaque insertion au-delà de cette limite provoque l'éviction de l'entrée la moins récemment utilisée. La consommation mémoire est bornée par construction.

### Pattern 5 : Implémenter un destructeur complet

Lorsqu'une classe gère manuellement des ressources (cas de code legacy ou de contraintes spécifiques), appliquez la Rule of Five (section 6.5) et vérifiez que le destructeur libère *toutes* les ressources acquises.

**Checklist destructeur :**

- Chaque `new` dans le constructeur a un `delete` correspondant dans le destructeur.
- Chaque `new[]` a un `delete[]` (pas un `delete` simple).
- Chaque `malloc` a un `free`.
- Chaque `open`/`fopen` a un `close`/`fclose`.
- Les cas d'erreur dans le constructeur libèrent les ressources déjà acquises avant de propager l'exception.

---

## Prévention : écrire du code résistant aux fuites

La meilleure fuite est celle qui ne peut pas se produire. Plusieurs principes de conception éliminent structurellement les fuites les plus courantes.

### Principe 1 : Propriété explicite

Chaque ressource a exactement un propriétaire clairement identifié. Le propriétaire est responsable de la libération. En C++ moderne, la propriété est exprimée par le type :

| Type | Sémantique de propriété |
|---|---|
| `T` (valeur) | L'objet possède la donnée directement |
| `std::unique_ptr<T>` | Propriété exclusive, transférable |
| `std::shared_ptr<T>` | Propriété partagée, comptée |
| `T&`, `T*`, `std::span<T>` | Observation sans propriété |
| `std::weak_ptr<T>` | Observation d'un `shared_ptr` sans propriété |

Si une signature de fonction retourne un `T*`, la question « qui libère ? » se pose immédiatement. Si elle retourne un `std::unique_ptr<T>`, la réponse est intégrée dans le type.

### Principe 2 : Préférer la pile au heap

Les objets alloués sur la pile sont automatiquement détruits à la sortie de leur portée. Aucune gestion manuelle n'est nécessaire, aucune fuite n'est possible.

```cpp
// Préférez ceci :
std::vector<int> donnees(1000);

// À ceci :
int* donnees = new int[1000];
// ... n'oubliez pas delete[] donnees ...
```

La majorité des conteneurs de la STL (`std::vector`, `std::string`, `std::map`) gèrent leur mémoire interne automatiquement. Les utiliser comme variables locales ou membres de classes élimine la quasi-totalité des fuites mémoire dans du code applicatif courant.

### Principe 3 : Interdire `new`/`delete` dans le code applicatif

La section 9.4 développe ce principe en détail. Dans un projet bien structuré, `new` et `delete` n'apparaissent que dans le code d'infrastructure de bas niveau (allocateurs personnalisés, wrappers C). Le code applicatif utilise exclusivement des constructeurs de conteneurs, `std::make_unique`, `std::make_shared` et des types valeur.

Certaines équipes renforcent cette règle via `clang-tidy` :

```yaml
# .clang-tidy
Checks: >
  cppcoreguidelines-owning-memory,
  cppcoreguidelines-no-malloc
```

Ces checks signalent les utilisations de `new`/`delete` et `malloc`/`free` qui ne sont pas encapsulées dans des types RAII.

### Principe 4 : Tests systématiques sous Valgrind en CI

La prévention la plus efficace est la détection automatique à chaque commit. Un job CI dédié exécute les tests unitaires sous Valgrind :

```yaml
# .gitlab-ci.yml (extrait)
test_memoire:
  stage: test
  script:
    - cmake --build build --target tests
    - valgrind --leak-check=full --error-exitcode=1 --quiet
        ./build/tests/test_unitaires
  allow_failure: false
```

Avec `--error-exitcode=1`, toute fuite fait échouer le pipeline. Les développeurs sont informés immédiatement, avant que la fuite n'atteigne la branche principale.

Pour les suites de tests volumineuses où le ralentissement Valgrind est prohibitif, un compromis courant est d'exécuter AddressSanitizer (plus rapide) à chaque commit et Valgrind en nightly ou hebdomadaire :

```yaml
# CI quotidienne : rapide
test_asan:
  script:
    - cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" ...
    - ./build/tests/test_unitaires

# CI nightly : exhaustive
test_valgrind_nightly:
  rules:
    - if: $CI_PIPELINE_SOURCE == "schedule"
  script:
    - valgrind --leak-check=full --error-exitcode=1 --quiet
        ./build/tests/test_unitaires
```

---

## Diagnostic des cas difficiles

### Fuite qui n'apparaît qu'en production

Certaines fuites dépendent de conditions impossibles à reproduire en test : charge concurrente, timing réseau, données spécifiques. Si la fuite ne se manifeste pas sous Valgrind avec vos tests :

- **Enrichissez la couverture de test.** Ajoutez des scénarios de stress : milliers de connexions simultanées, requêtes malformées, timeouts.
- **Utilisez AddressSanitizer en production** (si le surcoût de ~2× est acceptable pour un environnement de staging). ASan détecte les fuites à moindre coût.
- **Instrumentez le code.** Ajoutez des compteurs d'allocation/libération dans les modules suspects. Un compteur qui croît de façon monotone identifie le site de fuite.

### Fuite dans une librairie tierce

Si Memcheck signale une fuite dont la pile d'appels ne contient que des fonctions d'une librairie tierce, trois situations sont possibles :

1. **C'est un bug de la librairie.** Vérifiez le bug tracker du projet, mettez à jour vers la dernière version, ou soumettez un rapport de bug.
2. **C'est un pattern de la librairie.** Certaines librairies gèrent leur mémoire via des pools globaux libérés à la fin du processus. Consultez la documentation. Si c'est confirmé, ajoutez une suppression Valgrind documentée.
3. **C'est une erreur d'utilisation.** La librairie fournit peut-être une fonction de nettoyage (`cleanup`, `shutdown`, `destroy`) que votre code n'appelle pas. Vérifiez la documentation d'initialisation/finalisation.

### Fuite proportionnelle au nombre de threads

Les fuites liées aux threads sont souvent causées par des *thread-local storage* non nettoyés ou des threads joints trop tard. Si la fuite est proportionnelle au nombre de threads créés, examinez :

- Les variables `thread_local` qui allouent de la mémoire dans leur constructeur.
- Les threads détachés (`std::thread::detach`) dont les ressources ne sont jamais réclamées.
- Les pools de threads qui accumulent des tâches terminées sans les nettoyer.

`std::jthread` (section 21.7) simplifie la gestion du cycle de vie des threads en garantissant le join automatique à la destruction, éliminant une catégorie de fuites liées aux threads oubliés.

---

## Récapitulatif du chapitre 30

Ce chapitre a couvert l'analyse mémoire sous trois angles complémentaires :

**Valgrind/Memcheck** (section 30.1) détecte les erreurs mémoire et les fuites par instrumentation dynamique. C'est l'outil de référence pour un diagnostic précis et exhaustif, au prix d'un ralentissement significatif.

**Massif** (section 30.2) profile la consommation mémoire dans le temps et identifie les sites d'allocation les plus gourmands. C'est l'outil adapté aux problèmes de consommation excessive ou de croissance sans borne.

**La méthodologie de détection et résolution** (cette section) structure l'approche : confirmer le symptôme, reproduire, analyser, corriger, vérifier, et surtout prévenir. Les patterns de correction — smart pointers, RAII, `weak_ptr`, structures bornées — sont les briques qui transforment un code sujet aux fuites en un code structurellement sûr.

En combinant ces outils avec les sanitizers du chapitre 29 et les principes de C++ moderne (smart pointers du chapitre 9, RAII du chapitre 6.3, conteneurs STL du module 5), les fuites mémoire passent du statut de fatalité à celui de bug détectable et évitable dès les premières phases de développement.

⏭️ [Profiling de Performance](/31-profiling/README.md)
