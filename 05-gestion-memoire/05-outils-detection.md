🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 5.5 — Outils de détection : Valgrind, AddressSanitizer

## Le constat : l'humain ne suffit pas

La section précédente a catalogué les trois grandes catégories d'erreurs mémoire — fuites, dangling pointers, double free. Vous pourriez penser qu'avec de la rigueur et de l'attention, un développeur expérimenté peut les éviter. C'est partiellement vrai pour du code simple. Mais dès que la base de code atteint quelques milliers de lignes, que les chemins d'exécution se multiplient, que les exceptions entrent en jeu et que plusieurs développeurs contribuent au même projet, la vigilance humaine atteint ses limites.

Considérez ce cas réaliste :

```cpp
void traiter_batch(const std::vector<std::string>& fichiers) {
    for (const auto& chemin : fichiers) {
        auto* parser = new FormatParser(chemin);
        auto donnees = parser->extraire();

        if (!donnees.est_valide()) {
            log_erreur("Données invalides : " + chemin);
            continue;             // ❌ fuite — parser n'est jamais libéré
        }

        transformer(donnees);
        exporter(donnees);        // peut lancer une exception → fuite

        delete parser;
    }
}
```

Deux chemins de fuite coexistent dans une boucle : le `continue` et l'exception potentielle. En relisant le code rapidement, les deux sont faciles à manquer. Multipliez ce type de pattern par des centaines de fonctions dans un projet réel, et vous comprenez pourquoi les outils automatisés sont indispensables.

---

## Deux approches complémentaires

Il existe deux grandes familles d'outils pour détecter les erreurs mémoire. Elles ne fonctionnent pas de la même manière, n'ont pas les mêmes forces ni les mêmes compromis, et se complètent plutôt qu'elles ne se remplacent.

### Instrumentation dynamique externe : Valgrind

Valgrind est un **framework d'instrumentation binaire**. Il exécute votre programme dans un environnement simulé qui intercepte chaque accès mémoire, chaque allocation et chaque libération. Le programme n'a pas besoin d'être recompilé — Valgrind travaille directement sur l'exécutable.

Son outil phare, **Memcheck**, détecte les fuites mémoire, les lectures de mémoire non initialisée, les accès hors limites, les double free et les use-after-free. Il produit des rapports détaillés avec les piles d'appels complètes.

Le prix à payer est la **performance** : un programme sous Valgrind tourne typiquement 10 à 30 fois plus lentement qu'en exécution normale. C'est un outil de diagnostic, pas un outil de production.

### Instrumentation à la compilation : les Sanitizers

Les sanitizers (AddressSanitizer, LeakSanitizer, MemorySanitizer, etc.) sont des **passes de compilation** intégrées à GCC et Clang. Elles ajoutent du code d'instrumentation directement dans votre exécutable au moment de la compilation, via des flags comme `-fsanitize=address`.

L'approche est fondamentalement différente : plutôt que de simuler l'exécution, le compilateur insère des vérifications aux points critiques (avant chaque accès mémoire, autour de chaque allocation). Le résultat est un programme instrumenté qui tourne 2 à 3 fois plus lentement que l'original — bien plus rapide que Valgrind, ce qui le rend utilisable dans les suites de tests automatisées en CI/CD.

En contrepartie, le programme doit être recompilé, et certaines erreurs que Valgrind détecte (comme les lectures de mémoire non initialisée) nécessitent un sanitizer différent (MemorySanitizer) qui ne peut pas être combiné avec AddressSanitizer dans la même exécution.

---

## Comparaison en un coup d'œil

