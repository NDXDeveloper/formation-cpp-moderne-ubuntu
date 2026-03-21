🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 46.4 — Documentation : Doxygen et commentaires

> **Chapitre 46 : Organisation et Standards** · Module 17 : Architecture de Projet Professionnel  
> **Niveau** : Expert · **Prérequis** : Sections 46.1 à 46.3, chapitre 38 (CI/CD)

---

## Pourquoi documenter

Le code C++ bien écrit est lisible. Le code C++ documenté est *utilisable*. La distinction est fondamentale. Un développeur qui lit l'implémentation d'une classe peut comprendre *ce qu'elle fait*. Mais un consommateur de bibliothèque ne devrait pas avoir à lire l'implémentation pour savoir *comment l'utiliser*, quelles sont les préconditions d'un appel, les garanties de thread-safety, les cas d'erreur possibles, ou le modèle de propriété d'un pointeur retourné. C'est le rôle de la documentation.

En C++, le besoin est encore plus aigu que dans des langages comme Python ou Java. L'absence de garbage collector signifie que la documentation doit spécifier les responsabilités de gestion mémoire. Le modèle de templates signifie que les contraintes sur les types (avant les Concepts C++20) ne sont visibles que dans les messages d'erreur cryptiques du compilateur — sauf si elles sont documentées. La complexité des règles de move semantics, de lifetime et d'exception safety rend chaque API potentiellement ambiguë sans documentation explicite.

---

## Doxygen : l'outil de référence

Doxygen est le générateur de documentation standard de l'écosystème C++. Créé en 1997 par Dimitri van Heesch, il reste en 2026 l'outil dominant pour la documentation d'API C++, utilisé par des projets de toutes tailles — de petites bibliothèques open source aux bases de code de millions de lignes chez Google, Meta ou dans l'industrie automobile.

Le principe est simple : des commentaires structurés, écrits selon une syntaxe spécifique directement dans les fichiers sources, sont extraits par Doxygen pour générer une documentation navigable en HTML, PDF, LaTeX, ou même des pages man. Le code et sa documentation vivent dans les mêmes fichiers, versionnés ensemble dans Git, ce qui élimine le problème fondamental de la documentation externalisée — la désynchronisation.

### Ce que Doxygen génère

À partir de commentaires structurés et de l'analyse du code source, Doxygen produit automatiquement :

- **Des pages de référence d'API** : chaque classe, fonction, enum, typedef est documenté avec sa signature, sa description, ses paramètres et ses valeurs de retour.  
- **Des diagrammes de classes** : hiérarchies d'héritage, graphes de collaboration entre classes (via Graphviz).  
- **Des graphes d'inclusion** : quels headers incluent quels autres headers — utile pour diagnostiquer les dépendances excessives (section 46.2).  
- **Des graphes d'appel** : quelles fonctions appellent quelles autres fonctions.  
- **Un index navigable** : recherche par nom, par module, par fichier, avec liens croisés automatiques entre les symboles référencés.  
- **Des pages thématiques** : documentation narrative organisée en groupes logiques, tutoriels, guides d'architecture.

Le résultat est un site de documentation complet, généré à chaque commit via le pipeline CI/CD (chapitre 38), toujours synchronisé avec le code.

### Alternatives à Doxygen

Doxygen n'est pas sans défauts : sa configuration est verbeuse (le `Doxyfile` contient des centaines d'options), son thème HTML par défaut a vieilli, et son support des fonctionnalités C++ les plus récentes (concepts, modules) est encore partiel. Quelques alternatives méritent d'être mentionnées.

**Sphinx + Breathe** : Sphinx est le générateur de documentation standard de l'écosystème Python. Le plugin Breathe lui permet de consommer la sortie XML de Doxygen pour la rendre dans le format Sphinx, avec des thèmes modernes (comme Read the Docs). C'est la combinaison choisie par de nombreux projets qui veulent une documentation narrative riche mêlée à la référence d'API.

