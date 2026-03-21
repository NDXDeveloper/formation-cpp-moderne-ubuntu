🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 48.3 — Standards et évolutions futures (C++26 en ratification, cap sur C++29)

## Chapitre 48 : Ressources et Veille Technologique

---

## Objectifs de la section

- Comprendre le fonctionnement du comité de standardisation ISO C++ (WG21)  
- Maîtriser le calendrier de publication des standards et le cycle triennal  
- Savoir lire, trouver et suivre les proposals qui façonnent le futur du langage  
- Acquérir une visibilité sur les premiers travaux en direction de C++29

---

## Le C++ : un langage piloté par un processus ouvert

Le C++ est un cas particulier dans le paysage des langages de programmation. Contrairement à Python (piloté par la Python Software Foundation et historiquement par son BDFL), Rust (piloté par la Rust Foundation et ses équipes), ou Go (piloté par Google), le C++ est un **standard international ISO** dont l'évolution est gouvernée par un comité ouvert composé de volontaires, de représentants d'entreprises, d'universitaires et d'individus passionnés.

Ce modèle a des conséquences profondes sur la façon dont le langage évolue. Il n'y a pas de "propriétaire" du C++ qui décide unilatéralement de la direction du langage. Chaque fonctionnalité est le résultat de proposals écrites, de discussions techniques, de révisions multiples, et de votes formels. Le processus est lent — une idée met typiquement trois à dix ans entre sa première proposition et son intégration dans le standard — mais il produit des résultats remarquablement stables. Une fonctionnalité qui survit au processus de standardisation C++ a été examinée sous tous les angles : performance, compatibilité ascendante, interactions avec le reste du langage, implémentabilité dans les compilateurs, et pertinence pratique.

Pour le développeur, comprendre ce processus n'est pas une curiosité académique. C'est un **outil de veille stratégique**. En suivant les proposals et les discussions du comité, on peut anticiper ce qui arrivera dans le prochain standard, préparer sa codebase en conséquence, et prendre des décisions d'architecture éclairées. Un architecte qui savait en 2022 que les Contrats convergeaient vers C++26 a pu structurer son code pour en tirer parti dès la ratification. Un développeur qui suit aujourd'hui les discussions sur les extensions de la réflexion peut anticiper les patterns qui deviendront idiomatiques en C++29.

---

## Où en sommes-nous en mars 2026 ?

### C++26 : le standard en phase de ratification

C++26 est en cours de ratification par l'ISO en 2026 (ballot final prévu en mars 2026, publication mi-2026), conformément au cycle triennal établi depuis C++11. C'est un standard majeur qui apporte des fonctionnalités transformatrices :

- **Contrats (Contracts)** — Préconditions, postconditions et assertions standardisées, permettant de formaliser les invariants d'un programme directement dans le code (voir section 12.14.1).  
- **Réflexion statique (Static Reflection)** — La capacité d'inspecter et de manipuler les types, les membres et les métadonnées à la compilation, ouvrant la porte à la génération automatique de code, la sérialisation, et les frameworks de test (voir section 12.14.2).  
- **Pattern Matching** — L'expression `inspect` et l'opérateur `is`, qui apportent au C++ un mécanisme d'analyse structurelle comparable à celui de Rust ou Haskell (voir section 12.14.3).  
- **`std::execution` (Senders/Receivers)** — Le framework standardisé d'exécution asynchrone, destiné à remplacer `std::async` et `std::future` par un modèle composable et extensible (voir section 12.14.4).

