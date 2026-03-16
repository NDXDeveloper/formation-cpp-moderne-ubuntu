🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 32.3 — clang-format 19 : Formatage automatique

## Introduction

Les débats sur le style de formatage — où placer les accolades, combien d'espaces pour l'indentation, comment aligner les paramètres — sont parmi les plus anciens et les plus stériles de l'ingénierie logicielle. Chaque développeur a ses préférences, chaque code review risque de dégénérer en discussion de style, et le résultat est un code base hétérogène où chaque fichier reflète les habitudes de son dernier contributeur.

**clang-format** tranche définitivement cette question en automatisant le formatage. L'outil reformate le code source C++ selon un ensemble de règles configurées, de manière déterministe : le même code d'entrée produit toujours le même code de sortie, quel que soit le développeur. Les discussions de style se réduisent à une seule décision collective — le fichier `.clang-format` — après quoi le formatage est délégué à l'outil.

clang-format fait partie de l'écosystème LLVM et utilise le parser de Clang pour comprendre la structure syntaxique du code. Il ne se contente pas de compter les espaces : il comprend les templates, les lambdas, les listes d'initialisation, les concepts C++20 et les autres constructions du langage, ce qui lui permet de produire un formatage sémantiquement cohérent. La version 19 (LLVM 19), disponible sur Ubuntu en 2026, apporte un support amélioré des constructions C++23 et de nouvelles options de formatage.

---

## Installation sur Ubuntu

```bash
sudo apt update  
sudo apt install clang-format  
```

Pour une version spécifique :

```bash
sudo apt install clang-format-20
```

Vérification :

```bash
clang-format --version
```

Si plusieurs versions sont installées :

```bash
sudo update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-20 100
```

---

## Première utilisation

### Formater un fichier et afficher le résultat

```bash
clang-format mon_fichier.cpp
```

Par défaut, clang-format affiche le résultat formaté sur la sortie standard sans modifier le fichier original. Cela permet de prévisualiser les modifications.

### Formater en place

```bash
clang-format -i mon_fichier.cpp
```

L'option `-i` (*in-place*) modifie le fichier directement. C'est le mode d'utilisation le plus courant.

### Formater plusieurs fichiers

```bash
# Tous les fichiers C++ d'un répertoire
find src/ -name '*.cpp' -o -name '*.h' | xargs clang-format -i

# Avec gestion des noms contenant des espaces
find src/ \( -name '*.cpp' -o -name '*.h' \) -print0 | xargs -0 clang-format -i
```

### Vérifier sans modifier (mode diff)

```bash
clang-format --dry-run --Werror mon_fichier.cpp
```

L'option `--dry-run` simule le formatage et `--Werror` retourne un code de sortie non nul si le fichier n'est pas correctement formaté. C'est le mode adapté à la CI et aux pre-commit hooks : vérifier que le code est formaté sans le modifier.

```bash
# Vérifier tout le projet
find src/ include/ -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run --Werror
```

Si un fichier n'est pas conforme, clang-format affiche les modifications nécessaires sous forme de diagnostics et retourne un code d'erreur.

---

## Le fichier `.clang-format`

### Résolution

Comme `.clang-tidy`, clang-format recherche un fichier `.clang-format` en remontant l'arborescence des répertoires à partir du fichier traité. Le premier fichier trouvé est utilisé. Un fichier `_clang-format` (avec un underscore) est également reconnu, ce qui est utile sur les systèmes de fichiers où les fichiers commençant par un point sont masqués.

### Structure

Le fichier `.clang-format` est au format YAML :

```yaml
---
Language: Cpp  
BasedOnStyle: Google  

# Indentation
IndentWidth: 4  
TabWidth: 4  
UseTab: Never  
AccessModifierOffset: -4  

# Accolades
BreakBeforeBraces: Attach  
AllowShortFunctionsOnASingleLine: Inline  
AllowShortIfStatementsOnASingleLine: Never  
AllowShortLoopsOnASingleLine: false  

# Colonnes
ColumnLimit: 100  
ReflowComments: true  

# Includes
SortIncludes: CaseSensitive  
IncludeBlocks: Regroup  

# Alignement
AlignAfterOpenBracket: Align  
AlignConsecutiveAssignments: false  
AlignTrailingComments: true  

# Espaces
SpaceAfterCStyleCast: false  
SpaceAfterTemplateKeyword: true  
SpaceBeforeParens: ControlStatements  
SpacesInAngles: Never  

# Pointeurs et références
PointerAlignment: Left  
ReferenceAlignment: Pointer  

# Lambdas et C++ moderne
LambdaBodyIndentation: Signature  
RequiresClausePosition: OwnLine  
RequiresExpressionIndentation: OuterScope  

# Pénalités (contrôle fin des retours à la ligne)
PenaltyBreakComment: 300  
PenaltyBreakString: 1000  
PenaltyExcessCharacter: 50  
```

