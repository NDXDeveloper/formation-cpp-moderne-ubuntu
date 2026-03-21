🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 25.3 Cap'n Proto : Zéro-copie sans étape d'encodage

## Introduction

Cap'n Proto est un système de sérialisation et de RPC créé par **Kenton Varda**, l'un des principaux auteurs de Protocol Buffers v2 chez Google. Son développement a démarré en 2013 avec un objectif radical : **éliminer entièrement l'étape d'encodage/décodage** qui constitue le goulot d'étranglement de Protobuf et de la quasi-totalité des formats de sérialisation.

L'idée fondatrice est simple à énoncer mais profonde dans ses implications : les données sont écrites directement dans un format qui est à la fois exploitable en mémoire *et* transmissible sur le réseau ou sur disque, sans transformation. Lire un message Cap'n Proto reçu par le réseau ne nécessite aucune désérialisation — on accède directement aux champs via des pointeurs dans le buffer reçu. Il n'y a pas de « parsing », pas d'allocation d'objets, pas de copie de données.

Ce principe de **zéro-copie** (zero-copy) distingue Cap'n Proto de manière fondamentale :

- **Protobuf** : encode les données dans un format wire compact (varint, length-delimited), puis les décode dans des objets C++ générés. Chaque direction (sérialisation et désérialisation) a un coût proportionnel à la taille du message.  
- **FlatBuffers** : permet la lecture zero-copy (pas de désérialisation), mais la construction d'un message nécessite un builder qui assemble les données dans un buffer plat. Le format wire n'est pas identique à la représentation mémoire d'une struct C.  
- **Cap'n Proto** : le format mémoire *est* le format wire. La construction et la lecture se font directement sur le même buffer, sans transformation dans aucune direction.

---

## Origines et motivations

Kenton Varda a travaillé sur Protobuf chez Google de 2007 à 2012. Son expérience directe des limitations de Protobuf a motivé la conception de Cap'n Proto. Les irritants principaux qu'il cherchait à résoudre :

**Le coût du parsing.** Dans les systèmes à haute performance (bases de données, proxies, pipelines de données), la sérialisation/désérialisation Protobuf peut consommer une part significative du temps CPU. Pour un proxy qui reçoit un message et le retransmet, Protobuf impose un cycle complet decode → ré-encode, même si le proxy ne modifie aucun champ.

**L'impossibilité du mmap.** On ne peut pas mapper un fichier Protobuf en mémoire et accéder directement aux champs — il faut parser le fichier entier d'abord. Pour une base de données qui stocke des millions d'enregistrements, cela interdit l'accès aléatoire efficace.

**La complexité du schéma d'évolution.** Bien que Protobuf supporte l'évolution des schémas (ajout de champs), les règles sont parfois contre-intuitives (changement required → optional, suppression de champs) et source de bugs en production.

Cap'n Proto a été conçu dès le départ pour adresser ces trois points, avec des garanties de compatibilité de schéma intégrées au format wire lui-même.

> 📝 Le nom « Cap'n Proto » est un jeu de mots sur « Captain Proto » (Protocol), avec une typographie volontairement décalée. Le projet est souvent abrégé en « capnp » dans les outils en ligne de commande.

---

## Positionnement dans l'écosystème

Le paysage des formats de sérialisation binaire en C++ comprend plusieurs acteurs majeurs, chacun avec ses compromis :

| Critère | Protobuf | FlatBuffers | Cap'n Proto | MessagePack |
|---------|----------|-------------|-------------|-------------|
| **Créateur** | Google | Google | Kenton Varda (ex-Google) | Sadayuki Furuhashi |
| **Année** | 2008 (open source) | 2014 | 2013 | 2008 |
| **Sérialisation** | Encode/Decode | Build / Zero-copy read | Zero-copy read+write | Encode/Decode |
| **Schéma** | `.proto` | `.fbs` | `.capnp` | Sans schéma |
| **Taille wire** | Compacte (varint) | Moyenne (alignée) | Moyenne (alignée) | Compacte |
| **Vitesse de parsing** | Moyenne | Rapide (zero-copy read) | Instantanée (pas de parsing) | Moyenne |
| **RPC intégré** | gRPC (séparé) | Non | Oui (natif) | Non |
| **Mmap-friendly** | Non | Oui | Oui | Non |
| **Adoption industrie** | Très large | Large (gaming, mobile) | Moyenne (niche perf.) | Large (web, Redis) |

Cap'n Proto se positionne comme le format le plus radical en termes de performance brute de sérialisation, au prix d'une adoption industrielle moins large que Protobuf et d'un écosystème de langages plus restreint.

---

## Caractéristiques clés

### Zéro-copie dans les deux directions

C'est la caractéristique déterminante. Ni la lecture ni l'écriture ne nécessitent de copie ou de transformation des données. Le buffer en mémoire est directement le message wire. Les implications pratiques :

- **Réception réseau** : un message reçu via `recv()` est directement exploitable. Pas d'allocation, pas de `ParseFromString()`, pas de construction d'objets.  
- **Envoi réseau** : le buffer construit en mémoire est directement envoyable via `send()`. Pas de `SerializeToString()`.  
- **Stockage disque** : un message peut être écrit sur disque et relu via `mmap` sans transformation.  
- **Proxy / forwarding** : un intermédiaire peut lire certains champs d'un message, le modifier partiellement, et le retransmettre sans jamais le désérialiser ni le resérialiser intégralement.

### RPC intégré

Contrairement à Protobuf qui dépend de gRPC (un projet séparé) pour le RPC, Cap'n Proto inclut un **système RPC natif** basé sur le même format wire. Ce système RPC supporte le pipelining de promesses : on peut chaîner des appels RPC sans attendre les résultats intermédiaires, réduisant drastiquement la latence des interactions multi-étapes.

