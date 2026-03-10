# Chapitre 7 — Héritage et Polymorphisme : Exemples compilables

Ce dossier contient **29 fichiers** d'exemples compilables illustrant les concepts du chapitre 7.

## Compilation

Tous les exemples se compilent avec GCC 15 et le standard C++23 :

```bash
g++ -std=c++23 -Wall -Wextra -Wpedantic -g -o programme source.cpp
```

Pour le micro-benchmark (section 7.5), compiler avec optimisations :

```bash
g++ -std=c++23 -O2 -o programme 05_micro_benchmark.cpp
```

---

## Section 7.1.1 — Héritage simple

| Fichier | Description | Sortie attendue |
|---|---|---|
| `01_1_animal_chat.cpp` | Syntaxe de base : héritage public Animal/Chat, constructeurs, membres protégés, conversion implicite dérivée→base (upcast), sizeof | `Je suis Félix, 3 an(s)` / `Félix ronronne...` / `Je suis Félix, 3 an(s)` + tailles sizeof |
| `01_1_construction_destruction.cpp` | Ordre déterministe de construction (base→dérivée) et destruction (dérivée→base) | `Base::Base()` → `Derivee::Derivee()` → `Derivee::~Derivee()` → `Base::~Base()` |
| `01_1_name_hiding.cpp` | Name hiding (résolution statique), masquage de surcharges et résolution avec `using` | Le type statique détermine la méthode appelée. Sans `using`, `double` est converti en `int`. Avec `using`, la bonne surcharge est choisie |
| `01_1_slicing.cpp` | Problème du slicing : copie par valeur perd la partie dérivée. Comparaison avec référence/pointeur | `Rex aboie (Berger Allemand)` en direct, `Rex fait du bruit` après slicing |
| `01_1_operateur_portee.cpp` | Appel explicite à la méthode de base via `::` pour étendre le comportement | `Ouverture fichier standard` → `Initialisation du déchiffrement` |

**Fichier source :** `01.1-heritage-simple.md`

---

## Section 7.1.2 — Héritage multiple et problème du diamant

| Fichier | Description | Sortie attendue |
|---|---|---|
| `01_2_heritage_multiple.cpp` | Syntaxe de l'héritage multiple (Document), disposition mémoire (Voiture) avec vérification des adresses des sous-objets | Adresse Moteur == Voiture, Chassis décalée |
| `01_2_construction_order.cpp` | Ordre de construction suit l'ordre de déclaration des bases (pas la liste d'initialisation). Warning `-Wreorder` attendu | `B::B()` → `A::A()` → `C::C()` → `C::~C()` → `A::~A()` → `B::~B()` |
| `01_2_ambiguite_resolution.cpp` | Ambiguïté de noms et trois méthodes de résolution : qualification explicite, redéfinition, `using` | Chaque méthode est correctement dispatchée |
| `01_2_diamant.cpp` | Problème du diamant : double construction de Peripherique, ambiguïté d'accès, StationMeteo avec données ambiguës | `Peripherique::Peripherique(42)` affiché **deux fois** |
| `01_2_mixin.cpp` | Mix-ins indépendants (NonCopiable + Loggable) — usage idiomatique sans diamant | `[LOG] Connexion créée vers example.com` |

**Fichier source :** `01.2-heritage-multiple.md`

---

## Section 7.1.3 — Héritage virtuel

| Fichier | Description | Sortie attendue |
|---|---|---|
| `01_3_heritage_virtuel.cpp` | Résolution du diamant : un seul sous-objet Peripherique partagé, un seul appel constructeur, accès non ambigu | `Peripherique::Peripherique(42)` affiché **une seule fois**, `ID = 42` |
| `01_3_construction_order.cpp` | Ordre de construction : bases virtuelles en premier (parcours profondeur gauche-droite), puis bases non virtuelles | `V1` → `V2` → `A` → `B` → `C` |
| `01_3_destruction.cpp` | Construction et destruction complètes : destructeur de la base virtuelle appelé une seule fois, en dernier | Construction: `Peripherique()` → `PeripheriqueEntree()` → `PeripheriqueSortie()` → `PeripheriqueES()`. Destruction dans l'ordre inverse |

**Fichier source :** `01.3-heritage-virtuel.md`

---

## Section 7.2 — Fonctions virtuelles et mécanisme de vtable

