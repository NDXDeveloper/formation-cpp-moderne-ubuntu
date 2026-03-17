🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 34.2 Génération de rapports HTML

## De la donnée brute au rapport exploitable

La section 34.1 a couvert la chaîne complète de mesure, de la compilation instrumentée au fichier `.info` agrégé par lcov. Ce fichier `.info` contient toutes les données de couverture du projet, mais sous une forme textuelle difficilement exploitable à l'échelle d'un projet réel. Un projet de cinquante fichiers produit un `.info` de plusieurs milliers de lignes — personne ne va le lire manuellement.

`genhtml` transforme ces données en un site HTML statique navigable, avec coloration syntaxique, pourcentages hiérarchiques et code couleur immédiat. C'est le livrable que les développeurs consultent au quotidien et que les pipelines CI archivent comme artefact. Cette section détaille sa configuration, la personnalisation des rapports et surtout les techniques de lecture efficace qui transforment un rapport de couverture en actions concrètes.

## Configuration de genhtml

### Invocation de base

La commande minimale a été présentée en section 34.1 :

```bash
genhtml coverage_filtered.info \
       --output-directory coverage_report/ \
       --branch-coverage
```

En pratique, un rapport de qualité professionnelle bénéficie d'options supplémentaires :

```bash
genhtml coverage_filtered.info \
       --output-directory coverage_report/ \
       --title "Mon Projet v2.1.0 — Couverture" \
       --legend \
       --branch-coverage \
       --highlight \
       --demangle-cpp \
       --num-spaces 4 \
       --sort \
       --function-coverage \
       --prefix "$(pwd)/src"
```

Détaillons chaque option.

**`--title`** — Le titre affiché en en-tête de chaque page. Inclure le numéro de version ou le hash du commit facilite l'identification quand on archive plusieurs rapports successifs.

**`--legend`** — Ajoute un encadré en bas de page expliquant le code couleur et les symboles utilisés. Indispensable quand le rapport est consulté par des membres de l'équipe qui ne sont pas familiers avec l'outil.

**`--branch-coverage`** — Active l'affichage de la couverture de branches dans le rapport. Sans cette option, seule la couverture de lignes est visible — ce qui masque un pan entier de l'analyse. Cette option devrait être systématique.

**`--highlight`** — Met en surbrillance les lignes de code qui n'ont été couvertes que par une exécution récente (quand on utilise des tracefiles incrémentaux). Utile en développement pour visualiser l'impact de tests fraîchement ajoutés.

**`--demangle-cpp`** — Décode les noms C++ manglés dans les rapports de couverture de fonctions. Sans cette option, les noms de fonctions apparaissent sous leur forme manglée (`_ZN2mp3addEii` au lieu de `mp::add(int, int)`), ce qui les rend illisibles. Cette option utilise `c++filt` en interne.

**`--num-spaces 4`** — Remplace les tabulations par le nombre d'espaces spécifié dans la vue source. Permet d'aligner l'affichage sur la convention du projet.

**`--sort`** — Trie les fichiers et répertoires par couverture croissante. Les fichiers les moins couverts apparaissent en premier, ce qui dirige immédiatement l'attention vers les zones problématiques.

**`--function-coverage`** — Ajoute une colonne de couverture de fonctions dans la vue hiérarchique. Complémentaire aux couvertures de lignes et de branches.

**`--prefix`** — Supprime un préfixe commun dans les chemins de fichiers affichés. Évite que chaque fichier soit affiché comme `/home/dev/projets/mon_projet/src/module/file.cpp` alors que `module/file.cpp` suffit.

### Personnaliser les seuils de couleur

Par défaut, genhtml utilise des seuils de couverture à 75% (passage rouge→orange) et 90% (passage orange→vert). Ces seuils sont configurables :

```bash
genhtml coverage_filtered.info \
       --output-directory coverage_report/ \
       --branch-coverage \
       --hi-limit 90 \
       --med-limit 75
```

- **`--hi-limit 90`** — Au-dessus de 90% : vert.
- **`--med-limit 75`** — Entre 75% et 90% : orange. En dessous de 75% : rouge.

