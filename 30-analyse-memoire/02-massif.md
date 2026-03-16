🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 30.2 — Heap profiling avec Massif

## Introduction

Memcheck répond à la question « *mon programme a-t-il des erreurs mémoire ?* ». Massif répond à une question complémentaire et tout aussi importante : « *comment mon programme utilise-t-il la mémoire au fil du temps ?* ».

Un programme peut être exempt de fuites — chaque `new` a son `delete`, chaque allocation est correctement libérée — et pourtant consommer une quantité de mémoire déraisonnable. Un `std::vector` qui grossit sans jamais être réduit, une structure de cache qui accumule des entrées sans politique d'éviction, un algorithme qui crée des copies temporaires massives : autant de situations où la mémoire est techniquement correcte mais pratiquement problématique. En production, ces comportements se traduisent par des OOM kills, des contentions avec d'autres services sur la même machine, ou des coûts d'infrastructure gonflés.

Massif est l'outil de *heap profiling* de la suite Valgrind. Il échantillonne l'état du heap à intervalles réguliers pendant l'exécution du programme et produit un profil temporel de la consommation mémoire. Pour chaque échantillon (*snapshot*), Massif enregistre non seulement la quantité totale de mémoire allouée, mais aussi la répartition par site d'allocation — quelle fonction a alloué combien. Ce sont ces profils qui permettent de répondre aux questions d'optimisation : « *où part la mémoire ?* » et « *quand le pic se produit-il ?* ».

---

## Principe de fonctionnement

Massif s'exécute sur la même infrastructure de traduction binaire dynamique que Memcheck (section 30.1), mais son instrumentation est différente. Plutôt que de traquer chaque lecture et écriture, Massif intercepte les appels d'allocation (`malloc`, `new`, `realloc`) et de libération (`free`, `delete`) pour maintenir un modèle de l'état du heap.

À intervalles réguliers — déterminés par le volume d'allocations, pas par le temps horloge — Massif prend un *snapshot* qui enregistre :

- La **quantité totale** de mémoire allouée sur le heap à cet instant.
- L'**overhead** de l'allocateur : les octets consommés par les métadonnées internes de `malloc` (headers de blocs, alignement, padding). Cette surcharge est invisible pour le programme mais bien réelle pour le système.
- Pour les snapshots détaillés, l'**arbre d'allocation** complet : chaque branche représente une pile d'appels et indique combien d'octets sont alloués via ce chemin.

Le résultat est un fichier binaire (par défaut `massif.out.<pid>`) qui se visualise avec l'outil `ms_print` ou des interfaces graphiques comme **Massif-Visualizer**.

Le surcoût de Massif est nettement inférieur à celui de Memcheck : typiquement un ralentissement de 2 à 3× (contre 10 à 20× pour Memcheck), car il n'a pas besoin de maintenir un *shadow memory* complet.

---

## Lancer une analyse Massif

### Exécution de base

```bash
valgrind --tool=massif ./mon_programme
```

Cette commande exécute le programme sous Massif et produit un fichier `massif.out.<pid>` dans le répertoire courant. Le programme s'exécute normalement — sa sortie standard et ses interactions ne sont pas affectées.

### Avec des options courantes

```bash
valgrind --tool=massif \
    --pages-as-heap=no \
    --detailed-freq=10 \
    --max-snapshots=200 \
    --time-unit=B \
    ./mon_programme --ses-arguments
```

Les options les plus utiles :

**`--time-unit`** contrôle l'axe horizontal du profil. Trois valeurs sont possibles :

- `B` (par défaut) : le temps est mesuré en octets alloués et libérés. Chaque allocation/libération fait avancer le compteur. C'est le mode le plus utile pour comprendre le comportement mémoire, car il est indépendant de la vitesse d'exécution.
- `ms` : le temps est mesuré en millisecondes. Utile lorsque vous voulez corréler la consommation mémoire avec des événements temporels (requêtes, cycles de traitement).
- `i` : le temps est mesuré en instructions exécutées. Reproductible entre exécutions mais peu intuitif.

**`--detailed-freq=N`** : par défaut, Massif ne produit un snapshot détaillé (avec l'arbre d'allocation complet) qu'un snapshot sur 10. Réduire `N` (par exemple à 1) produit un profil plus riche mais un fichier de sortie plus volumineux.

**`--max-snapshots=N`** : le nombre maximum de snapshots conservés (défaut : 100). Si le programme s'exécute longtemps, Massif fusionne les anciens snapshots pour rester dans cette limite. Augmentez cette valeur pour les programmes à durée de vie longue.

