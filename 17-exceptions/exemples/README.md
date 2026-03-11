# Chapitre 17 — Exceptions et Gestion d'Erreurs : Exemples

## Compilation

Tous les exemples se compilent avec GCC 15 en mode C++23 :

```bash
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o <nom_binaire> <fichier>.cpp
```

### Compilation individuelle

```bash
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 01_syntaxe_try_catch 01_syntaxe_try_catch.cpp  
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 02_hierarchie_exceptions 02_hierarchie_exceptions.cpp  
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 03_exceptions_personnalisees 03_exceptions_personnalisees.cpp  
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 04_noexcept 04_noexcept.cpp  
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 04.1_specifier_noexcept 04.1_specifier_noexcept.cpp  
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 04.2_impact_performances 04.2_impact_performances.cpp  
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 05_alternatives_modernes 05_alternatives_modernes.cpp  
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 06_contrats_cpp26 06_contrats_cpp26.cpp  
```

### Compilation par lot

```bash
for f in *.cpp; do
    nom="${f%.cpp}"
    echo "Compilation de $f ..."
    g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o "$nom" "$f" || echo "ERREUR: $f"
done
```

### Nettoyage

```bash
rm -f 01_syntaxe_try_catch 02_hierarchie_exceptions 03_exceptions_personnalisees \
      04_noexcept 04.1_specifier_noexcept 04.2_impact_performances \
      05_alternatives_modernes 06_contrats_cpp26
```

---

## Liste des exemples

### 01_syntaxe_try_catch.cpp

**Section** : 17.1 — Syntaxe : try, catch, throw  
**Source** : 01-syntaxe-try-catch.md  
**Description** : Démonstration complète du mécanisme d'exceptions — throw, try/catch, stack unwinding avec destructeurs (struct Trace), ordre de capture (du plus spécifique au plus général), catch-all (`catch(...)`), RAII vs allocation brute, function-try-block.  

**Sortie attendue** :
```
=== 1. throw / try / catch basique ===
Erreur : Division par zéro

=== 2. Stack unwinding ===
  Construction de t1 (fonction_a)
  Construction de t2 (fonction_b)
  Construction de t3 (fonction_c)
→ throw dans fonction_c
  Destruction de t3 (fonction_c)
  Destruction de t2 (fonction_b)
Exception capturée dans fonction_a : Erreur dans fonction_c
  Destruction de t1 (fonction_a)

=== 3. Relance d'exception ===
Capturé : erreur originale

=== 4. Ordre des catch ===
Catch invalid_argument : argument invalide

=== 5. Catch-all ===
Exception inconnue capturée (catch-all)

=== 6. RAII vs allocation brute ===
RAII : exception capturée, pas de fuite mémoire

=== 7. Function-try-block ===
Échec initialisation Service : URL vide
Main : exception relancée par Service : URL vide

=== 8. Filet de sécurité main ===
Programme terminé normalement.
```

---

### 02_hierarchie_exceptions.cpp

**Section** : 17.2 — Hiérarchie des exceptions standard (`std::exception`)  
**Source** : 02-hierarchie-exceptions.md  
**Description** : Familles d'exceptions standard — `logic_error` (invalid_argument, domain_error, out_of_range, length_error, future_error), `runtime_error` (overflow_error, system_error, filesystem_error), exceptions autonomes (bad_alloc, bad_cast, bad_function_call, bad_optional_access, bad_variant_access), capture à différents niveaux de granularité.  

**Sortie attendue** (les messages exacts de `what()` peuvent varier selon le compilateur) :
```
=== 1. Famille logic_error ===
--- std::invalid_argument ---
  invalid_argument : Port hors limites [0, 65535] : 99999
  invalid_argument (stoi) : stoi
--- std::domain_error ---
  domain_error : logarithme() : argument non positif
--- std::out_of_range ---
  out_of_range : vector::_M_range_check: ...
--- std::length_error ---
  length_error : ...
--- std::future_error ---
  future_error : ... (code : ...)

=== 2. Famille runtime_error ===
--- std::overflow_error ---
  overflow_error : ...
--- std::system_error ---
  system_error : ... (code : 2, catégorie : generic)
--- std::filesystem::filesystem_error ---
  filesystem_error : ...

=== 3. Exceptions autonomes ===
--- std::bad_alloc ---
  bad_alloc : std::bad_alloc
--- std::bad_cast ---
  bad_cast : std::bad_cast
--- std::bad_function_call ---
  bad_function_call : bad_function_call
--- std::bad_optional_access ---
  bad_optional_access : bad optional access
--- std::bad_variant_access ---
  bad_variant_access : ...

=== 4. Capture à différents niveaux ===
  [logic] logic_error : arg invalide
  [runtime] runtime_error : erreur runtime
  [system] system_error : refusé: Permission denied
```

