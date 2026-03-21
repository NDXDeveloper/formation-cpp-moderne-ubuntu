🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 45.4 — Compilation avec protections

## Chapitre 45 — Sécurité en C++ ⭐

---

## Introduction

Les sections précédentes ont montré les vulnérabilités côté code : buffer overflows (45.1), integer overflows (45.2), use-after-free (45.3). La première réponse est d'écrire du code sûr avec les abstractions du C++ moderne. Mais le code parfait n'existe pas, et les bases de code réelles contiennent des millions de lignes héritées qui ne seront pas réécrites du jour au lendemain.

C'est ici qu'intervient la deuxième couche de la défense en profondeur : les protections activées à la compilation et par le système d'exploitation. Ces mécanismes ne préviennent pas les bugs — un buffer overflow se produit toujours — mais ils en réduisent drastiquement l'exploitabilité. Ils transforment une vulnérabilité potentiellement exploitable en un crash contrôlé, ce qui est toujours préférable à une exécution de code arbitraire par un attaquant.

Le principe est celui du filet de sécurité : même si le développeur commet une erreur, les protections compilateur et OS limitent les conséquences. Ces protections ont un coût en performance — parfois négligeable, parfois mesurable — et ce chapitre détaille pour chacune d'elles le compromis coût/bénéfice afin de permettre des décisions éclairées.

---

## Vue d'ensemble des couches de protection

La sécurité d'un binaire C++ en production résulte de l'empilement de plusieurs mécanismes indépendants, chacun ciblant un vecteur d'attaque spécifique. Ils se répartissent en deux catégories : les protections insérées par le compilateur dans le binaire, et les protections fournies par le système d'exploitation au moment de l'exécution.

### Protections côté compilateur

Le compilateur peut injecter du code supplémentaire dans le binaire pour détecter ou empêcher certaines classes d'exploitation :

**`-fstack-protector` (et ses variantes)** — insère un canary (valeur sentinelle) entre les variables locales et l'adresse de retour sur la pile. Si un buffer overflow écrase l'adresse de retour, il écrase nécessairement le canary au passage. Le code injecté vérifie l'intégrité du canary avant le retour de la fonction. Si le canary est corrompu, le programme est immédiatement terminé via `__stack_chk_fail`. Ce mécanisme est la défense la plus directe contre les stack buffer overflows classiques. Détail en **section 45.4.1**.

**`-D_FORTIFY_SOURCE`** — remplace à la compilation certaines fonctions C dangereuses (`memcpy`, `strcpy`, `sprintf`, etc.) par des variantes qui vérifient les tailles de tampon lorsque le compilateur peut les déduire. C'est une couche de durcissement qui attrape les débordements que le compilateur peut prouver statiquement, et insère des vérifications dynamiques dans les autres cas. Détail en **section 45.4.2**.

### Protections côté système d'exploitation

Le système d'exploitation fournit des mécanismes qui rendent l'exploitation plus difficile même si le binaire contient une vulnérabilité :

**ASLR (Address Space Layout Randomization)** — randomise les adresses de base du code, du heap, de la stack et des bibliothèques partagées à chaque exécution. Un attaquant qui a trouvé un moyen de rediriger le flux d'exécution doit encore deviner où se trouve le code à atteindre — une tâche rendue exponentiellement plus difficile par la randomisation.

**PIE (Position-Independent Executable)** — compile le binaire de façon à ce qu'il puisse être chargé à n'importe quelle adresse en mémoire. Sans PIE, le segment de code (.text) est chargé à une adresse fixe connue, ce qui annule le bénéfice de l'ASLR pour la partie principale du programme. PIE est le complément indispensable de l'ASLR.

Ces deux mécanismes sont couverts ensemble en **section 45.4.3**.

### Autres protections complémentaires

Au-delà des trois sous-sections de ce chapitre, d'autres protections méritent d'être mentionnées car elles font partie de la posture de sécurité standard d'un binaire C++ en production :

**NX / W^X (No Execute / Write XOR Execute)** — marque les pages mémoire comme exécutables ou inscriptibles, mais jamais les deux simultanément. Un attaquant qui parvient à injecter du shellcode sur la pile ou le heap ne peut pas l'exécuter car ces régions sont marquées non-exécutables. Ce mécanisme est activé par défaut sur tous les systèmes Linux modernes et ne nécessite aucun flag de compilation spécifique.

