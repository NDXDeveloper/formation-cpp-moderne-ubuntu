# Exemples — Chapitre 24 : Sérialisation et Configuration

## Fichiers d'exemples

### JSON (nlohmann/json)

| Fichier | Section | Description | Fichier source | Compilation |
|---------|---------|-------------|----------------|-------------|
| `01_json_apercu.cpp` | 24.1 | Aperçu de l'API — création, accès, itération, conversion conteneurs, `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE`, parsing et dump | `01-json-nlohmann.md` | `g++-15 -std=c++23 -O2 -o 01_json_apercu 01_json_apercu.cpp` |
| `01_1_verify_json.cpp` | 24.1.1 | Vérification d'installation — création JSON, sérialisation `dump(2)`, parsing `json::parse` | `01.1-installation.md` | `g++-15 -std=c++23 -O2 -o 01_1_verify_json 01_1_verify_json.cpp` |
| `01_2_parsing.cpp` | 24.1.2 | Parsing complet — fichier, chaîne, DOM, `.at()`, `.value()`, extraction typée, itération objet/tableau/flatten, JSON Pointer, parsing strict/tolérant, callback, SAX, Unicode, JSONC | `01.2-parsing.md` | `g++-15 -std=c++23 -O2 -o 01_2_parsing 01_2_parsing.cpp` |
| `01_3_serialisation.cpp` | 24.1.3 | Sérialisation d'objets — `to_json`/`from_json`, macros `NLOHMANN_DEFINE_TYPE_*`, renommage de clés, champs optionnels, enums, types imbriqués, héritage, round-trip | `01.3-serialisation.md` | `g++-15 -std=c++23 -O2 -o 01_3_serialisation 01_3_serialisation.cpp` |
| `01_4_erreurs.cpp` | 24.1.4 | Gestion des erreurs — hiérarchie des exceptions (`parse_error`, `type_error`, `out_of_range`), capture granulaire/globale, parsing sans exception, `std::expected`, messages enrichis (`byte_to_position`, `show_error_context`), validation structurelle (`ValidationResult`), chargement robuste complet | `01.4-gestion-erreurs.md` | `g++-15 -std=c++23 -O2 -o 01_4_erreurs 01_4_erreurs.cpp` |

### YAML (yaml-cpp)

| Fichier | Section | Description | Fichier source | Compilation |
|---------|---------|-------------|----------------|-------------|
| `02_yaml_apercu.cpp` | 24.2 | Aperçu yaml-cpp — chargement fichier, accès scalaires, valeur par défaut, itération séquence, `YAML::convert<T>` (Endpoint), gestion des erreurs (`ParserException`, `BadConversion`) | `02-yaml-cpp.md` | `g++-15 -std=c++23 -O2 -o 02_yaml_apercu 02_yaml_apercu.cpp -lyaml-cpp` |
| `02_1_lecture_yaml.cpp` | 24.2.1 | Lecture de configuration — chargement fichier/chaîne, multi-documents, navigation DOM, accès par index, test de type, extraction typée/conteneurs, `YAML::convert<T>` complet (TlsConfig, ServerConfig), itération map/séquence/récursive, ancres et alias, validation structurelle, détection clés inconnues | `02.1-lecture-config.md` | `g++-15 -std=c++23 -O2 -o 02_1_lecture_yaml 02_1_lecture_yaml.cpp -lyaml-cpp` |
| `02_2_ecriture_yaml.cpp` | 24.2.2 | Écriture YAML — construction via `YAML::Node`, `YAML::Dump`, écriture fichier, `YAML::Emitter` (base, séquences), styles flow/bloc, indentation, quotation (double, single, literal, globale), multi-documents, helper RAII (`YamlWriter`), combinaison Node+Emitter | `02.2-ecriture-yaml.md` | `g++-15 -std=c++23 -O2 -o 02_2_ecriture_yaml 02_2_ecriture_yaml.cpp -lyaml-cpp` |

### TOML (toml++)

