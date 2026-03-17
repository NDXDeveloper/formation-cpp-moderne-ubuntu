🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 34.4 Objectifs de couverture

## La question inévitable : quel pourcentage viser ?

Dès qu'une équipe met en place la mesure de couverture, la question surgit : "Quel taux devons-nous atteindre ?" La réponse honnête est qu'il n'existe pas de chiffre magique universel. Un taux de 80% peut être excellent pour un projet ou catastrophiquement insuffisant pour un autre, selon la nature du code, sa criticité et le type de couverture mesuré.

Ce qui existe, en revanche, ce sont des principes directeurs qui permettent de fixer un objectif pertinent pour un contexte donné, et surtout d'utiliser la métrique de couverture comme un outil d'amélioration plutôt que comme un objectif en soi.

## La loi de Goodhart appliquée à la couverture

> *"Quand une mesure devient un objectif, elle cesse d'être une bonne mesure."*  
> — Loi de Goodhart

Ce principe, issu de l'économie, s'applique parfaitement à la couverture de code. Dès qu'un seuil de couverture devient un critère de blocage dans la CI, une pression apparaît pour atteindre le chiffre — parfois au détriment de la qualité réelle des tests.

Considérons un développeur qui doit passer de 78% à 80% de couverture pour que sa merge request soit acceptée. La tentation est forte d'écrire des tests comme celui-ci :

```cpp
// ❌ Test qui augmente la couverture sans vérifier quoi que ce soit
TEST(ConfigLoader, CoversErrorPath) {
    mp::ConfigLoader loader;
    loader.load("nonexistent.toml");
    // Pas d'assertion — le test traverse le code d'erreur
    // La couverture augmente, la qualité ne bouge pas
}
```

Ce test exécute le chemin d'erreur (la couverture de lignes monte) mais ne vérifie rien (aucune assertion). Un bug dans la gestion d'erreur passerait inaperçu. Le chiffre de couverture s'améliore, la fiabilité du logiciel non.

L'antidote est de mesurer la couverture comme un **indicateur de diagnostic** plutôt que comme un **objectif de performance**. La couverture répond à la question "où manque-t-il des tests ?" — pas à la question "le code est-il suffisamment testé ?". La qualité des assertions (chapitre 33.3) et la pertinence des scénarios testés importent bien plus que le pourcentage affiché.

## Ordres de grandeur par contexte

Malgré l'absence de chiffre universel, l'expérience de l'industrie fournit des repères utiles. Ces fourchettes concernent la couverture de **lignes** — la couverture de branches est typiquement 10 à 20 points inférieure.

### Code critique : 90%+ de lignes, 80%+ de branches

Le code dont un dysfonctionnement a des conséquences graves — financières, sécuritaires ou humaines — justifie un investissement maximal en tests. Cela inclut les bibliothèques cryptographiques, les moteurs de calcul financier, les parsers de protocoles réseau exposés à l'extérieur, et le code embarqué critique.

À ce niveau, chaque ligne non couverte devrait être explicitement justifiée (code mort à supprimer, branche théoriquement inatteignable, cas de défaillance hardware). Les branches non couvertes sont systématiquement examinées en revue de code.

### Code métier standard : 75-85% de lignes, 60-70% de branches

La majorité du code de production dans un projet bien tenu. Les chemins principaux et les cas d'erreur courants sont testés. Certaines branches défensives (vérifications de préconditions qui ne devraient jamais échouer, catch-all de dernière ligne) peuvent ne pas être couvertes sans que cela pose un problème majeur.

C'est la fourchette que la plupart des équipes visent comme seuil de CI. Un projet qui maintient 80% de couverture de lignes et 65% de couverture de branches a un filet de sécurité solide pour les refactorings et les évolutions.

### Code utilitaire et glue : 60-75% de lignes

Le code de configuration, les wrappers fins autour de bibliothèques tierces, le code de logging, les adaptateurs d'interface. Ce code a peu de logique propre — il délègue à d'autres composants. Le tester exhaustivement apporte un retour sur investissement décroissant.

Un seuil plus bas sur ces modules est acceptable, à condition que les modules critiques qu'ils alimentent soient bien couverts.

### Code legacy en cours de reprise : seuil progressif

Un projet legacy sans tests démarre à 0%. Imposer un seuil de 80% du jour au lendemain est irréaliste et démotivant. La stratégie recommandée est le **ratchet** (cliquet) : mesurer la couverture actuelle, la fixer comme plancher, et exiger que chaque merge request ne la fasse pas descendre. Le seuil monte mécaniquement au fil du temps sans jamais redescendre :

