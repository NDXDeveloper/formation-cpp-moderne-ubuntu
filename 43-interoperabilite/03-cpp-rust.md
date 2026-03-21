🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 43.3 — C++ et Rust : FFI et interopérabilité

## Module 15 : Interopérabilité · Niveau Expert

---

## Introduction

L'interopérabilité C++ ↔ Rust n'est pas un sujet académique. C'est un enjeu industriel majeur en 2026, à l'intersection de deux forces : d'une part, des dizaines de milliards de lignes de C++ en production dans les infrastructures critiques mondiales ; d'autre part, une pression croissante — technique, réglementaire et institutionnelle — pour réduire les vulnérabilités liées à la gestion manuelle de la mémoire.

Rust n'a pas été conçu pour remplacer le C++. Il a été conçu pour offrir des garanties de sécurité mémoire **à la compilation**, sans garbage collector, avec des performances comparables. Ces deux langages occupent la même niche — la programmation système haute performance — mais avec des philosophies fondamentalement différentes sur la manière de garantir la correction des programmes.

La question qui se pose aux équipes en 2026 n'est plus "Rust ou C++ ?", mais **"Comment les faire cohabiter ?"**

---

## Le contexte de 2026

### La pression réglementaire

Le paysage réglementaire a considérablement évolué entre 2023 et 2026. Plusieurs événements ont rendu l'interopérabilité C++/Rust incontournable pour les organisations opérant dans des domaines réglementés :

**Les directives américaines.** La NSA, la CISA et la Maison Blanche ont publié entre 2022 et 2024 plusieurs rapports recommandant explicitement l'adoption de langages memory-safe pour les nouveaux développements. Le rapport technique de la NSA (*Software Memory Safety*, novembre 2022) cite nommément Rust comme alternative recommandée au C et au C++. Le rapport de la Maison Blanche (*Back to the Building Blocks*, février 2024) va plus loin en recommandant des feuilles de route de transition mesurables.

**Le Cyber Resilience Act européen.** Entré en vigueur dans l'Union européenne, il impose aux fabricants de produits contenant des éléments numériques de démontrer des pratiques de développement sécurisé. Si le texte ne mentionne pas de langage spécifique, les guidelines d'implémentation citent la sécurité mémoire comme critère évaluable — ce qui crée une incitation structurelle à documenter et réduire l'exposition aux vulnérabilités mémoire.

**Les réponses de l'industrie.** Google, Microsoft, Meta, Amazon et d'autres ont publié des données internes montrant que 60 à 70 % de leurs vulnérabilités de sécurité critiques sont liées à des erreurs mémoire (use-after-free, buffer overflow, double free). Ces chiffres, répétés année après année, ont transformé le débat technique en décision stratégique.

> 📎 *La section 45.6 (Sécurité mémoire : réponses concrètes du comité C++ en 2026) traite en détail du contexte réglementaire, des Safety Profiles C++ et de la question "C++ safe-by-default est-il atteignable ?".*

### La réalité du terrain

Face à cette pression, la réponse pragmatique de l'industrie n'est pas une réécriture massive. Les bases de code C++ existantes représentent des décennies d'investissement, de tests, de corrections de bugs et de connaissances métier intégrées. Les réécrire intégralement en Rust serait coûteux, risqué et souvent irréaliste.

La stratégie qui émerge est la **cohabitation progressive** :

- Les **nouveaux composants** — en particulier ceux exposés au réseau, manipulant des données non fiables, ou traitant des entrées utilisateur — sont développés en Rust.  
- Les **composants existants** en C++ sont conservés, durcis (sanitizers, analyse statique, fuzzing), et interfacés avec les nouveaux modules Rust via FFI.  
- Les **frontières** entre les deux langages sont soigneusement définies, testées et documentées.

Cette stratégie repose entièrement sur la capacité à faire communiquer C++ et Rust de manière fiable, performante et maintenable. C'est le sujet de cette section.

---

## Le défi technique : deux mondes, une frontière

### Ce que C++ et Rust ont en commun

Les deux langages partagent des caractéristiques fondamentales qui rendent leur interopérabilité possible :

- **Compilation native.** Les deux produisent du code machine natif, sans runtime ni garbage collector.  
- **Contrôle mémoire explicite.** Les deux permettent l'allocation sur la pile et sur le tas, avec un contrôle fin de la durée de vie des objets.  
- **Compatibilité avec l'ABI C.** Les deux peuvent exposer et consommer des fonctions avec le linkage C — le dénominateur commun décrit en section 43.1.  
- **Backend LLVM partagé.** GCC mis à part, Clang (C++) et rustc (Rust) utilisent tous deux LLVM comme backend de génération de code, ce qui garantit des conventions d'appel et des représentations mémoire compatibles pour les types C.

### Ce qui les sépare

Malgré ces points communs, les différences de design créent des frictions significatives à la frontière :

