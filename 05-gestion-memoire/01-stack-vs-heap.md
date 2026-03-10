🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 5.1 — Comprendre la Stack (Pile) vs le Heap (Tas)

## Vue d'ensemble

Chaque programme C++ en cours d'exécution dispose de deux grandes zones mémoire pour stocker ses données : la **stack** (pile) et le **heap** (tas). Comprendre la différence entre ces deux zones est probablement la compétence la plus fondamentale qu'un développeur C++ puisse acquérir. Elle conditionne votre capacité à écrire du code correct, performant et exempt de fuites mémoire.

La distinction est simple dans son principe : la stack est **automatique**, le heap est **manuel**. Mais les implications de cette différence traversent l'ensemble du langage, de la durée de vie des objets jusqu'aux stratégies d'optimisation en production.

---

## L'analogie : pile d'assiettes vs entrepôt

Pour fixer les idées avant d'entrer dans les détails techniques, voici une analogie qui capture l'essentiel.

La **stack** fonctionne comme une pile d'assiettes dans un restaurant. Vous posez une assiette sur le dessus, vous la retirez depuis le dessus. C'est rapide, ordonné, et vous n'avez aucune décision à prendre — la prochaine assiette à retirer est toujours celle du sommet. En revanche, la taille de la pile est limitée : essayez d'empiler mille assiettes et la pile s'effondre.

Le **heap** fonctionne comme un entrepôt. Vous pouvez y stocker autant d'objets que vous le souhaitez (dans la limite de l'espace disponible), les placer où bon vous semble, et les retirer dans n'importe quel ordre. C'est flexible et vaste, mais il faut tenir un registre de ce que vous avez stocké et où. Si vous oubliez de venir récupérer un objet, il occupe de la place indéfiniment. Et chaque opération de stockage ou de récupération prend plus de temps que sur la pile, parce qu'il faut chercher un emplacement libre, mettre à jour le registre, etc.

Cette analogie n'est pas parfaite, mais elle capture les deux tensions fondamentales :

- **Stack** : rapide et automatique, mais limitée en taille et en flexibilité.
- **Heap** : flexible et vaste, mais plus lent et entièrement sous votre responsabilité.

---

## Quand la stack est-elle utilisée ?

Le compilateur place automatiquement sur la stack toutes les variables **locales** — celles que vous déclarez à l'intérieur d'une fonction ou d'un bloc :

```cpp
void traiter_requete() {
    int code_retour = 200;              // stack
    double temps_reponse = 0.0;         // stack
    std::string message = "OK";         // l'objet string est sur la stack,
                                        // mais son contenu interne peut être sur le heap

    if (code_retour == 200) {
        bool succes = true;             // stack (portée limitée au bloc if)
    }
    // 'succes' n'existe plus ici — automatiquement dépilé
}
// code_retour, temps_reponse et message n'existent plus ici
```

Chaque appel de fonction crée un **stack frame** (cadre de pile) qui contient les variables locales de cette fonction, ses paramètres, et l'adresse de retour pour savoir où reprendre l'exécution une fois la fonction terminée. Quand la fonction se termine, son stack frame est intégralement détruit — c'est instantané, il suffit de déplacer le pointeur de pile.

Ce mécanisme est ce qui rend la stack si efficace : il n'y a aucune recherche d'emplacement libre, aucune fragmentation, aucun nettoyage différé. L'allocation et la désallocation se résument à un simple ajustement d'un registre CPU (le *stack pointer*).

---

## Quand le heap est-il utilisé ?

Le heap entre en jeu dès que vous avez besoin d'une mémoire dont la durée de vie **dépasse** le cadre de la fonction qui l'a créée, ou dont la taille n'est pas connue à la compilation :

```cpp
int* creer_tableau(int taille) {
    int* tableau = new int[taille];  // heap : taille déterminée à l'exécution
    return tableau;                  // le tableau survit à la fonction
}

void exemple() {
    int* t = creer_tableau(1000);
    // ... utilisation de t ...
    delete[] t;                      // libération manuelle obligatoire
}
```