| Fichier | Section | Description | Fichier source | Compilation |
|---------|---------|-------------|----------------|-------------|
| `03_toml.cpp` | 24.3 | TOML complet — parsing fichier/chaîne/flux, `value<T>()`/`value_or()`, `at_path()`, itération tables/tableaux, tableaux de tables (`[[section]]`), conversion types utilisateur, helper `required<T>`, dates et heures natives, écriture TOML, formateurs (`toml_formatter`, `json_formatter`), validation | `03-toml.md` | `g++-15 -std=c++23 -O2 -o 03_toml 03_toml.cpp` |

### XML (pugixml)

| Fichier | Section | Description | Fichier source | Compilation |
|---------|---------|-------------|----------------|-------------|
| `04_xml_pugixml.cpp` | 24.4 | XML complet — parsing fichier/chaîne, navigation DOM (enfants, attributs, texte), itération enfants/attributs, XPath (sélection, count, expressions), conversion types métier, écriture XML (construction DOM, sérialisation), émission depuis types métier, modification document, namespaces, gestion des erreurs | `04-xml-pugixml.md` | `g++-15 -std=c++23 -O2 -o 04_xml_pugixml 04_xml_pugixml.cpp -lpugixml` |

### Validation

| Fichier | Section | Description | Fichier source | Compilation |
|---------|---------|-------------|----------------|-------------|
| `05_validation.cpp` | 24.5 | Validation de schémas — `ConfigValidator` générique (require, in_range, not_empty, one_of, check, warn), application à JSON/YAML/TOML, validation sémantique (cohérence TLS, format hostname), clés inconnues (strict/tolérant), pipeline complet de chargement | `05-validation-schemas.md` | `g++-15 -std=c++23 -O2 -o 05_validation 05_validation.cpp -lyaml-cpp -lpugixml` |

## Fichiers de données

| Fichier | Description |
|---------|-------------|
| `config.json` | Configuration JSON de test (server, database) |
| `config.yaml` | Configuration YAML de test (server, database, logging) |
| `config.toml` | Configuration TOML de test (server, database, logging, deployment avec dates) |
| `config.xml` | Configuration XML de test (server avec attributs, database, endpoints, logging) |

## Comportement attendu

Chaque programme affiche les résultats de ses tests et termine par `Tous les tests passent !`.

### `01_json_apercu` — Sortie attendue (extrait)
```
=== Création directe ===
host=0.0.0.0, port=8080
=== Conversion ServerConfig ===
Sérialisé : {"host":"localhost","port":443,"tls":true}
```

### `01_1_verify_json` — Sortie attendue
```
JSON généré :
{
  "header_only": true,
  "library": "nlohmann/json",
  "version": 3
}
Status : ok  
Code   : 200  
```

### `01_2_parsing` — Sortie attendue (extrait)
```
=== Parsing fichier ===
Host : 0.0.0.0  
Port : 8080  
=== JSON Pointer ===
method=POST, same=POST
```

### `01_3_serialisation` — Sortie attendue (extrait)
```
=== Endpoint to_json/from_json ===
GET /health (1000ms)
=== Round-trip ===
Round-trip OK
```

### `01_4_erreurs` — Sortie attendue (extrait)
```
=== parse_error ===
  ID       : 101
  Position : octet 35
=== Parsing sans exception ===
event=click, priority=5, source=unknown
=== Validation structurelle ===
Config valide : true
```

### `02_yaml_apercu` — Sortie attendue (extrait)
```
Serveur : 0.0.0.0:8080  
Workers : 4  
Base de données : production  
GET /health (1000ms)  
```

### `02_1_lecture_yaml` — Sortie attendue (extrait)
```
Type du nœud racine : map  
Nombre de documents : 2  
api_timeout=30, worker_retries=3  
```

### `02_2_ecriture_yaml` — Sortie attendue (extrait)
```
ports: [8080, 8443, 9090]  
metadata: {env: prod, region: eu-west}  
"country": "NO"
```

