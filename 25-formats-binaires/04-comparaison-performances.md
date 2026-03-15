🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 25.4 — Comparaison de performances et cas d'usage

## Section du Module 8 — Parsing et Formats de Données

---

## Vue d'ensemble

Les sections précédentes ont présenté chaque format individuellement — ses principes, son API, ses forces. Cette section prend du recul pour comparer les sept formats couverts dans les chapitres 24 et 25 sur des critères concrets : taille des données sérialisées, vitesse de sérialisation/désérialisation, complexité d'intégration, et adéquation aux différents contextes d'un projet C++ professionnel.

L'objectif n'est pas de désigner un « meilleur » format — chacun occupe une niche où il excelle — mais de fournir un cadre de décision structuré pour choisir le format adapté à chaque situation.

---

## Benchmark comparatif

### Protocole de mesure

Les chiffres présentés ci-dessous sont des ordres de grandeur représentatifs, mesurés sur un processeur x86-64 moderne (AMD Zen 4 / Intel Alder Lake) avec GCC 14 ou Clang 19 en `-O2`. Le message de test est une structure réaliste contenant des chaînes, des entiers, des flottants, un sous-objet imbriqué et un tableau de 10 éléments — typique d'un message d'API ou d'un enregistrement de log structuré.

Les performances réelles varient selon la structure des messages, la distribution des types de champs, le hardware et le compilateur. Ces chiffres servent de base de comparaison relative, pas de valeurs absolues.

> 💡 *Pour des benchmarks précis sur vos messages et votre hardware, Google Benchmark (section 35.1) permet de mesurer les performances de sérialisation dans les conditions exactes de votre projet.*

### Taille des données sérialisées

