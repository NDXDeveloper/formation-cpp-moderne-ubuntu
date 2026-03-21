🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 24 : Sérialisation et Fichiers de Configuration 🔥

## Module 8 — Parsing et Formats de Données *(Niveau Avancé)*

---

## Pourquoi ce chapitre est critique

Tout programme C++ non trivial a besoin de communiquer avec le monde extérieur : lire une configuration au démarrage, échanger des données avec un service distant, persister un état sur disque, ou alimenter un pipeline de données. Ces opérations reposent toutes sur un même mécanisme fondamental : la **sérialisation** — la transformation de structures en mémoire en une représentation transportable, et l'opération inverse, la **désérialisation**.

En C++, contrairement à des langages comme Python ou Go qui proposent une introspection native des types, il n'existe pas (avant C++26 et la réflexion statique) de mécanisme intégré au langage pour convertir automatiquement un objet en texte ou en binaire. Chaque format nécessite donc une librairie dédiée et une compréhension claire des compromis impliqués.

Ce chapitre couvre les formats les plus utilisés en environnement professionnel, du texte lisible par l'humain au binaire haute performance.

---

## Concepts fondamentaux

### Sérialisation et désérialisation

La sérialisation consiste à encoder les données d'un programme — structures, objets, conteneurs — dans un format défini, qu'il soit textuel ou binaire. La désérialisation effectue le chemin inverse : elle reconstruit les structures C++ à partir du flux de données.

```
┌──────────────┐     sérialisation     ┌──────────────────┐
│  Objet C++   │ ───────────────────►  │  Flux de données │
│  en mémoire  │                       │  (texte/binaire) │
└──────────────┘ ◄───────────────────  └──────────────────┘
                   désérialisation
```

Deux grandes familles de formats coexistent, chacune avec ses forces :

**Formats textuels** — JSON, YAML, TOML, XML. Lisibles par un humain, faciles à éditer et à versionner avec Git. Leur coût de parsing est plus élevé et leur empreinte mémoire plus importante, mais ils sont incontournables pour la configuration, les API REST et le débogage.

**Formats binaires** — Protocol Buffers, FlatBuffers, MessagePack. Compacts, rapides à parser, souvent dotés d'un schéma formel. Ils s'imposent dans les contextes où la performance ou la bande passante sont critiques : communication inter-services (gRPC), stockage haute fréquence, systèmes embarqués.

### Fichiers de configuration

Un fichier de configuration est un cas particulier de sérialisation où les données sont principalement écrites par un humain et lues par le programme. Ce contexte impose des contraintes spécifiques : la syntaxe doit être intuitive, les commentaires doivent être supportés, et les messages d'erreur en cas de format invalide doivent être clairs et exploitables.

