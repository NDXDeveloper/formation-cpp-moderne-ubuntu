🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 25 : Formats Binaires et Sérialisation Performante

## Module 8 — Parsing et Formats de Données *(Niveau Avancé)*

---

## Pourquoi passer au binaire

Le chapitre 24 couvrait les formats textuels — JSON, YAML, TOML, XML — conçus pour être lisibles par un humain. Cette lisibilité a un coût. Considérons un message simple contenant un identifiant entier, un nom et un score flottant. En JSON :

```json
{"id": 42, "name": "alice", "score": 98.5}
```

Ce message pèse 43 octets. L'information utile (un entier 32 bits, 5 caractères, un flottant 64 bits) tient en 17 octets. Les 26 octets restants — guillemets, accolades, deux-points, noms de clés — sont de l'overhead structurel, nécessaire au parser mais sans valeur pour l'application.

À l'échelle d'un fichier de configuration de 2 Ko, cet overhead est négligeable. À l'échelle de millions de messages par seconde entre microservices, de téraoctets de logs stockés quotidiennement, ou de communications temps réel dans un jeu en ligne, il devient un facteur limitant. C'est dans ces contextes que les formats de sérialisation binaire prennent tout leur sens.

---

## Ce que les formats binaires apportent

### Compacité

Un format binaire encode les données dans leur représentation machine ou une représentation proche, sans métadonnées textuelles redondantes. Un entier 32 bits occupe 4 octets, pas les 1 à 10 caractères ASCII de sa représentation décimale. Une chaîne est préfixée par sa longueur, pas entourée de guillemets nécessitant un échappement.

En pratique, les formats binaires produisent des messages **2 à 10 fois plus compacts** que leur équivalent JSON, avec des gains encore supérieurs sur les données numériques denses.

### Vitesse de parsing

Le parsing d'un format textuel implique la tokenisation (identifier les guillemets, accolades, virgules), la conversion de chaînes en nombres (`"8080"` → `int`), la gestion de l'échappement Unicode, et la construction d'un arbre DOM en mémoire. Ces opérations sont coûteuses en cycles CPU.

Un format binaire est conçu pour être décodé efficacement. Les champs sont identifiés par des tags numériques, les longueurs sont explicites, les types sont encodés dans le flux. Certains formats (FlatBuffers) vont jusqu'à permettre l'accès direct aux données sans aucune phase de désérialisation — le buffer réseau *est* la structure de données.

### Schéma et évolution

Les formats binaires modernes reposent sur un schéma formel (fichier `.proto` pour Protobuf, fichier `.fbs` pour FlatBuffers) qui décrit la structure des messages. Ce schéma sert de contrat entre le producteur et le consommateur des données, et les outils de génération de code produisent automatiquement les classes C++ de sérialisation/désérialisation.

Un avantage souvent sous-estimé est la **rétrocompatibilité structurée**. Protobuf et FlatBuffers sont conçus pour supporter l'évolution du schéma — ajout de nouveaux champs, dépréciation d'anciens — sans casser les consommateurs existants. Dans un système distribué où les services sont déployés indépendamment, cette propriété est essentielle.

---

## Panorama des formats couverts

Ce chapitre couvre les trois formats binaires les plus utilisés en C++ en 2026, chacun avec un positionnement distinct :

### Protocol Buffers (Protobuf)

Créé par Google et utilisé massivement en interne depuis 2001, open-sourcé en 2008. Protobuf est le format binaire le plus répandu, porté par l'écosystème gRPC (couvert en section 22.6) qui l'utilise comme format de sérialisation par défaut.

Son modèle repose sur la **définition de messages dans des fichiers `.proto`**, compilés par `protoc` en code C++ (ou tout autre langage supporté). La sérialisation produit un flux binaire compact, et la désérialisation reconstruit les structures typées. Protobuf offre une excellente rétrocompatibilité lors de l'évolution des schémas.

**Cas d'usage typiques :** communication inter-services (gRPC), stockage structuré, pipelines de données, configuration distribuée (etcd).

### FlatBuffers

Créé par Google en 2014, initialement pour les jeux mobiles. FlatBuffers se distingue par son approche **zero-copy** : les données sérialisées sont accessibles directement dans le buffer, sans phase de désérialisation. Le programme lit les champs directement dans le buffer réseau ou mémoire mappé, ce qui élimine le coût d'allocation et de copie.

**Cas d'usage typiques :** jeux vidéo, applications mobiles, systèmes temps réel, traitement de flux à très haute fréquence, mémoire mappée (mmap).

### MessagePack

Format binaire compact qui se positionne comme un « JSON binaire ». Contrairement à Protobuf et FlatBuffers, MessagePack est **schema-less** — il ne nécessite pas de définition préalable de la structure des messages. Les données sont auto-descriptives, comme en JSON, mais encodées en binaire.

**Cas d'usage typiques :** cache (remplacement de JSON sérialisé), logs binaires, protocoles de communication légers, situations où la flexibilité d'un format sans schéma est nécessaire.

---

## Comparaison des caractéristiques