```
                          Valgrind (Memcheck)         AddressSanitizer (ASan)
                          ─────────────────────       ────────────────────────
Recompilation requise     Non                         Oui (-fsanitize=address)  
Ralentissement            10-30×                      2-3×  
Consommation mémoire      +2-3× la taille originale   +2-3× la taille originale  
Fuites mémoire            ✅ Excellent                ✅ Via LeakSanitizer (activé  
                                                         par défaut avec ASan)
Buffer overflow (heap)    ✅                          ✅ (détection plus précise)  
Buffer overflow (stack)   Partiel                     ✅  
Use-after-free            ✅                          ✅  
Double free               ✅                          ✅  
Mémoire non initialisée   ✅                          ❌ (nécessite MSan)  
Use-after-return          ❌                          ✅ (avec flag spécifique)  
Supporté par              Toute architecture Linux    GCC, Clang (Linux, macOS)  
Intégration CI/CD         Possible mais lent          ✅ Recommandé  
```

La recommandation pratique est d'utiliser **les deux** à des moments différents du cycle de développement. AddressSanitizer en compilation de développement quotidienne et dans les pipelines CI, pour sa vitesse. Valgrind ponctuellement, pour les audits approfondis et la détection des lectures de mémoire non initialisée.

---

## Un programme cobaye pour les deux outils

Pour illustrer concrètement les deux outils dans les sous-sections suivantes, nous allons utiliser un programme volontairement truffé d'erreurs. Prenez le temps de repérer les bugs avant de lire la suite — c'est un bon exercice de relecture :

```cpp
// bugs_memoire.cpp — programme volontairement bugué
#include <iostream>
#include <cstring>

int main() {
    // Bug 1 : fuite mémoire
    int* fuite = new int[100];
    // Jamais libéré

    // Bug 2 : buffer overflow (écriture hors limites)
    int* tableau = new int[5];
    tableau[5] = 999;          // index 5 → hors limites (0 à 4 valides)

    // Bug 3 : use-after-free
    int* ephemere = new int(42);
    delete ephemere;
    std::cout << *ephemere << "\n";   // lecture après libération

    // Bug 4 : double free
    int* victime = new int(7);
    delete victime;
    delete victime;

    delete[] tableau;
    return 0;
}
```

Ce programme contient quatre erreurs distinctes. Dans les deux sous-sections qui suivent, nous verrons comment Valgrind et AddressSanitizer les détectent, quels messages ils produisent, et comment interpréter leurs rapports.

---

## Quand utiliser quoi

La question "Valgrind ou ASan ?" n'a pas de réponse unique. Voici un guide en fonction du contexte.

**Pendant le développement quotidien**, compilez avec `-fsanitize=address -fsanitize=undefined -g`. Vous obtenez une détection immédiate des erreurs mémoire et des comportements indéfinis, avec un ralentissement acceptable pour le développement interactif et les tests unitaires.

**Dans le pipeline CI/CD**, intégrez un job dédié qui compile et exécute les tests avec AddressSanitizer. Le surcoût de 2-3× est compatible avec des pipelines raisonnables. C'est la pratique standard dans les projets C++ sérieux — Google l'applique à l'ensemble de son code C++.

**Pour un audit approfondi**, lancez Valgrind Memcheck sur votre suite de tests. Le ralentissement de 10-30× le rend inadapté à une exécution systématique, mais sa capacité à détecter les lectures de mémoire non initialisée (que ASan ne couvre pas) en fait un complément précieux.

**Pour diagnostiquer un bug spécifique**, choisissez l'outil le plus adapté au type de bug suspecté. Un crash intermittent suggère un use-after-free → ASan. Une consommation mémoire croissante → Valgrind Memcheck ou LeakSanitizer. Des valeurs aberrantes en sortie → Valgrind (mémoire non initialisée) ou MemorySanitizer.

Les deux sous-sections suivantes détaillent l'installation, l'utilisation et l'interprétation des rapports de chaque outil :

- **5.5.1 — Valgrind : Installation et utilisation** : installation sur Ubuntu, exécution avec Memcheck, lecture des rapports de fuites et d'erreurs, options les plus utiles.
- **5.5.2 — AddressSanitizer : Compilation avec `-fsanitize=address`** : flags de compilation, interprétation des rapports d'erreur, LeakSanitizer intégré, intégration dans CMake et dans un pipeline CI.

⏭️ [Valgrind : Installation et utilisation](/05-gestion-memoire/05.1-valgrind.md)