Les trois formats dominants pour la configuration en 2026 sont JSON (malgré l'absence de commentaires dans la spécification standard), YAML (puissant mais sujet à des pièges de parsing subtils), et TOML (conçu explicitement pour la configuration, de plus en plus adopté).

---

## Panorama des formats couverts

### Formats textuels

| Format | Librairie C++ | Forces | Faiblesses | Usage typique |
|--------|--------------|--------|------------|---------------|
| **JSON** | nlohmann/json | Ubiquité, API intuitive, excellent support C++ | Pas de commentaires, verbeux | API REST, config simple, échange de données |
| **YAML** | yaml-cpp | Lisibilité, commentaires, structures complexes | Spécification complexe, pièges de typage implicite | Fichiers de config (Kubernetes, Ansible, CI/CD) |
| **TOML** | toml++ | Clarté, conçu pour la configuration, sans ambiguïté | Moins adapté aux structures profondément imbriquées | Configuration applicative (Cargo, pip) |
| **XML** | pugixml | Standards industriels, schémas (XSD), namespaces | Extrêmement verbeux, API lourde | Systèmes legacy, SOAP, formats documentaires |

### Formats binaires *(couverts au chapitre 25)*

| Format | Librairie C++ | Forces | Faiblesses | Usage typique |
|--------|--------------|--------|------------|---------------|
| **Protobuf** | libprotobuf | Schéma strict, rétrocompatibilité, écosystème gRPC | Nécessite étape de génération de code | gRPC, stockage structuré, communication inter-services |
| **FlatBuffers** | flatbuffers | Zéro-copy, accès aléatoire sans parsing | API moins ergonomique | Jeux, systèmes temps réel, mobile |
| **MessagePack** | msgpack-c | Compact, drop-in replacement pour JSON | Pas de schéma, moins d'outillage | Cache, logs binaires, protocoles légers |

---

## Critères de choix d'un format

Le choix d'un format de sérialisation dépend de plusieurs facteurs qu'il faut évaluer en fonction du contexte du projet :

**Lisibilité humaine.** Si le fichier est destiné à être lu ou édité par un humain (configuration, fichiers de déploiement), un format textuel s'impose. JSON convient pour les structures simples, YAML pour les configurations complexes avec commentaires, et TOML pour une syntaxe sans ambiguïté.

**Performance.** Si le parsing ou la sérialisation se trouvent sur un chemin critique (boucle de traitement, communication haute fréquence), les formats binaires sont largement supérieurs. FlatBuffers permet même l'accès aux données sans phase de désérialisation grâce au zero-copy.

**Interopérabilité.** JSON est le format le plus universellement supporté à travers les langages et les plateformes. Protobuf offre une excellente interopérabilité grâce à la génération de code multi-langage. TOML et YAML sont bien supportés dans l'écosystème DevOps mais moins omniprésents dans les API.

**Évolution du schéma.** Dans un système distribué où les composants évoluent indépendamment, la rétrocompatibilité est essentielle. Protobuf a été conçu dès l'origine pour supporter l'évolution des schémas (ajout/suppression de champs). JSON et YAML laissent cette responsabilité au développeur.

**Taille des données.** Pour la transmission réseau ou le stockage à grande échelle, la taille du payload compte. Les formats binaires sont typiquement 2 à 10 fois plus compacts que leurs équivalents textuels.

---

## Bonnes pratiques transversales

Quel que soit le format choisi, certains principes s'appliquent systématiquement dans un projet C++ professionnel.

**Toujours valider les entrées.** Un fichier de configuration ou un message réseau peut être malformé, incomplet ou provenir d'une version différente du logiciel. Le code de désérialisation doit traiter ces cas explicitement, jamais les ignorer. Les sections suivantes montreront comment chaque librairie expose ses mécanismes de gestion d'erreurs.

**Séparer la couche de sérialisation du domaine métier.** La logique de parsing ne doit pas fuir dans le reste du code. Une bonne architecture définit des structures C++ claires pour le domaine, et cantonne la conversion depuis/vers le format de sérialisation dans une couche dédiée. Cela permet de changer de format (passer de JSON à TOML par exemple) sans impacter la logique applicative.

**Gérer les valeurs par défaut et les champs optionnels.** En C++ moderne, `std::optional` (C++17) et `std::expected` (C++23) sont les outils naturels pour représenter des champs qui peuvent être absents dans les données sérialisées. Ils rendent l'intention explicite dans le code et évitent les valeurs sentinelles fragiles.

**Tester la sérialisation comme du code critique.** Les fonctions de sérialisation/désérialisation méritent des tests unitaires rigoureux, incluant les cas limites : chaînes vides, valeurs numériques extrêmes, caractères Unicode, fichiers tronqués, encodages inattendus. Un bug de parsing peut être silencieux et se manifester bien plus tard dans l'exécution du programme.

---

## Organisation du chapitre

Ce chapitre se concentre sur les **formats textuels** et leurs librairies de référence en C++ :

- **Section 24.1** — JSON avec nlohmann/json : le format universel et la librairie C++ la plus populaire pour le manipuler.  
- **Section 24.2** — YAML avec yaml-cpp : le format de prédilection de l'écosystème DevOps.  
- **Section 24.3** — TOML avec toml++ : l'alternative moderne, claire et sans ambiguïté.  
- **Section 24.4** — XML avec pugixml : pour l'interfaçage avec les systèmes legacy.  
- **Section 24.5** — Bonnes pratiques de validation de schémas, applicables à tous les formats.  
- **Section 24.6** — Expressions régulières en C++ : `std::regex` et les alternatives performantes (CTRE, RE2, PCRE2).

Les formats binaires (Protocol Buffers, FlatBuffers, MessagePack) font l'objet du **chapitre 25**, qui suit immédiatement.

---

## Prérequis

Pour aborder ce chapitre confortablement, les notions suivantes doivent être acquises :

- Gestion de la mémoire et smart pointers *(chapitre 9)*  
- Move semantics et références rvalue *(chapitre 10)*  
- Conteneurs de la STL, en particulier `std::vector`, `std::map` et `std::unordered_map` *(chapitres 13-14)*  
- `std::optional` et `std::variant` *(section 12.2)*  
- CMake et gestion des dépendances avec Conan ou vcpkg *(chapitres 26-27)*  
- `std::filesystem` pour la manipulation de fichiers *(section 19.1)*

---

> **À venir** — La section 24.1 commence par le format le plus utilisé au monde : JSON, avec la librairie nlohmann/json qui a redéfini l'ergonomie du parsing en C++.

⏭️ [JSON : Lecture/Écriture avec nlohmann/json](/24-serialisation-config/01-json-nlohmann.md)