| Caractéristique | Protobuf | FlatBuffers | MessagePack |
|----------------|----------|-------------|-------------|
| **Schéma** | Obligatoire (`.proto`) | Obligatoire (`.fbs`) | Aucun (schema-less) |
| **Génération de code** | Oui (`protoc`) | Oui (`flatc`) | Non |
| **Désérialisation** | Complète (copie en mémoire) | Zero-copy (accès direct) | Complète (copie en mémoire) |
| **Compacité** | Très bonne | Bonne (légèrement plus gros) | Très bonne |
| **Vitesse d'encodage** | Rapide | Rapide | Rapide |
| **Vitesse de décodage** | Rapide | Quasi-instantanée | Rapide |
| **Rétrocompatibilité** | Excellente (conçue dès l'origine) | Très bonne | Responsabilité du développeur |
| **Accès aléatoire** | Non (lecture séquentielle) | Oui (offset dans le buffer) | Non (lecture séquentielle) |
| **Lisibilité humaine** | Aucune (mais `protoc --decode`) | Aucune | Aucune (mais outils de debug) |
| **Écosystème** | Massif (gRPC, Cloud, Big Data) | Jeux, mobile, embarqué | Modéré (cache, logs) |
| **Multi-langage** | 12+ langages officiels | 18+ langages | 50+ langages |
| **Complexité d'intégration** | Moyenne (protoc + build system) | Moyenne (flatc + build system) | Faible (header-only) |

---

## Critères de choix

Le choix entre ces formats dépend de plusieurs facteurs liés au contexte du projet.

**Si le projet utilise gRPC ou s'insère dans l'écosystème Google Cloud / Kubernetes** — Protobuf est le choix naturel. Le format est imposé par gRPC, et l'outillage est mature. La rétrocompatibilité native simplifie l'évolution des API.

**Si la latence de désérialisation est critique** — FlatBuffers élimine le coût de désérialisation grâce au zero-copy. C'est le choix pour les boucles de rendu de jeux, les systèmes de trading haute fréquence, ou tout contexte où chaque microseconde compte.

**Si la flexibilité prime sur la structure** — MessagePack ne nécessite pas de schéma ni de génération de code. C'est le remplacement naturel de JSON quand on veut gagner en compacité et en vitesse sans changer l'architecture. L'intégration est triviale (header-only en C++).

**Si les deux extrémités du canal sont sous contrôle** — Protobuf ou FlatBuffers permettent de définir un contrat strict et d'évoluer de manière contrôlée. Le schéma est une documentation vivante de l'interface.

**Si une extrémité est un système tiers non contrôlé** — MessagePack ou JSON sont préférables, car ils n'imposent pas un schéma partagé.

---

## Formats binaires et formats textuels : complémentaires, pas concurrents

Les formats binaires ne remplacent pas les formats textuels — ils servent des contextes différents. Un même projet utilise souvent les deux :

- **Configuration** → TOML, YAML ou JSON (écrits par des humains, lus au démarrage).
- **Communication inter-services** → Protobuf/gRPC (haute performance, schéma contractuel).
- **Stockage de sessions ou cache** → MessagePack (compact, rapide, schema-less).
- **Export de données pour analyse** → JSON ou CSV (compatibilité universelle avec les outils d'analyse).
- **Formats de fichiers internes** → FlatBuffers (mmap, accès aléatoire, performances de lecture).

L'erreur serait d'utiliser un format binaire pour un fichier de configuration (illisible, impossible à debugger sans outils) ou un format textuel pour la communication haute fréquence entre services (overhead inutile, latence de parsing).

---

## Impact sur le build system

Contrairement aux librairies du chapitre 24 (toutes utilisables directement après installation), Protobuf et FlatBuffers ajoutent une étape de **génération de code** dans le processus de build. Les fichiers de schéma (`.proto`, `.fbs`) sont compilés par un outil dédié (`protoc`, `flatc`) en code source C++ (`.pb.h`/`.pb.cc`, `_generated.h`) qui est ensuite compilé normalement.

Cette étape s'intègre dans CMake, mais ajoute de la complexité au build system :

```
┌──────────────┐     protoc / flatc     ┌───────────────────┐
│  Fichiers    │  ───────────────────►  │  Code C++ généré  │
│  .proto/.fbs │                        │  (.pb.h, .pb.cc)  │
└──────────────┘                        └────────┬──────────┘
                                                 │
                                          compilation C++
                                                 │
                                                 ▼
                                        ┌───────────────────┐
                                        │  Binaire final    │
                                        └───────────────────┘
```

MessagePack, étant schema-less, n'a pas cette contrainte — c'est l'un de ses avantages pratiques.

Les sections suivantes détaillent chaque format, son installation, son intégration CMake, et son utilisation en C++.

---

## Organisation du chapitre

- **Section 25.1** — Protocol Buffers : définition de messages `.proto`, génération de code, sérialisation/désérialisation, intégration CMake, évolution de schéma.
- **Section 25.2** — FlatBuffers : schémas `.fbs`, zero-copy serialization, accès direct aux données, cas d'usage temps réel.
- **Section 25.3** — MessagePack : sérialisation schema-less, intégration header-only, remplacement de JSON.
- **Section 25.4** — Comparaison de performances et cas d'usage : benchmarks, arbre de décision, combinaison de formats dans un projet.

---

## Prérequis

- Chapitre 24, en particulier les concepts de sérialisation/désérialisation et de validation (sections 24.1 et 24.5).
- CMake et gestion des dépendances *(chapitres 26-27)* — l'intégration de `protoc` et `flatc` dans CMake est un cas d'usage avancé.
- Networking et gRPC *(section 22.6)* — recommandé pour Protobuf, car les deux sont étroitement liés.
- Move semantics *(chapitre 10)* — les structures générées par Protobuf exploitent la sémantique de mouvement.

⏭️ [Protocol Buffers (Protobuf) : Sérialisation Google](/25-formats-binaires/01-protobuf.md)
