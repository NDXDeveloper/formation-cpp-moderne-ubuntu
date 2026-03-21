# Chapitre 46 — Organisation et Standards : Exemples

## Compilation

```bash
cd exemples/  
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++-15  
cmake --build build --parallel $(nproc)  
```

> Note : le chapitre 46 est principalement conceptuel (organisation des repertoires,
> conventions de nommage, syntaxe Doxygen, guides de style). Seuls les patterns C++
> testables sont inclus ici.

## Exemples

### 01_pimpl_test.cpp + 01_pimpl.h + 01_pimpl.cpp

| | |
|---|---|
| **Section** | 46.2 — Separation .h/.cpp et compilation incrementale |
| **Fichier source** | `02-separation-h-cpp.md` |
| **Description** | Idiome Pimpl (Pointer to Implementation). Le header public (`01_pimpl.h`) n'expose qu'un `unique_ptr<Impl>` vers un type incomplet. L'implementation (`01_pimpl.cpp`) definit `Impl` avec des types lourds (`atomic`, etc.) invisibles du header. Le destructeur et les move sont definis dans le `.cpp` (exigence de `unique_ptr` avec type incomplet). |
| **Sortie attendue** | `Start: OK`, `Count: 0`, `Pimpl pattern works correctly` |

---

### 02_namespaces.cpp

| | |
|---|---|
| **Section** | 46.3 — Namespaces et eviter la pollution globale |
| **Fichier source** | `03-namespaces.md` |
| **Description** | Namespaces imbriques C++17 (`monprojet::core::detail`), namespace anonyme (liaison interne), et using local. |
| **Sortie attendue** | `monprojet::initialize()`, `Config loaded`, `monprojet::core::detail::helper()`, `Internal counter: 1` |

---

### 03_inline_namespace.cpp

| | |
|---|---|
| **Section** | 46.3 — Namespaces et eviter la pollution globale |
| **Fichier source** | `03-namespaces.md` |
| **Description** | Inline namespaces pour le versioning d'API. `v2` est inline (par defaut), `v1` reste accessible explicitement. `monprojet::Config` resout vers `v2` grace au `inline`. |
| **Sortie attendue** | `Default (v2): localhost:8080 max_conn=100`, `Legacy (v1): old-server:80`, `Explicit v2: new-server tls=1` |

---

## Sections sans code compilable

| Section | Contenu | Pourquoi pas d'exemple compilable |
|---------|---------|----------------------------------|
| 46.1 — Organisation des repertoires | Structure `src/`, `include/`, `tests/` | Conventions de structure, pas de code C++ |
| 46.4.1 — Syntaxe Doxygen | Commentaires `/** */`, `///`, `@param`, `@return` | Syntaxe documentaire, pas executables |
| 46.4.2 — Generation de documentation | Doxyfile, integration CMake, CI/CD | Configuration d'outils, pas de code C++ |
| 46.5.1 — Google C++ Style Guide | Conventions de nommage PascalCase | Description de conventions |
| 46.5.2 — LLVM Style | Conventions CamelCase | Description de conventions |
| 46.5.3 — C++ Core Guidelines | Regles R/C/F, GSL | Principes de design |

---

## Nettoyage

```bash
rm -rf build
```
