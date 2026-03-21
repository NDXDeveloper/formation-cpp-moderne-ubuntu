🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Trajectoires professionnelles

## Six parcours concrets pour valoriser vos compétences C++

---

Les compétences acquises au cours de cette formation ouvrent des portes vers des domaines très différents. C++ reste l'un des rares langages qui se retrouve aussi bien dans le noyau d'un système d'exploitation que dans un moteur de trading haute fréquence, un firmware de capteur IoT, un pipeline CI/CD ou un moteur de jeu AAA.

Cette section explore six trajectoires professionnelles où le C++ est un atout central. Pour chacune, vous trouverez :

- une description du domaine et de ses exigences ;  
- les **modules clés** de la formation qui constituent votre socle ;  
- les **compétences complémentaires** à développer au-delà de cette formation ;  
- un aperçu du **marché et des perspectives** en 2026 ;  
- des **points d'entrée concrets** pour démarrer.

Ces trajectoires ne sont pas exclusives. Un ingénieur backend haute performance qui maîtrise la conteneurisation est aussi un profil DevOps/SRE. Un développeur système qui connaît gRPC et Protobuf peut évoluer vers la finance quantitative. Lisez les six parcours, identifiez celui qui résonne le plus avec vos intérêts, et construisez votre plan de progression en conséquence.

---

## 1. System Programming

### Le domaine

Le system programming couvre tout ce qui se situe entre le matériel et les applications : noyaux de systèmes d'exploitation, drivers, systèmes de fichiers, gestionnaires de mémoire, hyperviseurs, runtimes de langages, outils système (à la `systemd`, `coreutils`, `strace`). C'est le territoire historique de C et C++, où le contrôle sur les ressources matérielles — mémoire, CPU, I/O, interruptions — n'est pas un luxe mais une nécessité.

Un programmeur système écrit du code qui doit être correct dans des conditions adverses : mémoire limitée, exécution concurrente, signaux asynchrones, contraintes de latence strictes. Les bugs ne se manifestent pas par un message d'erreur dans un navigateur : ils se traduisent par des kernel panics, des corruptions de données ou des failles de sécurité exploitables.

### Modules clés de la formation

Les fondations de cette trajectoire reposent sur la **Partie III** dans son intégralité. La gestion de la mémoire (Module 2, chapitre 5) est votre base absolue : comprendre la stack, le heap, l'allocation dynamique, l'arithmétique des pointeurs, et savoir traquer les erreurs avec Valgrind et les sanitizers. La programmation système Linux (Module 7) est le cœur du parcours : manipulation du système de fichiers via POSIX, signaux, threads et synchronisation, sockets, multiplexage I/O avec `epoll`, IPC (fork/exec, pipes, shared memory, message queues).

La **Partie VI** complète votre profil avec l'optimisation CPU et mémoire (chapitre 41) — cache CPU, localité des données, SIMD, PGO, LTO — et la programmation bas niveau (chapitre 42) : inline assembly, manipulation de bits, memory ordering, structures lock-free. La sécurité (chapitre 45) est critique dans ce domaine : buffer overflows, integer overflows, use-after-free, compilation avec protections, fuzzing.

Le tooling avancé (Module 10) — GDB en profondeur, core dumps, sanitizers, profiling avec `perf`, flamegraphs — est votre quotidien dans cette trajectoire.

### Compétences complémentaires à développer

Le system programming demande des connaissances qui dépassent le cadre de cette formation, centrée sur l'espace utilisateur (userspace). Les axes de progression les plus importants sont les suivants.

**Architecture des systèmes d'exploitation.** Comprendre le fonctionnement d'un noyau — gestion de la mémoire virtuelle (pages, TLB, page faults), ordonnancement des processus, gestion des interruptions, appels système — est indispensable. Le livre *Operating Systems: Three Easy Pieces* (Arpaci-Dusseau) est un excellent point de départ, complété par la lecture du code source de Linux pour les plus motivés.

**Programmation noyau Linux.** Écrire des modules noyau, comprendre les mécanismes internes de Linux (VFS, scheduler CFS, netfilter, eBPF), maîtriser les outils de traçage noyau (ftrace, perf, bpftrace). Le livre *Linux Kernel Development* (Robert Love) reste une référence.

**Rust pour le system programming.** La trajectoire C++/Rust est de plus en plus valorisée. Le noyau Linux accepte du code Rust depuis la version 6.1, et plusieurs projets système majeurs (Redox OS, les drivers GPU) explorent activement cette voie. La section 43.3 de la formation (C++ et Rust) vous donne les bases de l'interopérabilité ; la maîtrise de Rust lui-même est un investissement à fort retour.