### Évolution de schéma robuste

Cap'n Proto garantit la compatibilité ascendante et descendante des schémas par construction. Les champs sont identifiés par des numéros ordonnés, et le format wire encode des pointeurs explicites vers chaque champ. Un ancien lecteur ignore les champs inconnus (ajoutés après), et un nouveau lecteur voit des valeurs par défaut pour les champs absents (supprimés depuis).

Cette compatibilité est plus stricte que celle de Protobuf : certaines modifications dangereuses (changer le type d'un champ, réutiliser un numéro de champ supprimé) sont détectées au moment de la compilation du schéma, pas en production.

### Sécurité par design

Le format inclut des protections contre les messages malformés : limites de profondeur de traversée, limites sur le nombre de pointeurs suivis (pour éviter les structures circulaires ou exponentielles), et validation des bornes de pointeurs. Ces protections sont actives par défaut, sans surcoût significatif, et rendent Cap'n Proto plus sûr que la lecture directe de structures C.

---

## Cas d'usage privilégiés

Cap'n Proto excelle dans des contextes spécifiques où ses propriétés uniques apportent un avantage décisif :

**IPC haute performance.** Pour la communication inter-processus sur la même machine (via Unix sockets, pipes, ou shared memory), le zero-copy élimine une part significative de l'overhead. C'est l'usage d'origine du projet — Cloudflare utilise Cap'n Proto en interne pour l'IPC entre leurs workers.

**Stockage structuré mmap-friendly.** Les bases de données ou systèmes de fichiers structurés qui souhaitent mapper des enregistrements en mémoire et y accéder sans parsing bénéficient directement du format.

**Proxies et middleware.** Tout système qui reçoit un message, inspecte quelques champs (routage, authentification), et le retransmet. Avec Protobuf, cela nécessite un cycle complet parse→inspect→reserialize. Avec Cap'n Proto, l'inspection est directe et la retransmission est une simple copie du buffer original.

**Systèmes embarqués et temps réel.** L'absence d'allocation dynamique pendant la désérialisation est précieuse dans les environnements où les allocations sont interdites ou coûteuses (boucles temps réel, microcontrôleurs).

---

## Limites et compromis

Cap'n Proto n'est pas un remplacement universel de Protobuf. Ses compromis doivent être compris :

**Taille des messages.** Le format wire est aligné (padding pour l'alignement des champs) et utilise des pointeurs 64 bits. Les messages Cap'n Proto sont typiquement **20 à 50 % plus grands** que leurs équivalents Protobuf, qui utilisent un encodage varint compact. Si la bande passante réseau est le facteur limitant (WAN, mobile), Protobuf peut être un meilleur choix.

**Écosystème de langages.** Protobuf supporte officiellement une dizaine de langages avec des implémentations matures. Cap'n Proto a des implémentations de qualité en C++, Rust, Go et TypeScript, mais le support dans d'autres langages (Python, Java) est moins mature ou maintenu par la communauté.

**Adoption et recrutement.** Protobuf est un standard de facto dans l'industrie. La documentation, les tutoriels, les réponses sur Stack Overflow, et l'expérience des développeurs sont incomparablement plus riches pour Protobuf. Choisir Cap'n Proto impose un coût d'onboarding pour l'équipe.

**Complexité du modèle mémoire.** Le zero-copy impose que le message reste valide en mémoire tant qu'on accède à ses champs. On ne peut pas désérialiser dans un objet indépendant puis libérer le buffer réseau — le buffer *est* l'objet. Cela demande une gestion plus attentive des durées de vie, particulièrement en C++.

**Pas de compression native.** Protobuf utilise des varints qui compriment naturellement les petits entiers. Cap'n Proto stocke tous les entiers dans leur taille native (int32 = 4 octets, même pour la valeur 0). Une compression externe (zstd, lz4) peut compenser, mais ajoute une étape.

---

## Prérequis : installation sur Ubuntu

```bash
# Depuis les paquets (version stable)
sudo apt update  
sudo apt install capnproto libcapnp-dev  

# Vérifier l'installation
capnp --version  
capnpc-c++ --version  
```

Pour la dernière version depuis les sources :

```bash
git clone https://github.com/capnproto/capnproto.git  
cd capnproto/c++  
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release  
cmake --build build -j$(nproc)  
sudo cmake --install build  
```

Intégration CMake :

```cmake
find_package(CapnProto REQUIRED)

capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS schema/message.capnp)

add_executable(my_app main.cpp ${CAPNP_SRCS})  
target_link_libraries(my_app PRIVATE CapnProto::capnp)  
# Pour le RPC :
# target_link_libraries(my_app PRIVATE CapnProto::capnp-rpc)
```

---

## Plan de la section

Les sous-sections suivantes approfondissent Cap'n Proto en trois axes :

- **25.3.1 — Philosophie : le format wire EST le format mémoire** : le modèle de données de Cap'n Proto, la structure interne des messages (segments, pointeurs, data sections), et la démonstration concrète du zero-copy avec des exemples compilables.

- **25.3.2 — Schémas, génération de code et RPC intégré** : la syntaxe des fichiers `.capnp`, la génération de code C++, la manipulation des messages (construction et lecture), et le système RPC natif avec pipelining de promesses.

- **25.3.3 — Cap'n Proto vs FlatBuffers vs Protobuf : compromis** : une comparaison détaillée des trois formats sur les axes performance, taille wire, facilité d'usage, évolution de schéma et écosystème, avec des benchmarks et des recommandations par cas d'usage.

⏭️ [Philosophie : le format wire EST le format mémoire](/25-formats-binaires/03.1-philosophie-capnproto.md)
