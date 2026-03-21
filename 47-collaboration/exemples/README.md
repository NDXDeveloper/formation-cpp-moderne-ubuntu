# Chapitre 47 — Collaboration et Maintenance : Exemples

## Compilation

Necessite **GCC 15+** pour `std::print`/`std::println` (C++23).

```bash
cd exemples/  
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++-15  
cmake --build build --parallel $(nproc)  
```

> Note : le chapitre 47 est principalement organisationnel (workflows Git,
> pre-commit hooks YAML, processus de code review, dette technique, SemVer).
> Les exemples ci-dessous sont construits a partir des snippets C++ illustratifs
> des sections techniques.

## Exemples

### 01_version_demo.cpp + 01_version.h.in

| | |
|---|---|
| **Section** | 47.6 — Semantic Versioning et changelogs |
| **Fichier source** | `06-semantic-versioning.md` |
| **Description** | Demonstration de `configure_file` CMake pour generer un `version.h` a partir d'un template. Le projet est versionne `2.4.1` dans le `CMakeLists.txt`, et le binaire affiche cette version via `--version`. |
| **Sortie attendue** | Sans argument : `myproject 2.4.1 — run with --version for version info` + Major/Minor/Patch. Avec `--version` : `myproject 2.4.1`. |

```bash
./build/01_version_demo
# myproject 2.4.1 — run with --version for version info
#   Major: 2
#   Minor: 4
#   Patch: 1

./build/01_version_demo --version
# myproject 2.4.1
```

---

### 02_review_bugs.cpp

| | |
|---|---|
| **Section** | 47.4 — Code reviews efficaces |
| **Fichier source** | `04-code-reviews.md` |
| **Description** | Trois bugs subtils C++ frequemment detectes en code review, chacun commente (version mauvaise) et corrige (version compilable). Bug 1 : dangling `string_view`. Bug 2 : utilisation apres `std::move`. Bug 3 : capture de reference vers temporaire. |
| **Sortie attendue** | `Greeting: Hello, Nicolas!`, `Processed 5 elements`, `Counter: 12, 11, 10`. |

---

## Sections sans code compilable

| Section | Contenu | Pourquoi pas d'exemple compilable |
|---------|---------|----------------------------------|
| 47.1 — Git workflows | GitFlow, trunk-based, commandes Git | Commandes shell, pas de C++ |
| 47.2 — Pre-commit hooks | Framework pre-commit, installation | Configuration Python/YAML |
| 47.3 — Configuration pre-commit | `.pre-commit-config.yaml` | Fichiers YAML |
| 47.5 — Dette technique | Taxonomie, metriques, planification | Processus organisationnel |

---

## Nettoyage

```bash
rm -rf build
```