**Le système de propriété de Rust.** Le *borrow checker* de Rust impose qu'à tout instant, un objet possède soit une référence mutable unique, soit un nombre quelconque de références immuables — mais jamais les deux. Ce modèle n'a aucun équivalent en C++. À la frontière FFI, les garanties du borrow checker s'arrêtent : le code Rust qui manipule des pointeurs bruts provenant de C++ opère en mode `unsafe`, et c'est au développeur de garantir manuellement l'absence de data races et de violations mémoire.

**Le name mangling.** Rust possède son propre schéma de mangling, incompatible avec celui du C++ (Itanium ABI). Les deux langages ne peuvent pas résoudre directement les symboles de l'autre. L'ABI C — via `extern "C"` côté C++ et `extern "C"` côté Rust — est le passage obligé.

**Les types.** Rust n'a pas de `std::string`, pas de `std::vector`, pas de classes avec vtable. C++ n'a pas de `String`, pas de `Vec<T>`, pas de `Result<T, E>`, pas de `Option<T>` au sens Rust. À la frontière, seuls les types C (`i32`, `f64`, `*const u8`, `*mut T`) passent directement. Tout le reste nécessite une conversion ou un wrapper.

**Les exceptions.** C++ utilise des exceptions. Rust utilise `Result<T, E>` et `panic!`. Un panic Rust qui traverse une frontière FFI est un comportement indéfini. Une exception C++ qui traverse la frontière vers Rust est également un comportement indéfini. Chaque côté doit intercepter ses propres erreurs avant la frontière.

**La gestion de la mémoire.** C++ et Rust utilisent des allocateurs potentiellement différents. Un objet alloué avec `new` en C++ ne peut pas être libéré avec `drop` en Rust (et inversement). La frontière FFI doit définir clairement quel côté alloue et quel côté libère chaque ressource.

---

## Le spectre des approches

L'interopérabilité C++/Rust peut être réalisée à différents niveaux d'abstraction, chacun avec ses compromis :

### Niveau 1 : FFI manuelle via `extern "C"`

Le mécanisme le plus bas niveau. Chaque côté expose une API C pure (`extern "C"`) et l'autre côté la consomme. C'est exactement le pattern décrit en section 43.1, appliqué à Rust au lieu de Python.

**Avantages :** contrôle total, aucune dépendance tierce, compréhension fine de ce qui se passe.

**Inconvénients :** verbeux, sujet aux erreurs (types incorrects, ownership ambigu), maintenance lourde quand l'API évolue.

### Niveau 2 : `cxx` — le bridge type-safe

Le crate `cxx` (développé par David Tolnay) définit un langage de description d'interface dans un bloc `#[cxx::bridge]` en Rust. À partir de cette description, `cxx` génère automatiquement les wrappers C++ et Rust nécessaires, avec des vérifications de type à la compilation des deux côtés.

**Avantages :** sécurité au compile-time, types riches (String, Vec, etc.) traversant la frontière sans code manuel, détection des erreurs d'interface avant l'exécution.

**Inconvénients :** le langage de description est un sous-ensemble limité — toutes les API C++ ne sont pas exprimables. Les templates, la surcharge, l'héritage virtuel ne sont pas supportés directement.

### Niveau 3 : `autocxx` — bindings automatiques

`autocxx` (développé par Adrian Taylor, Google) parse directement les headers C++ et génère automatiquement des bindings Rust. L'objectif est de réduire le travail manuel au minimum : pointer vers un header C++, et le code Rust peut appeler les fonctions et méthodes C++ presque directement.

**Avantages :** productivité maximale pour les API C++ existantes de grande taille.

**Inconvénients :** la génération automatique ne couvre pas tous les cas (templates complexes, macros, code conditionnel), les messages d'erreur peuvent être obscurs, et le contrôle fin est plus limité qu'avec `cxx`.

### Comparaison synthétique

| Critère | FFI manuelle | cxx | autocxx |
|---|---|---|---|
| Contrôle | Total | Élevé | Modéré |
| Sécurité compile-time | Faible | Élevée | Moyenne |
| Verbosité | Haute | Moyenne | Faible |
| Couverture API C++ | Totale (via C) | Sous-ensemble | Large (avec limites) |
| Maintenance | Lourde | Modérée | Légère |
| Dépendance tierce | Aucune | crate cxx | crate autocxx + bindgen |
| Courbe d'apprentissage | Faible | Moyenne | Moyenne |
| Adapté pour | Petites interfaces | Interfaces bien définies | Wrapping de grosses API existantes |

---

## Prérequis Rust pour le développeur C++

Cette section ne prétend pas enseigner Rust — ce serait un livre entier. Mais pour comprendre les exemples de code et les décisions de design, le développeur C++ doit connaître quelques concepts clés.

### Correspondance des concepts fondamentaux