**Standardese** : un générateur de documentation conçu spécifiquement pour le C++ moderne, avec un support natif des concepts et des templates avancés. Le projet est cependant moins mature et moins adopté que Doxygen.

**hdoc** : un outil plus récent, axé sur la performance et la simplicité, qui génère une documentation HTML moderne à partir du code C++. Il gagne en popularité mais reste un choix de niche en 2026.

Dans la pratique, Doxygen reste le choix par défaut pour la majorité des projets. Son écosystème (intégration CMake, support IDE, compatibilité CI) est inégalé, et ses limitations sont contournables.

---

## Quoi documenter, quoi ne pas documenter

La sur-documentation est presque aussi nuisible que l'absence de documentation. Un commentaire qui paraphrase le code sans apporter d'information supplémentaire est du bruit qui rend le fichier plus long sans le rendre plus clair.

### Ce qui doit être documenté

**L'API publique** : toute classe, fonction, constante ou type exposé dans les headers `include/` (section 46.1) doit être documenté. C'est le contrat avec les consommateurs de la bibliothèque. La documentation doit couvrir ce que fait la fonction (pas comment elle le fait), les préconditions sur les paramètres, les postconditions sur la valeur de retour, les exceptions éventuellement lancées, et les garanties de thread-safety.

**Les subtilités de propriété et de lifetime** : en C++, la documentation d'un paramètre `Widget*` doit indiquer si l'appelant conserve la propriété, si la fonction prend la propriété, ou si le pointeur doit rester valide pendant toute la durée de vie d'un autre objet. Les smart pointers documentent une partie de ces contrats par leur type (`unique_ptr` = transfert, `shared_ptr` = propriété partagée), mais pas toujours de façon suffisante.

**Les comportements non évidents** : effets de bord, invalidation d'itérateurs, réallocation possible, coût algorithmique inhabituel, dépendance à un état global. Tout ce qui pourrait surprendre un utilisateur raisonnable mérite un commentaire.

**Les décisions d'architecture** : pourquoi une approche a été choisie plutôt qu'une autre. Ces commentaires sont rares mais précieux. Ils appartiennent souvent aux ADR (section 46.1) plutôt qu'au code, sauf quand la décision est intimement liée à l'implémentation d'une fonction spécifique.

### Ce qui ne doit pas être documenté

**L'évident** : un commentaire `/// Retourne le nom` sur un getter `std::string name() const` n'apporte rien. Le nom de la méthode est déjà auto-descriptif.

**L'implémentation ligne par ligne** : des commentaires comme `// Incrémente i` au-dessus de `++i` sont du bruit. Si le code a besoin d'être expliqué à ce niveau de détail, c'est le code qui doit être refactoré, pas le commentaire qui doit être ajouté.

**Les headers internes dans `src/`** : la documentation Doxygen vise les consommateurs de l'API publique. Les détails d'implémentation internes peuvent être commentés de façon plus informelle, voire pas du tout si le code est suffisamment expressif.

### Le bon critère

Avant d'écrire un commentaire, posez-vous la question : *"Un développeur compétent qui ne connaît pas ce code pourrait-il utiliser cette API correctement sans ce commentaire ?"* Si la réponse est non, le commentaire est nécessaire. Si la réponse est oui, le commentaire est probablement superflu.

---

## Placement de la documentation : header ou source ?

La documentation Doxygen peut être placée dans les fichiers `.h` ou dans les fichiers `.cpp`. Les deux approches ont des arguments en leur faveur.

### Documentation dans les headers (approche dominante)

La majorité des projets et des guides de style placent la documentation Doxygen dans les headers, directement au-dessus des déclarations :