---

## Styles de base prédéfinis

clang-format propose plusieurs styles prédéfinis qui servent de point de départ. La clé `BasedOnStyle` sélectionne le style de base, que vous pouvez ensuite personnaliser :

```yaml
BasedOnStyle: Google    # Style Google C++  
BasedOnStyle: LLVM      # Style du projet LLVM  
BasedOnStyle: Mozilla   # Style Mozilla/Firefox  
BasedOnStyle: Chromium  # Style Chromium (variante Google)  
BasedOnStyle: Microsoft # Style Microsoft  
BasedOnStyle: GNU       # Style GNU  
BasedOnStyle: WebKit    # Style WebKit  
```

Pour appliquer un style prédéfini sans fichier de configuration :

```bash
clang-format --style=Google mon_fichier.cpp
```

### Comparaison des styles principaux

Le même fragment de code formaté selon différents styles illustre les différences :

**Google** (indentation 2, accolades attachées, limite 80 colonnes) :

```cpp
class Parser {
 public:
  void analyser(const std::string& input) {
    if (input.empty()) {
      return;
    }
    for (const auto& token : tokenize(input)) {
      traiter(token);
    }
  }
};
```

**LLVM** (indentation 2, accolades attachées, limite 80 colonnes, modificateurs non indentés) :

```cpp
class Parser {  
public:  
  void analyser(const std::string &input) {
    if (input.empty()) {
      return;
    }
    for (const auto &token : tokenize(input)) {
      traiter(token);
    }
  }
};
```

**Microsoft** (indentation 4, accolades sur ligne séparée, limite 120 colonnes) :

```cpp
class Parser
{
  public:
    void analyser(const std::string& input)
    {
        if (input.empty())
        {
            return;
        }
        for (const auto& token : tokenize(input))
        {
            traiter(token);
        }
    }
};
```

Les différences notables concernent le placement des accolades (`Attach` vs `Allman`), la taille de l'indentation, le placement du `&`/`*` (à gauche du type ou à droite), et l'indentation des modificateurs d'accès.

### Générer une configuration de base

Pour créer un fichier `.clang-format` à partir d'un style prédéfini et le personnaliser ensuite :

```bash
clang-format --style=Google --dump-config > .clang-format
```

Cette commande exporte la configuration complète du style sélectionné avec toutes les options explicites. Vous pouvez ensuite modifier les valeurs selon les conventions de votre projet.

---

## Options de configuration essentielles

clang-format propose plus de 150 options. Voici celles qui ont le plus d'impact sur l'apparence du code et qui sont les plus fréquemment personnalisées.

### Indentation et largeur

```yaml
# Nombre d'espaces par niveau d'indentation
IndentWidth: 4

# Largeur d'une tabulation (si UseTab != Never)
TabWidth: 4

# Jamais de tabulations, toujours des espaces
UseTab: Never

# Décalage des modificateurs d'accès (public/private/protected)
# -4 aligne au même niveau que la classe
AccessModifierOffset: -4

# Indenter le contenu des namespaces
IndentExternBlock: NoIndent  
NamespaceIndentation: None  
```

### Limite de colonnes et retours à la ligne

```yaml
# Nombre maximum de caractères par ligne
ColumnLimit: 100

# Reformater les commentaires pour respecter la limite
ReflowComments: true

# Permettre les paramètres de fonction sur une seule ligne
AllowAllParametersOfDeclarationOnNextLine: true  
BinPackParameters: true  
BinPackArguments: true  
```

Le `ColumnLimit` est la décision la plus visible. Les valeurs courantes sont 80 (tradition Unix, Google, LLVM), 100 (compromis populaire), et 120 (Microsoft, projets modernes avec écrans larges). La valeur 0 désactive la limite — clang-format ne cassera jamais les lignes automatiquement.

### Placement des accolades

```yaml
# Attach : accolade sur la même ligne que l'instruction
# Allman : accolade sur une nouvelle ligne
# Stroustrup : Attach pour les fonctions, nouvelle ligne pour else/catch
# GNU : accolade sur nouvelle ligne, contenu indenté d'un demi-niveau
BreakBeforeBraces: Attach
```