```
Sprint 1 : 12% → nouveau plancher = 12%  
Sprint 2 : 18% → nouveau plancher = 18%  
Sprint 3 : 17% → ❌ rejeté (régression)  
Sprint 3 : 22% → nouveau plancher = 22%  
...
Sprint 20 : 71% → le seuil est maintenant viable
```

Ce modèle est psychologiquement sain : il récompense chaque effort incrémental sans punir le point de départ.

## Couverture de lignes vs couverture de branches

Un piège fréquent est de fixer un objectif uniquement sur la couverture de lignes et d'ignorer la couverture de branches. Or les deux métriques racontent des histoires très différentes.

Considérons une fonction de validation :

```cpp
bool validate_config(const Config& cfg) {
    if (cfg.port < 1 || cfg.port > 65535)
        return false;
    if (cfg.host.empty())
        return false;
    if (cfg.timeout_ms <= 0)
        return false;
    return true;
}
```

Un test unique avec une configuration valide traverse toutes les lignes du `return true` et atteint 40% de couverture de lignes (2 lignes sur 5 du chemin heureux). Mais il ne couvre que 3 branches sur 8 (les trois `false` du côté "condition non remplie", plus le `true` final, soit en réalité seulement les branches "faux" des trois `if`). Les chemins d'erreur — port invalide, host vide, timeout négatif — restent inexplorés.

La couverture de branches est plus exigeante et plus révélatrice. Un projet qui affiche 85% de couverture de lignes et 55% de couverture de branches a un problème : les tests traversent le code sans explorer ses décisions. C'est pourquoi la section 34.3 recommande de configurer genhtml avec `--branch-coverage` et de fixer des seuils sur les deux métriques.

### Recommandation pratique

Fixez un seuil de CI sur les deux métriques, avec la couverture de branches environ 15-20 points sous la couverture de lignes :

| Métrique | Seuil recommandé (code standard) |
|----------|----------------------------------|
| Couverture de lignes | 80% |
| Couverture de branches | 60-65% |
| Couverture de fonctions | 90% (indicatif, pas de seuil CI) |

La couverture de fonctions n'a pas besoin d'être un critère bloquant — elle sert principalement à détecter le code mort.

## Stratégies pour augmenter la couverture de manière utile

Atteindre un seuil de couverture n'est pas une fin en soi — la manière dont on y arrive détermine la valeur réelle des tests. Voici des stratégies qui augmentent la couverture **et** la qualité.

### Cibler les chemins d'erreur

Les chemins d'erreur sont systématiquement les moins couverts. Ils sont aussi les plus dangereux en production — un `catch` jamais testé peut contenir un bug qui transforme une erreur récupérable en crash. Prioriser les tests sur les branches d'erreur apporte un double bénéfice : couverture en hausse et robustesse en hausse.

```cpp
// Test du chemin d'erreur — haute valeur
TEST(ConfigLoader, ReturnsErrorOnInvalidPort) {
    auto result = mp::load_config("test_data/invalid_port.toml");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, mp::ErrorCode::InvalidPort);
}
```

### Utiliser les tests paramétrés pour les cas limites

Les tests paramétrés (section 33.2.3) permettent d'explorer systématiquement les frontières sans effort disproportionné :

```cpp
INSTANTIATE_TEST_SUITE_P(
    BoundaryPorts,
    PortValidationTest,
    ::testing::Values(
        PortCase{0,     false},   // En dessous du minimum
        PortCase{1,     true},    // Borne inférieure
        PortCase{65535, true},    // Borne supérieure
        PortCase{65536, false},   // Au-dessus du maximum
        PortCase{-1,    false},   // Valeur négative
        PortCase{8080,  true}     // Cas nominal
    )
);
```

Six cas de test, une seule fonction de test. La couverture de branches de la validation de port passe probablement de 50% à 100%.

### Analyser le rapport avant d'écrire

La stratégie la plus efficace est de consulter le rapport de couverture (section 34.2) **avant** de décider quel test écrire. Plutôt que d'écrire des tests "au feeling", le rapport indique précisément quelles lignes et quelles branches ne sont pas couvertes. Le travail devient ciblé : ouvrir le rapport, identifier une branche rouge dans un module critique, écrire le test qui la traverse, vérifier que le rouge passe au vert.

### Supprimer le code mort

Une façon souvent négligée d'améliorer la couverture est de **supprimer** le code non couvert plutôt que de le tester. Si une fonction n'est appelée par aucun test et par aucun chemin de production, elle est probablement du code mort. La supprimer améliore la couverture (le dénominateur diminue), réduit la surface de maintenance et simplifie le code.

Le rapport de couverture de fonctions (function coverage) est l'outil idéal pour repérer ces candidates à la suppression. Une fonction avec un compteur à zéro est soit non testée, soit morte. La vérification est simple : une recherche dans le code source confirme si elle est appelée quelque part en dehors des tests.