**eBPF.** La programmation eBPF (extended Berkeley Packet Filter) permet d'écrire du code qui s'exécute dans le noyau Linux de manière sûre, sans écrire de module noyau. C'est devenu un outil central pour l'observabilité, le networking et la sécurité système. Les librairies comme libbpf et les frontends comme bpftrace s'intègrent bien avec du code C++.

### Marché et perspectives 2026

Le system programming connaît un regain d'intérêt porté par plusieurs tendances : la montée en puissance de l'edge computing et des architectures hétérogènes (ARM, RISC-V), les exigences de sécurité renforcées par les régulateurs (Cyber Resilience Act), et la demande en infrastructure pour l'IA (runtimes, drivers GPU, systèmes de stockage distribués). Les profils qui combinent C++ système et connaissance de Rust sont particulièrement recherchés.

Les employeurs typiques sont les éditeurs de systèmes d'exploitation (Canonical, Red Hat, Microsoft, Apple, Google), les entreprises d'infrastructure cloud (AWS, GCP, Azure), les fabricants de matériel (Intel, AMD, NVIDIA, Qualcomm, ARM) et les startups spécialisées en sécurité ou en virtualisation.

### Points d'entrée

Contribuer à des projets open source est le chemin le plus direct. Commencez par des outils en espace utilisateur (`systemd`, `util-linux`, `iproute2`) avant de vous attaquer au noyau. Reproduisez des outils système simples (`ls`, `cat`, `cp`) en C++ moderne pour ancrer vos compétences, puis attaquez des projets plus ambitieux : un allocateur mémoire, un mini-shell, un serveur HTTP minimaliste avec `epoll`.

---

## 2. Backend haute performance

### Le domaine

Le développement backend haute performance concerne les systèmes serveur où la latence, le débit et l'utilisation efficace des ressources sont des contraintes de premier plan. On parle de bases de données (ClickHouse, ScyllaDB, RocksDB), de serveurs de cache (Redis, Memcached), de message brokers (Kafka, ZeroMQ), de moteurs de recherche (Elasticsearch utilise des composants C++ via Lucene), de serveurs web et de passerelles API, de systèmes de stockage distribué, et plus généralement de toute infrastructure qui doit traiter des millions de requêtes par seconde avec des latences inférieures à la milliseconde.

C++ excelle dans ce domaine parce qu'il offre un contrôle fin sur la mémoire, la concurrence et les I/O, tout en permettant des abstractions de haut niveau. Un serveur écrit en C++ peut saturer une carte réseau 100 Gbps là où une implémentation en langage managé buterait sur le garbage collector.

### Modules clés de la formation

Le networking (chapitre 22) est le pilier : sockets TCP/UDP, multiplexage I/O avec `epoll`, Asio (standalone ou Boost), clients HTTP, gRPC et Protocol Buffers. La programmation concurrente (chapitre 21) est omniprésente : threads, synchronisation, atomiques, `std::jthread`, algorithmes parallèles.

Les formats de données (Module 8) sont quotidiens dans le backend : JSON (nlohmann/json), YAML pour la configuration, Protobuf et FlatBuffers pour la sérialisation performante entre services.

L'optimisation (Module 14) distingue un backend fonctionnel d'un backend performant : cache CPU et data-oriented design, branch prediction, SIMD pour le parsing (simdjson est un excellent exemple), PGO et LTO pour les builds de production. Les conteneurs `std::flat_map`/`std::flat_set` sont conçus exactement pour les scénarios de lookup fréquent que l'on retrouve côté serveur.

Le tooling DevOps (Module 13) complète le profil : dockerisation, CI/CD, observabilité (spdlog, Prometheus, OpenTelemetry), health checks. Un backend C++ en production sans métriques et sans logging structuré n'est pas un backend de production.

### Compétences complémentaires à développer

**Architecture distribuée.** Comprendre les patterns de systèmes distribués — consensus (Raft, Paxos), réplication, sharding, eventual consistency, circuit breakers — est essentiel. Le livre *Designing Data-Intensive Applications* (Martin Kleppermann) est la référence incontournable.

**Frameworks réseau spécialisés.** Au-delà d'Asio, des frameworks comme Seastar (utilisé par ScyllaDB) adoptent un modèle shared-nothing et thread-per-core qui pousse la performance à l'extrême. Le modèle io_uring pour les I/O asynchrones sur Linux est de plus en plus utilisé dans les serveurs haute performance.