**RELRO (Relocation Read-Only)** — rend les sections de relocation (GOT, Global Offset Table) en lecture seule après le chargement du binaire. Cela empêche un attaquant de détourner le flux d'exécution en écrasant une entrée de la GOT. Il existe en deux variantes : Partial RELRO (défaut de GCC) et Full RELRO (recommandé).

```bash
# Full RELRO — recommandé pour les binaires de production
g++ -Wl,-z,relro,-z,now main.cpp -o main

# -z,relro : rend les sections de relocation read-only
# -z,now   : résout tous les symboles au chargement (pas de lazy binding)
#            → la GOT entière peut être protégée en lecture seule
```

**CFI (Control-Flow Integrity)** — vérifie que les appels de fonctions indirects (via pointeurs de fonction ou vtables) ciblent des destinations valides. Clang propose une implémentation via `-fsanitize=cfi` qui ajoute des vérifications à faible coût pour les appels virtuels, les casts de types et les appels indirects.

```bash
# CFI avec Clang — nécessite LTO
clang++ -flto -fsanitize=cfi -fvisibility=hidden main.cpp -o main
```

**Shadow Call Stack** — maintient une copie de la pile des adresses de retour dans une zone mémoire séparée, inaccessible par les buffer overflows classiques. Disponible sur Clang pour les architectures AArch64.

---

## La ligne de compilation de production

En combinant toutes les protections abordées dans cette section, la ligne de compilation recommandée pour un binaire C++ de production sur Ubuntu avec GCC 15 ressemble à ceci :

```bash
g++ -std=c++23 \
    -O2 \
    -Wall -Wextra -Wpedantic -Werror \
    -fstack-protector-strong \
    -D_FORTIFY_SOURCE=2 \
    -fPIE -pie \
    -Wl,-z,relro,-z,now \
    -Wl,-z,noexecstack \
    -fstack-clash-protection \
    -fcf-protection \
    main.cpp -o main
```

Et l'équivalent avec Clang 20 :

```bash
clang++ -std=c++23 \
    -O2 \
    -Wall -Wextra -Wpedantic -Werror \
    -fstack-protector-strong \
    -D_FORTIFY_SOURCE=2 \
    -fPIE -pie \
    -Wl,-z,relro,-z,now \
    -Wl,-z,noexecstack \
    -fstack-clash-protection \
    -fcf-protection \
    main.cpp -o main
```

Les sections suivantes détaillent chaque flag, son mécanisme, son coût en performance et les cas où il est pertinent de l'ajuster.

### Intégration avec CMake

Dans un projet structuré avec CMake (chapitre 26), ces options de sécurité sont centralisées dans le `CMakeLists.txt` :

```cmake
# Options de sécurité — applicable à toutes les cibles du projet
add_compile_options(
    -fstack-protector-strong
    -fstack-clash-protection
    -fcf-protection
)

add_compile_definitions(
    _FORTIFY_SOURCE=2
)

# PIE
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Linker hardening
add_link_options(
    -Wl,-z,relro
    -Wl,-z,now
    -Wl,-z,noexecstack
)
```

> **Bonne pratique** : encapsuler ces options dans un CMake Preset (section 27.6) ou dans un fichier `cmake/SecurityFlags.cmake` inclus par tous les projets de l'équipe. Cela garantit une posture de sécurité cohérente sans dépendre de la mémoire de chaque développeur.

---

## Vérifier les protections d'un binaire existant

Avant de détailler chaque mécanisme, il est utile de savoir vérifier quelles protections sont effectivement présentes dans un binaire déjà compilé. L'outil `checksec` (disponible dans le paquet `checksec` sur Ubuntu ou via `pip install checksec.py`) fournit un résumé immédiat :

```bash
# Installation
sudo apt install checksec

# Vérification d'un binaire
checksec --file=./my_program
```

Sortie typique :

```
RELRO           STACK CANARY    NX          PIE          RPATH     RUNPATH     Fortify  Fortified  Fortifiable  
Full RELRO      Canary found    NX enabled  PIE enabled  No RPATH  No RUNPATH  Yes      3          5  
```

Cette sortie indique en un coup d'œil :

- **Full RELRO** — la GOT est protégée en lecture seule.  
- **Canary found** — `-fstack-protector` est actif.  
- **NX enabled** — les pages de données ne sont pas exécutables.  
- **PIE enabled** — le binaire est position-independent.  
- **Fortified 3 / Fortifiable 5** — 3 fonctions sur 5 éligibles sont protégées par `_FORTIFY_SOURCE`.

On peut également inspecter manuellement avec `readelf` :