| Concept C++ | Équivalent Rust | Différence clé |
|---|---|---|
| `std::unique_ptr<T>` | `Box<T>` | Ownership exclusif — sémantique similaire |
| `std::shared_ptr<T>` | `Arc<T>` (thread-safe) / `Rc<T>` | Arc = atomic ref count, Rc = single-thread |
| `const T&` | `&T` | Borrow immuable — vérifié au compile-time |
| `T&` | `&mut T` | Borrow mutable — exclusif au compile-time |
| `std::string` | `String` | UTF-8 garanti, pas null-terminated |
| `std::vector<T>` | `Vec<T>` | Sémantique très proche |
| `std::optional<T>` | `Option<T>` | Enum à deux variantes : `Some(T)` / `None` |
| `std::expected<T,E>` (C++23) | `Result<T, E>` | Enum : `Ok(T)` / `Err(E)` — idiomatique en Rust |
| Exception (`throw`/`catch`) | `Result<T, E>` / `panic!` | Pas d'exceptions — `?` pour la propagation |
| `extern "C"` | `extern "C"` | Même syntaxe, même effet (désactive le mangling) |
| `new` / `delete` | `Box::new` / drop automatique | Pas de `delete` explicite en Rust |
| `nullptr` | `Option<T>` / pointeur brut `*const T` | Les références Rust ne sont jamais null |

### Le bloc `unsafe` en Rust

Tout code Rust qui interagit avec du C++ via FFI est nécessairement dans un bloc `unsafe`. Le mot-clé `unsafe` ne signifie pas "dangereux" — il signifie "le compilateur ne peut pas vérifier ces garanties, le développeur en prend la responsabilité". C'est l'équivalent conceptuel de la frontière `extern "C"` côté C++ : un point où les garanties automatiques s'arrêtent et où la rigueur manuelle prend le relais.

```rust
// Appel d'une fonction C++ via FFI — nécessite unsafe
extern "C" {
    fn engine_create(config: *const c_char) -> *mut Engine;
    fn engine_destroy(e: *mut Engine);
}

fn main() {
    let config = CString::new("default").unwrap();
    unsafe {
        let engine = engine_create(config.as_ptr());
        // ... utilisation ...
        engine_destroy(engine);
    }
}
```

L'objectif des outils comme `cxx` est précisément de **réduire la surface `unsafe`** en encapsulant la FFI dans des abstractions vérifiées au compile-time. Le code applicatif Rust reste safe ; seuls les wrappers générés contiennent le `unsafe` nécessaire.

---

## Ce que couvre cette section

Les cinq sous-sections suivantes couvrent le spectre complet de l'interopérabilité C++/Rust, du mécanisme le plus bas niveau à la stratégie organisationnelle :

**43.3.1 — `extern "C"` et liaison manuelle.** Le mécanisme FFI brut. Écrire manuellement les déclarations `extern "C"` des deux côtés, passer des types C, gérer l'allocation et la désallocation, compiler et linker. C'est la base sur laquelle tout le reste s'appuie — et le fallback quand les outils de plus haut niveau ne suffisent pas.

**43.3.2 — `cxx` : le bridge Rust ↔ C++ de référence.** L'outil qui a transformé l'interopérabilité C++/Rust en pratique industrielle. Syntaxe du bridge, types supportés, intégration CMake/Cargo, et les limites de l'approche.

**43.3.3 — `autocxx` : bindings automatiques C++ → Rust.** Quand on veut consommer une API C++ existante depuis Rust sans réécrire chaque déclaration. Parsing automatique des headers, génération de bindings, complémentarité avec `cxx`.

**43.3.4 — Stratégie de migration progressive.** Le sujet le plus important pour les décideurs techniques : quand introduire Rust, par où commencer, comment structurer la frontière, comment gérer le build system mixte (CMake + Cargo), et comment former les équipes.

**43.3.5 — Cas d'usage et retours d'expérience industrie 2025-2026.** Ce que Google, Microsoft, Meta, l'écosystème Linux et d'autres ont publié sur leur expérience concrète de cohabitation C++/Rust. Succès, échecs, leçons apprises.

---

## Prérequis techniques

- La section 43.1 (`extern "C"`, ABI C, handle opaque) — les mécanismes sont identiques côté C++.  
- CMake : targets, `target_link_libraries`, bibliothèques statiques/dynamiques (chapitre 26, section 27.4).  
- Une installation de la toolchain Rust (`rustup`, `cargo`, `rustc`) est nécessaire pour compiler les exemples. L'installation se fait en une commande :

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh  
source $HOME/.cargo/env  
rustc --version  # 1.85+ recommandé  
```

- La familiarité avec les concepts Rust (ownership, borrowing, lifetimes) est utile mais pas strictement nécessaire — chaque sous-section introduit le contexte minimal requis.

> 🔥 **Pourquoi cette section est marquée comme critique** : en 2026, la capacité à faire cohabiter C++ et Rust dans une même architecture est un **différenciateur professionnel**. Les organisations qui recrutent des développeurs système attendent de plus en plus cette compétence, et les projets open source majeurs (Linux kernel, Android, Chromium, Firefox, Windows) ont tous intégré Rust dans leur base de code C/C++ existante. Comprendre les mécanismes, les outils et les stratégies de cette cohabitation est un atout concret sur le marché.

⏭️ [extern "C" et liaison manuelle](/43-interoperabilite/03.1-extern-c-liaison.md)
