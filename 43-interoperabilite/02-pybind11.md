🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 43.2 — Appeler du C++ depuis Python (pybind11)

## Module 15 : Interopérabilité · Niveau Expert

---

## Introduction

Le binôme C++/Python est l'une des associations les plus productives de l'industrie logicielle. Python excelle dans le prototypage rapide, le scripting, l'orchestration et l'écosystème data science/ML. Le C++ apporte la performance brute, le contrôle mémoire et l'accès au matériel. Plutôt que de choisir l'un au détriment de l'autre, les projets modernes combinent les deux : un cœur de calcul en C++ exposé à Python comme un module natif importable avec un simple `import`.

Cette combinaison est omniprésente. NumPy, PyTorch, TensorFlow, OpenCV, LLVM (via ses bindings), des simulateurs physiques, des moteurs de pricing financier — tous suivent ce modèle. Le code critique tourne en C++ ; Python sert d'interface utilisateur, de langage de glue et de couche de test.

La question n'est pas *si* vous rencontrerez ce besoin, mais *comment* le réaliser proprement.

---

## Le problème : la CPython C API

Python (l'implémentation CPython) est lui-même écrit en C et expose une API C pour créer des modules d'extension natifs. En théorie, on peut directement écrire un module Python en C/C++ en manipulant cette API : créer des objets `PyObject*`, gérer manuellement le comptage de références (`Py_INCREF`, `Py_DECREF`), enregistrer des méthodes dans des tableaux de structures `PyMethodDef`, etc.

En pratique, cette approche est **extrêmement verbeuse, fragile et sujette aux fuites mémoire**. Exposer une seule classe C++ avec quelques méthodes demande des centaines de lignes de boilerplate. La moindre erreur de comptage de références provoque des crashs ou des fuites silencieuses. Et chaque évolution de l'API CPython entre versions majeures peut casser le code.

Voici un aperçu de ce que représente l'exposition d'une fonction triviale via l'API C brute :

```cpp
// Extension CPython brute — NE PAS FAIRE en 2026
#include <Python.h>

static PyObject* my_add(PyObject* self, PyObject* args) {
    int a, b;
    if (!PyArg_ParseTuple(args, "ii", &a, &b))
        return nullptr;
    return PyLong_FromLong(a + b);
}

static PyMethodDef methods[] = {
    {"add", my_add, METH_VARARGS, "Add two integers"},
    {nullptr, nullptr, 0, nullptr}
};

static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT, "mymod", nullptr, -1, methods
};

PyMODINIT_FUNC PyInit_mymod(void) {
    return PyModule_Create(&module);
}
```

Cela représente 20 lignes pour une fonction qui additionne deux entiers. Pour une classe avec constructeur, destructeur, propriétés et méthodes, comptez des centaines de lignes de code mécanique, avec gestion manuelle de `PyObject*` partout.

C'est exactement le problème que pybind11 résout.

---

## pybind11 : l'outil de référence

### Philosophie

pybind11 est une bibliothèque header-only C++11 qui permet d'exposer des fonctions, des classes et des données C++ à Python avec un minimum de boilerplate. Son nom vient de sa parenté avec Boost.Python, dont il est un successeur spirituel, réécrit à partir de zéro en C++ moderne — plus léger, plus rapide à compiler, et sans dépendance à Boost.

Le principe directeur de pybind11 est la **correspondance naturelle** entre les types C++ et Python. Un `std::string` devient une `str` Python. Un `std::vector<int>` devient une `list`. Un `std::map<std::string, double>` devient un `dict`. Les exceptions C++ sont converties en exceptions Python. Les smart pointers sont gérés automatiquement. Le développeur décrit *ce qu'il veut exposer*, et pybind11 génère le code CPython nécessaire au moment de la compilation.

### L'exemple fondateur

Voici l'équivalent pybind11 de l'exemple CPython brut précédent, étendu à une classe complète :

```cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>        // Conversions automatiques std::vector, std::map, etc.
#include <string>
#include <vector>
#include <numeric>

namespace py = pybind11;

// Une classe C++ ordinaire — aucune modification nécessaire
class DataProcessor {  
public:  
    explicit DataProcessor(std::string name)
        : name_(std::move(name)) {}

    void load(const std::vector<double>& values) {
        data_ = values;
    }

    double mean() const {
        if (data_.empty()) return 0.0;
        double sum = std::accumulate(data_.begin(), data_.end(), 0.0);
        return sum / static_cast<double>(data_.size());
    }

    size_t size() const { return data_.size(); }
    const std::string& name() const { return name_; }

private:
    std::string          name_;
    std::vector<double>  data_;
};

// La totalité du binding — c'est tout
PYBIND11_MODULE(dataproc, m) {
    m.doc() = "Data processing module";

    py::class_<DataProcessor>(m, "DataProcessor")
        .def(py::init<std::string>(), py::arg("name"))
        .def("load", &DataProcessor::load, py::arg("values"))
        .def("mean", &DataProcessor::mean)
        .def("size", &DataProcessor::size)
        .def_property_readonly("name", &DataProcessor::name)
        .def("__repr__", [](const DataProcessor& dp) {
            return "<DataProcessor '" + dp.name() + "' size=" +
                   std::to_string(dp.size()) + ">";
        });
}
```

Côté Python, l'utilisation est immédiate et idiomatique :

```python
import dataproc

dp = dataproc.DataProcessor("sensor_a")  
dp.load([1.5, 2.3, 4.7, 3.1])  
print(dp.mean())    # 2.9  
print(dp.size)      # 4  — propriété, pas méthode  
print(dp.name)      # "sensor_a"  
print(dp)           # <DataProcessor 'sensor_a' size=4>  
```

Le contraste avec l'approche CPython brute est frappant : le binding pybind11 est déclaratif, compact, et le code C++ sous-jacent (`DataProcessor`) n'a subi **aucune modification** pour être exposé.

---

## Ce que pybind11 gère automatiquement

La puissance de pybind11 réside dans tout ce qu'il prend en charge de manière transparente, sans intervention du développeur :

### Conversion de types bidirectionnelle

pybind11 convertit automatiquement les types courants entre C++ et Python dans les deux sens :

| Type C++ | Type Python | Direction |
|---|---|---|
| `int`, `long`, `int64_t` | `int` | ↔ |
| `float`, `double` | `float` | ↔ |
| `bool` | `bool` | ↔ |
| `std::string` | `str` | ↔ |
| `const char*` | `str` | C++ → Python |
| `std::vector<T>` | `list` | ↔ |
| `std::map<K, V>` | `dict` | ↔ |
| `std::set<T>` | `set` | ↔ |
| `std::pair<A, B>` | `tuple` | ↔ |
| `std::tuple<A...>` | `tuple` | ↔ |
| `std::optional<T>` | `T` ou `None` | ↔ |
| `std::variant<A, B>` | `A` ou `B` | ↔ |
| `std::unique_ptr<T>` | objet Python (ownership transféré) | C++ → Python |
| `std::shared_ptr<T>` | objet Python (ref-counted) | ↔ |

Ces conversions sont activées par l'include `<pybind11/stl.h>`. Sans cet include, les conteneurs STL ne sont pas convertis automatiquement.

### Gestion de la mémoire et ownership

pybind11 gère la durée de vie des objets C++ en les attachant au système de garbage collection de Python. Quand un objet Python encapsulant un objet C++ est détruit par le GC, le destructeur C++ est appelé automatiquement.

Pour les smart pointers, le comportement est naturel : un `std::unique_ptr<T>` transfère la propriété à Python (move semantics), et un `std::shared_ptr<T>` partage la propriété entre C++ et Python via le comptage de références.

### Conversion des exceptions

Les exceptions C++ standard sont automatiquement converties en exceptions Python :

| Exception C++ | Exception Python |
|---|---|
| `std::runtime_error` | `RuntimeError` |
| `std::invalid_argument` | `ValueError` |
| `std::out_of_range` | `IndexError` |
| `std::domain_error` | `ValueError` |
| `std::overflow_error` | `OverflowError` |
| `std::bad_alloc` | `MemoryError` |
| `std::exception` (autre) | `RuntimeError` |

Contrairement à l'interface C manuelle (section 43.1), le développeur n'a pas besoin d'écrire de blocs `try/catch` dans le code de binding — pybind11 intercepte les exceptions C++ et les propage comme des exceptions Python avant qu'elles ne traversent la frontière C.

### Support du protocole Python

pybind11 permet d'implémenter les méthodes spéciales Python (`__repr__`, `__str__`, `__len__`, `__getitem__`, `__iter__`, etc.) directement dans le binding, rendant les objets C++ aussi idiomatiques que des objets Python natifs.

---

## Sous le capot : comment ça fonctionne

Il est utile de comprendre l'architecture interne de pybind11 pour diagnostiquer les problèmes et faire des choix éclairés.

### La chaîne de compilation

```
Code C++ (.cpp)
    │
    ├── Classe C++ (DataProcessor)          ← Code métier inchangé
    │
    └── Bloc PYBIND11_MODULE(...)           ← Déclarations de binding
            │
            ▼
    Compilateur C++ (g++ / clang++)
    avec includes pybind11 + Python
            │
            ▼
    Module .so (.pyd sur Windows)           ← Bibliothèque partagée
            │
            ▼
    import dataproc                         ← Python charge le .so
```

Le module compilé est une bibliothèque partagée (`.so` sur Linux) dont le point d'entrée est une fonction `extern "C"` nommée `PyInit_<module_name>`. C'est exactement le mécanisme décrit en section 43.1 — pybind11 génère cette fonction `extern "C"` automatiquement via la macro `PYBIND11_MODULE`.

### Ce que la macro PYBIND11_MODULE génère

La macro `PYBIND11_MODULE(dataproc, m)` génère, en simplifiant :

- Une fonction `extern "C" PyObject* PyInit_dataproc()` — le point d'entrée que CPython recherche lors de `import dataproc`.  
- L'initialisation d'un objet module Python.  
- L'appel du corps du bloc (les `.def(...)`, `.def_property_readonly(...)`, etc.) qui enregistre les fonctions et classes dans le module.

Chaque appel `.def("mean", &DataProcessor::mean)` enregistre un callable Python qui, à l'exécution :

1. Décompacte les arguments Python en types C++ (via les convertisseurs de types).
2. Appelle la méthode C++ réelle.
3. Convertit la valeur de retour C++ en objet Python.
4. Intercepte toute exception C++ et la convertit en exception Python.

Ce mécanisme repose sur les templates C++ — c'est de la métaprogrammation. Le type des arguments et de la valeur de retour est déduit à la compilation depuis le pointeur de fonction `&DataProcessor::mean`. Le code de conversion est instancié une seule fois par type et optimisé par le compilateur.

---

## pybind11 vs les alternatives

### Boost.Python

Boost.Python est l'ancêtre de pybind11 et partage une syntaxe très similaire. Cependant, il dépend de la bibliothèque Boost — une dépendance lourde — et les temps de compilation sont significativement plus élevés. En 2026, pybind11 l'a largement supplanté pour les nouveaux projets. Boost.Python reste pertinent uniquement pour les projets historiques déjà intégrés dans l'écosystème Boost.

### nanobind : le successeur

nanobind est développé par Wenzel Jakob, le créateur de pybind11, et représente la prochaine génération. Il est conçu pour être plus rapide à compiler, produire des binaires plus petits, et offrir de meilleures performances à l'exécution — au prix d'une compatibilité réduite avec les anciennes versions de Python et C++.

| Caractéristique | pybind11 | nanobind |
|---|---|---|
| Compatibilité C++ | C++11 et ultérieur | C++17 minimum |
| Compatibilité Python | Python 3.6+ | Python 3.8+ |
| Taille des binaires | Moyenne | Nettement plus petits |
| Temps de compilation | Modéré | Réduit (jusqu'à 2-4× plus rapide) |
| Maturité écosystème | Très large (2015+) | En croissance rapide (2022+) |
| API | Stable, riche | Très similaire, quelques différences |
| Sémantique de propriété | Copie par défaut | Move par défaut |

nanobind est couvert en détail dans la section 43.2.4. Pour les nouveaux projets en 2026 ciblant C++17 ou ultérieur, nanobind est le choix recommandé. Pour les bases de code existantes utilisant déjà pybind11, la migration n'est pas urgente — l'API est très proche et pybind11 reste activement maintenu.

### ctypes et cffi

Les modules `ctypes` et `cffi` de Python permettent d'appeler des fonctions C depuis Python sans code de binding C++ — uniquement depuis le côté Python. Ils s'appuient sur l'interface `extern "C"` décrite en section 43.1.

Leur principal avantage est l'absence de compilation côté C++ : on charge directement un `.so` existant. Leur principal inconvénient est l'absence de support natif pour les classes C++, les templates, les surcharges et les conversions de types complexes. Ils sont adaptés pour appeler des API C simples, pas pour exposer une API C++ riche.

### Cython

Cython est un langage hybride Python/C qui compile en code C. Il peut appeler du C et du C++ et offre d'excellentes performances pour le code numérique. Cependant, il introduit un langage supplémentaire (`.pyx`), une étape de compilation spécifique, et l'exposition de classes C++ complexes est plus laborieuse qu'avec pybind11. Cython reste très utilisé dans l'écosystème scientifique Python (SciPy, scikit-learn), mais pour de l'interopérabilité C++ pure, pybind11/nanobind offre une expérience développeur supérieure.

### SWIG

SWIG (*Simplified Wrapper and Interface Generator*) est un générateur de bindings multi-langage (Python, Java, Ruby, etc.) à partir de fichiers d'interface. Son approche par génération de code est puissante pour les API très larges, mais le code généré est difficile à déboguer, la syntaxe du fichier `.i` est spécifique, et le résultat côté Python est souvent moins idiomatique. SWIG reste pertinent dans les projets industriels qui ciblent plusieurs langages simultanément, mais pour du Python uniquement, pybind11 est préféré.

---

## Cas d'usage typiques

**Accélération de code critique.** Un pipeline de données Python identifie un goulot d'étranglement dans un algorithme de traitement. L'algorithme est réécrit en C++, exposé via pybind11, et appelé depuis le même code Python — sans modifier l'architecture globale.

**Wrapping de bibliothèques existantes.** Une bibliothèque C++ de simulation physique, de calcul financier ou de traitement d'image doit être rendue accessible aux data scientists de l'équipe, qui travaillent en Python. pybind11 fournit la couche de binding sans toucher au code C++ existant.

**Prototypage rapide avec backend performant.** Le prototype est développé en Python pour valider l'algorithme et l'UX. Le cœur de calcul est ensuite migré en C++ pour la production, exposé via pybind11, et le code Python d'orchestration reste inchangé.

**Tests d'une bibliothèque C++ depuis Python.** pytest est plus rapide à écrire que Google Test pour des tests d'intégration de haut niveau. Exposer la bibliothèque à Python permet d'utiliser l'écosystème de test Python pour valider le comportement fonctionnel, tout en conservant les tests unitaires C++ (GTest) pour le comportement bas niveau.

---

## Ce que couvrent les sous-sections

Les sous-sections suivantes détaillent la mise en œuvre concrète :

**43.2.1 — Installation et configuration.** Intégration de pybind11 dans un projet CMake via `FetchContent` ou le package système, configuration du compilateur et de l'interpréteur Python.

**43.2.2 — Exposition de fonctions et classes.** Syntaxe complète des bindings : fonctions libres, méthodes, constructeurs, propriétés, surcharges, héritage, méthodes statiques, enums.

**43.2.3 — Gestion des types et conversions.** Conversions automatiques, buffers NumPy (`py::array`), passage par référence vs par copie, return value policies, et gestion fine de la propriété mémoire.

**43.2.4 — nanobind : alternative moderne et plus rapide.** Migration depuis pybind11, différences d'API, gains de performance, et recommandations pour les nouveaux projets en 2026.

---

## Prérequis spécifiques à cette section

- Les concepts de la section 43.1 (`extern "C"`, ABI C, handle opaque) — pybind11 les abstrait, mais les comprendre aide au diagnostic.  
- CMake : `FetchContent`, `target_link_libraries`, generator expressions (chapitre 26).  
- Smart pointers : `std::unique_ptr`, `std::shared_ptr` (chapitre 9) — pybind11 les gère automatiquement, mais il faut comprendre les implications de propriété.  
- Une installation Python 3.8+ fonctionnelle avec les headers de développement (`python3-dev` sur Ubuntu).

> 💡 *pybind11 ne modifie pas votre code C++ existant. Le code de binding est séparé du code métier. C'est un principe architectural fondamental : la bibliothèque C++ ne dépend pas de Python, et le binding est une couche d'adaptation indépendante.*

⏭️ [Installation et configuration](/43-interoperabilite/02.1-installation-pybind11.md)