---

### 03_exceptions_personnalisees.cpp

**Section** : 17.3 — Exceptions personnalisées  
**Source** : 03-exceptions-personnalisees.md  
**Description** : Conception de classes d'exceptions métier — FichierIntrouvableError, ParseError avec localisation (ligne, colonne), NetworkError avec code et retry, hiérarchie AppError/DatabaseError/ConnectionError, ErreurCode (what() surchargé sans allocation), ServiceError avec std::source_location, nested exceptions avec std::throw_with_nested.  

**Sortie attendue** :
```
=== 1. FichierIntrouvableError ===
  Fichier introuvable : /etc/monapp/config.yaml (chemin: /etc/monapp/config.yaml)

=== 2. ParseError avec localisation ===
  config.yaml:12:5 — Clé dupliquée "port" (ligne:12, col:5)

=== 3. NetworkError ===
  Erreur réseau [1] vers api.example.com:443 — délai expiré (transitoire: oui)

=== 4. Hiérarchie AppError ===
  ConnectionError: Connexion DB échouée vers db.prod.local — connection refused (code: 1001, hote: db.prod.local)

=== 5. ErreurCode (what() surchargé, sans allocation) ===
  Erreur système (code 42) (code: 42)

=== 6. ServiceError avec source_location ===
  timeout réseau (fichier: ..., ligne: ...)

=== 7. Nested exceptions (throw_with_nested) ===
Causé par : Connexion DB échouée vers db.prod.local — Échec connexion POSIX
  Causé par : Connection refused [errno 111]: Connection refused

Programme terminé.
```

---

### 04_noexcept.cpp

**Section** : 17.4 — noexcept et garanties d'exception  
**Source** : 04-noexcept.md  
**Description** : Les trois niveaux de garantie d'exception (basique, forte, no-throw), idiome copy-modify-swap pour la garantie forte (struct Compte), impact de noexcept sur std::vector — move (Rapide, noexcept) vs copy (Lent, sans noexcept), classe Buffer avec move noexcept, static_assert sur les traits.  

**Sortie attendue** :
```
=== 1. Garantie forte (copy-modify-swap) ===
Avant : Alice=1000, Bob=500  
Après : Alice=800, Bob=700  
Erreur : Solde insuffisant → état inchangé : Alice=800, Bob=700  

=== 2. noexcept move → vector déplace ===
  push_back (réallocation) :
    move Rapide
    move Rapide

=== 3. move sans noexcept → vector copie ===
  push_back (réallocation) :
    move Lent
    copie Lent

=== 4. static_assert sur les traits noexcept ===
Tous les static_assert passés !

=== 5. Buffer dans un vector ===
vector<Buffer> : 2 éléments, tailles = 100 et 200

Programme terminé.
```

---

### 04.1_specifier_noexcept.cpp

**Section** : 17.4.1 — Spécifier noexcept  
**Source** : 04.1-specifier-noexcept.md  
**Description** : Syntaxe noexcept simple, forme conditionnelle noexcept(expr) avec templates, opérateur noexcept(...) pour tester les expressions, pattern noexcept(noexcept(...)) pour la propagation dans le code générique (Wrapper<T>), noexcept dans les lambdas, noexcept dans le système de types (C++17), vérification avec static_assert.  

**Sortie attendue** :
```
=== 1. Forme simple noexcept ===
  calculer_hash(42) = 42
  noexcept(calculer_hash(42)) = true

=== 2. Destructeurs implicitement noexcept ===
  is_nothrow_destructible<Widget> = true

=== 3. Fonctions = default : noexcept conditionnel ===
  Point nothrow-move-constructible : true
  Document nothrow-move-constructible : true

=== 4. noexcept conditionnel (template) ===
  echanger(int) noexcept : true (a=2, b=1)
  echanger(Bizarre) noexcept : false

=== 5. Opérateur noexcept(...) ===
  noexcept(42 + 1) = true
  noexcept(string::size()) = true
  noexcept(vector::push_back) = false
  noexcept(new int) = false

=== 6. Pattern noexcept(noexcept(...)) ===
  Wrapper<int> nothrow-move : true
  Wrapper<string> nothrow-move : true
  Wrapper<Bizarre> nothrow-move : false

=== 7. noexcept dans les lambdas ===
  diviser_lambda(10, 3) = 3.3333333333333335
  noexcept(diviser_lambda) = true
  compteur() = 3, 2, 1

=== 8. noexcept dans le système de types ===
  f_noexcept et g_normal ont des types différents : true
  Conversion noexcept → normal : OK

Tous les static_assert passés !
```

---

### 04.2_impact_performances.cpp