```bash
# Vérifier PIE
readelf -h ./my_program | grep Type
# Type: DYN (Position-Independent Executable)  → PIE activé
# Type: EXEC (Executable file)                 → PIE désactivé

# Vérifier RELRO
readelf -l ./my_program | grep GNU_RELRO
# Si présent → Partial RELRO au minimum

# Vérifier NX (stack non exécutable)
readelf -l ./my_program | grep GNU_STACK
# GNU_STACK 0x000000 ... RW  → NX activé (pas de flag E)
# GNU_STACK 0x000000 ... RWE → NX désactivé (dangereux)
```

Ces vérifications devraient faire partie du pipeline CI/CD (chapitre 38). Un job dédié peut exécuter `checksec` sur le binaire produit et échouer si une protection attendue est manquante :

```yaml
# Extrait de .gitlab-ci.yml
security-check:
  stage: verify
  script:
    - checksec --format=json --file=./build/my_program | python3 -c "
      import json, sys;
      data = json.load(sys.stdin);
      checks = data[list(data.keys())[0]];
      assert checks['pie'] == 'yes', 'PIE manquant';
      assert checks['canary'] == 'yes', 'Stack canary manquant';
      assert checks['nx'] == 'yes', 'NX manquant';
      assert checks['relro'] == 'full', 'Full RELRO manquant';
      print('Toutes les protections sont actives')
      "
```

---

## Coût en performance : ce que montrent les mesures

La question récurrente est : quel est le coût de ces protections ? La réponse varie selon la charge de travail, mais les mesures publiées par les grands projets convergent :

| Protection | Surcoût typique | Notes |
|---|---|---|
| `-fstack-protector-strong` | < 1 % | Protège les fonctions avec des tableaux locaux ou des variables dont l'adresse est prise |
| `-D_FORTIFY_SOURCE=2` | Négligeable | Remplacements inline, vérifications compilées avec le code |
| PIE + ASLR | < 1 % sur x86_64 | Surcoût sur les accès globaux (un niveau d'indirection supplémentaire via la GOT) |
| Full RELRO | Temps de chargement légèrement augmenté | Résolution eager de tous les symboles au démarrage au lieu de lazy |
| NX | Aucun | Géré par les bits de permission des pages mémoire |
| `-fstack-clash-protection` | < 1 % | Sonde les pages de la pile lors d'allocations larges |
| `-fcf-protection` | 1-2 % | Instructions CET (Intel) ou BTI (ARM) ajoutées aux branches indirectes |

Le coût cumulé de toutes ces protections est typiquement inférieur à 5 % sur des charges de travail réelles. C'est un prix dérisoire comparé au coût d'une exploitation réussie. Les rares cas où ce surcoût est significatif concernent des boucles internes ultra-optimisées dans du code de calcul scientifique ou de traitement de signal — et même dans ces cas, les protections peuvent être désactivées chirurgicalement pour les fichiers critiques tout en restant actives pour le reste du projet.

---

## Organisation des sous-sections

Les trois sous-sections suivantes détaillent les mécanismes les plus importants que le développeur contrôle directement via les flags de compilation :

- **Section 45.4.1** — `-fstack-protector` : les canaries sur la pile, les variantes (`-fstack-protector-all`, `-fstack-protector-strong`), le mécanisme de détection et le comportement en cas de corruption.  
- **Section 45.4.2** — `-D_FORTIFY_SOURCE` : le remplacement automatique des fonctions C dangereuses, les niveaux de protection (1, 2, 3), et les fonctions couvertes.  
- **Section 45.4.3** — ASLR et PIE : la randomisation de l'espace d'adressage, la compilation en position-independent code, et la vérification de l'activation sur un système Ubuntu.

---

## Pour aller plus loin

- **Section 2.6** — Options de compilation critiques : warnings, optimisation, debug, standard.  
- **Section 26.2** — CMake : structuration des options de compilation par cible.  
- **Section 27.6** — CMake Presets : standardisation des configurations de build.  
- **Section 29.4** — Sanitizers : protections dynamiques pour la phase de test (ASan, UBSan, TSan, MSan).  
- **Section 38.4** — Automatisation CI/CD : intégration des vérifications de sécurité dans le pipeline.  
- **Section 45.5** — Fuzzing : technique complémentaire pour découvrir les vulnérabilités que les protections compilateur ne préviennent pas.  
- **Section 45.6.3** — Hardening avec les sanitizers en production : quand et comment déployer des protections dynamiques au-delà du test.

⏭️ [-fstack-protector](/45-securite/04.1-stack-protector.md)
