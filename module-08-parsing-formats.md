🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Module 8 — Parsing et Formats de Données

> 🎯 Niveau : Avancé

Ce module traite un problème transversal à toute application système ou réseau : comment structurer les données échangées. Fichiers de configuration (JSON, YAML, TOML, XML), sérialisation d'objets C++, et formats binaires performants (Protobuf, FlatBuffers, MessagePack). Le choix du format a un impact direct sur la performance, la compatibilité inter-langages, et la maintenabilité — ce n'est pas une décision à prendre par défaut.

---

## Objectifs pédagogiques

1. **Utiliser** nlohmann/json pour parser et sérialiser du JSON en C++, avec gestion robuste des erreurs et validation des données.
2. **Parser** des fichiers de configuration YAML avec yaml-cpp et TOML avec toml++, en connaissant les pièges spécifiques à chaque format.
3. **Implémenter** la sérialisation/désérialisation avec Protocol Buffers : définition de messages `.proto`, génération de code C++, évolution de schémas.
4. **Comprendre** le modèle zéro-copy de FlatBuffers et le format compact de MessagePack, et identifier les cas d'usage où chacun est pertinent.
5. **Choisir** le format de sérialisation adapté à un contexte donné (configuration, IPC, réseau, stockage) en fonction de la performance, la taille, la lisibilité et la compatibilité.
6. **Appliquer** les bonnes pratiques de validation de schémas pour éviter les erreurs silencieuses au parsing.

---

## Prérequis

- **Module 4, chapitre 12** : `std::optional`, `std::variant`, `std::expected` — utilisés pour gérer les valeurs absentes et les erreurs de parsing de manière idiomatique.
- **Module 6** : gestion d'erreurs — nlohmann/json lève des exceptions par défaut, yaml-cpp aussi. La stratégie exceptions vs `std::expected` définie dans le Module 6 s'applique directement au parsing.
- **Module 7, chapitre 22** : networking et gRPC — les formats binaires (Protobuf en particulier) servent directement la communication réseau vue dans le module précédent.
- **Module 9, chapitre 27** (recommandé) : gestion des dépendances avec Conan/vcpkg — nlohmann/json, yaml-cpp, protobuf et FlatBuffers s'installent via ces gestionnaires.

---

## Chapitres

### Chapitre 24 — Sérialisation et Fichiers de Configuration

Les quatre formats texte courants pour la configuration et l'échange de données structurées. nlohmann/json et yaml-cpp sont les librairies de référence en C++ ; TOML (toml++) est l'alternative moderne pour la configuration pure ; XML (pugixml) reste nécessaire pour les systèmes legacy.

- **JSON avec nlohmann/json** : installation (header-only ou via Conan/vcpkg), parsing de fichiers et de chaînes, sérialisation d'objets C++ via `to_json`/`from_json` (ADL-based), accès aux valeurs (opérateur `[]` qui lève `json::type_error`, `.value()` avec valeur par défaut, `.contains()` pour la vérification), gestion des erreurs de parsing (`json::parse_error`).
- **YAML avec yaml-cpp** : lecture de fichiers de configuration (`YAML::LoadFile`), navigation dans les nœuds (`node["key"]`), types scalaires/séquences/maps, écriture YAML (`YAML::Emitter`).
- **TOML avec toml++** : header-only, typage strict (les dates sont des dates, pas des chaînes), syntaxe non ambiguë — adapté aux fichiers de configuration applicatifs.
- **XML avec pugixml** : parsing DOM léger, XPath pour les requêtes — pertinent pour l'intégration avec des systèmes legacy, SOAP, ou des formats de données existants (SVG, Maven, etc.).
- **Validation de schémas** : vérifier que les données parsées respectent une structure attendue avant de les utiliser — techniques de validation programmatique en C++.

### Chapitre 25 — Formats Binaires et Sérialisation Performante

Les formats binaires pour les cas où le texte (JSON, YAML) est trop lent, trop volumineux, ou insuffisamment typé. Protobuf est le standard de facto ; FlatBuffers et MessagePack couvrent des niches spécifiques.

