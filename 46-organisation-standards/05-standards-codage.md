🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 46.5 — Standards de codage

> **Chapitre 46 : Organisation et Standards** · Module 17 : Architecture de Projet Professionnel  
> **Niveau** : Expert · **Prérequis** : Sections 46.1 à 46.4, section 32.1 (clang-tidy), section 32.3 (clang-format)

---

## Pourquoi un standard de codage est nécessaire

Un projet C++ sans standard de codage converge inévitablement vers le chaos. Pas immédiatement — les premiers mois, chaque développeur écrit du code lisible selon ses propres critères. Mais ces critères divergent. L'un nomme ses classes en `PascalCase`, l'autre en `snake_case`. L'un place l'accolade ouvrante sur la même ligne, l'autre sur la ligne suivante. L'un utilise `auto` partout, l'autre jamais. L'un préfixe ses membres privés avec `m_`, l'autre avec un underscore trailing, un troisième ne préfixe rien du tout.

Individuellement, chacun de ces choix est défendable. Collectivement, leur coexistence dans la même base de code crée un bruit visuel permanent qui ralentit la lecture, complique les code reviews, et rend les diffs Git inutilement volumineux (des commits entiers de reformatage mêlés aux changements fonctionnels). Le problème n'est pas que tel style est meilleur qu'un autre — c'est que l'absence de choix commun est un coût payé à chaque ligne lue.

Un standard de codage est un contrat entre les membres d'une équipe. Il fixe les conventions une fois pour toutes, retire ces micro-décisions du quotidien, et permet à chaque développeur de se concentrer sur la logique plutôt que sur la forme. Le bénéfice principal n'est pas l'esthétique : c'est la **réduction de la charge cognitive** lors de la lecture de code écrit par d'autres.

---

## Ce qu'un standard de codage couvre

Un standard de codage complet pour un projet C++ adresse plusieurs dimensions, qui vont bien au-delà du simple formatage.

### Conventions de nommage

C'est l'aspect le plus visible et souvent le plus débattu. Le standard fixe la casse et le style pour chaque catégorie de symbole : classes, fonctions, variables locales, membres de données, constantes, namespaces, macros, paramètres de template. La cohérence du nommage est ce qui permet de déduire la nature d'un symbole à la lecture — sans chercher sa déclaration.

Par exemple, dans un code qui suit le Google Style :

```cpp
class ConnectionPool {                    // PascalCase → classe
    void acquire_connection();            // snake_case → méthode
    int max_connections_;                 // snake_case + trailing _ → membre privé
    static constexpr int kDefaultPort = 8080;  // k + PascalCase → constante
};
```

Chaque convention porte une information implicite. Le trailing underscore signale un membre privé sans avoir besoin de chercher `private:`. Le préfixe `k` signale une constante. Le `PascalCase` identifie un type. Ces signaux visuels accélèrent considérablement la lecture.

### Règles de formatage

L'indentation (espaces vs tabulations, largeur), le placement des accolades (K&R, Allman, GNU), la longueur maximale des lignes, l'espacement autour des opérateurs, le style des includes — tous ces aspects doivent être fixés. Mais contrairement au nommage, le formatage peut et doit être **automatisé intégralement** via `clang-format` (section 32.3). Un fichier `.clang-format` à la racine du projet applique les règles de formatage de façon déterministe, éliminant toute discussion et tout travail manuel.

### Règles d'usage du langage

C'est la dimension la plus riche et la plus spécifique au C++. Le langage offre souvent plusieurs façons d'accomplir la même chose, et un standard de codage tranche entre elles. Quelques exemples de questions couvertes :

