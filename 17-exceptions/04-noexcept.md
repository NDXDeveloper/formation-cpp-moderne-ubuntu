🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 17.4 — `noexcept` et garanties d'exception

## Exprimer, exploiter et garantir l'absence d'exceptions

---

## Introduction

Les sections précédentes ont couvert la mécanique des exceptions : comment les lancer, les capturer, et concevoir des hiérarchies adaptées à votre domaine. Mais il reste une question fondamentale que nous n'avons pas encore abordée : **que se passe-t-il quand une fonction promet de ne jamais lever d'exception ?**

Cette promesse, exprimée par le mot-clé `noexcept`, n'est pas un simple ornement syntaxique. Elle a des conséquences profondes sur trois aspects du développement C++ :

- **La performance** — le compilateur peut générer du code plus efficace lorsqu'il sait qu'aucune exception ne sera levée, en éliminant les structures nécessaires au stack unwinding.
- **La correction** — la bibliothèque standard modifie son comportement selon que vos opérations sont `noexcept` ou non. L'exemple le plus critique est `std::vector` : lors d'une réallocation, il utilisera le constructeur de déplacement de vos objets *uniquement* si celui-ci est `noexcept` ; dans le cas contraire, il se rabattra sur la copie pour préserver la **garantie forte** d'exception.
- **La documentation** — `noexcept` fait partie de l'interface publique d'une fonction. Il communique aux utilisateurs de votre API une information contractuelle exploitable : « cette opération ne peut pas échouer ».

Au-delà de `noexcept` lui-même, cette section explore le cadre plus large des **garanties d'exception** — un concept fondamental de la conception de bibliothèques C++ qui structure la manière dont les fonctions se comportent en présence d'erreurs.

---

## Les trois niveaux de garantie d'exception

Avant d'aborder la syntaxe de `noexcept`, il est essentiel de comprendre le cadre conceptuel dans lequel il s'inscrit. Depuis les travaux de Dave Abrahams dans les années 1990, la communauté C++ distingue trois niveaux de garantie qu'une fonction peut offrir face aux exceptions. Ces garanties ne sont pas exprimées dans le langage — ce sont des contrats de conception que le développeur s'engage à respecter.

### Garantie basique (*basic guarantee*)

Si une exception est levée, le programme reste dans un **état valide** : aucune ressource n'est fuie, aucun invariant de classe n'est brisé, aucune mémoire n'est corrompue. En revanche, l'état exact du programme après l'exception n'est pas spécifié — il peut avoir changé de manière observable.

C'est le niveau minimal acceptable pour tout code C++ professionnel. Le RAII (chapitre 6) et les smart pointers (chapitre 9) sont les outils qui rendent cette garantie naturelle à fournir.

```cpp
void transferer(Compte& source, Compte& destination, double montant) {
    source.debiter(montant);       // peut lever si solde insuffisant
    destination.crediter(montant); // peut lever si dépassement de plafond
    // Garantie basique : si crediter() lève, source a été débité
    // mais destination n'a pas été crédité → état valide mais incohérent
}
```

### Garantie forte (*strong guarantee*)

Si une exception est levée, l'état du programme est **identique à celui d'avant l'appel** — comme si la fonction n'avait jamais été invoquée. C'est la sémantique transactionnelle du « tout ou rien » (*commit-or-rollback*).

Cette garantie est plus coûteuse à fournir, car elle exige souvent de travailler sur une copie temporaire, puis de « committer » les modifications par une opération qui ne peut pas échouer.

```cpp
void transferer_fort(Compte& source, Compte& destination, double montant) {
    // Travailler sur des copies
    Compte source_copie = source;
    Compte destination_copie = destination;

    source_copie.debiter(montant);        // peut lever → les originaux sont intacts
    destination_copie.crediter(montant);   // peut lever → les originaux sont intacts

    // Commit : les deux swap sont noexcept → ne peuvent pas échouer
    using std::swap;
    swap(source, source_copie);            // noexcept
    swap(destination, destination_copie);  // noexcept
}
```

Le pattern visible ici — *copy, modify, swap* — est l'idiome classique pour fournir la garantie forte. Sa fiabilité repose entièrement sur le fait que `swap` soit `noexcept`. Si `swap` pouvait lever une exception entre les deux échanges, on se retrouverait dans un état incohérent pire qu'avec la garantie basique.

