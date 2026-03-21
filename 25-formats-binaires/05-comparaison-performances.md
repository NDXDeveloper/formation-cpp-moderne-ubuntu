🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 25.5 Comparaison de performances et cas d'usage

## Vue d'ensemble

Les sections précédentes ont présenté chaque format individuellement — ses principes, son API, ses forces. Cette section prend du recul pour comparer les huit formats couverts dans les chapitres 24 et 25 sur des critères concrets : taille des données sérialisées, vitesse de sérialisation/désérialisation, complexité d'intégration, et adéquation aux différents contextes d'un projet C++ professionnel.

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
| **JSON** (compact) | 210 | 1.0× | Noms de clés textuels, guillemets |
| **JSON** (indenté) | 385 | 1.8× | Espaces et retours à la ligne |
| **YAML** | 195 | 0.93× | Légèrement plus compact (pas de guillemets ni d'accolades) |
| **TOML** | 230 | 1.1× | En-têtes de section, guillemets obligatoires |
| **XML** | 420 | 2.0× | Balises ouvrantes et fermantes |
| **MessagePack** (map) | 145 | 0.69× | Noms de clés binaires, pas de guillemets |
| **MessagePack** (array) | 95 | 0.45× | Pas de noms de clés |
| **Protobuf** | 82 | 0.39× | Numéros de champ varint, pas de noms |
| **FlatBuffers** | 112 | 0.53× | Vtable + alignement, scalaires taille fixe |
| **Cap'n Proto** | 120 | 0.57× | Pointeurs 64 bits, alignement 8 octets |
| **Cap'n Proto** (packed) | 70 | 0.33× | Compression des zéros de padding |

Observations clés :

- **Protobuf est le plus compact en mode standard** grâce à l'encodage varint et à l'absence de noms de clés. Les champs à valeur par défaut (0, "", false) ne sont pas encodés du tout.  
- **Cap'n Proto non-packed est légèrement plus gros que FlatBuffers** en raison des pointeurs relatifs 64 bits et d'un alignement systématique à 8 octets (mots de 64 bits). En revanche, le **format packed** de Cap'n Proto comprime efficacement les séquences de zéros dues au padding et devient le plus compact de tous les formats — au prix d'une perte du zero-copy pur (une passe de décompression est nécessaire).  
- **FlatBuffers est légèrement plus gros que Protobuf** en raison des vtables (overhead de navigation pour le zero-copy) et de l'encodage à taille fixe des scalaires (pas de varint).  
- **MessagePack en mode array** approche la compacité de Protobuf car il élimine les noms de clés, mais conserve les tags de type auto-descriptifs.  
- **XML est le plus volumineux** — environ 2× JSON et 5× Protobuf pour la même information.  
- **YAML est marginalement plus compact que JSON** sur cet exemple, mais la différence est négligeable et peut s'inverser selon la structure.

### Vitesse de sérialisation

| Format | Débit (msg/s) | Ratio vs JSON | Notes |
|--------|-------------:|:-------------:|-------|
| **JSON** (nlohmann) | ~500 K | 1.0× | Parsing textuel, allocation DOM |
| **YAML** (yaml-cpp) | ~100 K | 0.2× | Spécification complexe, parsing coûteux |
| **TOML** (toml++) | ~400 K | 0.8× | Plus simple que YAML, parsing efficace |
| **XML** (pugixml) | ~600 K | 1.2× | Parsing in-situ optimisé |
| **MessagePack** | ~3 M | 6× | Encodage binaire direct |
| **Protobuf** | ~7 M | 14× | Code généré, encodage optimisé |
| **FlatBuffers** | ~5 M | 10× | Construction du buffer avec offsets |
| **Cap'n Proto** | ~8 M | 16× | Écriture directe dans le buffer wire |

Cap'n Proto affiche le débit de sérialisation le plus élevé car il n'y a pas de phase d'encodage distincte : les appels `set*` écrivent directement les valeurs aux offsets fixes dans le buffer qui sera transmis. Le coût de « sérialisation » se réduit à un `writev()` sur le buffer déjà constitué.

### Vitesse de désérialisation

| Format | Débit (msg/s) | Ratio vs JSON | Notes |
|--------|-------------:|:-------------:|-------|
| **JSON** (nlohmann) | ~350 K | 1.0× | Tokenisation, conversion texte → nombre |
| **YAML** (yaml-cpp) | ~80 K | 0.23× | Parsing le plus coûteux |
| **TOML** (toml++) | ~300 K | 0.86× | Comparable à JSON |
| **XML** (pugixml) | ~500 K | 1.4× | Parsing in-situ |
| **MessagePack** | ~2 M | 5.7× | Décodage binaire, allocation |
| **Protobuf** | ~5 M | 14× | Code généré, décodage optimisé |
| **FlatBuffers** | ~∞ | — | Zero-copy : pas de désérialisation |
| **Cap'n Proto** | ~∞ | — | Zero-copy : pas de désérialisation |

Les entrées FlatBuffers et Cap'n Proto « ~∞ » reflètent le fait qu'il n'y a pas de phase de désérialisation. L'accès au premier champ est une opération O(1) — un calcul d'offset et un déréférencement. En pratique, le temps d'accès est de l'ordre de quelques nanosecondes, dominé par les accès mémoire au buffer.

La distinction entre FlatBuffers et Cap'n Proto à la désérialisation est subtile : les deux sont zero-copy, mais Cap'n Proto inclut des validations de sécurité (traversal limits, bounds checking sur les pointeurs) actives par défaut, ce qui ajoute quelques nanosecondes par accès. FlatBuffers propose un `Verifier` optionnel qu'on exécute en une passe avant de lire le message. En pratique, la différence est négligeable.

### Synthèse visuelle

```
Compacité (taille du message, plus petit = mieux)
├── Cap'n Proto pk  ████░░░░░░░░░░░░░░░░  (70 octets)
├── Protobuf        █████░░░░░░░░░░░░░░░  (82 octets)
├── MessagePack arr ██████░░░░░░░░░░░░░░  (95 octets)
├── FlatBuffers     ███████░░░░░░░░░░░░░  (112 octets)
├── Cap'n Proto     ████████░░░░░░░░░░░░  (120 octets)
├── MessagePack map █████████░░░░░░░░░░░  (145 octets)
├── JSON compact    █████████████░░░░░░░  (210 octets)
├── TOML            ██████████████░░░░░░  (230 octets)
└── XML             ████████████████████  (420 octets)

Vitesse de désérialisation (plus rapide = mieux)
├── FlatBuffers     ████████████████████  (zero-copy)
├── Cap'n Proto     ████████████████████  (zero-copy)
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
| **Cap'n Proto** | Non | Oui (`capnp`) | Moyenne | `libcapnp` + `libkj` + `capnp` |

Les formats textuels et MessagePack s'intègrent en quelques lignes de CMake. Protobuf, FlatBuffers et Cap'n Proto ajoutent une étape de build (génération de code) et une dépendance à un outil de build (`protoc`/`flatc`/`capnp`). Protobuf et Cap'n Proto nécessitent en plus une librairie runtime, tandis que le runtime FlatBuffers est header-only. Ce surcoût est justifié dans les projets à grande échelle, mais disproportionné pour un petit outil CLI ou un prototype.

Cap'n Proto a un coût d'intégration légèrement supérieur à Protobuf en raison de la double dépendance runtime (`libcapnp` pour la sérialisation, `libkj` pour les primitives asynchrones et les types utilitaires). Si le RPC intégré est utilisé, `libcapnp-rpc` s'ajoute à la liste. En contrepartie, le RPC ne nécessite pas un framework séparé comme gRPC.

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
          │                   ├── OUI → La latence est-elle critique ?
          │                   │         ├── OUI → IPC même machine ?
          │                   │         │         ├── OUI → Cap'n Proto
          │                   │         │         └── NON → Cap'n Proto ou FlatBuffers
          │                   │         └── NON → Protobuf
          │                   └── NON → MessagePack
          │
          └── NON → Le fichier est-il un stockage sur disque ?
                    ├── Accès aléatoire / mmap → FlatBuffers ou Cap'n Proto
                    ├── Stockage séquentiel compact → Protobuf
                    ├── Cache / données temporaires → MessagePack
                    └── Export pour analyse externe → JSON / CSV
```

### Cas fréquents résumés

**« Je construis un outil CLI qui lit un fichier de configuration. »**  
→ **TOML** si le format est libre, **YAML** si l'écosystème l'impose, **JSON** si l'interopérabilité avec d'autres outils prime.

**« Je développe des microservices qui communiquent entre eux. »**  
→ **Protobuf + gRPC** pour la communication synchrone. Le schéma `.proto` sert de contrat d'API, la rétrocompatibilité facilite les déploiements indépendants.

**« J'ai des services sur la même machine qui communiquent par IPC à haute fréquence. »**  
→ **Cap'n Proto**. Le zero-copy intégral (pas de sérialisation ni de désérialisation) et le RPC natif sans couche HTTP/2 minimisent la latence. Le promise pipelining réduit les allers-retours réseau pour les chaînes d'appels.

**« Je stocke des sessions utilisateur dans Redis. »**  
→ **MessagePack** en remplacement de JSON. Gain immédiat en taille et en vitesse, sans changement d'architecture. Pas de schéma à gérer.

**« Je développe un moteur de jeu qui charge des assets. »**  
→ **FlatBuffers** pour les données de jeu (niveaux, configurations d'entités). Le zero-copy via `mmap` élimine les temps de chargement. L'Object API simplifie l'édition dans les outils.

**« J'écris un proxy qui doit inspecter un champ de routage puis retransmettre le message. »**  
→ **Cap'n Proto** ou **FlatBuffers**. Les deux permettent de lire un champ sans désérialiser le message entier et de retransmettre le buffer brut. Cap'n Proto a l'avantage si le proxy doit aussi modifier un champ avant retransmission (builders mutables).

**« Je dois interfacer avec un système bancaire existant. »**  
→ **XML** (pugixml) — les standards financiers (ISO 20022, FIXML) imposent XML. Pas de choix, mais pugixml rend l'expérience supportable.

**« Je prototype rapidement un format d'échange entre deux services. »**  
→ **MessagePack** pour commencer (intégration triviale, pas de schéma), puis migration vers **Protobuf** si le format se stabilise et que la rigueur d'un schéma devient nécessaire.

**« J'ai besoin du format le plus compact possible pour des messages IoT sur un réseau contraint. »**  
→ **Protobuf** (le plus compact en standard) ou **Cap'n Proto packed** (le plus compact en absolu, mais impose la librairie Cap'n Proto aux deux extrémités). **Protobuf Lite / nanopb** pour les environnements très contraints.

**« J'écris une base de données qui stocke des enregistrements structurés accessibles par mmap. »**  
→ **Cap'n Proto** ou **FlatBuffers**. Les deux sont mmap-friendly. Cap'n Proto offre un RPC intégré et une évolution de schéma stricte au compile-time. FlatBuffers a un écosystème plus large (gaming, mobile).

---

## Combinaison de formats dans un projet réaliste

Un projet professionnel utilise rarement un seul format. Voici une architecture typique d'un service C++ Cloud Native et les formats associés à chaque couche :

```
┌───────────────────────────────────────────────────┐
│                Application C++                    │
│                                                   │
│  ┌───────────────┐  ┌─────────────────────────┐   │
│  │ Configuration │  │ Communication externe   │   │
│  │ TOML / YAML   │  │ Protobuf + gRPC         │   │
│  │ (au démarrage)│  │ (API inter-services)    │   │
│  └───────────────┘  └─────────────────────────┘   │
│                                                   │
│  ┌───────────────┐  ┌─────────────────────────┐   │
│  │ IPC interne   │  │ Stockage fichiers       │   │
│  │ Cap'n Proto   │  │ FlatBuffers (mmap)      │   │
│  │ (même machine)│  │ ou Protobuf (séquentiel)│   │
│  └───────────────┘  └─────────────────────────┘   │
│                                                   │
│  ┌───────────────┐  ┌─────────────────────────┐   │
│  │ Cache / Redis │  │ Logs & métriques        │   │
│  │ MessagePack   │  │ JSON structuré          │   │
│  │               │  │ ou MessagePack          │   │
│  └───────────────┘  └─────────────────────────┘   │
│                                                   │
│  ┌───────────────────────────────────────────┐    │
│  │ API externe (clients tiers)               │    │
│  │ JSON (REST) ou Protobuf (gRPC public)     │    │
│  └───────────────────────────────────────────┘    │
└───────────────────────────────────────────────────┘
```

Chaque format occupe la niche où il excelle :

- **TOML/YAML** pour la configuration humainement éditable.  
- **Protobuf/gRPC** pour la communication inter-services typée et performante.  
- **Cap'n Proto** pour l'IPC haute performance sur la même machine (entre workers, entre un proxy et un backend, entre un processus principal et ses enfants).  
- **MessagePack** pour le cache et les données semi-structurées transitoires.  
- **FlatBuffers** pour les fichiers de données à accès aléatoire.  
- **JSON** pour les API externes et les logs destinés aux outils d'agrégation.

La clé est d'isoler chaque format dans sa couche. Le code métier ne manipule que des types C++ natifs (`ServerConfig`, `User`, `Event`). Les conversions depuis/vers les formats de sérialisation sont confinées dans des couches d'adaptation dédiées, comme décrit en section 24.5.

---

## Matrice de décision récapitulative

| Critère | JSON | YAML | TOML | XML | MsgPack | Protobuf | FlatBuf. | Cap'n Proto |
|---------|:----:|:----:|:----:|:---:|:-------:|:--------:|:--------:|:-----------:|
| Lisibilité humaine | ★★★ | ★★★★ | ★★★★★ | ★★ | ✗ | ✗ | ✗ | ✗ |
| Commentaires | ✗ | ★★★★★ | ★★★★★ | ★★★★ | ✗ | ✗ | ✗ | ✗ |
| Compacité | ★★ | ★★ | ★★ | ★ | ★★★★ | ★★★★★ | ★★★★ | ★★★★ |
| Vitesse sérialisation | ★★ | ★ | ★★ | ★★ | ★★★★ | ★★★★★ | ★★★★ | ★★★★★ |
| Vitesse désérialisation | ★★ | ★ | ★★ | ★★★ | ★★★★ | ★★★★★ | ★★★★★ | ★★★★★ |
| Facilité d'intégration | ★★★★★ | ★★★★ | ★★★★★ | ★★★★ | ★★★★★ | ★★★ | ★★★ | ★★★ |
| Schéma formel | ★★★ | ★ | ★ | ★★★★★ | ✗ | ★★★★★ | ★★★★★ | ★★★★★ |
| Rétrocompatibilité | ★★ | ★★ | ★★ | ★★ | ★★ | ★★★★★ | ★★★★ | ★★★★★ |
| Écosystème / adoption | ★★★★★ | ★★★★ | ★★★ | ★★★★ | ★★★ | ★★★★★ | ★★★ | ★★ |
| Multi-langage | ★★★★★ | ★★★★ | ★★★ | ★★★★★ | ★★★★★ | ★★★★★ | ★★★★ | ★★★ |
| Zero-copy | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | Lecture | Lecture + écriture |
| RPC intégré | ✗ | ✗ | ✗ | ✗ | ✗ | gRPC (ext.) | ✗ | Oui (natif) |

---

## Principes à retenir

**Mesurer avant d'optimiser.** Ne pas choisir un format binaire « parce que c'est plus rapide ». Si le parsing JSON prend 0.1 % du temps total de traitement, passer à Protobuf n'apportera rien de mesurable. Utiliser le profiling (chapitre 31) pour identifier les vrais goulots d'étranglement.

**Le format le plus simple qui répond au besoin est le meilleur.** JSON et MessagePack n'exigent aucun schéma ni outil de build — leur simplicité d'intégration est une valeur en soi. Ne pas payer le coût d'un schéma Protobuf ou Cap'n Proto quand un `nlohmann::json` ou un `msgpack::pack` suffit.

**Séparer le format de sérialisation du code métier.** Les types C++ du domaine (`ServerConfig`, `User`) ne doivent pas dépendre du format de sérialisation. Une couche d'adaptation isole la conversion et permet de changer de format sans impacter la logique applicative.

**Le zero-copy n'est pas gratuit.** FlatBuffers et Cap'n Proto éliminent le coût de désérialisation, mais imposent que le buffer reste en mémoire tant qu'on accède aux données. Ce modèle de durée de vie (similaire à `std::string_view` ou `std::span`) exige plus de rigueur que les objets C++ indépendants produits par Protobuf. Le gain ne se justifie que si la désérialisation est un goulot d'étranglement mesuré.

**Planifier l'évolution.** Tout format finit par évoluer. Protobuf, FlatBuffers et Cap'n Proto ont des mécanismes intégrés (numéros de champs stables, détection de compatibilité). Pour JSON, YAML, TOML et MessagePack, la responsabilité incombe au développeur — d'où l'importance de la validation (section 24.5) et des valeurs par défaut raisonnables.

**Combiner les formats.** Un projet réaliste utilise plusieurs formats, chacun dans le contexte où il excelle. Ce n'est pas de l'incohérence — c'est du pragmatisme.

⏭️ [PARTIE IV : TOOLING ET BUILD SYSTEMS](/partie-04-tooling-build-systems.md)