### `03_toml` — Sortie attendue (extrait)
```
Serveur : 0.0.0.0:8080  
Créé le 2026-03-14 à 10:30:00  
Release : 2026-03-14  
```

### `04_xml_pugixml` — Sortie attendue (extrait)
```
Serveur : api-prod  
Port    : 8080  
GET /health (timeout: 1000ms)  
DB host : db.internal  
```

### `05_validation` — Sortie attendue (extrait)
```
JSON valide : true  
YAML valide : true  
TOML valide : true  
443 sans TLS invalide : true
Pipeline OK : 0.0.0.0:8080
```

### Expressions regulieres (section 24.6)

| Fichier | Section | Description | Fichier source | Compilation |
|---------|---------|-------------|----------------|-------------|
| `06_1_std_regex.cpp` | 24.6.1 | std::regex complet — regex_search (captures, prefix/suffix), regex_match (IPv4), regex_replace ($1/$2), sregex_iterator, regex_error | `06.1-std-regex.md` | `g++-15 -std=c++23 -O2 -o 06_1_std_regex 06_1_std_regex.cpp` |
| `06_2_ctre.cpp` | 24.6.2 | CTRE (Compile-Time Regex) — search, match (static_assert), captures avec get<N>(), search_all. Necessite FetchContent pour CTRE. | `06.2-ctre.md` | Via CMake FetchContent (voir ci-dessous) |
| `06_3_re2.cpp` | 24.6.3 | RE2 (Google) — FullMatch, PartialMatch, captures typees (int, string), pattern pre-compile | `06.3-re2-pcre2.md` | `g++-15 -std=c++23 -O2 -o 06_3_re2 06_3_re2.cpp -lre2` |
| `06_3_pcre2.cpp` | 24.6.3 | PCRE2 wrapper RAII — lookbehind (impossible avec RE2), JIT, iteration sur captures | `06.3-re2-pcre2.md` | `g++-15 -std=c++23 -O2 -o 06_3_pcre2 06_3_pcre2.cpp -lpcre2-8` |

#### Sorties attendues

**06_1_std_regex** :
```
Correspondance : 2026-03-21, Année: 2026, Mois: 03, Jour: 21
IPv4 valide: 192.168.1.1
Dupont (nom), Jean (prenom), 42 (age)
E001, E042, E107, E999
Erreur regex détectée
```

**06_2_ctre** :
```
Trouvé : 2026-03-21
static_assert OK, match complet/partiel correct
Année: 2026, Mois: 03, Jour: 21
E001, E042, E107, E999
```

**06_3_re2** :
```
Format de date valide, Date : 21/3/2026
Niveau: ERROR, Port: 5432
Pattern pre-compile OK
```

**06_3_pcre2** :
```
Prix USD : 42.99, Prix USD : 100
Date: 2026-03-21, Année: 2026, Mois: 03, Jour: 21
```

#### Compilation CTRE (FetchContent)

CTRE est header-only et se telecharge via CMake FetchContent :

```cmake
include(FetchContent)
FetchContent_Declare(ctre
    GIT_REPOSITORY https://github.com/hanickadot/compile-time-regular-expressions.git
    GIT_TAG v3.9.0)
FetchContent_MakeAvailable(ctre)
target_link_libraries(mon_projet PRIVATE ctre::ctre)
```

## Prerequis

- **Compilateur** : g++-15 (ou compatible C++23 avec `<print>`)
- **nlohmann/json** : `sudo apt install nlohmann-json3-dev`
- **yaml-cpp** : `sudo apt install libyaml-cpp-dev` (linker avec `-lyaml-cpp`)
- **toml++** : `sudo apt install libtomlplusplus-dev` (header-only, pas de link)
- **pugixml** : `sudo apt install libpugixml-dev` (linker avec `-lpugixml`)
- **RE2** : `sudo apt install libre2-dev` (linker avec `-lre2`)
- **PCRE2** : `sudo apt install libpcre2-dev` (linker avec `-lpcre2-8`)
- **CTRE** : Via CMake FetchContent (header-only, pas d'installation systeme)