Adapter ces seuils à la politique qualité du projet est recommandé. Un projet legacy en cours de couverture progressive pourrait démarrer avec des seuils à 40/60, puis les relever à mesure que la couverture s'améliore. Un projet critique (finance, embarqué) pourrait exiger 85/95.

### Fichier de configuration lcovrc

Pour éviter de répéter les options à chaque invocation, lcov et genhtml lisent un fichier de configuration `.lcovrc` dans le répertoire home ou à un chemin spécifié :

```bash
# .lcovrc (à la racine du projet ou dans ~/.lcovrc)
genhtml_hi_limit = 90  
genhtml_med_limit = 75  
genhtml_branch_coverage = 1  
genhtml_function_coverage = 1  
genhtml_demangle_cpp_tool = c++filt  
genhtml_legend = 1  
genhtml_sort = 1  
genhtml_num_spaces = 4  
lcov_branch_coverage = 1  
```

Avec ce fichier en place, l'invocation se simplifie :

```bash
genhtml coverage_filtered.info --output-directory coverage_report/
```

Toutes les options sont lues depuis `.lcovrc`. Le fichier peut être versionné avec le projet pour garantir la cohérence entre les développeurs et la CI.

## Structure du rapport généré

`genhtml` produit un répertoire de fichiers HTML statiques. Comprendre sa structure aide à l'intégrer dans d'autres outils et à le publier efficacement :

```
coverage_report/
├── index.html                    # Page d'accueil (vue projet)
├── index-sort-f.html             # Même vue, triée par fonctions
├── index-sort-l.html             # Même vue, triée par lignes
├── gcov.css                      # Feuille de style
├── amber.png                     # Icônes de statut
├── emerald.png
├── ruby.png
├── glass.png
├── snow.png
├── updown.png
└── src/
    └── mon_projet/
        ├── index.html            # Vue répertoire
        ├── math.cpp.gcov.html    # Vue source annotée
        ├── validator.cpp.gcov.html
        └── ...
```

Le rapport est entièrement autonome — aucune dépendance externe, pas de JavaScript tiers, pas de CDN. Il peut être ouvert depuis le système de fichiers local, servi par n'importe quel serveur HTTP statique, ou archivé comme artefact CI sans configuration supplémentaire.

## Lire un rapport efficacement

Un rapport de couverture sur un projet de taille moyenne peut contenir des centaines de fichiers. Le consulter fichier par fichier est inefficace. Voici une méthode de lecture structurée qui maximise le rapport information/temps.

### Étape 1 : la vue d'ensemble

La page `index.html` affiche la couverture globale du projet et la décompose par répertoire. Trois chiffres résument la situation :

- **Couverture de lignes** — Le pourcentage le plus intuitif. Un projet mature vise typiquement 80-90%.
- **Couverture de fonctions** — Si certaines fonctions ne sont jamais appelées, c'est soit du code mort à supprimer, soit un manque de tests sur un module entier.
- **Couverture de branches** — Toujours inférieure à la couverture de lignes, souvent significativement. Un écart important (par exemple 85% de lignes mais 55% de branches) signale que les tests traversent le code sans explorer ses décisions.

### Étape 2 : identifier les zones critiques

Avec l'option `--sort`, les répertoires les moins couverts apparaissent en premier. Concentrez-vous sur les modules dont la couverture est rouge ou orange **et** qui sont fonctionnellement critiques. Un module utilitaire de logging à 60% de couverture est moins prioritaire qu'un module de validation d'entrées à 60%.

### Étape 3 : la vue source annotée

En cliquant sur un fichier, on accède à la vue ligne par ligne. Voici comment lire les indicateurs :

```
      Compteur | Branches | Code source
      ---------|----------|--------------------------------------------------
            12 |          | int process(const Request& req) {
             8 | [+ -]    |     if (req.is_valid()) {
             8 |          |         auto result = compute(req);
             5 | [+ -]    |         if (result.has_value()) {
             5 |          |             return result->code;
             - |          |         }
        #####  |          |         log_error("computation failed");
        #####  |          |         return -1;
             - |          |     }
             4 |          |     return reject(req);
             - |          | }
```

