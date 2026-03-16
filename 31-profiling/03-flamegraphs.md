🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 31.3 — Flamegraphs et visualisation

## Introduction

Les rapports textuels de `perf report` et `gprof` sont précis, mais ils atteignent rapidement leurs limites face à un programme complexe. Un profil avec des centaines de fonctions, des dizaines de niveaux de profondeur dans les piles d'appels, et des chemins d'exécution qui divergent selon les données d'entrée produit un volume d'information difficile à appréhender sous forme de tableaux ou d'arbres textuels. L'œil humain est bien meilleur pour repérer des patterns visuels — une bande de couleur anormalement large, une tour qui dépasse — que pour comparer des colonnes de pourcentages.

Les **flamegraphs** (*graphiques en flammes*), inventés par Brendan Gregg en 2011, résolvent ce problème. Ils condensent un profil de performance entier en une seule image SVG interactive où la largeur de chaque rectangle est proportionnelle au temps CPU consommé. En un coup d'œil, les hotspots sautent aux yeux — littéralement les « flammes » les plus larges du graphique. Depuis leur création, les flamegraphs sont devenus l'outil de visualisation de référence dans l'industrie, utilisés aussi bien par les développeurs individuels que par les équipes SRE des plus grandes plateformes.

---

## Anatomie d'un flamegraph

Un flamegraph est un graphique en rectangles empilés (*stacked bar chart*) où chaque rectangle représente une fonction dans une pile d'appels :

```
┌──────────────────────────────────────────────────────────────────────┐
│                              main                                    │
├──────────────────────────────────┬───────────────────────────────────┤
│      Pipeline::executer          │       Config::charger             │
├──────────────┬───────────────────┤                                   │
│ Parser::     │ Resultat::        │                                   │
│ analyser_    │ stocker            │                                   │
│ token        │                   │                                   │
├──────┬───────┤                   │                                   │
│Buffer│is_del │                   │                                   │
│copier│imiter │                   │                                   │
└──────┴───────┴───────────────────┴───────────────────────────────────┘
```

Les conventions de lecture :

**L'axe vertical** représente la profondeur de la pile d'appels. La fonction racine (`main`) est en bas. Chaque niveau au-dessus est une fonction appelée par la fonction du niveau inférieur. Le sommet du graphique représente les fonctions *feuilles* — celles qui étaient en cours d'exécution au moment du sampling.

**L'axe horizontal** représente le **temps CPU cumulé**, pas le temps chronologique. La largeur de chaque rectangle est proportionnelle au nombre de samples dans lesquels cette fonction apparaît dans la pile. Une fonction qui occupe 40% de la largeur totale du graphique consomme 40% du temps CPU.

**L'ordre horizontal** n'a pas de signification temporelle. Les fonctions au même niveau sont triées alphabétiquement (pas dans l'ordre d'appel). Cela permet de fusionner les occurrences d'une même fonction appelée depuis le même parent, quelle que soit leur position dans le temps. Ne cherchez pas à lire un flamegraph de gauche à droite comme une timeline.

**Les couleurs** sont arbitraires dans un flamegraph classique — elles servent uniquement à distinguer visuellement les rectangles adjacents. Certains outils assignent des couleurs par catégorie (espace utilisateur, noyau, librairies), mais la largeur est toujours l'information primaire.

### Ce qu'il faut chercher

Les **plateaux larges au sommet** sont les hotspots. Un rectangle large au sommet du graphique signifie que la fonction correspondante est fréquemment la fonction en cours d'exécution (self time élevé). C'est l'équivalent visuel d'un pourcentage élevé dans la colonne `Self` de `perf report`.

Les **tours étroites et profondes** indiquent des chaînes d'appels profondes qui consomment peu de temps individuellement. Elles sont rarement problématiques du point de vue performance.

Les **colonnes larges traversant plusieurs niveaux** indiquent une fonction de haut niveau qui est responsable d'une grande part du temps CPU total (children time élevé). C'est un point d'entrée pour l'optimisation architecturale : même si aucune de ses sous-fonctions n'est individuellement dominante, l'ensemble du sous-arbre mérite attention.

---

## Générer un flamegraph depuis `perf`

### Installation des outils de Brendan Gregg

Les scripts de génération de flamegraphs sont maintenus dans un dépôt GitHub :

```bash
git clone https://github.com/brendangregg/FlameGraph.git
```

Le dépôt contient deux scripts essentiels :

