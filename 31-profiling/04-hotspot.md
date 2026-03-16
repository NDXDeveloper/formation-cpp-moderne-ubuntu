🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 31.4 — Hotspot et outils graphiques

## Introduction

Les sections précédentes ont couvert les outils en ligne de commande : `perf record`/`perf report` pour l'analyse interactive en terminal, `perf stat` pour les compteurs matériels, `gprof` pour le profiling par instrumentation, et les scripts de Brendan Gregg pour les flamegraphs. Ces outils sont puissants et omniprésents sur les serveurs Linux, mais leur interface textuelle impose une charge cognitive élevée lorsqu'il s'agit d'explorer un profil complexe — naviguer dans des arbres de centaines de fonctions, corréler un flamegraph avec une timeline, ou comparer visuellement le profil de deux threads.

Les outils graphiques comblent cette lacune. Ils consomment les mêmes données que les outils en ligne de commande (`perf.data`, fichiers Callgrind, fichiers collapsed) mais les présentent dans des interfaces interactives avec des graphiques, des filtres visuels et des vues multiples synchronisées. Pour le développement quotidien sur une machine de bureau, ils accélèrent considérablement le cycle d'investigation.

Cette section présente les principaux outils graphiques de l'écosystème Linux pour le profiling de performance C++.

---

## Hotspot : l'interface graphique de référence pour `perf`

### Présentation

[Hotspot](https://github.com/KDAB/hotspot) est un outil open source développé par KDAB (les auteurs de Valgrind-visualizer et de nombreux outils Qt/KDE). Il fonctionne comme un frontend graphique pour les fichiers `perf.data` : il lit directement les fichiers produits par `perf record` et les présente dans une interface riche et intuitive.

Hotspot est à `perf report` ce qu'un IDE est à un éditeur de texte en terminal : il offre les mêmes fonctionnalités fondamentales, mais les rend plus accessibles grâce à la navigation visuelle, les filtres interactifs et les vues multiples.

### Installation

Sur Ubuntu :

```bash
sudo apt install hotspot
```

Si la version des dépôts est trop ancienne, Hotspot est également distribué en AppImage depuis le dépôt GitHub :

```bash
wget https://github.com/KDAB/hotspot/releases/download/v1.6.0/hotspot-v1.6.0-x86_64.AppImage  
chmod +x hotspot-v1.6.0-x86_64.AppImage  
./hotspot-v1.6.0-x86_64.AppImage
```

### Workflow

Le workflow est le même qu'avec `perf report`, à l'étape d'analyse près :

```bash
# 1. Enregistrer le profil (identique)
perf record --call-graph fp -F 4000 ./mon_programme < donnees_test.txt

# 2. Ouvrir dans Hotspot (remplace perf report)
hotspot perf.data
```

Hotspot peut aussi lancer l'enregistrement lui-même via son interface graphique, sans passer par la ligne de commande.

### Les vues de Hotspot

Hotspot organise l'information en plusieurs onglets, chacun offrant une perspective différente sur le même profil.

#### Vue Summary

L'écran d'accueil affiche un résumé du profil : durée d'exécution, nombre de samples, nombre de threads, et un top 5 des fonctions les plus coûteuses. C'est l'équivalent visuel du haut de `perf report --stdio`. Un flamegraph miniature donne une vue d'ensemble immédiate.

#### Vue Bottom-Up

Équivalente à la colonne `Self` de `perf report` : les fonctions sont triées par self time décroissant. En dépliant une fonction, on voit ses **appelants** (qui appelle cette fonction ?). C'est la vue la plus directe pour identifier les hotspots et comprendre d'où viennent les appels.

Cette vue est l'analogue visuel du flamegraph inversé (section 31.3) : elle part des fonctions feuilles et remonte vers les appelants. Pour chaque fonction, Hotspot affiche le self time, le inclusive time, le nombre de samples, et la librairie d'origine.

#### Vue Top-Down

L'inverse de Bottom-Up : les fonctions sont présentées comme un arbre enraciné à `main` (ou aux points d'entrée des threads). En dépliant un nœud, on voit ses **appelés** (quelles fonctions cette fonction appelle-t-elle ?). C'est l'équivalent de la colonne `Children` de `perf report`.