**Bases de données.** Comprendre les structures de données internes (B-trees, LSM-trees, bloom filters, skip lists), le fonctionnement des moteurs de stockage, et les protocoles de réplication. Contribuer à un projet comme RocksDB est un accélérateur de carrière considérable.

**Kubernetes et orchestration.** Un backend haute performance déployé en production s'exécute presque toujours dans un cluster Kubernetes. Comprendre les concepts (pods, services, deployments, HPA, resource limits) et savoir configurer les probes de santé et les métriques Prometheus est indispensable.

### Marché et perspectives 2026

La demande en backends haute performance est portée par la croissance du volume de données, les exigences de latence des applications temps réel (streaming, gaming, trading), et l'infrastructure nécessaire aux systèmes d'IA (serving de modèles, vector databases, pipelines de données). Les entreprises qui développent des bases de données, des systèmes de messaging ou des infrastructures cloud recrutent activement des profils C++ backend.

Les salaires dans ce domaine sont parmi les plus élevés du développement logiciel, reflétant la rareté des profils qui combinent maîtrise du C++, compréhension des systèmes distribués et culture opérationnelle.

### Points d'entrée

Implémentez un serveur TCP multi-threadé avec `epoll`, puis ajoutez-y un protocole (HTTP/1.1, Redis RESP, ou un protocole binaire custom). Profilez-le avec `perf`, optimisez-le, dockerisez-le, ajoutez des métriques Prometheus. Ensuite, étudiez le code source de projets comme Redis, RocksDB ou ClickHouse — ils sont open source et richement documentés.

---

## 3. Embedded / IoT

### Le domaine

L'embedded (embarqué) et l'IoT (Internet of Things) couvrent le développement logiciel pour des dispositifs à ressources contraintes : microcontrôleurs, capteurs, systèmes embarqués temps réel, passerelles IoT, équipements industriels, systèmes automobiles (ADAS, infotainment), dispositifs médicaux, drones. Les contraintes sont radicalement différentes du développement serveur : mémoire limitée (parfois quelques kilo-octets), absence de système d'exploitation ou OS temps réel (FreeRTOS, Zephyr), exigences de consommation énergétique, certifications de sécurité (IEC 62443, ISO 26262, DO-178C).

C++ est le langage dominant dans l'embarqué de taille moyenne et supérieure (ARM Cortex-M4+, Cortex-A, processeurs automobiles), où il apporte les abstractions de la POO et du C++ moderne tout en permettant un contrôle fin sur les ressources matérielles. Le C pur reste présent sur les microcontrôleurs les plus petits, mais le C++ moderne gagne du terrain grâce à ses abstractions zero-cost.

### Modules clés de la formation

Les fondamentaux du langage (Module 2) sont particulièrement critiques en embarqué : compréhension précise des types, de la taille et de l'alignement (`sizeof`, `alignof`, types à largeur fixe), `constexpr` et `consteval` pour le calcul à la compilation, maîtrise de la mémoire (stack vs heap, allocation dynamique, pointeurs). En embarqué, chaque octet compte et chaque allocation dynamique est suspecte.

La programmation orientée objet (Module 3) s'applique avec des contraintes : on privilégie le polymorphisme statique (CRTP, templates) au polymorphisme dynamique (vtable) pour éviter l'indirection et l'overhead mémoire. La Règle des 5 et le RAII sont essentiels pour garantir la gestion déterministe des ressources.

