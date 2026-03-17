🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 33.5 Test-Driven Development (TDD) en C++

## Qu'est-ce que le TDD ?

Le Test-Driven Development est une discipline de développement logiciel où les tests sont écrits **avant** le code de production. L'idée est contre-intuitive pour quiconque a l'habitude d'écrire le code d'abord puis de le tester ensuite. Pourtant, le TDD inverse délibérément ce flux pour produire un code mieux conçu, mieux couvert et plus facilement maintenable.

La pratique repose sur un cycle court et répétitif en trois phases, universellement connu sous le nom de **Red-Green-Refactor** :

1. **Red** — Écrire un test qui échoue. Le test décrit un comportement attendu que le code de production ne supporte pas encore. La compilation peut même échouer à ce stade (la fonction n'existe pas encore) — c'est normal et attendu.

2. **Green** — Écrire le minimum de code de production nécessaire pour faire passer le test. Pas d'optimisation, pas de généralisation, pas d'élégance. L'objectif unique est de voir le test passer.

3. **Refactor** — Améliorer la structure du code (production et tests) sans changer le comportement. Les tests qui passent garantissent que le refactoring ne casse rien. C'est dans cette phase qu'on élimine la duplication, qu'on extrait des fonctions, qu'on renomme des variables.

Le cycle dure typiquement entre une et dix minutes. Un cycle plus long est le signal que le pas est trop grand — il faut décomposer le problème en incréments plus petits.

## Pourquoi le TDD est différent en C++

Le TDD est né dans l'écosystème Smalltalk puis s'est popularisé avec Java et les langages dynamiques (Python, Ruby, JavaScript). Son application en C++ présente des spécificités qui méritent d'être explicités.

### Le coût de la compilation

Dans un langage interprété, le cycle Red-Green-Refactor est quasi instantané : on modifie le code, on relance les tests, le feedback est immédiat. En C++, chaque itération du cycle implique une recompilation. Sur un projet mal structuré, cela peut prendre trente secondes à plusieurs minutes — un temps mort qui brise le rythme du TDD et pousse à écrire des incréments plus gros (ce qui va à l'encontre de la pratique).

La solution est de structurer le projet pour minimiser le temps de recompilation :

- **Compilation incrémentale** via CMake et Ninja (sections 26.5 et 28.3) : seuls les fichiers modifiés sont recompilés.
- **ccache** (section 2.3) : les compilations identiques sont servies depuis le cache.
- **Découpage en bibliothèques** : un test qui ne dépend que de `math_lib` ne recompile pas `network_lib` quand celle-ci change.
- **Binaires de test séparés** : un binaire par module plutôt qu'un unique binaire monolithique. On relance uniquement le binaire concerné pendant le développement.

Avec ces pratiques en place, le cycle de recompilation pour un module individuel descend typiquement sous les deux secondes, ce qui rend le TDD fluide.

### Les erreurs de compilation comme premier feedback

En C++, la phase Red commence souvent par une **erreur de compilation** plutôt qu'un test qui échoue à l'exécution. Écrire un test qui appelle une fonction inexistante ne compile pas. C'est une forme de Red valide et utile — l'erreur de compilation est le premier feedback qui guide l'écriture du code.

Le cycle en C++ est donc souvent :

1. **Red (compilation)** — Le test ne compile pas : la fonction/classe n'existe pas.
2. **Red (exécution)** — Le test compile mais échoue : le comportement n'est pas implémenté.
3. **Green** — Le test passe.
4. **Refactor** — Nettoyage.

Cette double phase Red est naturelle en C++ et ne doit pas être perçue comme un obstacle. L'erreur de compilation force à définir l'interface publique (signature de fonction, nom de classe, types de paramètres) avant l'implémentation — ce qui est exactement l'objectif du TDD.

### Le typage statique comme filet de sécurité

Le système de types de C++ attrape à la compilation toute une catégorie de bugs que les langages dynamiques ne détectent qu'à l'exécution (ou pas du tout). Cela signifie qu'en C++, les tests se concentrent davantage sur la **logique métier** et les **cas limites** que sur la vérification de types — le compilateur s'en charge. C'est un avantage : les tests TDD en C++ sont généralement plus ciblés et moins redondants avec le système de types.

## Le cycle Red-Green-Refactor en pratique

Illustrons le cycle complet sur un exemple concret : l'implémentation d'une classe `RangeSet` qui représente un ensemble d'intervalles entiers fusionnés (par exemple, pour gérer des plages de ports réseau ou des intervalles de mémoire).

### Spécification informelle

`RangeSet` doit supporter les opérations suivantes :

- Ajouter un intervalle `[low, high]`.
- Vérifier si une valeur appartient à l'ensemble.
- Fusionner les intervalles qui se chevauchent ou sont adjacents.
- Retourner le nombre d'intervalles distincts après fusion.

### Itération 1 : un ensemble vide ne contient rien

**Red** — On écrit le test avant que la classe existe :

```cpp
// tests/test_range_set.cpp
#include <gtest/gtest.h>
#include "mon_projet/range_set.hpp"  // N'existe pas encore

TEST(RangeSet, EmptySetContainsNothing) {
    mp::RangeSet set;
    EXPECT_FALSE(set.contains(0));
    EXPECT_FALSE(set.contains(42));
    EXPECT_FALSE(set.contains(-1));
}
```

Résultat : erreur de compilation — `range_set.hpp` n'existe pas. C'est notre première phase Red.

**Green** — On crée le minimum pour compiler et faire passer le test :

```cpp
// include/mon_projet/range_set.hpp
#pragma once

namespace mp {

class RangeSet {  
public:  
    bool contains(int /*value*/) const { return false; }
};

} // namespace mp
```

Le test passe. La méthode retourne `false` inconditionnellement — et c'est correct pour un ensemble vide. L'implémentation semble triviale, voire stupide. C'est intentionnel : on ne code que ce que les tests exigent.

**Refactor** — Rien à améliorer pour l'instant. On passe à l'itération suivante.

### Itération 2 : ajouter un intervalle et vérifier l'appartenance

**Red** — Nouveau test :

```cpp
TEST(RangeSet, ContainsValueWithinAddedRange) {
    mp::RangeSet set;
    set.add(10, 20);

    EXPECT_TRUE(set.contains(10));   // Borne inférieure incluse
    EXPECT_TRUE(set.contains(15));   // Milieu
    EXPECT_TRUE(set.contains(20));   // Borne supérieure incluse
    EXPECT_FALSE(set.contains(9));   // Juste en dessous
    EXPECT_FALSE(set.contains(21));  // Juste au-dessus
}
```

Résultat : erreur de compilation — `add()` n'existe pas. Puis, une fois la signature ajoutée, le test échoue à l'exécution car `contains` retourne toujours `false`.

**Green** — Implémentation minimale :

```cpp
#pragma once
#include <vector>
#include <utility>

namespace mp {

class RangeSet {  
public:  
    void add(int low, int high) {
        ranges_.emplace_back(low, high);
    }

    bool contains(int value) const {
        for (const auto& [low, high] : ranges_) {
            if (value >= low && value <= high) return true;
        }
        return false;
    }

private:
    std::vector<std::pair<int, int>> ranges_;
};

} // namespace mp
```

Les deux tests passent.

**Refactor** — L'implémentation est propre pour l'instant. On pourrait renommer `ranges_` ou extraire un type `Range`, mais ce serait prématuré. On continue.

### Itération 3 : fusionner les intervalles chevauchants

**Red** — Le test qui va forcer la fusion :

```cpp
TEST(RangeSet, MergesOverlappingRanges) {
    mp::RangeSet set;
    set.add(10, 20);
    set.add(15, 25);  // Chevauche [10, 20]

    EXPECT_TRUE(set.contains(10));
    EXPECT_TRUE(set.contains(22));   // Dans la zone fusionnée
    EXPECT_TRUE(set.contains(25));
    EXPECT_EQ(set.range_count(), 1); // Un seul intervalle après fusion
}
```

Résultat : `contains(22)` passe (la recherche linéaire trouve `[15, 25]`), mais `range_count()` n'existe pas. Après ajout d'un `range_count()` naïf, le test échoue : `range_count()` retourne 2 au lieu de 1.

**Green** — On implémente la fusion dans `add()` :

```cpp
void add(int low, int high) {
    // Fusionner avec les intervalles existants qui chevauchent
    std::vector<std::pair<int, int>> merged;
    bool inserted = false;

    for (const auto& [rlow, rhigh] : ranges_) {
        if (rlow > high + 1) {
            // L'intervalle existant est entièrement après
            if (!inserted) {
                merged.emplace_back(low, high);
                inserted = true;
            }
            merged.emplace_back(rlow, rhigh);
        } else if (rhigh < low - 1) {
            // L'intervalle existant est entièrement avant
            merged.emplace_back(rlow, rhigh);
        } else {
            // Chevauchement ou adjacence : fusionner
            low = std::min(low, rlow);
            high = std::max(high, rhigh);
        }
    }
    if (!inserted) merged.emplace_back(low, high);
    ranges_ = std::move(merged);
}

std::size_t range_count() const { return ranges_.size(); }
```

Tous les tests passent.

**Refactor** — L'algorithme de fusion mérite d'être clarifié. On trie les intervalles et on simplifie la logique :

```cpp
void add(int low, int high) {
    ranges_.emplace_back(low, high);
    merge_ranges();
}

private:  
void merge_ranges() {  
    if (ranges_.empty()) return;

    std::sort(ranges_.begin(), ranges_.end());

    std::vector<std::pair<int, int>> merged;
    merged.push_back(ranges_.front());

    for (std::size_t i = 1; i < ranges_.size(); ++i) {
        auto& last = merged.back();
        if (ranges_[i].first <= last.second + 1) {
            last.second = std::max(last.second, ranges_[i].second);
        } else {
            merged.push_back(ranges_[i]);
        }
    }
    ranges_ = std::move(merged);
}
```

On relance les tests — ils passent toujours. Le refactoring n'a pas changé le comportement, seulement la structure. C'est le contrat fondamental de la phase Refactor.

### Itération 4 : fusionner les intervalles adjacents

**Red** :

```cpp
TEST(RangeSet, MergesAdjacentRanges) {
    mp::RangeSet set;
    set.add(10, 20);
    set.add(21, 30);  // Adjacent à [10, 20]

    EXPECT_TRUE(set.contains(20));
    EXPECT_TRUE(set.contains(21));
    EXPECT_EQ(set.range_count(), 1); // Fusionnés en [10, 30]
}
```

Résultat : le test passe directement. La logique `<= last.second + 1` dans `merge_ranges()` gère déjà l'adjacence. C'est une situation courante en TDD : un nouveau test passe sans modification du code. Ce test a quand même de la valeur — il documente le comportement et le protège contre les régressions futures.

### Itération 5 : intervalles disjoints

**Red** :

```cpp
TEST(RangeSet, KeepsDisjointRangesSeparate) {
    mp::RangeSet set;
    set.add(10, 20);
    set.add(30, 40);

    EXPECT_TRUE(set.contains(15));
    EXPECT_TRUE(set.contains(35));
    EXPECT_FALSE(set.contains(25));  // Entre les deux
    EXPECT_EQ(set.range_count(), 2);
}
```

Le test passe immédiatement — l'implémentation gère correctement ce cas. On a maintenant cinq tests qui couvrent les comportements fondamentaux de `RangeSet`.

## Les trois lois du TDD

Robert C. Martin (Uncle Bob) a formalisé le TDD en trois règles simples qui résument la discipline :

1. **On n'écrit pas de code de production sans avoir d'abord un test qui échoue.** Le test échoue — y compris par erreur de compilation — avant toute écriture de code de production.

2. **On n'écrit pas plus de test que ce qui est nécessaire pour échouer.** Un seul `EXPECT_*` qui échoue suffit pour passer à la phase Green. Ne pas écrire dix assertions d'un coup avant d'avoir vu le premier test échouer.

3. **On n'écrit pas plus de code de production que ce qui est nécessaire pour faire passer le test qui échoue.** Pas de généralisation anticipée, pas de "tant qu'on y est, ajoutons aussi cette fonctionnalité".

Ces trois lois semblent restrictives, mais elles produisent un code qui évolue par petits incréments vérifiés, avec une couverture de test naturellement élevée puisque chaque ligne de production a été écrite pour satisfaire un test.

## L'art de choisir le prochain test

La progression des tests n'est pas arbitraire. Un bon praticien TDD choisit le prochain test selon une stratégie qui fait avancer l'implémentation de manière progressive et contrôlée.

**Commencer par les cas dégénérés.** Le cas le plus simple possible : entrée vide, ensemble vide, valeur nulle. Ces tests sont rapides à écrire, rapides à faire passer, et ils posent les fondations de l'API.

**Avancer vers les cas nominaux.** Un cas d'usage standard qui force l'écriture de la logique principale.

**Ajouter les cas limites.** Bornes d'intervalles, valeurs extremes, entrées à la frontière entre deux comportements.

**Terminer par les cas d'erreur.** Entrées invalides, préconditions violées, exceptions.

Cette progression — parfois appelée **stratégie de triangulation** — construit l'implémentation par couches successives. Chaque test ajoute une contrainte supplémentaire qui force le code à devenir plus général.

## Quand le TDD est particulièrement utile en C++

Le TDD n'est pas dogmatique — il n'est pas toujours le meilleur outil. Voici les situations où il brille particulièrement en C++ :

**Logique métier complexe.** Algorithmes, machines à états, parsers, validateurs — tout code dont le comportement est riche et bien spécifiable par des entrées/sorties. L'exemple `RangeSet` illustre ce cas.

**Conception d'API.** Écrire les tests en premier force à utiliser l'API avant de l'implémenter. Si le test est difficile à écrire, l'API est probablement mal conçue. Le TDD produit des interfaces ergonomiques parce qu'elles sont conçues du point de vue de l'utilisateur (le test) plutôt que de l'implémenteur.

**Code critique.** Code financier, code de sécurité, code embarqué — tout contexte où un bug coûte cher. La couverture quasi-totale produite par le TDD est un investissement qui se rembourse rapidement.

**Refactoring de code legacy.** Avant de modifier du code existant, écrire des tests qui documentent le comportement actuel (on parle de *characterization tests*). Ensuite, refactorer sous la protection de ces tests.

## Quand le TDD est moins adapté

**Exploration et prototypage.** Quand on ne sait pas encore à quoi l'API va ressembler, écrire des tests en premier peut être une friction inutile. Il est souvent plus efficace de prototyper librement, puis de couvrir le code retenu par des tests a posteriori.

**Code fortement dépendant du hardware.** Pilotes, code GPU, interactions avec des périphériques — le feedback loop est trop long et les résultats trop dépendants de l'environnement pour un cycle TDD rapide.

**Interface graphique et rendu visuel.** La correction d'un rendu se vérifie visuellement, pas par assertion. Les tests automatisés de GUI existent mais relèvent davantage du test d'intégration que du TDD unitaire.

**Code glue trivial.** Un constructeur qui affecte trois membres, un getter, un wrapper fin autour d'une bibliothèque — le coût d'écriture du test dépasse la valeur apportée. Le TDD ne signifie pas "100% de couverture à tout prix".

## Intégration du TDD dans le workflow quotidien

### Configuration recommandée

Pour pratiquer le TDD confortablement en C++, l'environnement de développement doit minimiser le temps entre l'écriture du code et le feedback des tests :

- **Un raccourci IDE pour compiler et lancer les tests** du module en cours. Dans VS Code, une tâche configurée dans `tasks.json` qui exécute `cmake --build build --target mon_module_tests && cd build && ctest -R MonModule --output-on-failure`. Dans CLion, la configuration Run associée au binaire de test.

- **Un terminal dédié** avec une commande de watch qui recompile et relance les tests à chaque sauvegarde. Des outils comme `entr` sur Linux rendent cela simple :

```bash
# Recompile et relance les tests à chaque modification d'un fichier .cpp ou .hpp
find src/ include/ tests/ -name '*.cpp' -o -name '*.hpp' | \
    entr -c sh -c 'cmake --build build --target range_set_tests 2>&1 && \
    cd build && ctest -R RangeSet --output-on-failure'
```

- **Filtrage des tests** pour ne relancer que ceux du comportement en cours de développement :

```bash
./build/tests/range_set_tests --gtest_filter="RangeSet.MergesOverlapping*"
```

### Le rythme

Un cycle TDD sain en C++ dure entre une et cinq minutes. Si vous passez plus de dix minutes sans voir un test passer, le pas est probablement trop grand. Décomposez : quel est le plus petit incrément de comportement que vous pouvez ajouter et vérifier ?

Le rythme se résume ainsi : écrire quelques lignes de test, voir le rouge, écrire quelques lignes de production, voir le vert, nettoyer si nécessaire, recommencer. La vitesse vient de la petitesse des incréments, pas de la vitesse de frappe.

## TDD et qualité de conception

Au-delà de la couverture de test, le bénéfice le plus profond du TDD est son influence sur la **conception** du code. Écrire un test avant l'implémentation force à réfléchir à l'interface publique, aux dépendances et à la testabilité dès le départ. Les symptômes de mauvaise conception deviennent immédiatement visibles :

- **Un test nécessite beaucoup de setup ?** La classe a trop de dépendances — elle viole le principe de responsabilité unique.
- **Un test est difficile à nommer ?** La méthode fait trop de choses — elle devrait être décomposée.
- **Un mock a trop d'`EXPECT_CALL` ?** Le code de production est trop couplé à ses collaborateurs.
- **Changer une implémentation interne casse des tests ?** Les tests vérifient l'implémentation au lieu du comportement.

Le TDD n'est pas seulement une technique de test — c'est une technique de conception qui utilise les tests comme outil de feedback. Le code résultant est naturellement modulaire, faiblement couplé et facile à maintenir, parce que ces propriétés sont la conséquence directe d'une bonne testabilité.

---


⏭️ [Couverture de Code](/34-couverture-code/README.md)