- **Protocol Buffers (Protobuf)** : définition de messages dans des fichiers `.proto` (syntaxe proto3), génération de code C++ avec `protoc`, sérialisation/désérialisation (`SerializeToString`, `ParseFromString`), évolution de schémas (ajout de champs, backward/forward compatibility), intégration directe avec gRPC.
- **FlatBuffers** : sérialisation zéro-copy — les données sont accessibles directement dans le buffer sans phase de désérialisation. Temps de désérialisation nul au prix d'un accès un peu plus verbeux. Pertinent pour le game development, les systèmes embarqués, et les cas où la latence de désérialisation est critique.
- **MessagePack** : format binaire compact compatible avec le modèle de données JSON. Plus petit et plus rapide que JSON, mais sans schéma — adapté aux caches, logs binaires, et communication inter-services où JSON est trop verbeux.
- **Comparaison de performances** : taille sérialisée, temps de sérialisation/désérialisation, compatibilité inter-langages, évolution de schémas — critères de décision selon le cas d'usage.

---

## Points de vigilance

- **nlohmann/json qui lève des exceptions sur des clés absentes.** `j["key"]` lève `json::type_error` si la clé n'existe pas dans un objet, ou `json::out_of_range` si l'index dépasse la taille d'un tableau. Dans du code de parsing de configuration, c'est un crash en production sur un fichier incomplet. Utilisez `.contains("key")` pour vérifier la présence, ou `.value("key", default_value)` pour obtenir une valeur par défaut. Autre option : parser dans un `try`/`catch` avec un message d'erreur clair indiquant la clé manquante.

- **YAML qui interprète silencieusement "yes"/"no" comme booléens.** En YAML 1.1 (le standard que yaml-cpp implémente par défaut), les chaînes `yes`, `no`, `on`, `off`, `true`, `false` sont interprétées comme des booléens, pas comme des chaînes. `country: no` parse comme `country: false`. La solution est de quoter les valeurs ambiguës (`country: "no"`) ou de documenter cette contrainte dans vos schémas de configuration. YAML 1.2 corrige ce comportement, mais yaml-cpp utilise YAML 1.1.

- **Coût caché de Protobuf sur les petits messages fréquents.** Protobuf a un overhead fixe par message : allocation mémoire pour la structure générée, sérialisation du header, etc. Sur des messages de quelques octets envoyés à haute fréquence (métriques, heartbeats), cet overhead peut dominer le temps utile. FlatBuffers (zéro allocation) ou MessagePack (sérialisation inline) sont plus adaptés à ce pattern. Mesurez avec Google Benchmark avant de choisir.

- **Endianness non gérée dans les formats binaires maison.** Si vous sérialisez des structs C++ directement en binaire (`write(&obj, sizeof(obj))`), le format dépend de l'endianness de la machine. Un fichier écrit sur x86 (little-endian) sera illisible sur ARM big-endian. Les formats standards (Protobuf, FlatBuffers, MessagePack) gèrent l'endianness. Si vous devez écrire un format binaire custom, utilisez `std::byteswap` (C++23) ou `htonl`/`ntohl` explicitement — ou mieux, ne faites pas de format custom quand un format standard convient.

---

## Compétences acquises

À l'issue de ce module, vous savez :
- Parser et produire du JSON, YAML, TOML et XML en C++ avec les librairies de référence, en gérant les erreurs de parsing proprement.
- Sérialiser des objets C++ avec nlohmann/json (`to_json`/`from_json`) et avec Protobuf (`.proto` + `protoc`).
- Choisir entre Protobuf, FlatBuffers et MessagePack selon les contraintes de performance, taille et compatibilité.
- Valider les données parsées avant de les utiliser, et anticiper les pièges spécifiques à chaque format (booléens YAML, clés absentes JSON).
- Éviter les formats binaires maison quand un standard existe, et gérer l'endianness quand c'est inévitable.

---


⏭️ [Sérialisation et Fichiers de Configuration](/24-serialisation-config/README.md)