### Garantie no-throw (*nothrow guarantee*)

La fonction **ne lèvera jamais d'exception**, quelles que soient les circonstances. C'est la garantie la plus forte et la plus contraignante. Elle est indispensable pour certaines opérations critiques : les destructeurs, les opérations `swap`, les fonctions de nettoyage, et — c'est là le point clé — les constructeurs de déplacement et opérateurs d'affectation par déplacement que la STL souhaite exploiter.

C'est précisément cette garantie que `noexcept` permet d'exprimer dans le code.

---

## Pourquoi `noexcept` est critique pour la performance

### Le cas `std::vector` : déplacement vs copie

L'impact le plus concret de `noexcept` se manifeste dans les conteneurs de la STL, en particulier `std::vector`. Lorsqu'un `vector` doit réallouer son buffer interne (par exemple lors d'un `push_back` qui dépasse la capacité), il doit transférer tous les éléments existants vers le nouveau buffer.

Deux stratégies sont possibles :

- **Déplacer** les éléments (move) — rapide, car il s'agit typiquement d'un échange de pointeurs internes, sans allocation ni copie profonde.
- **Copier** les éléments (copy) — plus lent, mais sûr : si la copie du n-ième élément échoue, les éléments précédents restent intacts dans l'ancien buffer, et le `vector` peut annuler l'opération.

Le problème du déplacement est que si le move constructor du n-ième élément lève une exception, les éléments déjà déplacés ne sont plus dans l'ancien buffer — ils ont été « vidés » par le mouvement. Le `vector` ne peut ni terminer l'opération (l'exception l'en empêche), ni revenir en arrière (les sources ont été modifiées). L'état est irrémédiablement corrompu.

C'est pourquoi `std::vector` applique la règle suivante, implémentée via `std::move_if_noexcept` :

- Si le move constructor est `noexcept` → **déplacement** (rapide, garanti sans risque).
- Si le move constructor peut lever → **copie** (lente, mais garantie forte préservée).
- Si ni le move ni la copie ne sont disponibles → erreur de compilation.

```cpp
class Rapide {  
public:  
    Rapide(Rapide&&) noexcept = default;  // ✅ vector utilisera le move
    // ...
};

class Lent {  
public:  
    Lent(Lent&&);  // ⚠️ pas noexcept → vector copiera au lieu de déplacer
    // ...
};
```

La différence de performance peut être considérable. Pour un `vector` de 10 000 objets contenant chacun une `std::string` et un buffer interne, passer de la copie au déplacement lors d'une réallocation peut représenter un gain de plusieurs ordres de grandeur — la copie alloue et duplique tous les buffers, le déplacement ne fait qu'échanger des pointeurs.

### Au-delà de `std::vector`

Ce comportement n'est pas limité à `std::vector`. De nombreux algorithmes et conteneurs de la STL utilisent `std::move_if_noexcept` ou vérifient `std::is_nothrow_move_constructible` pour prendre des décisions à la compilation. Parmi eux : `std::deque`, `std::vector::resize`, `std::swap`, `std::variant` (pour l'affectation sans état *valueless_by_exception*), et les algorithmes comme `std::rotate` ou `std::inplace_merge`.

Marquer vos move constructors et move assignment operators `noexcept` n'est donc pas une optimisation marginale — c'est une condition nécessaire pour que votre type s'intègre efficacement dans tout l'écosystème STL.

---

## `noexcept` et le compilateur : ce qui change concrètement

Lorsqu'une fonction est marquée `noexcept`, le compilateur sait qu'aucun mécanisme de stack unwinding ne sera nécessaire pour cette fonction. Selon l'implémentation, cela peut se traduire par :

- L'élimination des tables d'exception (*unwind tables*) pour cette fonction, réduisant la taille du binaire.
- Des optimisations plus agressives à l'intérieur de la fonction, puisque le compilateur n'a pas à préserver l'état nécessaire au déroulement de la pile.
- Une meilleure inlining, la fonction étant considérée comme « plus simple » du point de vue du flux de contrôle.

En pratique, le gain sur une fonction isolée est souvent modeste. Le bénéfice majeur est indirect : c'est la décision move-vs-copy dans les conteneurs STL qui produit l'essentiel des gains mesurables.