- Quand utiliser `auto` vs un type explicite ?  
- Les exceptions sont-elles autorisées ? Si oui, dans quelles circonstances ?  
- Le RTTI (`dynamic_cast`, `typeid`) est-il permis ?  
- L'héritage multiple est-il autorisé ? Avec quelles restrictions ?  
- Les fonctions de plus de N lignes doivent-elles être découpées ?  
- Les paramètres de sortie sont-ils acceptables ou faut-il préférer les valeurs de retour (tuples, structs, `std::optional`) ?  
- Les macros du préprocesseur sont-elles interdites sauf cas spécifiques ?  
- Quelles fonctionnalités C++ récentes sont autorisées et lesquelles sont encore trop immatures ?

Ces règles d'usage sont les plus difficiles à automatiser. `clang-tidy` (section 32.1) en vérifie certaines via ses checks, mais beaucoup relèvent du jugement humain et des code reviews (section 47.4).

### Règles de documentation

Le standard définit ce qui doit être documenté (section 46.4), le style de commentaire Doxygen retenu, et le niveau de documentation attendu selon le type de symbole (API publique vs implémentation interne).

### Règles d'organisation

Le standard peut aussi fixer des conventions d'organisation : structure des répertoires (section 46.1), ordre des sections dans un fichier header (includes, forward declarations, types, fonctions), ordre des membres dans une classe (public avant private, méthodes avant données), et conventions de nommage des fichiers.

---

## Les trois grands guides de l'industrie

L'écosystème C++ dispose de trois guides de style majeurs, chacun issu d'un contexte différent et portant une philosophie distincte. Plutôt que d'inventer un standard de zéro — exercice coûteux et sujet à l'oubli de cas importants — la plupart des équipes partent de l'un de ces guides et l'adaptent à leur contexte.

### Google C++ Style Guide

Publié par Google et utilisé sur l'ensemble de leur base de code C++ (des centaines de millions de lignes), c'est le guide le plus prescriptif et le plus détaillé. Il couvre le nommage, le formatage, les règles d'usage du langage, et même des choix controversés (comme la restriction historique des exceptions, bien que cette position ait évolué). Sa force est sa complétude : presque chaque question a une réponse tranchée. Son risque est l'excès de rigidité dans des contextes différents de celui de Google.

### LLVM Coding Standards

Utilisé par le projet LLVM/Clang et les projets associés, ce guide reflète les pratiques d'un projet open source de grande envergure centré sur les compilateurs et les outils. Il est moins prescriptif que le Google Style sur les règles d'usage, mais très cohérent sur le nommage et le formatage. Son influence est forte dans l'écosystème des outils C++ : si vous contribuez à Clang, LLDB, ou un projet dérivé, c'est ce style que vous suivrez.

### C++ Core Guidelines

Créées par Bjarne Stroustrup (créateur du C++) et Herb Sutter (président du comité de standardisation), les Core Guidelines sont le guide le plus ambitieux : elles ne fixent pas un style de formatage mais formulent des **règles de design et de sécurité** applicables à tout projet C++ moderne. Elles sont organisées en profils (Safety, Lifetime, Bounds) qui visent à éliminer des catégories entières de bugs. Leur portée est plus large et plus profonde que les deux autres guides, mais elles sont aussi moins opérationnelles sur les questions de formatage pur.

---

## Adopter, adapter, appliquer

### Choisir un guide de base

Le choix du guide de base dépend du contexte de l'équipe :

- **Google Style** convient aux équipes qui veulent un standard complet et prêt à l'emploi, avec une réponse à presque chaque question. C'est le choix le plus courant dans l'industrie hors écosystème LLVM.  
- **LLVM Style** convient aux équipes qui travaillent sur des outils, des compilateurs, ou qui contribuent à l'écosystème LLVM. C'est aussi un bon choix pour les projets qui préfèrent un standard moins contraignant sur les pratiques du langage.  
- **C++ Core Guidelines** conviennent comme couche de règles de design au-dessus d'un guide de formatage. Elles ne remplacent pas un standard de formatage — elles le complètent.

En pratique, de nombreuses équipes combinent un guide de formatage (Google ou LLVM) avec les C++ Core Guidelines pour les règles de design.

### Adapter au contexte