## Anti-patterns à éviter

### Le test sans assertion

Déjà évoqué plus haut — un test qui exécute du code sans vérifier son comportement. La couverture monte, la confiance non. Chaque test devrait contenir au moins une assertion significative.

### Le test qui vérifie l'implémentation

```cpp
// ❌ Ce test vérifie comment le code fait les choses, pas ce qu'il fait
TEST(Cache, UsesHashMapInternally) {
    mp::Cache cache;
    cache.put("key", "value");
    EXPECT_EQ(cache.bucket_count(), 16);  // Détail d'implémentation
}
```

Ce test augmente la couverture mais se cassera au moindre changement d'implémentation interne du cache. Préférez tester le comportement observable : "une valeur insérée peut être retrouvée", "le cache respecte sa capacité maximale".

### L'objectif de 100%

Viser 100% de couverture de lignes est un objectif séduisant mais contreproductif. Les derniers pourcentages sont disproportionnément coûteux à atteindre et apportent un retour décroissant. Ils concernent typiquement du code défensif théoriquement inatteignable (`default` dans un `switch` exhaustif, `catch(...)` de dernier recours), des chemins de défaillance matérielle ou des branches de compatibilité pour des configurations non testables en CI.

Le coût pour passer de 90% à 95% est souvent supérieur au coût pour passer de 60% à 90%. Ce surinvestissement se fait au détriment d'activités plus productives : tests d'intégration, revue de code, fuzzing (section 45.5).

### Le seuil unique pour tout le projet

Appliquer un seuil uniforme de 80% à tous les modules traite le code critique et le code glue de la même manière. Une approche plus nuancée est de fixer des seuils par module ou par répertoire :

```bash
# Seuils différenciés dans le script de vérification CI
check_coverage "src/core/"    90  # Code critique  
check_coverage "src/api/"     80  # Code métier standard  
check_coverage "src/utils/"   65  # Utilitaires  
```

lcov et genhtml supportent le filtrage par chemin (section 34.1), ce qui permet de générer des rapports et des vérifications par module.

## Métriques complémentaires

La couverture de code n'est qu'une pièce du puzzle de la qualité logicielle. D'autres métriques la complètent utilement.

**Mutation testing.** Le mutation testing modifie délibérément le code source (inverser une condition, changer un `+` en `-`, supprimer un `return`) et vérifie que les tests détectent chaque mutation. Un test qui n'échoue sur aucune mutation est un test faible — il traverse le code sans vérifier sa correction. Le mutation testing mesure la **qualité** des tests, là où la couverture ne mesure que leur **étendue**. En C++, des outils comme `mull` ou `dextool` implémentent cette approche, bien qu'ils restent moins matures que leurs équivalents Java (PIT).

**Couverture des exigences.** Dans les projets réglementés, la traçabilité entre les exigences fonctionnelles et les cas de test importe autant que la couverture de code. Un test peut couvrir du code sans couvrir d'exigence, et une exigence peut être non couverte même avec 100% de couverture de code.

**Métriques de complexité.** La complexité cyclomatique (nombre de chemins indépendants dans une fonction) indique le nombre minimal de tests nécessaires pour couvrir toutes les branches. Une fonction avec une complexité de 15 nécessite au moins 15 tests pour une couverture de branches complète. Croiser complexité et couverture permet d'identifier les fonctions les plus risquées : haute complexité + basse couverture = zone de danger prioritaire.

## Recommandation finale

La couverture de code est un outil de diagnostic puissant quand elle est utilisée avec discernement. Les principes à retenir :

**Mesurer systématiquement.** Intégrer la mesure dans la CI (section 34.3) pour que la couverture soit toujours visible et à jour. Une métrique qu'on ne regarde jamais n'a aucune valeur.

**Fixer un plancher, pas un plafond.** Le seuil de CI empêche les régressions — il garantit que la couverture ne descend jamais sous un niveau acceptable. Il ne devrait pas être un objectif à atteindre coûte que coûte.

**Prioriser la qualité des tests sur la quantité.** Un test avec une assertion pertinente sur un chemin d'erreur vaut plus que dix tests sans assertion qui traversent le chemin heureux. La couverture est un moyen, pas une fin.

**Lire le rapport, pas juste le chiffre.** Le pourcentage global est un résumé grossier. La valeur réelle du rapport est dans la vue source annotée (section 34.2) — les lignes rouges et les branches partielles qui pointent vers les tests manquants.

**Évoluer progressivement.** Commencer modeste (60%), stabiliser, puis monter graduellement les seuils au fil des sprints. La couverture est un investissement à long terme, pas un sprint.

---


⏭️ [Benchmarking](/35-benchmarking/README.md)