Cette vue est idéale pour comprendre la structure d'exécution du programme : quels sous-systèmes sont les plus coûteux dans leur ensemble, même si aucune de leurs fonctions individuelles ne domine le profil.

#### Vue Flame Graph

Un flamegraph interactif intégré directement dans l'interface. Contrairement aux SVG statiques générés par les scripts de Gregg, le flamegraph de Hotspot est synchronisé avec les autres vues : cliquer sur une fonction dans le flamegraph la sélectionne dans les vues Bottom-Up et Top-Down, et inversement.

Le flamegraph de Hotspot supporte le zoom, le filtrage par recherche textuelle (les rectangles correspondants sont mis en surbrillance), et la bascule entre vue classique et vue inversée (*icicle*) d'un clic.

#### Vue Caller/Callee

Pour une fonction sélectionnée, cette vue affiche simultanément ses appelants (à gauche) et ses appelés (à droite), avec les pourcentages de temps pour chaque relation. C'est l'équivalent visuel du call graph de `gprof`, mais interactif et navigable.

#### Vue Timeline

La timeline est la fonctionnalité qui distingue le plus Hotspot des outils en ligne de commande. Elle affiche l'activité CPU de chaque thread sur un axe temporel horizontal :

- Chaque thread est une ligne horizontale.
- Les zones colorées indiquent les périodes d'activité CPU.
- Les zones vides indiquent les périodes d'attente (off-CPU).

En sélectionnant un intervalle de temps sur la timeline, toutes les autres vues (flamegraph, Bottom-Up, Top-Down) se mettent à jour pour ne montrer que les données de cet intervalle. Cette fonctionnalité est précieuse pour les programmes dont le profil varie dans le temps : un serveur qui a un comportement différent au démarrage, pendant la charge, et à l'arrêt peut être analysé phase par phase.

#### Vue Source Code

Hotspot intègre un visualiseur de code source annoté, équivalent à `perf annotate` mais avec la coloration syntaxique et la navigation source complète. En double-cliquant sur une fonction dans n'importe quelle vue, le code source correspondant s'affiche avec les pourcentages de samples par ligne.

### Filtrage par thread et par processus

Pour les programmes multithreads, Hotspot permet de filtrer le profil par thread. Un menu déroulant liste tous les threads avec leur TID et leur proportion du temps CPU total. En sélectionnant un thread spécifique, toutes les vues se mettent à jour pour ne montrer que l'activité de ce thread. Cette fonctionnalité est nettement plus ergonomique que le filtrage par `--tid` de `perf report`.

### Enregistrement intégré

Hotspot peut lancer `perf record` depuis son interface graphique. Le dialogue d'enregistrement permet de configurer :

- Le programme à exécuter et ses arguments.
- La fréquence d'échantillonnage.
- L'événement à échantillonner (cycles, cache-misses, etc.).
- La méthode de reconstruction des piles d'appels (fp, dwarf, lbr).
- L'élévation de privilèges via `pkexec` si nécessaire.

Ce mode « tout-en-un » est pratique pour le développement interactif, où le cycle enregistrer → analyser → modifier → ré-enregistrer se répète rapidement.

---

## KCachegrind : visualisation de profils Callgrind

### Présentation