Aucun guide externe ne correspond parfaitement au contexte de chaque projet. L'adaptation est non seulement acceptable, elle est nécessaire. Les déviations doivent être documentées dans un fichier dédié (souvent `CODING_STYLE.md` ou `CONTRIBUTING.md` à la racine du dépôt), avec la justification de chaque écart.

Les adaptations typiques portent sur les exceptions (autorisées ou interdites selon le domaine — l'embarqué les interdit souvent), les fonctionnalités C++ récentes (un projet qui doit supporter GCC 12 ne peut pas utiliser les fonctionnalités C++23), et les conventions de nommage (certaines équipes préfèrent un préfixe `m_` au trailing underscore pour les membres privés).

La règle d'or : **chaque déviation doit être une décision consciente et documentée, pas un oubli ou une habitude individuelle.**

### Appliquer automatiquement

Un standard de codage qui n'est pas automatiquement vérifié est un standard qui sera violé. Les outils d'application sont :

**`clang-format`** (section 32.3) pour le formatage. Le fichier `.clang-format` à la racine du projet encode les règles de formatage. `clang-format` supporte nativement les styles Google, LLVM, Chromium, Mozilla et WebKit comme bases de configuration :

```yaml
# .clang-format
BasedOnStyle: Google  
IndentWidth: 4  
ColumnLimit: 100  
```

**`clang-tidy`** (section 32.1) pour les règles d'usage du langage. Les checks `google-*`, `llvm-*` et `cppcoreguidelines-*` vérifient automatiquement le respect des règles correspondantes :

```yaml
# .clang-tidy
Checks: >
  -*,
  cppcoreguidelines-*,
  google-*,
  modernize-*,
  readability-*,
  -google-build-using-namespace
WarningsAsErrors: '*'
```

**Pre-commit hooks** (chapitre 47) pour l'application au moment du commit. Chaque commit est vérifié localement avant d'atteindre le dépôt distant. C'est la première ligne de défense.

**Pipeline CI** (chapitre 38) pour la vérification finale. Le CI exécute `clang-format --dry-run --Werror` et `clang-tidy` sur chaque merge request. Un formatage incorrect ou une violation de règle fait échouer le pipeline — aucune déviation ne passe en production.

Cette chaîne — `.clang-format` + `.clang-tidy` + pre-commit hooks + CI — transforme le standard de codage d'un document théorique en une contrainte appliquée mécaniquement. Les code reviews peuvent alors se concentrer sur la logique, le design et l'architecture, plutôt que sur le placement des accolades.

---

## Le piège du débat sans fin

Le choix d'un standard de codage est l'un des sujets les plus propices aux débats interminables et stériles dans une équipe technique. Tabulations vs espaces. `camelCase` vs `snake_case`. Accolades K&R vs Allman. Chaque développeur a ses préférences, souvent fortes, rarement justifiées par autre chose que l'habitude.

La réalité est que **la plupart de ces choix n'ont pas d'impact mesurable sur la qualité du code**. Ce qui a un impact, c'est la cohérence. Un projet entièrement en `snake_case` est aussi lisible qu'un projet entièrement en `camelCase`. Un projet qui mélange les deux est plus difficile à lire que n'importe lequel des deux.

La stratégie la plus efficace est de choisir un guide de base reconnu, de l'adopter rapidement avec un minimum d'adaptations, et de fermer le débat. Le temps passé à discuter du style optimal est du temps qui n'est pas passé à écrire du code, et la réponse "parfaite" n'existe pas. La meilleure convention est celle que l'équipe adopte et applique réellement.

---

## Sommaire de la section

- **46.5.1** — Google C++ Style Guide : philosophie, conventions de nommage, règles d'usage, forces et limites.  
- **46.5.2** — LLVM Style : philosophie, différences avec Google, cas d'usage.  
- **46.5.3** — C++ Core Guidelines : approche par profils, règles de design et de sécurité, intégration avec clang-tidy.

---


⏭️ [Google C++ Style Guide](/46-organisation-standards/05.1-google-style.md)