**`--pages-as-heap=yes`** : instrumente les allocations au niveau des pages système (`mmap`, `brk`) plutôt qu'au niveau de `malloc`. Utile pour les programmes qui utilisent des allocateurs mémoire personnalisés qui contournent `malloc`, ou pour mesurer la mémoire totale du processus incluant les mappings de fichiers.

**`--threshold=N`** : pourcentage minimal pour qu'un site d'allocation apparaisse individuellement dans l'arbre. Par défaut 1.0 (1%). Les sites en dessous sont regroupés dans un nœud « below threshold ». Réduisez cette valeur pour un profil plus granulaire.

---

## Visualiser les résultats avec `ms_print`

`ms_print` est l'outil en ligne de commande fourni avec Valgrind pour lire les fichiers Massif. Il produit un graphique ASCII et des arbres d'allocation textuels.

```bash
ms_print massif.out.12345
```

### Le graphique ASCII

La première section de la sortie est un graphique montrant l'évolution de la consommation mémoire dans le temps :

```
    MB
3.500^                                              ##                     
     |                                           @@##::                    
     |                                        @@:@@##::@                   
     |                                     @@:@@:@@##::@:                  
     |                                  @@:@@:@@:@@##::@::                 
     |                               @@:@@:@@:@@:@@##::@:::               
     |                            @@:@@:@@:@@:@@:@@##::@::::              
     |                         @@:@@:@@:@@:@@:@@:@@##::@:::::             
     |                      @@:@@:@@:@@:@@:@@:@@:@@##::@::::::            
     |                   @@:@@:@@:@@:@@:@@:@@:@@:@@##::@:::::::           
     |                @@:@@:@@:@@:@@:@@:@@:@@:@@:@@##::@::::::::          
     |            .:::@@:@@:@@:@@:@@:@@:@@:@@:@@:@@##::@:::::::::         
     |         .:::.::@@:@@:@@:@@:@@:@@:@@:@@:@@:@@##::@::::::::::        
     |      .:::.:::.::@@:@@:@@:@@:@@:@@:@@:@@:@@:@@##::@:::::::::::      
     |   .:::.:::.:::.::@@:@@:@@:@@:@@:@@:@@:@@:@@:@@##::@::::::::::::    
     | :::.:::.:::.:::.::@@:@@:@@:@@:@@:@@:@@:@@:@@:@@##::@:::::::::::::  
   0 +----------------------------------------------------------------------> MB
     0                                                                  350.0
```

Chaque colonne représente un snapshot. Les symboles différents (`:`, `@`, `#`, `.`) représentent les différentes sources d'allocation, ce qui permet de voir visuellement quelle partie du code domine la consommation mémoire et comment elle évolue.

Le pic du graphique correspond au **moment de consommation maximale**. C'est souvent le snapshot le plus intéressant à examiner en détail.

### Les snapshots détaillés

Sous le graphique, `ms_print` affiche les arbres d'allocation des snapshots détaillés. Voici un exemple annoté :

```
--------------------------------------------------------------------------------
  n        time(B)         total(B)   useful-heap(B) extra-heap(B)    stacks(B)
--------------------------------------------------------------------------------
 45      2,800,000        3,670,016      3,500,000       170,016            0
```

Cette ligne résume le snapshot n°45 :

- **time(B)** : 2 800 000 octets ont été alloués et libérés au total depuis le début de l'exécution (c'est le compteur de « temps » en mode `--time-unit=B`).
- **total(B)** : 3 670 016 octets sont actuellement consommés sur le heap.
- **useful-heap(B)** : 3 500 000 octets sont effectivement demandés par le programme via `new`/`malloc`.
- **extra-heap(B)** : 170 016 octets d'overhead de l'allocateur (headers, alignement, padding).
- **stacks(B)** : 0 par défaut ; Massif peut aussi profiler la pile avec `--stacks=yes`, mais cette option est rarement nécessaire et coûteuse.

Suit l'arbre d'allocation pour ce snapshot :

```
99.54% (3,484,000B) (heap allocation functions) malloc/new/new[], --alloc-fns, etc.
->74.29% (2,600,000B) 0x109A50: ServeurHTTP::accepter_connexion() (serveur.cpp:87)
| ->74.29% (2,600,000B) 0x109D20: ServeurHTTP::boucle_principale() (serveur.cpp:142)
|   ->74.29% (2,600,000B) 0x10A100: main (main.cpp:35)
|
->18.57% (650,000B) 0x109B80: Cache::inserer(std::string const&, Donnee const&) (cache.cpp:52)
| ->18.57% (650,000B) 0x109C40: ServeurHTTP::traiter_requete(Requete const&) (serveur.cpp:110)
|   ->18.57% (650,000B) 0x109D20: ServeurHTTP::boucle_principale() (serveur.cpp:142)
|     ->18.57% (650,000B) 0x10A100: main (main.cpp:35)
|
->06.69% (234,000B) in 12 places, all below massif's threshold (1.00%)
```