- `stackcollapse-perf.pl` : convertit la sortie de `perf script` en un format intermédiaire (une ligne par pile d'appels avec comptage).
- `flamegraph.pl` : génère le fichier SVG interactif à partir du format intermédiaire.

### Pipeline complet

Le workflow standard en trois commandes :

```bash
# 1. Enregistrer le profil avec piles d'appels
perf record --call-graph fp -F 4000 ./mon_programme < donnees_test.txt

# 2. Exporter les samples bruts
perf script > profil_brut.txt

# 3. Générer le flamegraph
cat profil_brut.txt | ./FlameGraph/stackcollapse-perf.pl | ./FlameGraph/flamegraph.pl > profil.svg
```

Ou en une seule ligne de pipeline :

```bash
perf script | ./FlameGraph/stackcollapse-perf.pl | ./FlameGraph/flamegraph.pl > profil.svg
```

Le fichier `profil.svg` peut être ouvert dans n'importe quel navigateur web. Le SVG est interactif : survoler un rectangle affiche le nom complet de la fonction et son pourcentage de samples, cliquer sur un rectangle « zoome » pour montrer uniquement ce sous-arbre en pleine largeur.

### Options de génération

Le script `flamegraph.pl` accepte de nombreuses options de personnalisation :

```bash
perf script | ./FlameGraph/stackcollapse-perf.pl | ./FlameGraph/flamegraph.pl \
    --title "Profil mon_programme v2.3 — charge 10K requêtes" \
    --subtitle "perf record -F 4000, 32541 samples" \
    --width 1400 \
    --height 16 \
    --minwidth 0.5 \
    --colors hot \
    > profil.svg
```

| Option | Effet |
|---|---|
| `--title` | Titre affiché en haut du flamegraph |
| `--subtitle` | Sous-titre (conditions de mesure, nombre de samples) |
| `--width` | Largeur du SVG en pixels (défaut : 1200) |
| `--height` | Hauteur de chaque rectangle en pixels (défaut : 16) |
| `--minwidth` | Largeur minimale en pixels pour qu'une fonction apparaisse (défaut : 0.1) |
| `--colors` | Palette de couleurs : `hot`, `mem`, `io`, `java`, `js`, `aqua` |
| `--countname` | Unité affichée au survol (défaut : `samples`) |
| `--reverse` | Génère un *icicle graph* inversé (voir ci-dessous) |
| `--inverted` | Idem `--reverse` |

---

## Flamegraphs inversés (Icicle Graphs)

Un flamegraph classique a la racine (`main`) en bas et les fonctions feuilles en haut. Un **flamegraph inversé** — aussi appelé *icicle graph* — inverse cette orientation : les fonctions feuilles sont en bas et les appelants s'empilent au-dessus.

```bash
perf script | ./FlameGraph/stackcollapse-perf.pl | ./FlameGraph/flamegraph.pl \
    --reverse > profil_inverse.svg
```

L'intérêt de cette vue est de répondre à une question différente. Le flamegraph classique répond à « *par quel chemin arrive-t-on dans cette fonction ?* » (lecture de bas en haut). Le flamegraph inversé répond à « *qui appelle le plus cette fonction coûteuse ?* » (les appelants les plus gourmands sont les rectangles les plus larges au-dessus de la fonction).

Concrètement, si `memcpy` apparaît comme un hotspot dans le flamegraph classique, il est difficile de voir rapidement *qui* l'appelle le plus, car les appels à `memcpy` proviennent de dizaines de sites différents dispersés horizontalement. Dans le flamegraph inversé, `memcpy` est un rectangle large en bas, et ses appelants sont empilés au-dessus, triés visuellement par contribution. Le plus gros appelant est immédiatement visible.

Les deux vues sont complémentaires. En pratique, commencez par le flamegraph classique pour identifier les hotspots, puis passez au flamegraph inversé pour comprendre qui les déclenche.

---

## On-CPU vs Off-CPU flamegraphs

Les flamegraphs décrits jusqu'ici sont des **on-CPU flamegraphs** : ils visualisent le temps passé à *exécuter des instructions* sur le CPU. Ils ne montrent pas le temps passé à *attendre* — I/O disque, réseau, locks, sleep. Pour un programme I/O-bound, un on-CPU flamegraph peut être presque vide (le programme passe peu de temps sur le CPU) alors que le programme est objectivement lent.

Les **off-CPU flamegraphs** visualisent le temps passé *hors* du CPU — bloqué en attente. Ils utilisent les tracepoints du scheduler du noyau pour enregistrer le moment où un thread est dé-schedulé et le moment où il reprend :

```bash
# Enregistrer les événements de scheduling (nécessite root ou perf_event_paranoid=-1)
perf record -e sched:sched_switch -e sched:sched_stat_sleep \
    --call-graph fp -p $(pgrep mon_service) -- sleep 30

# Générer le off-CPU flamegraph
perf script | ./FlameGraph/stackcollapse-perf.pl | ./FlameGraph/flamegraph.pl \
    --colors io --title "Off-CPU Flamegraph" > offcpu.svg
```

Dans un off-CPU flamegraph, les rectangles larges indiquent les fonctions qui provoquent les plus longues attentes. Un rectangle large étiqueté `__pthread_mutex_lock` pointe vers une contention de verrou ; un rectangle `read` ou `recv` pointe vers une attente I/O.

| Type | Ce qu'il montre | Quand l'utiliser |
|---|---|---|
| On-CPU | Temps CPU actif (instructions exécutées) | Programme CPU-bound (`CPUs utilized` ≈ 1.0) |
| Off-CPU | Temps d'attente (I/O, locks, sleep) | Programme I/O-bound (`CPUs utilized` << 1.0) |

Le diagnostic complet d'un programme lent combine souvent les deux : le on-CPU flamegraph pour optimiser le calcul, le off-CPU flamegraph pour optimiser les attentes.

---

## Differential Flamegraphs

Les *differential flamegraphs* (ou *diff flamegraphs*) visualisent la **différence** entre deux profils — typiquement avant et après une optimisation, ou entre deux versions du programme. Les rectangles sont colorés selon le delta : rouge pour les fonctions qui ont ralenti, bleu pour celles qui ont accéléré.

```bash
# Profiler la version avant
perf record -o avant.data --call-graph fp ./programme_v1 < donnees.txt  
perf script -i avant.data > avant.txt  

# Profiler la version après
perf record -o apres.data --call-graph fp ./programme_v2 < donnees.txt  
perf script -i apres.data > apres.txt  

# Générer les formats intermédiaires
./FlameGraph/stackcollapse-perf.pl avant.txt > avant.folded
./FlameGraph/stackcollapse-perf.pl apres.txt > apres.folded

# Générer le differential flamegraph
./FlameGraph/difffolded.pl avant.folded apres.folded | ./FlameGraph/flamegraph.pl > diff.svg
```

Le script `difffolded.pl` calcule le delta pour chaque pile d'appels. Dans le SVG résultant :

- Les rectangles **rouges** indiquent les fonctions dont la part de samples a *augmenté* (régression).
- Les rectangles **bleus** indiquent les fonctions dont la part a *diminué* (amélioration).
- Les rectangles **blancs/gris** sont stables.

Les differential flamegraphs sont un outil puissant de code review : en incluant le SVG dans une pull request, les reviewers voient immédiatement l'impact de la modification sur le profil de performance. Un rectangle rouge large et inattendu signale une régression qui mérite investigation.

> ⚠️ **Même limitation que `perf diff`** : les differential flamegraphs comparent des pourcentages relatifs, pas des temps absolus. Si le programme global est 2× plus rapide, les fonctions non modifiées apparaissent en rouge (leur part relative augmente dans un total plus petit). Combinez toujours le diff flamegraph avec une mesure du temps d'exécution total.

---

## Alternatives pour la génération de flamegraphs

Les scripts Perl de Brendan Gregg sont l'outil historique, mais d'autres outils produisent des flamegraphs avec des fonctionnalités supplémentaires.

### `perf` natif (versions récentes)

Depuis les versions récentes du noyau (5.8+), `perf` intègre un générateur de flamegraphs natif :

```bash
perf script report flamegraph
```

Cette commande produit directement un fichier HTML interactif sans dépendance externe. La fonctionnalité est encore moins riche que les scripts de Gregg (moins d'options de personnalisation), mais elle a l'avantage de ne nécessiter aucune installation supplémentaire.

### Hotspot

**Hotspot** (section 31.4) est un outil graphique qui génère des flamegraphs interactifs à partir de fichiers `perf.data`, sans passer par `perf script` ni les scripts Perl. Il offre des fonctionnalités supplémentaires comme le filtrage par thread, le zoom temporel, et la combinaison flamegraph + timeline. C'est l'outil recommandé pour le développement interactif.

### speedscope

[speedscope](https://www.speedscope.app/) est un visualiseur de profils web qui fonctionne entièrement dans le navigateur. Il accepte les fichiers au format `collapsed` (produits par `stackcollapse-perf.pl`) et offre trois vues : flamegraph classique, flamegraph gauche-droite (chronologique), et *sandwich view* (qui isole une fonction et montre à la fois ses appelants et ses appelés).

```bash
# Générer le format collapsed
perf script | ./FlameGraph/stackcollapse-perf.pl > profil.collapsed

# Ouvrir dans speedscope (glisser-déposer le fichier sur speedscope.app)
```

La *sandwich view* de speedscope est particulièrement utile pour les fonctions appelées depuis de nombreux sites (comme `malloc`, `memcpy`, ou un logger) : elle agrège tous les appelants d'un côté et tous les appelés de l'autre, offrant une perspective impossible dans un flamegraph standard.

### Inferno

[Inferno](https://github.com/jonhoo/inferno) est une réécriture en Rust des scripts de Brendan Gregg. Il produit des flamegraphs identiques mais avec des performances de génération nettement supérieures sur les gros profils (millions de samples). L'installation se fait via `cargo` :

```bash
cargo install inferno

# Usage
perf script | inferno-collapse-perf | inferno-flamegraph > profil.svg
```

---

## Bonnes pratiques de visualisation

### Nommer et documenter les profils

Un flamegraph sans contexte est difficile à interpréter. Utilisez toujours `--title` et `--subtitle` pour documenter :

- La version du programme.
- La charge de travail utilisée (fichier d'entrée, nombre de requêtes, configuration).
- Les conditions de mesure (machine, fréquence d'échantillonnage, nombre de samples).

```bash
flamegraph.pl \
    --title "serveur-http v3.1.0 — 50K requêtes GET /api/users" \
    --subtitle "$(date +%Y-%m-%d) — perf -F 4000 — $(hostname) — $(nproc) cores" \
    > profil_v3.1.0.svg
```

### Archiver les profils de référence

Conservez les flamegraphs des versions majeures comme baseline. Lorsqu'une régression de performance est suspectée, le differential flamegraph entre la baseline et la version courante identifie immédiatement les fonctions responsables.

### Filtrer le bruit

Pour les programmes qui passent du temps significatif dans les librairies système, filtrer les piles d'appels peut clarifier la visualisation :

```bash
# Exclure les fonctions de la librairie C du collapsing
perf script | ./FlameGraph/stackcollapse-perf.pl | \
    grep -v "^__libc" | \
    ./FlameGraph/flamegraph.pl > profil_filtre.svg
```

Soyez prudent avec le filtrage : supprimer `memcpy` ou `malloc` masque un temps CPU réel. Ne filtrez que les fonctions d'infrastructure dont vous savez qu'elles ne sont pas optimisables dans votre contexte.

### Combiner on-CPU et off-CPU

Pour un diagnostic complet, générez les deux flamegraphs et placez-les côte à côte. Le on-CPU révèle les hotspots de calcul, le off-CPU révèle les goulots d'I/O et de contention. L'ensemble couvre la totalité du temps d'exécution du programme.

---

## Intégration en CI

Les flamegraphs peuvent être générés automatiquement dans un pipeline CI et attachés comme artefacts à chaque build ou release :

```yaml
# .gitlab-ci.yml (extrait)
profiling:
  stage: benchmark
  script:
    - g++ -std=c++23 -O2 -g -fno-omit-frame-pointer -o mon_programme src/*.cpp
    - perf record --call-graph fp -F 4000 -o perf.data ./mon_programme < benchmark_data.txt
    - perf script -i perf.data | stackcollapse-perf.pl | flamegraph.pl
        --title "CI Build $CI_PIPELINE_ID"
        > flamegraph.svg
  artifacts:
    paths:
      - flamegraph.svg
    expire_in: 30 days
```

En comparant les flamegraphs de builds successifs (visuellement ou via differential flamegraphs automatisés), l'équipe détecte les régressions de performance avant qu'elles n'atteignent la production. C'est l'équivalent pour la performance de ce que Valgrind en CI fait pour les fuites mémoire (section 30.3).

⏭️ [Hotspot et outils graphiques](/31-profiling/04-hotspot.md)