Pour un contrôle plus fin, la clé `BraceWrapping` permet de configurer le placement des accolades indépendamment pour chaque contexte :

```yaml
BreakBeforeBraces: Custom  
BraceWrapping:  
  AfterClass: false
  AfterControlStatement: Never
  AfterFunction: false
  AfterNamespace: false
  AfterStruct: false
  AfterEnum: false
  BeforeCatch: false
  BeforeElse: false
  BeforeLambdaBody: false
  BeforeWhile: false
  IndentBraces: false
  SplitEmptyFunction: false
  SplitEmptyRecord: false
```

### Tri des includes

```yaml
# Trier les #include alphabétiquement
SortIncludes: CaseSensitive

# Regrouper les includes par catégorie (séparés par une ligne vide)
IncludeBlocks: Regroup

# Définir les catégories de priorité
IncludeCategories:
  # 1. Header correspondant au fichier source (parser.h pour parser.cpp)
  - Regex: '".*"'
    Priority: 1
  # 2. Headers du projet
  - Regex: '<mon_projet/.*>'
    Priority: 2
  # 3. Headers tiers
  - Regex: '<(boost|fmt|spdlog|nlohmann)/.*>'
    Priority: 3
  # 4. Headers système et STL
  - Regex: '<.*>'
    Priority: 4
```

Le tri et le regroupement des includes sont l'un des gains de productivité les plus tangibles de clang-format. Les développeurs n'ont plus à se soucier de l'ordre des includes — clang-format les réorganise automatiquement selon la politique du projet.

### Constructions C++ moderne

```yaml
# Position de la clause requires (C++20)
RequiresClausePosition: OwnLine

# Indentation des expressions requires
RequiresExpressionIndentation: OuterScope

# Espaces dans les templates
SpacesInAngles: Never

# Formatage des lambdas
LambdaBodyIndentation: Signature  
AllowShortLambdasOnASingleLine: Inline  

# Séparateur de chiffres (C++14)
IntegerLiteralSeparator:
  Binary: 4
  Decimal: 3
  Hex: 2
```

L'option `RequiresClausePosition` est particulièrement utile pour les projets utilisant les concepts C++20. Sans elle, le placement de la clause `requires` est imprévisible et varie selon la longueur de la ligne :

```cpp
// Avec RequiresClausePosition: OwnLine
template<typename T>  
requires std::integral<T>  
T additionner(T a, T b) {  
    return a + b;
}
```

---

## Désactiver le formatage sur une section

Certaines portions de code — tableaux de données alignés manuellement, macros complexes, art ASCII dans les commentaires — sont volontairement formatées d'une manière que clang-format perturberait. Les commentaires de contrôle permettent de désactiver le formatage localement :

```cpp
// clang-format off
int matrice[3][3] = {
    { 1,  0,  0 },
    { 0,  1,  0 },
    { 0,  0,  1 }
};
// clang-format on
```

Tout le code entre `// clang-format off` et `// clang-format on` est préservé tel quel. Ces marqueurs doivent être utilisés avec parcimonie — chaque section exemptée est une section qui peut dériver en termes de style sans être rattrapée par l'outil.

---

## Intégration dans l'IDE

### VS Code

L'extension **C/C++** (Microsoft) ou **clangd** prend en charge clang-format nativement. Pour formater automatiquement à chaque sauvegarde :

```json
// .vscode/settings.json
{
    "editor.formatOnSave": true,
    "editor.defaultFormatter": "llvm-vs-code-extensions.vscode-clangd",
    "C_Cpp.clang_format_style": "file"
}
```

L'option `"file"` indique à l'extension d'utiliser le fichier `.clang-format` du projet. Le raccourci `Shift+Alt+F` (ou `Ctrl+Shift+I` sur Linux) formate le fichier courant à la demande.

### CLion

CLion intègre clang-format directement. Dans les paramètres (`Settings → Editor → Code Style → C/C++`), activez « Enable ClangFormat (only if .clang-format is found) ». CLion utilisera alors le `.clang-format` du projet pour tout formatage automatique et pour le raccourci `Ctrl+Alt+L`.

### Vim / Neovim

Avec le plugin `vim-clang-format` ou via le LSP (clangd) :

```vim
" .vimrc — formatage via clang-format sur sauvegarde
autocmd BufWritePre *.cpp,*.h :silent! %!clang-format
```

