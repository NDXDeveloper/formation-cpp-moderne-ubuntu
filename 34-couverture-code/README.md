🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 34. Couverture de Code

## Introduction

Le chapitre 33 a couvert l'écriture de tests unitaires avec Google Test. Mais une question reste en suspens : **quel pourcentage du code est réellement exercé par ces tests ?** Un projet peut afficher cinq cents tests qui passent et pourtant laisser des pans entiers de logique sans couverture — branches d'erreur jamais traversées, cas limites jamais atteints, chemins de code morts que personne n'a pensé à tester.

La **couverture de code** (*code coverage*) est la mesure qui répond à cette question. Elle instrumente le binaire pour comptabiliser, lors de l'exécution des tests, quelles lignes de code source ont été exécutées et quelles branches conditionnelles ont été empruntées. Le résultat est un rapport chiffré — et souvent visuel — qui révèle les zones non couvertes.

Cette mesure est un outil de diagnostic, pas un objectif en soi. Comprendre ce qu'elle dit, ce qu'elle ne dit pas, et comment l'utiliser sans tomber dans les pièges classiques est essentiel pour en tirer une valeur réelle.

## Ce que la couverture mesure

La couverture de code se décline en plusieurs métriques complémentaires, chacune capturant un aspect différent de l'exécution.

### Couverture de lignes (line coverage)

La métrique la plus intuitive : parmi toutes les lignes exécutables du code source, combien ont été exécutées au moins une fois pendant les tests ? Une ligne non couverte est une ligne que les tests n'ont jamais atteinte — elle peut contenir un bug que personne ne détectera avant la production.

### Couverture de branches (branch coverage)

Plus fine que la couverture de lignes. Pour chaque point de décision (`if`, `else`, `switch`, `? :`), les deux chemins (vrai et faux) ont-ils été empruntés ? Un `if` dont seule la branche `true` a été testée affiche 50% de couverture de branches, même si la ligne elle-même est marquée comme couverte.

Considérons un exemple concret :

```cpp
std::string classify(int value) {
    if (value > 0)          // Branche 1: true / false
        return "positive";
    else if (value == 0)    // Branche 2: true / false
        return "zero";
    else
        return "negative";
}
```

Un test qui appelle `classify(42)` couvre la ligne du `if` et la ligne du `return "positive"`, mais ne couvre qu'une seule branche sur quatre. La couverture de lignes dirait 40% (2 lignes sur 5), la couverture de branches dirait 25% (1 branche sur 4). Les deux métriques racontent une histoire différente — la couverture de branches est ici plus révélatrice du travail restant.

### Couverture de fonctions (function coverage)

Parmi toutes les fonctions et méthodes du code source, combien ont été appelées au moins une fois ? C'est la métrique la plus grossière — une fonction peut être "couverte" avec un seul appel qui ne teste qu'un chemin heureux — mais elle est utile pour repérer du code mort (fonctions jamais appelées par aucun test).

## Ce que la couverture ne mesure pas

La couverture est un outil puissant mais limité. Comprendre ses angles morts évite de lui accorder une confiance excessive.

**La couverture ne prouve pas la correction.** Un test peut exécuter une ligne sans vérifier son résultat. Le code suivant a 100% de couverture de lignes mais ne teste rien :

```cpp
TEST(Calculator, Divides) {
    calculator.divide(10, 3);  // Pas d'assertion — le résultat est ignoré
}
```

La couverture dit "cette ligne a été exécutée", pas "cette ligne produit le bon résultat". C'est la qualité des assertions (chapitre 33.3) qui détermine si un test est réellement utile, pas le simple fait qu'il traverse du code.

**La couverture ne capture pas les combinaisons.** Une fonction avec trois `if` indépendants a 8 chemins d'exécution possibles (2³). Couvrir chaque branche individuellement nécessite au minimum 2 tests (un pour chaque valeur de chaque condition), mais explorer toutes les combinaisons en nécessiterait 8. La couverture de branches ne distingue pas ces deux cas.

**La couverture ne détecte pas le code manquant.** Si une spécification requiert une validation d'entrée mais que le développeur a oublié de l'implémenter, la couverture ne peut pas révéler l'absence — il n'y a rien à mesurer. Seule une revue de spécification ou un test fonctionnel peut combler ce vide.