[KCachegrind](https://kcachegrind.github.io/) est un visualiseur de profils développé dans le cadre du projet KDE. Il est conçu principalement pour les fichiers produits par **Callgrind** (un outil de la suite Valgrind qui profile par instrumentation dynamique), mais il peut aussi consommer des données converties depuis `perf` ou `gprof`.

Callgrind et KCachegrind forment un couple complémentaire à `perf` et Hotspot. Là où `perf` fournit un profil statistique à faible surcoût, Callgrind fournit un profil déterministe et exhaustif — chaque appel de fonction est comptabilisé avec exactitude, et le coût en instructions (pas en temps) est mesuré instruction par instruction. Le surcoût est considérable (10 à 50×), mais le profil est d'une précision absolue.

### Installation

```bash
sudo apt install kcachegrind valgrind
```

### Générer un profil Callgrind

```bash
valgrind --tool=callgrind ./mon_programme < donnees_test.txt
```

Callgrind produit un fichier `callgrind.out.<pid>` dans le répertoire courant.

Pour réduire le volume de données sur un programme de longue durée, on peut activer et désactiver la collecte dynamiquement :

```bash
# Lancer sans collecte
valgrind --tool=callgrind --collect-atstart=no ./mon_service &

# Activer la collecte pour une période spécifique
callgrind_control -i on  
sleep 10  
callgrind_control -i off  

# Arrêter et générer le fichier
callgrind_control -k
```

### Ouvrir dans KCachegrind

```bash
kcachegrind callgrind.out.12345
```

### Les vues de KCachegrind

KCachegrind offre une interface en panneaux multiples :

**Flat Profile (panneau gauche)** : liste des fonctions triées par coût, avec les colonnes `Self` (instructions exécutées dans la fonction) et `Incl.` (instructions incluant les appelés). Le coût est mesuré en nombre d'instructions, pas en temps — ce qui rend les mesures parfaitement reproductibles entre exécutions.

**Call Graph (panneau central)** : un graphe d'appels interactif où les nœuds sont les fonctions et les arêtes les relations d'appel. L'épaisseur de chaque arête est proportionnelle au nombre d'appels ou au coût. Le graphe est navigable : double-cliquer sur un nœud le centre et affiche ses voisins immédiats.

**Source/Annotation (panneau droit)** : le code source annoté avec le coût par ligne, similaire à `perf annotate`. KCachegrind affiche aussi l'assembleur annoté en parallèle du code source, avec des flèches montrant les sauts et les boucles.

**Treemap** : une vue en *treemap* (rectangles imbriqués) où la surface de chaque rectangle est proportionnelle au coût. Les treemaps sont moins intuitives que les flamegraphs pour la plupart des développeurs, mais elles exploitent mieux l'espace 2D pour montrer les proportions relatives.

**Callers/Callees** : pour chaque fonction sélectionnée, deux panneaux montrent la liste de ses appelants et de ses appelés avec le nombre d'appels et le coût de chaque relation.

### Convertir des données `perf` pour KCachegrind

Si vous souhaitez utiliser l'interface de KCachegrind avec des données `perf` (plutôt que Callgrind), l'outil `perf2calltree` (du paquet `kcachegrind-converters` sur certaines distributions, ou l'outil `hotspot` avec export) permet la conversion :

```bash
# Méthode 1 : via perf script et conversion
perf script | python3 -m flamegraph --format callgrind > perf_callgrind.out  
kcachegrind perf_callgrind.out  

# Méthode 2 : via le script perf2calltree (si disponible)
perf script -i perf.data | perf2calltree --all > callgrind_from_perf.out  
kcachegrind callgrind_from_perf.out  
```

La qualité de la conversion dépend de la richesse des données `perf` (piles d'appels complètes, symboles). Un profil `perf` enregistré avec `--call-graph dwarf` et des symboles complets produit une conversion exploitable.

### Quand choisir KCachegrind

KCachegrind est le meilleur choix lorsque :

- Vous avez besoin d'un **comptage exact des instructions et des appels** (impossible avec le sampling de `perf`).
- Vous analysez un **graphe d'appels complexe** avec de nombreuses relations et souhaitez le naviguer visuellement.
- La **reproductibilité** du profil est critique (les profils Callgrind en instructions sont déterministes, contrairement aux profils en cycles de `perf`).
- Le surcoût de 10 à 50× est acceptable (programme de courte durée, test ciblé).

---

## Autres outils notables

### Flamescope

[Flamescope](https://github.com/Netflix/flamescope), développé par Netflix, combine une **heatmap temporelle** avec des flamegraphs. L'interface montre d'abord une heatmap où l'axe X est le temps, l'axe Y la fréquence d'échantillonnage par seconde, et la couleur l'intensité de l'activité CPU. En sélectionnant un rectangle sur la heatmap, un flamegraph est généré pour cet intervalle temporel spécifique.

Ce couplage heatmap + flamegraph est particulièrement utile pour les programmes dont le comportement varie dans le temps : serveurs avec des pics de charge, processus batch avec des phases distinctes, programmes avec des initialisations coûteuses. Flamescope permet de « zoomer » visuellement sur une anomalie temporelle avant de l'analyser en détail.

```bash
# Installation
pip install flamescope

# Générer les données
perf record --call-graph fp -F 49 ./mon_service -- sleep 120  
perf script --header > profil.linux-perf.txt  

# Lancer l'interface web
flamescope
# Ouvrir http://localhost:5000 et charger profil.linux-perf.txt
```

### Intel VTune Profiler

**Intel VTune** est un profiler propriétaire (gratuit depuis 2020) spécialisé dans l'analyse micro-architecturale des processeurs Intel. Il exploite les compteurs de performance matériels avec une granularité bien supérieure à `perf stat`, en identifiant automatiquement les goulots d'étranglement dans le pipeline du processeur (*front-end bound*, *back-end bound*, *memory bound*, *core bound*).

VTune offre une interface graphique riche avec des vues dédiées à chaque type de problème matériel. Il est particulièrement pertinent pour les optimisations de bas niveau couvertes au chapitre 41 (SIMD, localité de cache, branch prediction). Son inconvénient principal est d'être limité aux processeurs Intel et de ne pas être open source.

```bash
# Installation via le dépôt Intel
sudo apt install intel-oneapi-vtune
```

### heaptrack (rappel)

Mentionné en section 30.2 comme alternative à Massif, **heaptrack** dispose d'une interface graphique (`heaptrack_gui`) qui combine un flamegraph des allocations mémoire, un graphique temporel de la consommation heap, et un tableau des sites d'allocation. Bien qu'il ne s'agisse pas d'un outil de profiling CPU, il complète utilement le diagnostic lorsque les problèmes de performance sont liés à la gestion mémoire (allocations excessives, surcharge de l'allocateur).

```bash
sudo apt install heaptrack heaptrack-gui

heaptrack ./mon_programme < donnees_test.txt  
heaptrack_gui heaptrack.mon_programme.12345.zst  
```

---

## Choisir le bon outil graphique

| Besoin | Outil recommandé |
|---|---|
| Analyse interactive d'un profil `perf` | **Hotspot** |
| Flamegraphs rapides en SVG | Scripts Gregg ou **Inferno** |
| Flamegraphs + timeline + filtrage par thread | **Hotspot** |
| Graphe d'appels exact avec comptage d'instructions | **KCachegrind** + Callgrind |
| Analyse temporelle (heatmap + flamegraph) | **Flamescope** |
| Diagnostic micro-architectural Intel | **Intel VTune** |
| Profiling des allocations mémoire | **heaptrack_gui** |
| Visualisation dans le navigateur, sans installation | **speedscope** |

Pour la majorité des développeurs C++ sur Linux, **Hotspot** est l'outil graphique de première intention. Il couvre 90% des besoins de profiling CPU avec une interface intuitive et une intégration directe avec `perf`. Les autres outils interviennent pour des besoins spécifiques : KCachegrind pour le comptage exact, VTune pour le diagnostic matériel Intel, Flamescope pour l'analyse temporelle.

---

## Récapitulatif du chapitre 31

Ce chapitre a couvert le profiling de performance sous quatre angles complémentaires :

**`perf`** (section 31.1) est l'outil de profiling natif de Linux. `perf record`/`perf report` localise les hotspots par sampling statistique avec un surcoût minimal. `perf stat` donne une radiographie matérielle (IPC, cache misses, branch mispredictions) qui oriente la stratégie d'optimisation. C'est l'outil de première intention pour tout profiling CPU.

**`gprof`** (section 31.2) offre un profiling par instrumentation avec comptage exact des appels. Son usage est aujourd'hui limité aux contextes où `perf` n'est pas disponible ou où le comptage exact est indispensable.

**Les flamegraphs** (section 31.3) transforment les données de profiling en visualisations immédiatement lisibles. Les variantes on-CPU, off-CPU et differential couvrent l'ensemble des scénarios de diagnostic.

**Les outils graphiques** (cette section) — Hotspot, KCachegrind, Flamescope, VTune — rendent l'analyse interactive et accessible, accélérant le cycle d'investigation pour le développement quotidien.

L'ensemble de ces outils, combiné avec les techniques d'analyse mémoire du chapitre 30 et les micro-benchmarks du chapitre 35, forme la boîte à outils complète du développeur C++ soucieux de la performance de ses programmes.

⏭️ [Analyse Statique et Linting](/32-analyse-statique/README.md)