| Format | Taille (octets) | Ratio vs JSON | Notes |
|--------|---------------:|:-------------:|-------|
| **JSON** (compact) | 210 | 1.0x | Noms de clés textuels, guillemets |
| **JSON** (indenté) | 385 | 1.8x | Espaces et retours à la ligne |
| **YAML** | 195 | 0.93x | Légèrement plus compact (pas de guillemets ni d'accolades) |
| **TOML** | 230 | 1.1x | En-têtes de section, guillemets obligatoires |
| **XML** | 420 | 2.0x | Balises ouvrantes et fermantes |
| **MessagePack** (map) | 145 | 0.69x | Noms de clés binaires, pas de guillemets |
| **MessagePack** (array) | 95 | 0.45x | Pas de noms de clés |
| **Protobuf** | 82 | 0.39x | Numéros de champ varint, pas de noms |
| **FlatBuffers** | 112 | 0.53x | Vtable + alignement, scalaires taille fixe |

Observations clés :

- **Protobuf est le plus compact** grâce à l'encodage varint et à l'absence de noms de clés. Les champs à valeur par défaut (0, "", false) ne sont pas encodés du tout.
- **FlatBuffers est légèrement plus gros que Protobuf** en raison des vtables (overhead de navigation pour le zero-copy) et de l'encodage à taille fixe des scalaires (pas de varint).
- **MessagePack en mode array** approche la compacité de Protobuf car il élimine les noms de clés, mais conserve les tags de type auto-descriptifs.
- **XML est le plus volumineux** — environ 2x JSON et 5x Protobuf pour la même information.
- **YAML est marginalement plus compact que JSON** sur cet exemple, mais la différence est négligeable et peut s'inverser selon la structure.

### Vitesse de sérialisation

| Format | Débit (msg/s) | Ratio vs JSON | Notes |
|--------|-------------:|:-------------:|-------|
| **JSON** (nlohmann) | ~500 K | 1.0x | Parsing textuel, allocation DOM |
| **YAML** (yaml-cpp) | ~100 K | 0.2x | Spécification complexe, parsing coûteux |
| **TOML** (toml++) | ~400 K | 0.8x | Plus simple que YAML, parsing efficace |
| **XML** (pugixml) | ~600 K | 1.2x | Parsing in-situ optimisé |
| **MessagePack** | ~3 M | 6x | Encodage binaire direct |
| **Protobuf** | ~7 M | 14x | Code généré, encodage optimisé |
| **FlatBuffers** | ~5 M | 10x | Construction du buffer avec offsets |

### Vitesse de désérialisation

| Format | Débit (msg/s) | Ratio vs JSON | Notes |
|--------|-------------:|:-------------:|-------|
| **JSON** (nlohmann) | ~350 K | 1.0x | Tokenisation, conversion texte → nombre |
| **YAML** (yaml-cpp) | ~80 K | 0.23x | Parsing le plus coûteux |
| **TOML** (toml++) | ~300 K | 0.86x | Comparable à JSON |
| **XML** (pugixml) | ~500 K | 1.4x | Parsing in-situ |
| **MessagePack** | ~2 M | 5.7x | Décodage binaire, allocation |
| **Protobuf** | ~5 M | 14x | Code généré, décodage optimisé |
| **FlatBuffers** | ~∞ | — | Zero-copy : pas de désérialisation |

L'entrée FlatBuffers « ~∞ » reflète le fait qu'il n'y a pas de phase de désérialisation. L'accès au premier champ est une opération O(1) — un calcul d'offset. En pratique, le temps d'accès est de l'ordre de quelques nanosecondes, dominé par les accès mémoire au buffer.

### Synthèse visuelle

```
Compacité (taille du message, plus petit = mieux)
├── Protobuf        ████░░░░░░░░░░░░░░░░  (82 octets)
├── MessagePack arr ██████░░░░░░░░░░░░░░  (95 octets)
├── FlatBuffers     ███████░░░░░░░░░░░░░  (112 octets)
├── MessagePack map █████████░░░░░░░░░░░  (145 octets)
├── JSON compact    █████████████░░░░░░░  (210 octets)
├── TOML            ██████████████░░░░░░  (230 octets)
└── XML             ████████████████████  (420 octets)

Vitesse de désérialisation (plus rapide = mieux)
├── FlatBuffers     ████████████████████  (zero-copy)
├── Protobuf        ██████████████░░░░░░  (~5 M msg/s)
├── MessagePack     ██████████░░░░░░░░░░  (~2 M msg/s)
├── pugixml         ████░░░░░░░░░░░░░░░░  (~500 K msg/s)
├── nlohmann/json   ██░░░░░░░░░░░░░░░░░░  (~350 K msg/s)
├── toml++          ██░░░░░░░░░░░░░░░░░░  (~300 K msg/s)
└── yaml-cpp        █░░░░░░░░░░░░░░░░░░░  (~80 K msg/s)
```

---

## Coût d'intégration

La performance brute n'est pas le seul critère. Le coût d'intégration dans un projet — installation, configuration du build, courbe d'apprentissage, maintenance — est souvent le facteur décisif.

| Format | Header-only | Génération de code | Complexité CMake | Dépendances externes |
|--------|:-----------:|:------------------:|:-----------------:|:-------------------:|
| **nlohmann/json** | Oui | Non | Minimale | Aucune |
| **yaml-cpp** | Non | Non | Faible | Aucune |
| **toml++** | Oui | Non | Minimale | Aucune |
| **pugixml** | Quasi (3 fichiers) | Non | Minimale | Aucune |
| **MessagePack** | Oui | Non | Minimale | Aucune |
| **Protobuf** | Non | Oui (`protoc`) | Moyenne | `libprotobuf` + `protoc` |
| **FlatBuffers** | Oui (runtime) | Oui (`flatc`) | Moyenne | `flatc` (build-time) |

Les formats textuels et MessagePack s'intègrent en quelques lignes de CMake. Protobuf et FlatBuffers ajoutent une étape de build (génération de code), une dépendance à un outil de build (`protoc`/`flatc`), et dans le cas de Protobuf, une librairie runtime. Ce surcoût est justifié dans les projets à grande échelle, mais disproportionné pour un petit outil CLI ou un prototype.

---

## Arbre de décision

Le diagramme suivant guide le choix du format selon le contexte du projet :

```
Le fichier est-il écrit/lu par un humain ?
├── OUI → Le fichier est-il un fichier de configuration ?
│         ├── OUI → L'écosystème impose-t-il un format ?
│         │         ├── Kubernetes/Ansible/CI → YAML
│         │         ├── Aucune contrainte → TOML (préféré)
│         │         └── Interopérabilité maximale → JSON
│         └── NON → Le fichier est-il un échange de données ?
│                   ├── API REST / web → JSON
│                   └── Documentation structurée → XML (si legacy) / JSON
│
└── NON → La communication est-elle entre programmes ?
          ├── OUI → gRPC est-il utilisé ?
          │         ├── OUI → Protobuf (imposé par gRPC)
          │         └── NON → Un schéma formel est-il nécessaire ?
          │                   ├── OUI → La latence de décodage est-elle critique ?
          │                   │         ├── OUI → FlatBuffers
          │                   │         └── NON → Protobuf
          │                   └── NON → MessagePack
          │
          └── NON → Le fichier est-il un stockage sur disque ?
                    ├── Accès aléatoire / mmap → FlatBuffers
                    ├── Stockage séquentiel compact → Protobuf
                    ├── Cache / données temporaires → MessagePack
                    └── Export pour analyse externe → JSON / CSV
```

### Cas fréquents résumés

**« Je construis un outil CLI qui lit un fichier de configuration. »**
→ **TOML** si le format est libre, **YAML** si l'écosystème l'impose, **JSON** si l'interopérabilité avec d'autres outils prime.

**« Je développe des microservices qui communiquent entre eux. »**
→ **Protobuf + gRPC** pour la communication synchrone. Le schéma `.proto` sert de contrat d'API, la rétrocompatibilité facilite les déploiements indépendants.

**« Je stocke des sessions utilisateur dans Redis. »**
→ **MessagePack** en remplacement de JSON. Gain immédiat en taille et en vitesse, sans changement d'architecture. Pas de schéma à gérer.

**« Je développe un moteur de jeu qui charge des assets. »**
→ **FlatBuffers** pour les données de jeu (niveaux, configurations d'entités). Le zero-copy via `mmap` élimine les temps de chargement. L'Object API simplifie l'édition dans les outils.

**« Je dois interfacer avec un système bancaire existant. »**
→ **XML** (pugixml) — les standards financiers (ISO 20022, FIXML) imposent XML. Pas de choix, mais pugixml rend l'expérience supportable.

**« Je prototype rapidement un format d'échange entre deux services. »**
→ **MessagePack** pour commencer (intégration triviale, pas de schéma), puis migration vers **Protobuf** si le format se stabilise et que la rigueur d'un schéma devient nécessaire.

**« J'ai besoin du format le plus compact possible pour des messages IoT sur un réseau contraint. »**
→ **Protobuf** (le plus compact) ou **Protobuf Lite / nanopb** pour les environnements très contraints.

---

## Combinaison de formats dans un projet réaliste

Un projet professionnel utilise rarement un seul format. Voici une architecture typique d'un service C++ Cloud Native et les formats associés à chaque couche :

```
┌───────────────────────────────────────────────────┐
│                Application C++                    │
│                                                   │
│  ┌───────────────┐  ┌─────────────────────────┐   │
│  │ Configuration │  │ Communication           │   │
│  │ TOML / YAML   │  │ inter-services          │   │
│  │ (au démarrage)│  │ Protobuf + gRPC         │   │
│  └───────────────┘  └─────────────────────────┘   │
│                                                   │
│  ┌───────────────┐  ┌─────────────────────────┐   │
│  │ Cache local   │  │ Stockage fichiers       │   │
│  │ MessagePack   │  │ FlatBuffers (mmap)      │   │
│  │ (Redis/mémoire│  │ ou Protobuf (séquentiel │   │
│  └───────────────┘  └─────────────────────────┘   │
│                                                   │
│  ┌───────────────┐  ┌─────────────────────────┐   │
│  │ API externe   │  │ Logs & métriques        │   │
│  │ JSON (REST)   │  │ JSON structuré          │   │
│  │ ou Protobuf   │  │ ou MessagePack          │   │
│  └───────────────┘  └─────────────────────────┘   │
└───────────────────────────────────────────────────┘
```

Chaque format occupe la niche où il excelle :

- **TOML/YAML** pour la configuration humainement éditable.
- **Protobuf/gRPC** pour la communication inter-services typée et performante.
- **MessagePack** pour le cache et les données semi-structurées transitoires.
- **FlatBuffers** pour les fichiers de données à accès aléatoire.
- **JSON** pour les API externes et les logs destinés aux outils d'agrégation.

La clé est d'isoler chaque format dans sa couche. Le code métier ne manipule que des types C++ natifs (`ServerConfig`, `User`, `Event`). Les conversions depuis/vers les formats de sérialisation sont confinées dans des couches d'adaptation dédiées, comme décrit en section 24.5.

---

## Matrice de décision récapitulative

| Critère | JSON | YAML | TOML | XML | MsgPack | Protobuf | FlatBuffers |
|---------|:----:|:----:|:----:|:---:|:-------:|:--------:|:-----------:|
| Lisibilité humaine | ★★★ | ★★★★ | ★★★★★ | ★★ | ✗ | ✗ | ✗ |
| Commentaires | ✗ | ★★★★★ | ★★★★★ | ★★★★ | ✗ | ✗ | ✗ |
| Compacité | ★★ | ★★ | ★★ | ★ | ★★★★ | ★★★★★ | ★★★★ |
| Vitesse sérialisation | ★★ | ★ | ★★ | ★★ | ★★★★ | ★★★★★ | ★★★★ |
| Vitesse désérialisation | ★★ | ★ | ★★ | ★★★ | ★★★★ | ★★★★★ | ★★★★★ |
| Facilité d'intégration | ★★★★★ | ★★★★ | ★★★★★ | ★★★★ | ★★★★★ | ★★★ | ★★★ |
| Schéma formel | ★★★ | ★ | ★ | ★★★★★ | ✗ | ★★★★★ | ★★★★★ |
| Rétrocompatibilité | ★★ | ★★ | ★★ | ★★ | ★★ | ★★★★★ | ★★★★ |
| Écosystème | ★★★★★ | ★★★★ | ★★★ | ★★★★ | ★★★ | ★★★★★ | ★★★ |
| Multi-langage | ★★★★★ | ★★★★ | ★★★ | ★★★★★ | ★★★★★ | ★★★★ | ★★★★ |

---

## Principes à retenir

**Mesurer avant d'optimiser.** Ne pas choisir un format binaire « parce que c'est plus rapide ». Si le parsing JSON prend 0.1% du temps total de traitement, passer à Protobuf n'apportera rien de mesurable. Utiliser le profiling (chapitre 31) pour identifier les vrais goulots d'étranglement.

**Le format le plus simple qui répond au besoin est le meilleur.** JSON et MessagePack n'exigent aucun schéma ni outil de build — leur simplicité d'intégration est une valeur en soi. Ne pas payer le coût d'un schéma Protobuf quand un `nlohmann::json` ou un `msgpack::pack` suffit.

**Séparer le format de sérialisation du code métier.** Les types C++ du domaine (`ServerConfig`, `User`) ne doivent pas dépendre du format de sérialisation. Une couche d'adaptation isole la conversion et permet de changer de format sans impacter la logique applicative.

**Planifier l'évolution.** Tout format finit par évoluer. Protobuf et FlatBuffers ont des mécanismes intégrés. Pour JSON, YAML, TOML et MessagePack, la responsabilité incombe au développeur — d'où l'importance de la validation (section 24.5) et des valeurs par défaut raisonnables.

**Combiner les formats.** Un projet réaliste utilise plusieurs formats, chacun dans le contexte où il excelle. Ce n'est pas de l'incohérence — c'est du pragmatisme.

⏭️ [PARTIE IV : TOOLING ET BUILD SYSTEMS](/partie-04-tooling-build-systems.md)