**Section** : 17.4.2 — Impact sur les performances  
**Source** : 04.2-impact-performances.md  
**Description** : std::move_if_noexcept en action — Deplacable (noexcept move → déplacement) vs NonSur (move sans noexcept → copie), MoveOnly sans noexcept (move forcé car pas de copie), Ressource dans un vector, impact sur std::variant (valueless_by_exception), vérification des traits noexcept.  

**Sortie attendue** :
```
=== 1. std::move_if_noexcept — Deplacable (noexcept move) ===
move_if_noexcept :
  → déplacement

=== 2. std::move_if_noexcept — NonSur (move sans noexcept) ===
move_if_noexcept :
  → copie

=== 3. MoveOnly sans noexcept — move forcé ===
  → déplacement (move-only)
  vector<MoveOnly> accepte le move-only

=== 4. Ressource dans un vector ===
  vector<Ressource> : 3 éléments (tailles : 1024, 2048, 4096)

=== 5. Impact sur std::variant (valueless_by_exception) ===
  Avant : variant contient int = 42
  valueless_by_exception = false
  Exception : construction échouée
  valueless_by_exception = true

=== 6. Vérification des traits noexcept ===
  Deplacable nothrow-move : true
  NonSur nothrow-move : false
  MoveOnly nothrow-move : false
  Ressource nothrow-move : true

Tous les static_assert passés !
```

---

### 05_alternatives_modernes.cpp

**Section** : 17.5 — Alternatives modernes : std::expected (C++23), codes d'erreur  
**Source** : 05-alternatives-modernes.md  
**Description** : std::expected construction et inspection, value_or, interface monadique (and_then, transform, or_else) avec pipeline complet, std::expected<void, E>, std::optional (absence sans explication), pont bidirectionnel exception ↔ expected (capturer_exception).  

**Sortie attendue** :
```
=== 1. std::expected : construction et inspection ===
  Erreur : fichier introuvable
  Succès : 28 octets lus

=== 2. value_or ===
  Config (avec fallback) : port: 8080

=== 3. Interface monadique : and_then, transform ===
  Pipeline OK : Serveur sur port 8080 (log: debug)

=== 4. or_else : récupération d'erreur ===
  → Fallback : config par défaut
  Config récupérée : port=3000, log=warn

=== 5. Pipeline avec erreur en milieu de chaîne ===
  Erreur détectée : port hors limites

=== 6. std::expected<void, E> ===
  Opération réussie : true
  Opération échouée : true

=== 7. std::optional : absence sans explication ===
  HOME = /home/...
  VARIABLE_INEXISTANTE_XYZ : non définie (nullopt)

=== 8. Pont exception → expected ===
  stoi("42") → 42
  stoi("pas_un_nombre") → erreur : stoi

Programme terminé.
```

---

### 06_contrats_cpp26.cpp

**Section** : 17.6 — Contrats (C++26) : Préconditions et postconditions  
**Source** : 06-contrats-cpp26.md  
**Description** : Simulation des contrats C++26 (pre, post, contract_assert) avec des macros, car GCC 15 et Clang 20 ne supportent pas encore les contrats (P2900). Démonstration : préconditions/postconditions simples et multiples, distinction contrat vs exception, combinaison contrat + std::expected, constexpr avec vérification.  
**Note** : La vraie syntaxe C++26 est montrée en commentaire dans le code source.  

**Sortie attendue** :
```
=== 1. Précondition et postcondition (racine_carree) ===
  racine_carree(25.0) = 5
  racine_carree(2.0) = 1.4142135623730951

=== 2. Préconditions multiples (recherche_binaire) ===
  recherche_binaire(30) = 2
  recherche_binaire(35) = -1
  recherche_binaire(10) = 0

=== 3. Contrat vs Exception — deux rôles distincts ===
  diviser_contrat(10, 3) = 3
  charger_fichier → contenu de config.yaml
  charger_fichier("") → erreur : chemin vide

=== 4. Combinaison contrat + expected ===
  trouver_element(3) → index 2
  trouver_element(99) → élément non trouvé

=== 5. constexpr avec vérification (simule contrat compile-time) ===
  factorielle(5) = 120 (compile-time)
  factorielle(10) = 3628800 (compile-time)

=== 6. Tableau récapitulatif des mécanismes ===
  Contrats (pre/post)    → Erreurs de programmation (bugs)
  Exceptions (throw)     → Erreurs d'exécution rares
  std::expected          → Erreurs d'exécution fréquentes
  noexcept               → Garantie d'absence d'exception
  assert / static_assert → Vérifications héritées (C)

=== Note sur le support compilateur ===
  Les contrats C++26 (P2900) ne sont pas encore supportés
  par GCC 15 ni Clang 20 (mars 2026).
  Ce fichier utilise des macros pour simuler le comportement.
  Des forks expérimentaux sont disponibles sur Compiler Explorer.

Programme terminé.
```