| Fichier | Description | Sortie attendue |
|---|---|---|
| `02_resolution_statique.cpp` | Sans `virtual` : résolution statique, `afficher(Forme&)` appelle toujours `Forme::dessiner()` | `Forme::dessiner()` × 2 |
| `02_resolution_dynamique.cpp` | Avec `virtual` : dispatch dynamique + démonstration des 4 conditions (référence, pointeur, slicing, qualifié) | Cercle/Rectangle via ref/ptr, Forme via val/qualifié |
| `02_sizeof_vptr.cpp` | Impact du vptr sur `sizeof` : 4 octets sans virtual, 16 octets avec | `sizeof(SansVirtual) = 4`, `sizeof(AvecVirtual) = 16` |
| `02_vptr_constructeur.cpp` | Initialisation progressive du vptr : appel virtuel dans le constructeur de Base appelle `Base::qui_suis_je()`, pas `Derivee::` | `Je suis Base` pendant construction de Base, `Je suis Derivee` pendant construction de Derivee |
| `02_destructeur_virtuel.cpp` | Sans destructeur virtuel : ~Cercle() non appelé (UB). Avec : chaîne complète. Démonstration avec unique_ptr | Sans virtual: seul `~Forme()`. Avec: `~Cercle()` puis `~Forme()` |
| `02_rtti.cpp` | `typeid` retourne le type dynamique réel via le RTTI stocké dans la vtable | Noms manglés GCC : `6Cercle`, `9Rectangle` |

**Fichier source :** `02-fonctions-virtuelles-vtable.md`

---

## Section 7.3 — Polymorphisme dynamique : virtual, override, final

| Fichier | Description | Sortie attendue |
|---|---|---|
| `03_pipeline_tache.cpp` | Exemple complet : pipeline de tâches polymorphiques avec `virtual`, `override`, `final`, `unique_ptr`, factory, méthode avec implémentation par défaut | 3 tâches exécutées avec description et dispatch dynamique correct |

**Fichier source :** `03-polymorphisme-dynamique.md`

---

## Section 7.4 — Classes abstraites et interfaces pures

| Fichier | Description | Sortie attendue |
|---|---|---|
| `04_classe_abstraite.cpp` | Classe abstraite Forme avec fonctions virtuelles pures, classes concrètes Cercle et Carre | Dessin + aire + périmètre pour chaque forme |
| `04_pure_virtual_impl.cpp` | Fonction virtuelle pure avec implémentation : `Connexion::fermer()` appelable via `Base::methode()` | `Fermeture du socket TCP` → `Nettoyage générique de la connexion` |
| `04_template_method.cpp` | Pattern Template Method : ILogger → AbstractLogger → ConsoleLogger/FileLogger. Factory function et collection polymorphique | Logs console et fichier, filtrage par niveau |
| `04_interfaces_multiples.cpp` | Implémentation de plusieurs interfaces (ISerializable + IPrintable). Principe de ségrégation des interfaces (ISP) | JSON sérialisé + impression texte du rapport |

**Fichier source :** `04-classes-abstraites.md`

---

## Section 7.5 — Coût du polymorphisme dynamique

| Fichier | Description | Sortie attendue |
|---|---|---|
| `05_sizeof_vptr_impact.cpp` | Surcoût mémoire du vptr : Point3D (12 octets) vs Point3DVirtual (24 octets) — ratio 2× | `sizeof = 12` vs `sizeof = 24` |
| `05_micro_benchmark.cpp` | Micro-benchmark : appel direct vs appel virtuel sur 10M éléments. Compiler avec `-O2` pour observer la dévirtualisation | Avec `-g` : ratio ~2×. Avec `-O2` : ratio ~1× (dévirtualisé car `final`) |
| `05_crtp.cpp` | CRTP (Curiously Recurring Template Pattern) : polymorphisme statique sans vtable, résolu à la compilation | Formes dessinées avec aire calculée, sans surcoût virtuel |
| `05_variant_visit.cpp` | `std::variant` + `std::visit` : polymorphisme avec ensemble fermé de types, stockage par valeur, dispatch à la compilation | Cercle et Rectangle avec dessins et aires |
| `05_concepts.cpp` | Concepts C++20 : contrat d'interface vérifié à la compilation, sans vtable | Cercle et Triangle avec dessins et aires |

**Fichier source :** `05-cout-polymorphisme.md`