---

## `noexcept` est un engagement irrévocable

Un point crucial mérite d'être souligné : si une fonction marquée `noexcept` lève malgré tout une exception, le programme appelle immédiatement `std::terminate()`. Il n'y a pas de stack unwinding, pas de chance de récupération, pas de `catch` qui puisse intercepter l'exception. Le programme s'arrête brutalement.

```cpp
void fonction_sure() noexcept {
    throw std::runtime_error("oups"); // → appel immédiat à std::terminate()
}
```

Ce comportement est intentionnel : `noexcept` est un **contrat binaire**. Soit la fonction ne lève jamais, soit le programme meurt. Il n'y a pas de demi-mesure. C'est pourquoi marquer une fonction `noexcept` ne doit pas être pris à la légère — c'est une garantie que vous vous engagez à maintenir pour toute la durée de vie de votre API.

Retirer un `noexcept` d'une fonction publique dans une version ultérieure est un **changement d'API cassant** : le code client qui dépendait de cette garantie (via `std::is_nothrow_move_constructible`, par exemple) pourrait silencieusement changer de comportement, passant du déplacement à la copie, avec des conséquences potentiellement graves sur les performances.

---

## Quelles fonctions marquer `noexcept` ?

La question « faut-il mettre `noexcept` partout ? » revient souvent, et la réponse est non. Voici les catégories de fonctions qui **doivent** ou **devraient** être `noexcept`, et celles pour lesquelles la question mérite réflexion.

### Doivent être `noexcept`

- **Destructeurs** — ils sont implicitement `noexcept` depuis C++11. Un destructeur qui lève pendant le stack unwinding provoque un appel à `std::terminate()`. Ne surchargez jamais ce comportement.
- **Move constructors et move assignment operators** — c'est la condition pour que la STL utilise le déplacement au lieu de la copie.
- **Fonctions `swap`** — elles sont la brique de base de la garantie forte (idiome copy-and-swap).
- **Fonctions de nettoyage et de libération de ressources** — `close()`, `release()`, `clear()` — une libération qui échoue est généralement irrécupérable.

### Devraient être `noexcept`

- **Accesseurs simples** (getters) — un accesseur qui retourne un membre par valeur ou par référence n'a aucune raison de lever.
- **Opérateurs de comparaison** — `operator==`, `operator<`, `operator<=>`.
- **Fonctions `hash`** — les hash functions personnalisées utilisées avec `std::unordered_map`.
- **Fonctions `empty()` et `size()`** — interroger l'état d'un conteneur ne devrait jamais échouer.

### Ne pas marquer `noexcept` sans réflexion

- **Fonctions qui allouent de la mémoire** — `new` peut lever `std::bad_alloc`.
- **Fonctions qui appellent du code tiers** dont vous ne contrôlez pas les garanties.
- **Fonctions susceptibles d'évoluer** — si une future version pourrait légitimement avoir besoin de lever, ne vous engagez pas prématurément.

---

## Plan de la section

Les deux sous-sections suivantes approfondissent les aspects pratiques :

**Section 17.4.1 — Spécifier `noexcept`**
La syntaxe détaillée de `noexcept` : forme simple, forme conditionnelle `noexcept(expression)`, l'opérateur `noexcept(...)` pour tester si une expression peut lever, et la combinaison avec les templates via `noexcept(noexcept(...))`. Exemples de propagation conditionnelle dans les wrappers génériques.

**Section 17.4.2 — Impact sur les performances**
Mesures concrètes de l'impact de `noexcept` : benchmarks de `std::vector` avec et sans move `noexcept`, analyse de la taille des binaires, visualisation des tables d'exception avec `readelf`. Démonstration de `std::move_if_noexcept` et diagnostic des situations où la STL recourt à la copie.

> 📎 *La sémantique de mouvement, prérequis essentiel pour comprendre l'impact de `noexcept` sur les conteneurs, est couverte en détail au **chapitre 10** (Move Semantics). La règle des cinq, qui spécifie quand définir les opérations de mouvement, est traitée en **section 6.5** (Rule of Five).*

⏭️ [Spécifier noexcept](/17-exceptions/04.1-specifier-noexcept.md)
