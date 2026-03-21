🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 47.4 — Code reviews efficaces

> **Chapitre 47 : Collaboration et Maintenance** · Module 17 : Architecture de Projet Professionnel  
> **Niveau** : Expert  
> **Prérequis** : Sections 47.1 à 47.3 (Git, pre-commit hooks, clang-format/clang-tidy), chapitre 46 (Organisation et Standards)

---

## Introduction

Les sections précédentes ont automatisé tout ce qui peut l'être : le formatage est garanti par `clang-format`, les bugs mécaniques sont détectés par `clang-tidy`, l'hygiène du dépôt est assurée par les hooks d'hygiène. Ce qui reste — et c'est l'essentiel — ne peut être jugé que par un humain : la pertinence des choix d'architecture, la clarté des abstractions, la gestion des cas limites, la cohérence avec le reste du codebase, la maintenabilité à long terme.

La code review est le mécanisme qui capture ces défauts. Mais une code review mal pratiquée est pire que pas de review du tout : elle ralentit le développement, frustre les participants, crée des conflits interpersonnels et donne une fausse impression de qualité. Cette section détaille comment structurer les reviews pour qu'elles soient constructives, rapides et focalisées sur ce qui compte — dans le contexte spécifique du C++, où la complexité du langage rend la review à la fois plus difficile et plus critique que dans d'autres écosystèmes.

---

## Ce que la code review doit vérifier — et ce qu'elle ne doit pas

Le principe fondamental est simple : **la review humaine se concentre sur ce que les outils ne détectent pas**. Si les pre-commit hooks et la CI sont correctement configurés (sections 47.2-47.3 et chapitre 38), le reviewer n'a pas besoin de vérifier le formatage, les warnings de compilation, la présence de tests qui passent ou les violations de `clang-tidy`. Ces vérifications sont automatiques et le résultat est visible dans l'interface de la merge request (pipeline vert/rouge).

### Ce que la review doit couvrir

**Correction logique** — Le code fait-il ce qu'il est censé faire ? Les cas limites sont-ils gérés ? Les invariants sont-ils respectés ? En C++, cela inclut des préoccupations spécifiques :

- Les durées de vie des objets sont-elles correctes ? Un `std::string_view` ou un `std::span` pointe-t-il vers de la mémoire qui sera encore valide quand il sera utilisé ?  
- La sémantique de mouvement est-elle correctement utilisée ? Un objet est-il accidentellement copié là où un `std::move` serait approprié — ou inversement, un `std::move` sur un objet encore utilisé ensuite ?  
- Les conversions implicites sont-elles intentionnelles ? Un narrowing conversion silencieux (`double` → `int`) peut introduire des bugs subtils.  
- Le code est-il exception-safe ? En cas de `throw`, les ressources sont-elles correctement libérées (RAII) ? Les invariants de classe sont-ils préservés ?

**Choix d'architecture et de design** — Les abstractions choisies sont-elles appropriées ? Le code respecte-t-il les patterns établis dans le projet ? Un héritage est-il justifié ou une composition serait-elle préférable ? Le couplage entre modules est-il acceptable ?

**Maintenabilité** — Le code sera-t-il compréhensible dans six mois par un développeur qui ne l'a pas écrit ? Les noms de variables et de fonctions sont-ils explicites ? La complexité est-elle justifiée ? Les commentaires expliquent-ils le *pourquoi*, pas le *quoi* ?