Avec le heap, c'est **vous** qui décidez quand la mémoire est allouée (via `new`) et quand elle est libérée (via `delete`). Cette liberté est puissante — elle permet de créer des structures de données dynamiques, des objets à durée de vie arbitraire, des buffers de taille variable — mais elle a un coût : chaque allocation implique une recherche d'espace libre par l'allocateur mémoire, et chaque oubli de `delete` provoque une fuite.

---

## Les différences en un coup d'œil

Voici les caractéristiques clés qui distinguent les deux zones. Nous approfondirons chacune d'elles dans les sous-sections suivantes.

```
                    STACK                           HEAP
               ─────────────                  ─────────────
Gestion        Automatique (compilateur)      Manuelle (développeur)  
Allocation     Déplacement du stack pointer   Recherche d'espace libre (allocateur)  
Vitesse        Très rapide (~1 cycle CPU)     Plus lent (appel système possible)  
Taille         Limitée (1-8 Mo typique)       Limitée par la RAM disponible  
Durée de vie   Liée au scope (bloc/fonction)  Jusqu'au delete explicite  
Fragmentation  Aucune                         Possible avec le temps  
Accès          LIFO uniquement                Aléatoire (via pointeur)  
Thread-safety  Chaque thread a sa stack       Partagé entre threads  
```

Quelques précisions importantes sur ce tableau. La taille par défaut de la stack sous Linux est généralement de 8 Mo (configurable via `ulimit -s`). Ce n'est pas un bug ou une limitation arbitraire : cette taille suffit pour la grande majorité des programmes, et la limiter permet au système d'exploitation de détecter les récursions infinies via un *stack overflow* plutôt que de laisser le programme dévorer toute la mémoire.

Le heap, en revanche, peut théoriquement utiliser toute la mémoire virtuelle disponible — soit plusieurs téraoctets sur un système 64 bits moderne. En pratique, la limite est la RAM physique plus le swap.

---

## Un piège courant : confondre l'objet et ses ressources

Un point subtil mérite d'être soulevé dès maintenant, car il est source de confusion fréquente. Considérez ce code :

```cpp
void traiter() {
    std::vector<int> nombres = {1, 2, 3, 4, 5};
    // Où est 'nombres' ?
}
```

La réponse est : **les deux**. L'objet `nombres` lui-même — c'est-à-dire la structure de contrôle du vector (pointeur interne, taille, capacité) — est sur la **stack**. Mais les éléments `{1, 2, 3, 4, 5}` sont stockés dans un buffer alloué sur le **heap** par le vector en interne.

Quand la fonction `traiter` se termine, l'objet `nombres` est détruit (dépilé de la stack). Son destructeur est automatiquement appelé, et ce destructeur libère le buffer interne sur le heap. C'est le principe **RAII** en action — nous y reviendrons en détail au chapitre 6.

Cette dualité stack/heap est omniprésente avec les conteneurs de la STL (`std::vector`, `std::string`, `std::map`...) et les smart pointers (`std::unique_ptr`, `std::shared_ptr`). L'objet de contrôle vit sur la stack, les données qu'il gère vivent sur le heap, et le destructeur fait le pont entre les deux.

---

## Ce que nous allons explorer

Les trois sous-sections suivantes détaillent chaque aspect :

- **5.1.1 — Diagramme mémoire d'un processus** : une vue complète et annotée de l'espace d'adressage d'un programme Linux, avec des exemples d'inspection via `/proc` et des outils en ligne de commande.
- **5.1.2 — Caractéristiques de la Stack** : fonctionnement du stack pointer, stack frames, récursion et risque de stack overflow, taille configurable sous Linux.
- **5.1.3 — Caractéristiques du Heap** : fonctionnement de l'allocateur (malloc/new), fragmentation, coût des allocations, et premières intuitions sur pourquoi le C++ moderne cherche à minimiser les allocations dynamiques.

⏭️ [Diagramme mémoire d'un processus](/05-gestion-memoire/01.1-diagramme-memoire.md)