La lecture se fait en trois passes.

**Première passe : les lignes rouges (`#####`).** Ce sont les lignes jamais exécutées. Dans l'exemple, les lignes `log_error("computation failed")` et `return -1` ne sont jamais atteintes — aucun test ne provoque un échec de `compute()` quand la requête est valide. C'est un chemin d'erreur non testé.

**Deuxième passe : les branches partielles (`[+ -]`).** Le `[+ -]` à la ligne du `if (req.is_valid())` indique que la branche "vrai" a été empruntée (`+`) mais pas la branche "faux" (`-`). Pourtant, la ligne 10 (`return reject(req)`) montre un compteur de 4 — ce qui semble contradictoire. En réalité, le marqueur de branche se réfère aux branches au niveau du code machine, qui peuvent différer de la lecture intuitive du code source. Cet exemple illustre pourquoi la couverture de branches nécessite parfois une analyse plus attentive que la simple couverture de lignes.

**Troisième passe : les compteurs élevés.** Une ligne avec un compteur de 100 000 dans une boucle critique confirme que les tests exercent bien les itérations. Un compteur de 1 sur une fonction complexe suggère qu'un seul cas de test la traverse — les cas limites sont probablement absents.

### Technique : comparer la couverture entre deux périodes

Pour suivre la progression de la couverture entre deux périodes (avant/après un sprint, entre deux releases), on compare les résumés de deux fichiers `.info` :

```bash
# Couverture de la semaine dernière (archivée)
lcov --summary last_week.info 2>&1 | grep "lines\|branches"

# Couverture actuelle
lcov --capture --directory build/ --output-file this_week.info  
lcov --summary this_week.info 2>&1 | grep "lines\|branches"  
```

Pour une vue visuelle, on peut générer deux rapports HTML côte à côte et comparer les chiffres par module. Des outils tiers comme `diff-cover` (Python) permettent aussi de mesurer la couverture uniquement sur les lignes modifiées dans un diff Git — une approche plus ciblée pour les merge requests :

```bash
# diff-cover : couverture sur les lignes changées uniquement
pip install diff-cover  
diff-cover coverage.xml --compare-branch=main --html-report delta.html  
```

> 💡 **Note** : l'option `--diff` de lcov ne compare pas deux fichiers `.info` — elle transforme les données de couverture en appliquant un diff de code source (format unified diff). Pour un suivi de progression simple, comparer les résumés `--summary` de deux captures successives est la méthode la plus directe.

## Rapport en ligne de commande

Parfois, un résumé textuel suffit — en CI par exemple, quand le rapport HTML est archivé mais que le log de pipeline doit montrer les chiffres clés. `lcov` propose un résumé intégré :

```bash
lcov --summary coverage_filtered.info
```

La sortie ressemble à :

```
Reading tracefile coverage_filtered.info  
Summary coverage rate:  
  lines......: 84.2% (1053 of 1250 lines)
  functions..: 91.3% (126 of 138 functions)
  branches...: 67.8% (543 of 801 branches)
```

Ces trois chiffres peuvent être extraits par un script et comparés à un seuil pour décider du succès ou de l'échec du pipeline (détaillé en section 34.3).

Pour un résumé par fichier sans générer de HTML :

```bash
lcov --list coverage_filtered.info
```

La sortie est un tableau texte avec la couverture de chaque fichier :

```
|Filename                      |Lines   |Functions|Branches|
|------------------------------|--------|---------|--------|
|src/math.cpp                  |  100.0%|   100.0%|   87.5%|
|src/validator.cpp             |   72.4%|    80.0%|   55.0%|
|src/order_processor.cpp       |   91.2%|   100.0%|   78.3%|
|src/config_loader.cpp         |   65.8%|    75.0%|   42.1%|
```

Ce tableau identifie immédiatement `config_loader.cpp` comme le fichier le moins couvert — point de départ pour un effort ciblé d'amélioration de la couverture.

## Rapports pour les merge requests

Dans un workflow de revue de code, la couverture la plus pertinente n'est pas celle du projet entier mais celle des **lignes modifiées** dans la merge request. Deux approches permettent d'obtenir cette vue.