## La chaîne d'outils sur Linux

Sur Ubuntu, la mesure de couverture repose sur une chaîne d'outils matures et bien intégrés :

**GCC + gcov** forment le socle. GCC compile le code avec des drapeaux d'instrumentation (`--coverage` ou `-fprofile-arcs -ftest-coverage`) qui génèrent des fichiers de compteurs d'exécution. `gcov`, distribué avec GCC, lit ces fichiers et produit des rapports texte ligne par ligne. C'est le moteur de bas niveau — fonctionnel mais austère.

**lcov** est une surcouche qui agrège les données `gcov` de l'ensemble du projet et génère des rapports HTML navigables. C'est l'outil qui transforme des milliers de fichiers de compteurs en un rapport visuel exploitable, avec coloration syntaxique, pourcentages par fichier et par répertoire, et navigation hiérarchique.

**genhtml** (distribué avec `lcov`) convertit les données lcov en pages HTML statiques que l'on peut ouvrir dans un navigateur, servir sur un intranet ou archiver comme artefact CI.

**LLVM + llvm-cov** offrent une chaîne alternative quand on compile avec Clang. L'approche est conceptuellement similaire (instrumentation à la compilation, collecte à l'exécution, génération de rapports) mais les formats de fichiers et les outils diffèrent. `llvm-cov` produit des rapports nativement plus détaillés sur la couverture de régions (plus fine que la couverture de lignes). Cette formation se concentre sur la chaîne GCC/gcov/lcov, la plus répandue dans l'écosystème Ubuntu, mais les principes sont transposables.

## Place de la couverture dans le workflow

La mesure de couverture s'insère à deux endroits dans le flux de développement.

**En local, pendant le développement.** Après avoir écrit ou modifié des tests, le développeur génère un rapport de couverture pour identifier les lignes et branches non couvertes dans le module qu'il vient de modifier. C'est un usage ponctuel et ciblé — on ne couvre pas l'ensemble du projet à chaque itération, seulement les fichiers concernés.

**En CI, à chaque merge request.** Le pipeline de CI compile avec l'instrumentation, exécute la suite de tests complète, génère le rapport de couverture et le publie comme artefact. Un seuil minimal (par exemple 80%) peut être configuré comme critère de blocage. La section 34.3 détaille l'intégration de cette chaîne dans CMake, et la section 38 (CI/CD pour C++) montre comment l'intégrer dans GitHub Actions et GitLab CI.

## Ce que vous apprendrez dans ce chapitre

La **section 34.1** détaille l'utilisation de `gcov` et `lcov` : flags de compilation, génération des données de couverture, agrégation et production de rapports HTML.

La **section 34.2** se concentre sur la génération de rapports HTML avec `genhtml`, leur personnalisation et leur interprétation : lecture des résultats, identification des zones critiques non couvertes, et navigation efficace dans un rapport de projet complet.

La **section 34.3** couvre l'intégration de la mesure de couverture dans CMake : création d'une cible `coverage` dédiée, automatisation de la chaîne complète (build → test → rapport), et intégration dans les pipelines CI.

La **section 34.4** aborde la question des objectifs de couverture : quel taux viser, comment interpréter les chiffres sans tomber dans le piège de la métrique-objectif, et stratégies pour augmenter la couverture de manière ciblée et utile.

## Prérequis

Ce chapitre s'appuie sur les compétences suivantes :

- **Google Test** (chapitre 33) : une suite de tests fonctionnelle est nécessaire — la couverture mesure l'effet de ces tests sur le code source.
- **CMake** (chapitre 26) : les exemples utilisent CMake pour la configuration des flags d'instrumentation et la création de cibles de couverture.
- **GCC** (section 2.1) : la chaîne gcov/lcov est spécifique à GCC. L'alternative Clang/llvm-cov sera mentionnée quand elle diffère significativement.
- **Ligne de commande Linux** (module 1) : les outils de couverture sont principalement en ligne de commande, avec des rapports HTML comme livrable final.

---


⏭️ [gcov et lcov : Mesure de la couverture](/34-couverture-code/01-gcov-lcov.md)