Le C++ moderne (Module 4) est applicable en embarqué, à condition de choisir les bonnes fonctionnalités : `std::optional` et `std::expected` (pas d'allocation dynamique), `std::span` (vue sans copie), `std::array` (taille fixe), Concepts pour les erreurs de compilation lisibles. En revanche, `std::shared_ptr` (overhead du comptage de références) et les exceptions (coût en taille de binaire) sont souvent évités.

L'optimisation (Module 14) et la programmation bas niveau (chapitre 42) — manipulation de bits, inline assembly, memory ordering — sont directement applicables. La sécurité (chapitre 45) est critique : les dispositifs embarqués sont des cibles d'attaque de plus en plus fréquentes, et les protections de compilation (`-fstack-protector`, ASLR) ne sont pas toujours disponibles.

La cross-compilation (section 38.6) est un savoir-faire quotidien : vous compilez sur x86_64 pour des cibles ARM ou RISC-V, avec des toolchains croisées gérées via CMake.

### Compétences complémentaires à développer

**Systèmes d'exploitation temps réel (RTOS).** FreeRTOS, Zephyr, et RT-Linux sont les plateformes cibles. Comprendre l'ordonnancement temps réel (priorités, préemption, deadlines), les mécanismes de communication inter-tâches (queues, sémaphores, event groups) et les contraintes de déterminisme est fondamental.

**Protocoles de communication embarqués.** I²C, SPI, UART, CAN (automobile), MQTT et CoAP (IoT), BLE (Bluetooth Low Energy). Chaque domaine a ses protocoles, et savoir les implémenter ou les intégrer en C++ est un prérequis.

**Hardware et électronique de base.** Lire un schéma électronique, comprendre un datasheet de microcontrôleur, utiliser un oscilloscope et un analyseur logique. Vous n'avez pas besoin d'être ingénieur électronique, mais l'interface entre le logiciel et le matériel doit être familière.

**Normes et certifications.** Selon le secteur — automobile (ISO 26262, AUTOSAR), aéronautique (DO-178C), médical (IEC 62304), industriel (IEC 61508) — des standards de développement stricts encadrent le code embarqué. Les sous-ensembles de C++ autorisés (MISRA C++, AUTOSAR C++14 Guidelines, et leur évolution vers C++ moderne) définissent ce que vous pouvez et ne pouvez pas utiliser.

**CMake pour l'embarqué et toolchains croisées.** La section 26.6 de la formation pose les bases. En pratique, configurer un toolchain file CMake pour un ARM Cortex-M avec un linker script custom, gérer les options de compilation spécifiques au target (FPU, instruction set), et intégrer un SDK constructeur (STM32CubeMX, ESP-IDF, Nordic nRF SDK) demande une expérience spécifique.

### Marché et perspectives 2026

L'embarqué et l'IoT sont en expansion continue, portés par l'automobile connectée et autonome, l'industrie 4.0, la santé connectée et l'edge AI (inférence de modèles directement sur les dispositifs). L'arrivée de processeurs RISC-V ouvre de nouvelles opportunités, et la montée en puissance de Zephyr comme RTOS de référence (soutenu par la Linux Foundation) crée un écosystème de plus en plus standardisé.

Les profils C++ embarqué qui maîtrisent aussi le C++ moderne (au lieu de se limiter au C++03 encore courant dans l'industrie) et qui comprennent les enjeux de cybersécurité IoT sont particulièrement recherchés.

### Points d'entrée

Procurez-vous une carte de développement (STM32 Nucleo, ESP32, Raspberry Pi Pico) et implémentez des projets concrets : lecture de capteurs, communication UART/I²C, serveur MQTT, OTA update. Faites-le en C++ moderne, avec CMake et une toolchain croisée. Le framework PlatformIO simplifie considérablement l'environnement de développement pour débuter, avant de passer à des configurations CMake manuelles pour les projets plus exigeants.

---

## 4. Finance quantitative

### Le domaine

La finance quantitative (quant finance) utilise C++ pour les systèmes de trading haute fréquence (HFT), les moteurs de pricing d'instruments financiers (options, dérivés, produits structurés), les systèmes de gestion des risques, les plateformes d'exécution d'ordres, et les simulateurs de marché (Monte Carlo, modèles stochastiques). C'est un domaine où la latence se mesure en microsecondes (voire en nanosecondes pour le HFT), où la justesse numérique est critique, et où un bug peut coûter des millions.

C++ domine ce secteur pour une raison simple : aucun autre langage ne combine la même performance brute, le contrôle sur la mémoire, le déterminisme d'exécution (pas de garbage collector), et un écosystème mathématique mature (Eigen, QuantLib, Boost.Math). Python est omniprésent pour le prototypage et l'analyse, mais les composants sensibles à la latence sont systématiquement écrits en C++.

### Modules clés de la formation

L'optimisation (Module 14) est le module le plus critique pour cette trajectoire. La connaissance du cache CPU, du data-oriented design, de la branch prediction, du SIMD et de la vectorisation, du PGO et du LTO est directement exploitable. La programmation lock-free (chapitre 42) — compare-and-swap, structures sans verrou, memory ordering — est au cœur des moteurs de trading où chaque nanoseconde de contention sur un mutex est inacceptable.

La programmation concurrente (chapitre 21) est omniprésente : les systèmes de trading sont multi-threadés par nature (threads de réception de données de marché, threads de calcul, threads d'exécution d'ordres). Les atomiques, le memory ordering et les algorithmes parallèles sont des outils quotidiens.

Le networking (chapitre 22) est essentiel : sockets TCP/UDP en mode non-bloquant, `epoll` pour le multiplexage, protocoles binaires custom pour la communication avec les exchanges. Le parsing de formats binaires (chapitre 25) — Protobuf, FlatBuffers — est utilisé pour la sérialisation des messages internes.

Les templates et la métaprogrammation (chapitre 16) permettent de créer des abstractions zero-cost pour les types financiers (prix, quantités, dates, identifiants d'instruments) avec validation à la compilation. Les Concepts (C++20) rendent ces templates maintenables. `constexpr` et `consteval` permettent de précalculer des tables à la compilation.

Le C++ moderne (Module 4) dans son ensemble est applicable : move semantics pour éviter les copies dans les chemins critiques, `std::expected` pour la gestion d'erreurs sans le coût des exceptions sur le chemin de succès, `std::flat_map` pour les lookups cache-friendly sur les carnets d'ordres.

### Compétences complémentaires à développer

**Mathématiques financières.** Calcul stochastique (mouvement brownien, équations différentielles stochastiques), modèles de pricing (Black-Scholes, modèles locaux de volatilité, Monte Carlo), gestion des risques (VaR, Greeks). Le livre *Options, Futures, and Other Derivatives* (John Hull) est le point de départ classique ; *C++ Design Patterns and Derivatives Pricing* (Mark Joshi) fait le pont entre la théorie financière et l'implémentation C++.

**Librairies spécialisées.** QuantLib (pricing et modélisation financière open source en C++), Eigen (algèbre linéaire haute performance), Boost.Math (fonctions mathématiques spéciales). L'interopérabilité C++/Python via pybind11 (section 43.2) est particulièrement valorisée : les quants prototypent en Python et les développeurs C++ implémentent les composants critiques.

**Architecture de systèmes à faible latence.** Kernel bypass (DPDK, Solarflare OpenOnload), affinité CPU (cpu pinning), NUMA-aware allocation, réseaux FPGA pour l'accélération matérielle, protocoles FIX et ITCH pour la communication avec les marchés. C'est un domaine très spécialisé où l'expertise se construit sur le terrain.

**Calcul numérique et précision.** Comprendre les limites de la représentation flottante IEEE 754, les erreurs d'arrondi, les techniques de sommation compensée (Kahan), et les cas où `double` ne suffit pas. En finance, une erreur d'arrondi cumulée peut fausser un P&L de plusieurs millions.

### Marché et perspectives 2026

La finance quantitative reste l'un des domaines les mieux rémunérés pour les développeurs C++. Les hedge funds (Citadel, Two Sigma, Jane Street, DE Shaw), les banques d'investissement (Goldman Sachs, Morgan Stanley, JP Morgan), les exchanges (CME, Eurex, LSEG) et les firmes de trading propriétaire recrutent continuellement. L'expansion du trading algorithmique vers les crypto-monnaies et les marchés émergents élargit le marché.

Le profil idéal combine une solide compétence C++ bas niveau avec des connaissances mathématiques et une compréhension des marchés financiers. Les juniors entrent souvent via des rôles de développeur quantitatif (quant dev), où l'accent est mis sur l'implémentation performante de modèles conçus par les quants/chercheurs.

### Points d'entrée

Implémentez un pricer d'options en C++ (Black-Scholes analytique, puis Monte Carlo), en portant une attention particulière à la performance (profilez avec `perf`, optimisez la localité mémoire, vectorisez les boucles critiques). Construisez un carnet d'ordres en mémoire (order book) avec insertion et suppression en O(1), et simulez un flux de données de marché. Contribuez à QuantLib pour vous familiariser avec une codebase financière de production. Le site QuantNet et les forums Wilmott sont de bonnes ressources communautaires.

---

## 5. DevOps / SRE

### Le domaine

Le DevOps (Development Operations) et le SRE (Site Reliability Engineering) se situent à l'intersection du développement et des opérations. Le rôle consiste à construire et maintenir l'infrastructure qui permet aux applications de fonctionner en production de manière fiable, scalable et observable. C'est un domaine souvent associé à Go, Python et Bash, mais C++ y occupe une place structurante que beaucoup sous-estiment.

C++ intervient dans le DevOps/SRE de deux manières. Premièrement, de nombreux outils d'infrastructure critiques sont écrits en C++ : bases de données (MySQL, PostgreSQL, MongoDB, ClickHouse, ScyllaDB), caches (Redis, Memcached), proxies et load balancers (Envoy), runtimes de conteneurs (composants de containerd, crun), outils de monitoring (collectd, certains exporters Prometheus), et outils système (systemd). Un SRE qui sait lire, déboguer et profiler du code C++ peut diagnostiquer des problèmes que d'autres ne font que contourner.

Deuxièmement, écrire des outils internes en C++ est pertinent quand la performance compte : agents de monitoring avec un footprint mémoire minimal, parsers de logs à haut débit, outils CLI qui doivent s'exécuter en millisecondes, extensions ou plugins pour des systèmes existants écrits en C++.

### Modules clés de la formation

La **Partie V** (DevOps et Cloud Native) est évidemment centrale. La dockerisation (chapitre 37) — multi-stage builds, images distroless, gestion des librairies partagées — est la brique de base. Les pipelines CI/CD (chapitre 38) sur GitLab CI et GitHub Actions, avec ccache/sccache pour l'accélération, matrix builds multi-compilateur, et cross-compilation, constituent votre workflow quotidien. Le packaging (chapitre 39) — DEB, RPM, AppImage — vous permet de distribuer vos outils. L'observabilité (chapitre 40) — spdlog, Prometheus, OpenTelemetry, structured logging JSON — est le cœur du métier SRE.

La création d'outils CLI (Module 12) est directement applicable : CLI11 pour le parsing d'arguments professionnel, fmt pour le formatage, architecture modulaire à la `kubectl` ou `git`.

Le débogage et le profiling (Module 10) sont vos armes pour le diagnostic en production : GDB sur des core dumps de processus serveur, sanitizers pour reproduire les bugs, `perf` pour identifier les points chauds, flamegraphs pour la visualisation.

Le networking (chapitre 22) est pertinent pour comprendre et déboguer les systèmes distribués : sockets, protocoles, gRPC (utilisé par Kubernetes, Envoy, etcd), Protocol Buffers.

### Compétences complémentaires à développer

**Kubernetes en profondeur.** L'orchestration de conteneurs est le centre de gravité du DevOps moderne. Comprendre l'architecture de Kubernetes (API server, etcd, kubelet, kube-proxy), configurer des deployments, services, ingress, HPA, PDB, et maîtriser Helm pour le packaging. Le CKA (Certified Kubernetes Administrator) est une certification reconnue.

**Infrastructure as Code.** Terraform, Pulumi ou OpenTofu pour provisionner l'infrastructure, Ansible pour la configuration. Savoir décrire une infrastructure complète de manière déclarative et versionnable.

**Observabilité à grande échelle.** La stack Prometheus/Grafana/Loki/Tempo est le standard de facto. Savoir instrumenter une application C++ avec des métriques custom, configurer des alertes, corréler logs/métriques/traces dans un système distribué.

**Go.** La grande majorité des outils DevOps modernes (Docker, Kubernetes, Terraform, Prometheus, Grafana Agent) sont écrits en Go. Maîtriser Go en complément du C++ vous donne la capacité de contribuer à l'écosystème DevOps natif tout en apportant votre expertise C++ pour les composants critiques en performance.

**Scripting avancé.** Bash et Python restent indispensables pour l'automatisation rapide, le glue code et les scripts opérationnels. Votre formation C++ vous donne un avantage structurel (rigueur, compréhension système), mais les tâches d'automatisation quotidiennes se font rarement en C++.

### Marché et perspectives 2026

Le DevOps et le SRE sont parmi les rôles les plus demandés en informatique. Le profil "DevOps + C++" est un différenciateur rare : la plupart des DevOps viennent d'un background Python/Go/sysadmin et n'ont pas la capacité de plonger dans le code C++ de l'infrastructure qu'ils opèrent. Si vous savez à la fois écrire un pipeline CI/CD pour un projet C++ et déboguer un core dump d'un serveur gRPC en production, vous occupez une niche très valorisée.

Les entreprises qui déploient des systèmes critiques en C++ (finance, télécoms, gaming, infrastructure cloud) ont un besoin spécifique de SRE capables de comprendre le comportement bas niveau de leurs applications.

### Points d'entrée

Commencez par conteneuriser un projet C++ existant avec un Dockerfile multi-stage et une image distroless. Configurez un pipeline CI/CD complet (build, test, analyse statique, packaging) sur GitHub Actions. Instrumentez l'application avec spdlog et des métriques Prometheus, déployez-la sur un cluster Kubernetes local (minikube ou kind), et configurez des dashboards Grafana et des alertes. Ce cycle complet — du code au monitoring en production — est exactement ce que l'on attend d'un profil DevOps/SRE.

---

## 6. Game Development / Moteurs 3D

### Le domaine

Le développement de jeux vidéo et de moteurs 3D est le domaine le plus visible du C++ grand public. Les deux principaux moteurs de jeu du marché — Unreal Engine (Epic Games, entièrement en C++) et les composants natifs de Unity (runtime C++) — sont écrits en C++. Les moteurs propriétaires des grands studios (DICE Frostbite, Rockstar RAGE, Naughty Dog engine, id Tech) le sont également. Au-delà du jeu vidéo, les moteurs 3D C++ sont utilisés en simulation (défense, aéronautique), en visualisation architecturale, en réalité virtuelle/augmentée et en production cinématographique.

Le game dev en C++ est exigeant : il combine programmation graphique (OpenGL, Vulkan, DirectX), physique temps réel, intelligence artificielle, networking multijoueur, audio, gestion de ressources (assets), scripting, outils de création de contenu, et le tout doit tourner à 60 FPS (voire 120 FPS) sans à-coups. C'est un environnement où la performance n'est pas une optimisation a posteriori mais une contrainte architecturale de chaque instant.

### Modules clés de la formation

Les fondamentaux du langage (Modules 2-3) sont critiques : la POO est omniprésente dans les architectures de moteur (hiérarchies d'entités, systèmes de composants), la gestion de la mémoire (allocateurs custom, pools, arenas) est un sujet central, et la Règle des 5 / RAII sont incontournables.

Le C++ moderne (Module 4) s'applique pleinement : move semantics pour le transfert de ressources graphiques (textures, buffers), smart pointers pour la gestion de la durée de vie des objets de jeu (avec les nuances d'utilisation — `std::unique_ptr` est préféré ; `std::shared_ptr` est utilisé avec parcimonie à cause de l'overhead), lambdas pour les callbacks et les systèmes d'événements.

Les templates (chapitre 16) sont utilisés massivement : systèmes de sérialisation, Entity Component Systems (ECS) génériques, conteneurs spécialisés. Le CRTP (chapitre 44) et le type erasure sont des patterns courants dans les moteurs.

L'optimisation (Module 14) est votre avantage compétitif : cache CPU et data-oriented design (la base de l'architecture ECS), SIMD pour les mathématiques 3D (transformations matricielles, intersection ray/triangle), branch prediction pour les boucles de rendu, PGO et LTO pour les builds release. La programmation concurrente (chapitre 21) est essentielle pour exploiter les CPU multi-cœurs modernes : job systems, task graphs, parallélisation du rendu et de la physique.

Le profiling (chapitre 31) est un outil quotidien : identifier les goulots d'étranglement GPU et CPU par frame, optimiser les budgets de temps (16.6ms par frame à 60 FPS), traquer les stutters et les allocations inutiles.

Les design patterns (chapitre 44) sont omniprésents en game dev : Factory pour la création d'entités, Observer pour les événements, Strategy pour les comportements d'IA, Command pour les systèmes d'input et d'undo, State machines pour les animations et la logique de jeu.

### Compétences complémentaires à développer

**Programmation graphique.** OpenGL (en déclin mais toujours enseigné), Vulkan (le standard moderne, beaucoup plus complexe mais beaucoup plus performant), et les concepts fondamentaux : pipeline de rendu, shaders (vertex, fragment, compute), transformations 3D (matrices, quaternions), éclairage (PBR), gestion des textures et des framebuffers. Le livre *Real-Time Rendering* (Akenine-Möller, Haines, Hoffman) est la bible du domaine.

**Mathématiques 3D.** Algèbre linéaire (vecteurs, matrices, transformations), géométrie (intersection, projection, clipping), quaternions pour les rotations, interpolation (LERP, SLERP), courbes (Bézier, splines). La librairie GLM (OpenGL Mathematics) fournit les types et fonctions de base compatibles avec le style GLSL.

**Moteurs de jeu existants.** Apprendre Unreal Engine (C++ et Blueprints) ou Godot (dont le cœur est en C++) vous donne un contexte pratique immédiat. Unreal est le plus pertinent pour un profil C++ : son API est vaste et ses conventions (UObject, garbage collector, macros de réflexion) demandent un apprentissage spécifique, mais c'est l'outil standard de l'industrie AAA.

**Entity Component System (ECS).** L'architecture ECS — où les entités sont de simples identifiants, les composants sont des données pures, et les systèmes traitent ces données — est le paradigme dominant dans les moteurs modernes. Des frameworks C++ comme EnTT implémentent ce pattern de manière cache-friendly et sont un excellent sujet d'étude.

**Audio et physique.** Les moteurs de physique (Bullet, PhysX, Jolt) et les moteurs audio (FMOD, Wwise, OpenAL) sont des composants majeurs. Comprendre leurs API et leurs contraintes de performance (physique à pas de temps fixe, audio en temps réel avec des buffers de quelques millisecondes) élargit votre profil.

### Marché et perspectives 2026

L'industrie du jeu vidéo est la plus grande industrie du divertissement en termes de revenus. La demande en programmeurs C++ gameplay, engine, graphics et tools est soutenue, dans les grands studios (Ubisoft, EA, Epic, Naughty Dog, Rockstar) comme dans les studios indépendants qui travaillent avec Unreal ou leurs propres moteurs.

Au-delà du jeu, les compétences en moteur 3D C++ sont de plus en plus valorisées dans la simulation (jumeaux numériques, formation militaire, simulation chirurgicale), la réalité augmentée et virtuelle (Meta, Apple Vision Pro, Varjo), et l'industrie cinématographique (virtual production, LED walls). L'arrivée du ray tracing matériel et de l'IA générative dans le pipeline graphique (DLSS, FSR, Nanite) ouvre de nouveaux axes de spécialisation.

Le principal défi de ce domaine est la culture de l'industrie : les horaires peuvent être exigeants (crunch), et les salaires, bien que compétitifs, sont souvent inférieurs à ceux de la finance ou du backend infrastructure pour un niveau de compétence technique comparable. C'est un secteur où la passion pour le produit final est un facteur de motivation important.

### Points d'entrée

Commencez par un projet graphique minimal : un triangle rendu avec Vulkan ou OpenGL, puis une scène 3D avec caméra, éclairage et textures. Utilisez les librairies GLFW (fenêtre et input) et GLM (mathématiques). Implémentez ensuite un mini-moteur : boucle de jeu, ECS basique, rendu de sprites ou de meshes, système d'input. Ce projet intégrateur mobilise quasiment toutes les compétences de la formation.

Pour une entrée plus directe dans l'industrie, apprenez Unreal Engine avec un focus sur le C++ (pas uniquement les Blueprints) : créez un gameplay feature en C++, exposez-le aux designers via les Blueprints, et maîtrisez le cycle de vie des objets UE (UObject, UActorComponent, AActor).

---

## Synthèse croisée

Le tableau suivant met en relation les trajectoires avec les modules les plus différenciants. Les modules fondamentaux (1-3) sont un prérequis commun à toutes les trajectoires et ne sont pas répétés.

| Module / Chapitre | System | Backend | Embedded | Finance | DevOps | Game Dev |
|---|---|---|---|---|---|---|
| **Smart pointers, Move semantics** (9-10) | ● | ● | ◐ | ● | ○ | ● |
| **C++17/20/23/26** (12) | ● | ● | ◐ | ● | ○ | ◐ |
| **STL et algorithmes** (13-16) | ● | ● | ◐ | ● | ○ | ● |
| **Gestion d'erreurs** (17-18) | ● | ● | ● | ● | ○ | ● |
| **Filesystem, signaux POSIX** (19-20) | ● | ◐ | ◐ | ○ | ● | ○ |
| **Threads et concurrence** (21) | ● | ● | ◐ | ● | ◐ | ● |
| **Networking, gRPC** (22) | ◐ | ● | ◐ | ● | ● | ◐ |
| **IPC** (23) | ● | ◐ | ◐ | ○ | ◐ | ○ |
| **Parsing JSON/YAML/Protobuf** (24-25) | ◐ | ● | ◐ | ● | ● | ◐ |
| **CMake avancé** (26-28) | ● | ● | ● | ◐ | ● | ● |
| **Débogage, profiling** (29-31) | ● | ● | ● | ● | ● | ● |
| **Analyse statique, tests** (32-35) | ● | ● | ● | ● | ● | ● |
| **Outils CLI** (36) | ● | ◐ | ○ | ○ | ● | ○ |
| **Docker, CI/CD, packaging** (37-39) | ◐ | ● | ◐ | ◐ | ● | ◐ |
| **Observabilité** (40) | ◐ | ● | ○ | ● | ● | ◐ |
| **Optimisation CPU/mémoire** (41) | ● | ● | ● | ● | ○ | ● |
| **Programmation bas niveau** (42) | ● | ◐ | ● | ● | ○ | ◐ |
| **Interopérabilité** (43) | ● | ◐ | ◐ | ● | ○ | ◐ |
| **Design patterns** (44) | ◐ | ● | ◐ | ◐ | ○ | ● |
| **Sécurité** (45) | ● | ● | ● | ● | ● | ◐ |

*● = critique · ◐ = important · ○ = utile mais secondaire*

---

Quelle que soit la trajectoire choisie, deux constantes demeurent : la nécessité de pratiquer intensivement sur des projets concrets, et l'importance de rester à jour dans un écosystème qui évolue rapidement. La section suivante — **[Ressources pour aller plus loin](/conclusion/03-ressources.md)** — vous fournit les outils pour les deux.

---


⏭️ [Ressources pour aller plus loin](/conclusion/03-ressources.md)