Cet arbre se lit de haut en bas :

1. 99.54% de la mémoire heap provient de fonctions d'allocation standard.
2. 74.29% (2,6 Mo) est alloué par `ServeurHTTP::accepter_connexion()` à la ligne 87 de `serveur.cpp`. C'est le site d'allocation dominant — la première cible d'investigation.
3. 18.57% (650 Ko) est alloué par `Cache::inserer()`. C'est la seconde cible.
4. Les 6.69% restants sont répartis entre 12 sites d'allocation, chacun en dessous du seuil de 1%. Ils sont regroupés pour la lisibilité.

La structure hiérarchique de l'arbre montre les chaînes d'appels. Si `accepter_connexion()` est appelée depuis plusieurs sites différents, chaque chemin apparaît comme une branche distincte, ce qui permet de comprendre quel flux d'exécution est responsable de la consommation.

---

## Visualisation graphique avec Massif-Visualizer

`ms_print` est pratique pour une analyse rapide en terminal, mais pour les profils complexes, **Massif-Visualizer** offre une visualisation interactive nettement plus lisible.

### Installation

```bash
sudo apt install massif-visualizer
```

### Utilisation

```bash
massif-visualizer massif.out.12345
```

L'outil ouvre une fenêtre graphique avec :

- Un **graphique en aires empilées** (*stacked area chart*) montrant l'évolution de chaque site d'allocation dans le temps. Chaque couleur correspond à un site, et les aires sont empilées pour que la hauteur totale représente la consommation globale. Ce graphique révèle immédiatement les tendances : croissance linéaire (fuite probable), dents de scie (allocation/libération cyclique), pic isolé (traitement ponctuel gourmand).

- Un **arbre d'allocation interactif** pour chaque snapshot. Un clic sur un point du graphique affiche l'arbre correspondant avec des nœuds dépliables, des pourcentages et des liens vers le code source.

- Un **tableau des snapshots** avec les métriques détaillées (useful-heap, extra-heap, total) pour chaque point de mesure.

Pour les analyses approfondies ou les rapports à partager avec une équipe, Massif-Visualizer est l'outil recommandé.

---

## Scénarios d'analyse courants

### Identifier un pic de consommation mémoire

Un service backend consomme 4 Go de RAM au pic, alors que les données traitées ne devraient pas dépasser quelques centaines de Mo. L'analyse Massif révèle :

```bash
valgrind --tool=massif --time-unit=ms --detailed-freq=1 ./mon_service
```

En examinant le snapshot au pic dans `ms_print` ou Massif-Visualizer, on découvre que 80% de la mémoire est allouée par une fonction `deserialiser_batch()` qui crée des copies temporaires de chaque enregistrement. L'arbre d'allocation montre la chaîne complète :

```
->80.12% (3,280,000B) 0x10B200: deserialiser_batch() (parser.cpp:156)
  ->80.12% (3,280,000B) 0x10B450: Pipeline::executer() (pipeline.cpp:78)
```

La solution : traiter les enregistrements un par un (*streaming*) plutôt que de tout charger en mémoire, ou utiliser `std::string_view` et `std::span` pour éviter les copies.

### Détecter une croissance mémoire sans fuite

Le programme ne fuit pas (Memcheck confirme zéro fuite), mais sa consommation mémoire augmente de façon monotone au fil des heures. C'est le signe d'une structure de données qui grossit sans limite.

```bash
valgrind --tool=massif --time-unit=ms --max-snapshots=500 ./mon_service_long
```

Le graphique montre une rampe ascendante continue. L'arbre d'allocation du dernier snapshot pointe vers :

```
->62.40% (850,000,000B) 0x10C300: Cache::ajouter(Cle const&, Valeur const&) (cache.cpp:44)
```

Le cache accumule des entrées sans jamais en supprimer. C'est un problème de conception : il manque une politique d'éviction (LRU, TTL, taille maximale). Ce n'est pas une fuite au sens de Memcheck — chaque entrée est accessible via le `std::unordered_map` du cache — mais c'est un problème tout aussi sérieux en production.

### Mesurer l'overhead de l'allocateur

La colonne `extra-heap(B)` de Massif quantifie l'overhead de `malloc`. Pour un programme qui effectue un grand nombre de petites allocations, cet overhead peut représenter 20 à 50% de la mémoire totale.

Par exemple, un programme qui alloue des millions d'objets de 16 octets :

```
  n        time(B)         total(B)   useful-heap(B) extra-heap(B)    stacks(B)
 80    200,000,000      280,000,000    160,000,000   120,000,000            0
```