Le support compilateur progresse rapidement. GCC 15 et Clang 20 implémentent une partie significative des nouvelles fonctionnalités, avec des niveaux de complétude qui varient selon les features (voir section 12.14.5 pour l'état détaillé). MSVC suit une trajectoire similaire. Il faudra néanmoins probablement encore un à deux ans avant que le support soit suffisamment mature et homogène pour une adoption en production sans réserve.

### Le cycle triennal : une horloge prévisible

Depuis C++11, le comité s'est engagé dans un cycle de publication triennal : C++11, C++14, C++17, C++20, C++23, C++26. Cette régularité est un atout considérable pour la planification. On sait dès aujourd'hui que le prochain standard sera **C++29**, avec un Feature Freeze prévu aux alentours de 2027-2028 et une ratification en 2029.

Ce cycle triennal signifie aussi qu'une fonctionnalité qui n'est pas prête à temps pour un standard donné est reportée au suivant — elle n'est pas perdue, simplement décalée. Les Concepts, par exemple, ont été repoussés de C++17 à C++20. Les Contrats ont traversé un parcours encore plus long, avec une première tentative avortée pour C++20 avant leur intégration réussie en C++26. Comprendre ce mécanisme de report évite les frustrations et aide à calibrer ses attentes.

### Le cap sur C++29

Avec C++26 en ratification, le comité a commencé à orienter ses travaux vers C++29. Les premières proposals sont en discussion, les Study Groups explorent de nouvelles directions, et les sujets qui n'ont pas été retenus pour C++26 sont réévalués. Il est encore trop tôt pour prédire le contenu définitif de C++29, mais les grandes lignes se dessinent déjà dans les papers et les discussions du comité.

---

## Pourquoi suivre le processus de standardisation

### Anticiper plutôt que subir

Le développeur qui découvre les Concepts le jour de la sortie de GCC 10 est en retard. Celui qui a lu les proposals et regardé les talks de Bjarne Stroustrup sur le sujet trois ans plus tôt a eu le temps de comprendre la philosophie, d'expérimenter avec les implémentations expérimentales, et de réfléchir à la façon d'intégrer les Concepts dans son code. Le jour de la disponibilité compilateur, il est prêt à en tirer parti immédiatement.

Cette anticipation est d'autant plus précieuse pour les fonctionnalités qui changent les paradigmes de développement. La réflexion statique (C++26) va transformer la façon dont on écrit les frameworks, les sérialiseurs et les outils de test. `std::execution` va redéfinir l'architecture du code asynchrone. Le pattern matching va modifier les idiomes de traitement des variantes et de l'analyse de données. Dans chacun de ces cas, comprendre la fonctionnalité *avant* sa disponibilité générale donne un avantage compétitif significatif.

### Comprendre les compromis

Les papers du comité ne se contentent pas de spécifier une fonctionnalité : ils documentent les **alternatives rejetées** et les **raisons des choix de design**. Cette information est inestimable pour utiliser la fonctionnalité correctement. Quand on sait pourquoi une syntaxe particulière a été choisie, quels cas d'usage ont été considérés et quels compromis ont été acceptés, on utilise la fonctionnalité dans l'esprit de ses concepteurs plutôt que de lutter contre son design.

Par exemple, comprendre que `std::expected` (C++23) a été conçu comme un type de retour monadic pour la gestion d'erreurs — et non comme un remplacement universel des exceptions — éclaire les contextes où son utilisation est idiomatique et ceux où elle est forcée (voir section 12.8).

### Influencer le langage

Le processus de standardisation C++ est ouvert. N'importe qui peut lire les papers, assister aux réunions du comité (sous certaines conditions), ou soumettre une proposal. En pratique, la majorité des développeurs n'iront pas jusque-là, mais il existe un niveau d'influence intermédiaire : les retours de la communauté sur les proposals en cours, exprimés via les conférences (section 48.2), les forums (section 48.4), et les implémentations expérimentales, pèsent réellement dans les décisions du comité. La fonctionnalité `std::print` (C++23), par exemple, a bénéficié de retours communautaires significatifs pendant sa phase de review qui ont influencé son API finale.

---

## Les sources primaires d'information

Suivre le processus de standardisation repose sur quelques sources clés que les sous-sections suivantes détailleront.

### Les papers (open-std.org)

Toutes les proposals, les rapports de réunions, et les documents techniques du comité sont publiés gratuitement sur [open-std.org](https://open-std.org). C'est la source primaire, non filtrée, de tout ce qui se passe dans le processus de standardisation. Le volume est considérable — plusieurs centaines de papers par an — mais des techniques de filtrage efficaces existent pour identifier les documents pertinents.

→ Détails en **section 48.3.2**

### Les dépôts GitHub du comité

Le comité utilise de plus en plus GitHub pour le travail collaboratif sur les proposals et sur le draft du standard lui-même. Le dépôt [cplusplus/papers](https://github.com/cplusplus/papers) centralise les issues et les discussions en cours. C'est une source complémentaire à open-std.org, plus dynamique et plus facile à suivre au quotidien.

→ Détails en **section 48.3.2**

### Les trip reports

Après chaque réunion du comité (trois à quatre par an), des participants publient des **trip reports** — des comptes rendus résumant les décisions prises, les proposals avancées ou rejetées, et l'ambiance générale des discussions. Ces rapports, publiés sur des blogs personnels, sur Reddit r/cpp, ou sur meetingcpp.com, sont la source la plus accessible pour suivre l'actualité du comité sans lire les papers techniques.

→ Détails en **section 48.3.1**

### Les talks de conférence

Comme évoqué en section 48.2, les membres du comité présentent régulièrement à CppCon, Meeting C++, C++ Now et ACCU. Ces talks sont souvent le meilleur moyen de comprendre une proposal, car le speaker la replace dans son contexte, montre des exemples concrets, et répond aux questions de l'audience.

---

## Niveaux d'engagement dans le suivi

Suivre la standardisation C++ n'exige pas de devenir soi-même membre du comité. Voici trois niveaux d'engagement, du plus léger au plus profond.

### Niveau 1 : Veille passive (30 minutes/mois)

Lisez les trip reports après chaque réunion du comité et regardez les keynotes de Herb Sutter à CppCon qui résument l'état d'avancement. Ce minimum suffit pour savoir ce qui arrive dans le prochain standard et dans quelle timeline. C'est le niveau recommandé pour tout développeur C++ professionnel.

### Niveau 2 : Suivi ciblé (1–2 heures/mois)

En plus du niveau 1, suivez les proposals qui concernent votre domaine de travail. Si vous travaillez en programmation concurrente, suivez l'évolution de `std::execution` et des primitives de synchronisation. Si vous développez des librairies génériques, suivez les extensions des Concepts et de la Reflection. Utilisez les discussions GitHub et les listes de diffusion pour rester informé des évolutions spécifiques.

### Niveau 3 : Participation active (variable)

Contribuez aux discussions, testez les implémentations expérimentales, et fournissez des retours sur les proposals. C'est le niveau des développeurs qui veulent influencer la direction du langage. Il requiert un investissement significatif mais produit un impact disproportionné sur la communauté et sur votre propre expertise.

---

## Plan de la section

- **48.3.1** — [Calendrier du comité ISO et processus de standardisation](/48-ressources/03.1-calendrier-iso.md) : Comment le comité fonctionne, le cycle triennal, les Study Groups, et le parcours d'une proposal  
- **48.3.2** — [Suivre les proposals (open-std.org, GitHub)](/48-ressources/03.2-suivre-proposals.md) : Sources, outils de filtrage, et techniques pour suivre efficacement les évolutions du standard  
- **48.3.3** — [Premières proposals C++29 : Ce qui se prépare](/48-ressources/03.3-cpp29-preview.md) : Les sujets en discussion, les directions probables, et ce que cela signifie pour la planification technique

---

> 💡 **Note** : Le processus de standardisation C++ est souvent perçu comme opaque et inaccessible. En réalité, il est l'un des processus de design de langage les plus transparents qui existent — chaque paper est public, chaque discussion est documentée, et les trip reports offrent une fenêtre lisible sur les délibérations. La difficulté n'est pas l'accès à l'information mais sa curation. Les sous-sections qui suivent fournissent les outils et les méthodes pour transformer ce flux d'information en veille actionnable.

⏭️ [Calendrier du comité ISO et processus de standardisation](/48-ressources/03.1-calendrier-iso.md)