**Performance (quand c'est pertinent)** — En C++, les choix de structure de données et d'allocation ont un impact direct sur les performances. Le reviewer doit vérifier :

- Le choix du conteneur est-il adapté au pattern d'accès ? Un `std::vector` (cache-friendly) plutôt qu'une `std::list` (pointer chasing) quand l'accès séquentiel domine ?  
- Y a-t-il des allocations dynamiques inutiles dans un chemin critique ?  
- Les objets lourds sont-ils passés par `const&` plutôt que par valeur ?  
- Les algorithmes ont-ils la bonne complexité pour la taille des données attendue ?

**Sécurité** — Buffers overflows potentiels, utilisation de `reinterpret_cast` non justifiée, données utilisateur non validées, secrets hardcodés.

**Tests** — Non pas "les tests passent-ils" (c'est le travail de la CI), mais "les tests couvrent-ils les bons cas ?" Les cas limites sont-ils testés ? Les tests sont-ils lisibles et maintenables ? Un test vérifie-t-il le comportement ou l'implémentation (ce dernier étant fragile) ?

### Ce que la review ne doit PAS couvrir

**Le formatage et le style** — C'est le travail de `clang-format`. Si un commentaire de review porte sur le placement d'une accolade, le nombre d'espaces ou le nommage d'une variable, c'est le signe que les outils automatiques sont mal configurés. Corrigez la configuration, pas le code à la main.

**Les warnings de compilation et d'analyse statique** — C'est le travail de `clang-tidy` et de la CI. Si le pipeline est vert, ces vérifications ont été faites.

**Les préférences personnelles** — "J'aurais écrit ça autrement" n'est pas un commentaire de review valide, sauf si la version proposée est objectivement moins claire, moins performante ou moins maintenable. La review n'est pas un véhicule pour imposer son style personnel au-delà des standards du projet.

---

## Anatomie d'une bonne merge request

La qualité de la review dépend en grande partie de la qualité de la merge request (MR/PR). Un reviewer qui reçoit une MR de 2000 lignes sans description ni contexte ne peut pas faire un travail efficace. L'auteur de la MR est responsable de faciliter le travail du reviewer.

### Taille de la MR

La taille est le facteur le plus déterminant pour la qualité de la review. Les études sur le sujet convergent : au-delà de 400 lignes de diff, la capacité de détection de défauts chute drastiquement. Le reviewer entre en mode "skim" — il parcourt le code sans vraiment l'analyser.

**Règle pratique** : visez des MR de 100 à 300 lignes de diff effectif (hors fichiers générés et tests mécaniques). Si un développement dépasse cette taille, découpez-le en MR successives avec une relation de dépendance claire.

En C++, le découpage est parfois contraint par la structure du langage : ajouter une méthode virtuelle dans une classe de base implique de modifier toutes les classes dérivées dans le même commit (sinon le code ne compile pas). Dans ces cas, documentez dans la description pourquoi la MR est volumineuse.

### Structure de la description

Une description de MR efficace répond à quatre questions :

```markdown
## Contexte
Pourquoi ce changement ? Quel problème résout-il ? Quel ticket référence-t-il ?

## Changements
Quoi concrètement ? Résumé des modifications, organisé par module si nécessaire.

## Décisions de design
Pourquoi cette approche plutôt qu'une autre ? Les alternatives envisagées  
et les raisons du choix. En C++, c'est souvent le point le plus important :  
pourquoi un shared_ptr plutôt qu'un unique_ptr, pourquoi un template plutôt  
qu'un héritage, pourquoi une allocation stack plutôt que heap.  

## Comment tester
Instructions pour vérifier le changement manuellement si les tests  
automatiques ne suffisent pas (scénarios de test, commandes à exécuter).  
```

### Template de MR

Intégrez ce template dans votre forge Git pour que chaque MR soit créée avec une structure cohérente :

```markdown
<!-- .gitlab/merge_request_templates/Default.md -->
<!-- ou .github/pull_request_template.md -->

## Contexte

<!-- Décrivez le problème résolu. Référencez le ticket : Fixes #123 -->

## Changements

<!-- Résumé des modifications, organisé par module -->

## Décisions de design

<!-- Alternatives envisagées et raisons du choix -->

## Checklist

- [ ] Les tests couvrent les nouveaux cas
- [ ] La documentation est à jour (si applicable)
- [ ] Pas de TODO sans ticket
- [ ] Les changements d'API publique sont documentés
- [ ] Impact performance évalué (si chemin critique)
```

---

## Processus de review

### Combien de reviewers ?

**Un reviewer minimum** — C'est le strict minimum pour que la review ait un sens. Adapté aux petites équipes (2-5 personnes) et aux changements simples.

**Deux reviewers pour le code critique** — Code touche le système de fichiers, le réseau, la gestion mémoire, la sécurité, ou l'API publique d'une bibliothèque. Le second reviewer apporte un regard différent et réduit le risque d'angle mort.

En pratique, exiger systématiquement deux reviewers sur tous les changements ralentit le flux. Réservez le double review aux modules critiques. Les forges Git modernes supportent les fichiers `CODEOWNERS` qui assignent automatiquement les reviewers par répertoire :

```
# .github/CODEOWNERS (ou CODEOWNERS à la racine pour GitLab)
# Chaque ligne associe un pattern de fichier à un ou plusieurs reviewers

# Module réseau : review obligatoire par l'équipe réseau
src/network/          @team-network  
include/network/      @team-network  

# API publique : review par un lead
include/public/       @lead-alice @lead-bob

# Sécurité : double review
src/crypto/           @security-team  
src/auth/             @security-team  

# Build system : review par le build engineer
CMakeLists.txt        @build-engineer  
cmake/                @build-engineer  
conanfile.py          @build-engineer  

# Tout le reste : n'importe quel membre de l'équipe
*                     @team-core
```

### Délai de review

Un délai de review long est destructeur pour la vélocité d'une équipe. Le développeur perd le contexte, la branche diverge, les conflits de merge s'accumulent. En C++, où la recompilation après un rebase peut prendre du temps, chaque jour de retard a un coût amplifié.

**Cible** : première réponse (même partielle) dans les **4 heures ouvrées**. Review complète dans les **24 heures ouvrées**. Si ces délais ne sont pas tenus, c'est un signal organisationnel (trop de MR en parallèle, reviewers surchargés, MR trop volumineuses).

**Pratique utile** : bloquer un créneau quotidien dédié aux reviews (par exemple 30-45 minutes le matin). Traiter les reviews en batch est plus efficace que de les intercaler entre des sessions de développement profond.

### L'ordre de lecture

Un reviewer efficace ne lit pas le diff de haut en bas. L'ordre de lecture qui maximise la compréhension :

1. **La description de la MR** — comprendre le contexte et l'objectif avant de regarder le code.
2. **Les tests** — les tests décrivent le comportement attendu. Les lire en premier donne un cadre pour évaluer l'implémentation.
3. **Les fichiers d'en-tête modifiés (`.h`, `.hpp`)** — l'interface publique révèle le design. C'est le niveau le plus important à reviewer en C++ : une mauvaise interface est coûteuse à corriger (ABI, utilisateurs downstream).
4. **Les fichiers d'implémentation (`.cpp`)** — le détail de l'implémentation, guidé par la compréhension de l'interface.
5. **Les fichiers de build (`CMakeLists.txt`, Conan, presets)** — vérifier que les dépendances et les flags sont corrects.

---

## Rédiger des commentaires de review constructifs

La qualité des commentaires de review est ce qui distingue une revue utile d'une revue toxique. Chaque commentaire doit aider l'auteur à améliorer son code, pas le mettre en défaut.

### Taxonomie des commentaires

Préfixer chaque commentaire avec une étiquette qui indique son niveau d'importance évite les malentendus :

**`blocker:`** — Le code a un bug, une fuite mémoire, un comportement indéfini, ou une violation de sécurité. La MR ne peut pas être mergée sans correction.

```
blocker: Ce string_view pointe vers un temporary qui sera détruit
à la fin de l'expression. Undefined behavior en lecture.
Suggestion : stocker le résultat dans un std::string.
```

**`issue:`** — Un problème significatif qui devrait être corrigé, mais qui n'est pas un bug bloquant. Mauvaise complexité algorithmique, abstraction inadaptée, test manquant pour un cas limite important.

```
issue: Cette recherche linéaire dans un vector de 10k éléments sera  
appelée en boucle. Considérer un unordered_map pour un accès O(1).  
```

**`nit:`** (nitpick) — Une suggestion mineure, cosmétique ou stylistique. L'auteur peut l'ignorer sans discussion. Le préfixe `nit:` signale explicitement que ce n'est pas bloquant.

```
nit: Le nom 'process_data' est un peu générique.
'validate_and_transform_input' serait plus descriptif.
```

**`question:`** — Le reviewer ne comprend pas le code et demande une explication. Ce n'est pas une critique — c'est un signal que le code pourrait être plus clair, ou que le reviewer a besoin de contexte.

```
question: Pourquoi un weak_ptr ici plutôt qu'un raw pointer ?  
Y a-t-il un risque que l'objet soit détruit pendant l'appel ?  
```

**`suggestion:`** — Une proposition d'amélioration, ni bloquante ni mineure. L'auteur est libre de l'adopter ou de justifier son approche actuelle.

```
suggestion: std::expected<Config, ParseError> serait une alternative  
propre aux exceptions ici, et rendrait l'interface plus explicite  
pour l'appelant. Voir section 12.8.  
```

**`praise:`** — Un commentaire positif. Trop souvent oublié. Signaler ce qui est bien fait est aussi important que signaler les problèmes — cela renforce les bonnes pratiques et maintient la motivation.

```
praise: L'utilisation de std::span ici est exactement la bonne  
abstraction. Ça évite la copie et accepte vector, array et C-array.  
```

### La règle du "comment, pas quoi"

Un bon commentaire de review ne dit pas "c'est faux" — il explique **pourquoi** c'est problématique et **comment** corriger :

**Mauvais** :
```
Ce code est faux.
```

**Médiocre** :
```
Il y a un problème de lifetime ici.
```

**Bon** :
```
blocker: Le std::string_view retourné par get_name() pointe vers  
le buffer interne du std::string local 'full_name', qui est détruit  
à la sortie de la fonction. L'appelant obtient un dangling reference.

Deux solutions :
1. Retourner std::string (copie, mais safe)
2. Stocker le string dans le membre de classe et retourner
   une vue sur le membre (zero-copy, mais couplage lifetime)
```

Le reviewer a identifié le problème, expliqué le mécanisme sous-jacent (dangling reference), et proposé deux solutions avec leurs trade-offs. L'auteur peut prendre une décision éclairée.

### Éviter le bikeshedding

Le *bikeshedding* (la loi de Parkinson appliquée aux reviews) est la tendance à discuter longuement de détails triviaux tout en survolant les décisions structurantes. En C++, le bikeshedding prend souvent ces formes :

- Débat sur `auto` vs type explicite quand le type est évident du contexte.  
- Discussion sur `const int` vs `int const` (east const vs west const).  
- Préférence `size_t` vs `std::size_t`.  
- Style de commentaires (`//` vs `/* */`).

Si ces choix ne sont pas couverts par le `.clang-format`, le `.clang-tidy` ou le guide de style du projet, ils ne méritent pas un commentaire de review. Et s'ils méritent une convention, ajoutez-la aux outils automatiques plutôt que de la faire respecter humainement.

**Indicateur de bikeshedding** : si une discussion de review dépasse 3 allers-retours sur un point non fonctionnel, c'est du bikeshedding. Prenez une décision, documentez-la dans le guide de style, et passez à la suite.

---

## Checklist de review C++

Une checklist aide le reviewer à couvrir systématiquement les points importants sans en oublier. Elle ne remplace pas le jugement — elle le structure.

### Correction et sécurité

- [ ] Les durées de vie des objets sont-elles correctes ? (pas de dangling references, pas de use-after-move)  
- [ ] Les ressources sont-elles gérées par RAII ? (pas de `new`/`delete` manuels, pas de `fopen` sans `fclose` garanti)  
- [ ] Les conversions de types sont-elles explicites et sûres ? (pas de narrowing implicite, `static_cast` plutôt que C-style cast)  
- [ ] Le code est-il exception-safe ? (strong guarantee si possible, basic guarantee au minimum)  
- [ ] Les entrées utilisateur/externes sont-elles validées avant utilisation ?  
- [ ] Les accès concurrents sont-ils protégés ? (mutex, atomic, ou documentation que le code est single-threaded)

### Design et architecture

- [ ] Le changement respecte-t-il les patterns et abstractions existants du projet ?  
- [ ] Les nouvelles classes/fonctions ont-elles une responsabilité unique et claire ?  
- [ ] L'interface publique (header) est-elle minimale et expressive ?  
- [ ] Les dépendances entre modules sont-elles acceptables ? (pas de dépendance circulaire, couplage raisonnable)  
- [ ] Les smart pointers sont-ils utilisés avec la bonne sémantique ? (`unique_ptr` par défaut, `shared_ptr` uniquement si ownership partagé documenté)

### Performance (si applicable au contexte)

- [ ] Les structures de données sont-elles adaptées au pattern d'accès ?  
- [ ] Les objets lourds sont-ils passés par `const&` ou par `std::move` ?  
- [ ] Y a-t-il des allocations évitables dans les chemins critiques ?  
- [ ] Les algorithmes ont-ils la bonne complexité ?

### Maintenabilité

- [ ] Les noms sont-ils descriptifs et cohérents avec le reste du projet ?  
- [ ] Les commentaires expliquent-ils le *pourquoi*, pas le *quoi* ?  
- [ ] La complexité est-elle justifiée ? (un code simple qui fonctionne bat un code élégant difficile à maintenir)  
- [ ] Les TODO référencent-ils un ticket ?

### Tests

- [ ] Les nouveaux comportements sont-ils couverts par des tests ?  
- [ ] Les cas limites sont-ils testés ? (collections vides, valeurs nulles, overflow, timeout)  
- [ ] Les tests sont-ils lisibles et indépendants les uns des autres ?  
- [ ] Les tests vérifient-ils le comportement, pas l'implémentation ?

---

## Les pièges spécifiques au C++ en review

Le C++ a des particularités qui rendent certaines catégories de bugs invisibles à l'œil non averti. Un reviewer C++ expérimenté porte une attention spéciale à ces patterns.

### Durées de vie et références pendantes

C'est la catégorie de bugs la plus dangereuse en C++ — et la plus facile à manquer en review :

```cpp
// ⚠️ Bug subtil — le reviewer doit le détecter
std::string_view get_greeting(const std::string& name) {
    std::string result = "Hello, " + name + "!";
    return result;  // string_view pointe vers 'result' qui est détruit
}
```

```cpp
// ⚠️ Bug subtil — range-based for avec temporary
for (auto c : get_string().substr(0, 5)) {  // temporary détruit avant la boucle
    process(c);  // undefined behavior
}
```

```cpp
// ⚠️ Bug subtil — capture de référence dans lambda retournée
auto make_counter(int& start) {
    return [&start]() { return start++; };  // 'start' peut être détruit
}
```

Ces bugs compilent sans warning (même avec `-Wall -Wextra`), passent souvent les tests unitaires (le stack n'est pas encore réécrit), et crashent en production sous des conditions spécifiques. Le reviewer est parfois la dernière ligne de défense.

### Sémantique de mouvement incorrecte

```cpp
// ⚠️ Bug — utilisation après move
void process(std::vector<Data> dataset) {
    auto backup = std::move(dataset);
    
    // ... 50 lignes plus tard ...
    
    for (const auto& item : dataset) {  // dataset est vidé par le move
        analyze(item);
    }
}
```

```cpp
// ⚠️ Sous-optimal — move sur un objet trivially copyable
int value = 42;  
auto result = std::move(value);  // move sur un int = copie, mais confusion  
```

### Thread-safety implicitement violée

```cpp
// ⚠️ Data race — shared_ptr copié dans un thread, modifié dans un autre
class Cache {
    std::shared_ptr<Config> config_;  // Lecture thread-safe, mais...
    
    void update(std::shared_ptr<Config> new_config) {
        config_ = new_config;  // Assignation non atomique du shared_ptr lui-même
    }
    
    Config get() const {
        return *config_;  // Course avec update()
    }
};
```

Le reviewer doit vérifier que les objets partagés entre threads sont protégés — pas seulement les données qu'ils contiennent, mais les pointeurs eux-mêmes.

### Exceptions et garanties

```cpp
// ⚠️ Pas exception-safe
void transfer(Account& from, Account& to, double amount) {
    from.withdraw(amount);   // Si ceci réussit...
    to.deposit(amount);      // ...mais ceci lance une exception → argent perdu
}
```

Le reviewer doit évaluer si le code offre au minimum la *basic exception guarantee* (pas de fuite de ressource, invariants préservés) et idéalement la *strong guarantee* (opération atomique — soit tout réussit, soit rien ne change).

---

## IA et code review en 2026

Les outils d'assistance IA pour la review de code sont de plus en plus présents dans les forges Git. Ils peuvent automatiquement détecter certaines catégories de problèmes et suggérer des améliorations. Leur place dans le workflow mérite une discussion nuancée.

### Ce que l'IA fait bien

- **Détection de patterns connus** : bugs de lifetime, use-after-move, conversions dangereuses — des patterns que `clang-tidy` détecte déjà, mais que l'IA peut formuler en langage naturel avec une explication contextuelle.  
- **Résumé des changements** : générer un résumé lisible d'une MR volumineuse, aidant le reviewer à se former un modèle mental avant de plonger dans le code.  
- **Vérification de la documentation** : détecter si les changements d'API publique sont accompagnés d'une mise à jour de la documentation.

### Ce que l'IA ne remplace pas

- **Le jugement architectural** : l'IA n'a pas la vision globale du projet, de sa trajectoire, de ses contraintes de compatibilité.  
- **La connaissance du domaine** : un reviewer humain sait que telle opération financière doit être atomique, ou que tel protocole réseau a des contraintes de latence spécifiques.  
- **La relation humaine** : une review est aussi un acte de mentorat et de communication. Le feedback constructif d'un collègue expérimenté a un impact formatif que l'IA ne peut pas reproduire.

### Recommandation

Utilisez l'IA comme un **premier filtre** qui complète (mais ne remplace pas) la review humaine. L'IA attrape les erreurs mécaniques que le reviewer humain aurait pu manquer par fatigue ou par familiarité excessive avec le code. Le reviewer humain se concentre sur les décisions de design, la cohérence architecturale et le mentorat.

---

## Anti-patterns de review

### Le "Rubber Stamp"

Le reviewer approuve la MR en quelques secondes sans la lire, souvent par pression de délai ou par confiance excessive dans l'auteur. C'est l'anti-pattern le plus dangereux car il donne l'illusion d'un processus de qualité sans en fournir les bénéfices.

**Signal** : temps entre l'assignation et l'approbation inférieur à 5 minutes pour une MR de plus de 100 lignes.

**Solution** : les forges Git peuvent imposer un délai minimum entre l'ouverture de la MR et l'approbation, ou exiger au moins un commentaire substantiel du reviewer.

### Le "Gatekeeper"

Le reviewer bloque systématiquement les MR pour des raisons mineures, exige de multiples modifications cosmétiques, ou refuse d'approuver tant que le code ne correspond pas exactement à la manière dont il l'aurait écrit. Cet anti-pattern ralentit l'équipe et démoralise les auteurs.

**Signal** : un reviewer qui a un taux de "request changes" significativement plus élevé que les autres, principalement pour des raisons `nit:`.

**Solution** : la checklist de review aide à objectiver les critères. Les nitpicks ne doivent jamais bloquer une MR. Le lead technique peut intervenir pour arbitrer si un reviewer bloque de manière déraisonnable.

### L'Absence de feedback positif

Une review qui ne contient que des critiques, même justifiées, est décourageante. L'auteur a l'impression que rien de ce qu'il fait n'est assez bien. Avec le temps, il perd la motivation de soumettre du code propre ("de toute façon, le reviewer trouvera toujours quelque chose").

**Solution** : inclure au moins un commentaire `praise:` par review. Signaler une bonne utilisation de `std::expected`, un test bien conçu, ou un refactoring qui améliore la lisibilité prend 10 secondes et a un impact disproportionné sur la dynamique d'équipe.

### La Review de 2000 lignes

L'auteur soumet une MR massive parce que "tout est lié" ou "je n'ai pas eu le temps de découper". Le reviewer est submergé, la qualité de la review chute, les bugs passent au travers.

**Solution** : établir une limite souple (par exemple 400 lignes) au-delà de laquelle l'auteur doit justifier pourquoi la MR ne peut pas être découpée. Le reviewer a le droit de retourner une MR trop grande avec un commentaire "merci de découper cette MR en parties plus petites".

---

## Métriques de review

Mesurer le processus de review permet d'identifier les goulots d'étranglement et d'améliorer le flux. Quelques métriques utiles :

| Métrique | Cible indicative | Ce qu'elle révèle |
|---|---|---|
| **Temps jusqu'à la première réponse** | < 4h ouvrées | Réactivité de l'équipe |
| **Temps jusqu'au merge** | < 48h ouvrées | Fluidité du cycle |
| **Taille médiane des MR** | 100-300 lignes | Granularité des changements |
| **Nombre d'allers-retours** | 1-2 | Clarté du code et des reviews |
| **Taux de "request changes"** | 30-50 % | Calibrage de la qualité pré-review |
| **Commentaires par MR** | 3-10 | Profondeur de la review |

Ces métriques sont des indicateurs, pas des objectifs. Les optimiser directement (par exemple en décourageant les commentaires pour réduire les allers-retours) détruirait la valeur du processus. Utilisez-les pour détecter les tendances et les anomalies, pas pour juger les individus.

---

## Résumé

La code review efficace en C++ repose sur trois conditions : des MR petites et bien documentées par l'auteur, des reviewers qui se concentrent sur ce que les outils ne détectent pas (architecture, lifetime safety, thread-safety, exception safety), et une culture de feedback constructif où les commentaires sont étiquetés par importance et où le feedback positif a sa place. Les pre-commit hooks et la CI éliminent les frictions mécaniques (formatage, warnings, tests) pour que la conversation humaine porte sur le fond. Et cette conversation, bien menée, est le mécanisme le plus puissant dont dispose une équipe pour produire du code C++ robuste et maintenable sur la durée.

---


⏭️ [Gestion de la dette technique](/47-collaboration/05-dette-technique.md)