Ici, 120 Mo sont consommés uniquement par les métadonnées de l'allocateur — 75% de surcharge par rapport aux données utiles. Ce profil justifie l'utilisation d'un pool allocator ou le regroupement des objets dans des conteneurs contigus (`std::vector<Objet>` plutôt que `std::vector<std::unique_ptr<Objet>>`).

### Comparer deux versions du même programme

Massif est un excellent outil de régression mémoire. En profilant la même charge de travail sur deux versions du code, vous pouvez vérifier qu'une optimisation a bien réduit la consommation ou qu'un refactoring n'a pas introduit de dégradation.

```bash
# Version avant optimisation
valgrind --tool=massif --massif-out-file=avant.massif ./mon_programme_v1 < donnees_test.txt

# Version après optimisation
valgrind --tool=massif --massif-out-file=apres.massif ./mon_programme_v2 < donnees_test.txt

# Comparaison
ms_print avant.massif > avant.txt  
ms_print apres.massif > apres.txt  
diff avant.txt apres.txt  
```

Pour une comparaison visuelle, ouvrez les deux fichiers dans deux instances de Massif-Visualizer côte à côte. Le pic de consommation et la forme générale de la courbe sont les indicateurs les plus révélateurs.

---

## Profiler la pile (stack)

Par défaut, Massif ne profile que le heap. L'option `--stacks=yes` active le profiling de la pile :

```bash
valgrind --tool=massif --stacks=yes ./mon_programme
```

La consommation de la pile apparaît alors dans la colonne `stacks(B)` des snapshots. Cette option est rarement nécessaire, car la pile est généralement petite (quelques Mo par thread). Elle est utile dans deux cas spécifiques :

- Les **programmes récursifs** où la profondeur de récursion n'est pas bornée.
- Les **programmes embarqués** ou à pile limitée où chaque kilo-octet compte.

Le surcoût de `--stacks=yes` est significatif (ralentissement supplémentaire de 2 à 3×), raison pour laquelle cette option est désactivée par défaut.

---

## Limites de Massif

**Granularité temporelle.** Massif échantillonne à des intervalles déterminés par le volume d'allocations, pas en continu. Une allocation très brève (allouer puis libérer immédiatement) peut ne pas apparaître dans un snapshot si elle tombe entre deux points d'échantillonnage. Pour les pics de très courte durée, augmentez `--max-snapshots` et `--detailed-freq`.

**Allocateurs personnalisés.** Si le programme utilise un allocateur mémoire personnalisé (pool allocator, arena allocator, `jemalloc`) qui effectue ses propres subdivisions internes à partir de grands blocs obtenus via `malloc` ou `mmap`, Massif ne voit que les grands blocs, pas les allocations individuelles à l'intérieur. L'option `--pages-as-heap=yes` peut aider, mais elle perd la granularité des piles d'appels.

**Pas de détection de fuites.** Massif profile la consommation mémoire ; il ne détecte pas les erreurs. Un programme qui fuit 10 octets par seconde sans jamais crasher produira un profil Massif parfaitement lisible montrant une rampe ascendante — mais Massif ne *signalera* pas que c'est une fuite. Pour la détection, utilisez Memcheck.

**Surcoût.** Bien que nettement plus léger que Memcheck, le ralentissement de 2 à 3× reste trop important pour un profiling en production. Massif est un outil de développement et de pré-production.

---

## Quand utiliser Massif vs d'autres outils

| Besoin | Outil recommandé |
|---|---|
| Détecter les fuites et erreurs mémoire | Memcheck (section 30.1) |
| Profiler la consommation heap dans le temps | **Massif** |
| Profiler la consommation en production | `heaptrack`, ou compteurs internes applicatifs |
| Mesurer la mémoire résidente (RSS) du processus | `top`, `htop`, `/proc/<pid>/status` |
| Profiler les allocations avec granularité fine | `heaptrack` (plus rapide que Massif, visualisation avec `heaptrack_gui`) |
| Identifier les sites d'allocation chauds pour optimisation | Massif ou `heaptrack` |

`heaptrack` est une alternative moderne à Massif, développée par KDE. Il fonctionne par préchargement de librairie (`LD_PRELOAD`) plutôt que par traduction binaire, ce qui le rend 5 à 10× plus rapide que Massif. Pour les programmes de longue durée ou les suites de tests volumineuses, `heaptrack` est souvent un meilleur choix. Massif reste pertinent pour sa disponibilité universelle (installé avec Valgrind) et sa capacité à mesurer l'overhead de l'allocateur.

⏭️ [Memory leaks : Détection et résolution](/30-analyse-memoire/03-memory-leaks.md)