Ou plus finement, pour formater uniquement les lignes modifiées :

```bash
# Formater uniquement les lignes modifiées depuis le dernier commit
git diff --name-only HEAD -- '*.cpp' '*.h' | xargs clang-format -i
```

---

## Formater uniquement le code modifié

Sur un projet existant avec un historique Git important, reformater tous les fichiers en une seule fois produit un commit massif qui rend `git blame` inutilisable — chaque ligne est attribuée au commit de reformatage. Deux stratégies atténuent ce problème.

### git clang-format

L'outil `git-clang-format` (distribué avec LLVM) formate uniquement les lignes modifiées dans le working tree par rapport au dernier commit :

```bash
# Formater les lignes modifiées (non encore commitées)
git clang-format

# Formater les lignes modifiées par rapport à une branche
git clang-format main

# Prévisualiser sans modifier
git clang-format --diff
```

Cette approche est idéale pour l'adoption progressive : chaque développeur formate les lignes qu'il touche, et le formatage se propage naturellement au fil des commits. Après quelques mois, la majorité du code est formatée sans commit de reformatage massif.

### git blame --ignore-rev

Si vous optez pour un reformatage global, Git permet d'ignorer des commits spécifiques dans `git blame` :

```bash
# Après le commit de reformatage massif
echo "<hash-du-commit-de-reformatage>" >> .git-blame-ignore-revs

# Configurer Git pour utiliser ce fichier
git config blame.ignoreRevsFile .git-blame-ignore-revs
```

Le fichier `.git-blame-ignore-revs` est versionné dans le dépôt. Les services comme GitHub et GitLab le reconnaissent automatiquement dans leur interface web.

---

## Intégration en CI

La CI vérifie que le code committé est correctement formaté, sans le modifier :

```yaml
# .gitlab-ci.yml (extrait)
format_check:
  stage: quality
  script:
    - find src/ include/ \( -name '*.cpp' -o -name '*.h' \) -print0
      | xargs -0 clang-format --dry-run --Werror
  allow_failure: false
```

Si un fichier n'est pas conforme, le job échoue et le développeur doit formater son code avant de repousser. Avec le formatage automatique à la sauvegarde dans l'IDE et un pre-commit hook (section 32.4), ce garde-fou CI est rarement déclenché — il sert de filet de sécurité ultime.

Une alternative plus explicite pour le message d'erreur :

```yaml
format_check:
  stage: quality
  script:
    - |
      DIFF=$(find src/ include/ \( -name '*.cpp' -o -name '*.h' \) -print0 \
        | xargs -0 clang-format --dry-run 2>&1)
      if [ -n "$DIFF" ]; then
        echo "Les fichiers suivants ne sont pas formatés :"
        echo "$DIFF"
        echo ""
        echo "Exécutez 'find src/ include/ -name \"*.cpp\" -o -name \"*.h\" | xargs clang-format -i'"
        exit 1
      fi
```

---

## Bonnes pratiques

**Choisir le style une fois, puis ne plus en parler.** Le fichier `.clang-format` est une décision d'équipe prise en début de projet. Une fois validé, il ne devrait évoluer que rarement et de manière délibérée. Les modifications fréquentes du style génèrent du bruit dans l'historique Git et frustrent les développeurs.

**Partir d'un style prédéfini.** Construire un style de zéro en configurant les 150+ options est un exercice pénible et inutile. Partez de `Google`, `LLVM`, ou `Microsoft` selon les habitudes de l'équipe, puis ajustez les 5 à 10 options qui divergent de vos conventions.

**Versionner le fichier `.clang-format`.** Comme `.clang-tidy`, le fichier de formatage fait partie du code source. Il doit être dans le dépôt Git, versionné, et modifié via des pull requests.

**Formater à la sauvegarde dans l'IDE.** C'est le point d'intégration le plus naturel. Le développeur ne pense jamais au formatage — il écrit du code, sauvegarde, et le code est automatiquement formaté. Le pre-commit hook et la CI ne sont que des filets de sécurité pour les cas où l'IDE n'est pas configuré.

**Ne pas reformater le code tiers.** Le répertoire `third_party/` ou `vendor/` devrait être exclu du formatage. Un fichier `.clang-format` contenant `DisableFormat: true` dans ce répertoire empêche tout reformatage :

```yaml
# third_party/.clang-format
---
DisableFormat: true  
SortIncludes: Never  
```

⏭️ [Intégration dans le workflow de développement](/32-analyse-statique/04-integration-workflow.md)