### Couverture différentielle avec git diff

Le principe consiste à croiser le rapport de couverture avec le diff de la branche :

```bash
# Extraire les fichiers modifiés
git diff --name-only origin/main...HEAD -- '*.cpp' '*.hpp' > changed_files.txt

# Générer un rapport restreint aux fichiers modifiés
lcov --extract coverage_filtered.info \
     $(sed 's|^|*/|' changed_files.txt | tr '\n' ' ') \
     --output-file coverage_mr.info

genhtml coverage_mr.info \
       --output-directory coverage_mr_report/ \
       --title "MR #237 — Couverture des fichiers modifiés" \
       --branch-coverage
```

Ce rapport ne contient que les fichiers touchés par la merge request, ce qui le rend compact et directement actionnable en revue.

### Intégration avec les plateformes CI

Les plateformes CI modernes (GitHub Actions, GitLab CI) supportent nativement ou via des plugins l'affichage de la couverture directement dans l'interface de merge request. La section 34.3 détaille l'intégration CMake et la section 38 couvre la configuration CI complète. Le format Cobertura XML, généré par lcov ou par des convertisseurs tiers, est le format le plus universellement supporté :

```bash
# Conversion vers Cobertura XML (avec lcov 2.x)
lcov --summary coverage_filtered.info --output-file coverage.xml --xml
```

Certains outils tiers comme `gcovr` offrent une génération Cobertura plus directe et sont parfois préférés pour cette tâche spécifique :

```bash
pip install gcovr

gcovr --root . --filter 'src/' \
      --xml-pretty --output coverage_cobertura.xml \
      --branches
```

Le fichier XML résultant est ensuite déclaré comme artefact dans le pipeline CI, et la plateforme l'affiche en annotation sur les lignes de code dans la vue de merge request.

## Personnalisation du style visuel

Le rapport genhtml utilise une feuille de style CSS intégrée (`gcov.css`). Pour l'adapter à la charte graphique de l'équipe ou améliorer la lisibilité, deux approches sont possibles.

### Surcharger le CSS après génération

Le fichier `gcov.css` est un CSS standard modifiable directement :

```bash
genhtml coverage_filtered.info --output-directory coverage_report/

# Personnaliser le style
cat >> coverage_report/gcov.css << 'EOF'  
body { font-family: "JetBrains Mono", "Fira Code", monospace; }  
td.coverFn { min-width: 300px; }  
pre.source { font-size: 13px; line-height: 1.5; }  
EOF  
```

### Fournir un CSS personnalisé

genhtml accepte un CSS externe via l'option `--css-file` :

```bash
genhtml coverage_filtered.info \
       --output-directory coverage_report/ \
       --css-file custom_coverage.css
```

En pratique, la personnalisation du style est rarement nécessaire — le style par défaut est fonctionnel et lisible. Les ajustements les plus courants concernent la taille de police pour les écrans haute résolution et la largeur des colonnes pour les noms de fonctions longs (fréquents en C++ templatisé).

## Bonnes pratiques de gestion des rapports

**Versionner la configuration, pas les rapports.** Le fichier `.lcovrc` et les scripts de génération appartiennent au dépôt. Les rapports HTML sont des artefacts générés — ils ne doivent pas être commités. Ajoutez `coverage_report/` et `*.info` au `.gitignore`.

**Archiver les rapports en CI.** Chaque pipeline devrait publier le rapport HTML comme artefact téléchargeable et, idéalement, le déployer sur un serveur interne (GitLab Pages, GitHub Pages, S3 statique) pour consultation sans téléchargement.

**Nommer les rapports avec le contexte.** Inclure la branche, le hash de commit ou le numéro de pipeline dans le titre (`--title`) et le répertoire de sortie évite la confusion quand plusieurs rapports coexistent.

**Automatiser.** La génération manuelle est acceptable en développement. En CI, tout le pipeline doit être automatisé dans une cible CMake ou un script dédié — c'est l'objet de la section 34.3.

---


⏭️ [Intégration dans CMake](/34-couverture-code/03-integration-cmake.md)