```cpp
// include/monprojet/core/engine.h

namespace monprojet::core {

/// Moteur principal du système de traitement.
///
/// Le moteur gère un pool de workers qui traitent les éléments
/// de la file d'attente interne. Il est thread-safe : toutes les
/// méthodes publiques peuvent être appelées depuis n'importe quel thread.
class Engine {  
public:  
    /// Construit un moteur avec le fichier de configuration spécifié.
    ///
    /// @param config_path Chemin vers le fichier de configuration.
    ///        Le fichier doit exister et être lisible.
    /// @throws std::invalid_argument si config_path est vide.
    /// @throws std::runtime_error si le fichier ne peut pas être lu.
    explicit Engine(std::string config_path);
    // ...
};

} // namespace monprojet::core
```

L'avantage principal : la documentation est visible là où le développeur regarde naturellement quand il veut utiliser l'API. Pas besoin d'ouvrir un autre fichier. Les IDE (VS Code avec clangd, CLion) affichent les commentaires Doxygen du header au survol de tout appel dans le code, même depuis un fichier `.cpp` distant.

L'inconvénient : les headers deviennent plus volumineux, ce qui peut affecter la lisibilité des déclarations quand la documentation est dense.

### Documentation dans les sources

Certains projets (notamment dans l'écosystème LLVM) placent la documentation dans les fichiers `.cpp`, au-dessus des définitions. Les headers restent légers, avec uniquement les signatures.

L'avantage : les headers sont compacts et servent de référence rapide — on voit l'ensemble de l'API d'un coup d'œil. La documentation détaillée est dans les sources, pour ceux qui veulent approfondir.

L'inconvénient : la documentation est séparée de l'endroit où les consommateurs regardent (le header), et les IDE n'affichent pas toujours les commentaires placés dans les `.cpp` lors du survol d'une déclaration depuis un autre fichier.

### Recommandation

Pour la majorité des projets, la documentation dans les headers est préférable. C'est l'approche qui maximise la visibilité de la documentation, tant pour les humains que pour les outils. Si la densité des commentaires rend les headers illisibles, c'est souvent le signe que les classes ont trop de méthodes publiques — le problème est dans le design, pas dans la documentation.

---

## Intégration dans le workflow de développement

La documentation n'est utile que si elle est maintenue. Et elle n'est maintenue que si elle fait partie du workflow quotidien, pas d'une tâche périodique qu'on repousse indéfiniment.

### Documentation as code

Traiter la documentation comme du code signifie qu'elle est versionnée dans Git avec le code, revue dans les merge requests, et testée dans le pipeline CI. Concrètement :

- Toute merge request qui ajoute ou modifie une API publique doit inclure la documentation correspondante. C'est un critère de code review (section 47.4).  
- Le pipeline CI génère la documentation à chaque commit et échoue si Doxygen produit des warnings (paramètres non documentés, liens cassés).  
- La documentation HTML générée est déployée automatiquement (GitHub Pages, GitLab Pages, serveur interne).

### Vérification par le CI

Doxygen peut être configuré pour traiter les commentaires manquants ou incohérents comme des erreurs. Combiné avec un job CI dédié, cela crée un filet de sécurité qui empêche la dette documentaire de s'accumuler. Nous verrons la configuration concrète en section 46.4.2.

### Complémentarité avec les outils d'analyse

`clang-tidy` (section 32.1) dispose de checks liés à la documentation, notamment la vérification que les commentaires Doxygen sont syntaxiquement corrects et cohérents avec les signatures de fonctions. Activé dans les pre-commit hooks (chapitre 47), ce check attrape les erreurs de documentation avant même qu'elles n'atteignent le CI.

---

## Sommaire de la section

- **46.4.1** — Syntaxe Doxygen : les commandes essentielles, les formats de commentaires, la documentation des classes, fonctions, paramètres, valeurs de retour, exceptions et exemples de code.  
- **46.4.2** — Génération de documentation : configuration du `Doxyfile`, intégration CMake, déploiement automatisé via CI/CD, personnalisation du thème et de la sortie.

---


⏭️ [Syntaxe Doxygen](/46-organisation-standards/04.1-syntaxe-doxygen.md)
