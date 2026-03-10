🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 4.3 — Passage de paramètres

## Chapitre 4 · Structures de Contrôle et Fonctions · Niveau Débutant

---

## Introduction

En Python, JavaScript ou Java, quand on passe une variable à une fonction, on n'a généralement pas à se poser de question : le langage s'en charge. En C++, le **mode de passage** d'un paramètre est un choix explicite du développeur, et ce choix a des conséquences directes sur trois aspects fondamentaux du programme : la **performance** (y a-t-il une copie ?), la **sémantique** (la fonction peut-elle modifier l'original ?) et la **sûreté** (risque-t-on un dangling pointer ou une référence invalide ?).

C'est un sujet que les développeurs venant d'autres langages sous-estiment régulièrement. Pourtant, choisir le bon mode de passage est l'un des gestes les plus fréquents et les plus impactants de la programmation C++ au quotidien.

---

## Les quatre modes de passage

C++ propose quatre façons de transmettre une donnée à une fonction. Chacune répond à un besoin différent :

| Mode | Syntaxe | Copie ? | Fonction peut modifier l'original ? | Section |  
|------|---------|---------|-------------------------------------|---------|  
| Par valeur | `void f(int x)` | Oui | Non — travaille sur une copie | 4.3.1 |  
| Par référence | `void f(int& x)` | Non | Oui | 4.3.2 |  
| Par référence constante | `void f(const int& x)` | Non | Non | 4.3.3 |  
| Par pointeur | `void f(int* x)` | Non (le pointeur est copié) | Oui (via déréférencement) | 4.3.4 |

Un exemple rapide pour visualiser les différences :

```cpp
#include <iostream>

void par_valeur(int x)        { x = 99; }  
void par_reference(int& x)    { x = 99; }  
void par_ref_const(const int& x) {  
    // x = 99;  // ❌ Erreur de compilation — const
    std::cout << x << "\n";
}
void par_pointeur(int* x)     { *x = 99; }

int main() {
    int a = 0, b = 0, c = 0, d = 0;

    par_valeur(a);
    par_reference(b);
    par_ref_const(c);
    par_pointeur(&d);

    std::cout << "a=" << a << "\n";  // a=0  (copie modifiée, original intact)
    std::cout << "b=" << b << "\n";  // b=99 (original modifié via référence)
    std::cout << "c=" << c << "\n";  // c=0  (lecture seule, original intact)
    std::cout << "d=" << d << "\n";  // d=99 (original modifié via pointeur)
}
```

Les quatre sous-sections qui suivent détaillent chaque mode en profondeur. Mais avant d'y plonger, il est utile de comprendre les principes qui guident le choix.

---

## Comment choisir le bon mode

Le choix du mode de passage repose sur deux questions simples.

**Question 1 — La fonction doit-elle modifier la donnée originale ?**

Si oui, il faut passer par référence (`&`) ou par pointeur (`*`). Si non, il faut passer par valeur ou par référence constante (`const &`).

**Question 2 — Quel est le coût de la copie ?**

Si le type est « petit » (types primitifs comme `int`, `double`, `char`, `bool`, ou de petites structures de quelques octets), la copie est quasi gratuite — le passage par valeur est approprié. Si le type est « gros » (conteneurs comme `std::vector`, `std::string`, structures complexes, objets polymorphiques), la copie a un coût significatif — le passage par référence constante est préférable.

Ces deux questions se combinent en un arbre de décision simple :

```
La fonction doit-elle modifier l'original ?
│
├── OUI → Passer par référence (Type&)
│         ou par pointeur (Type*) si nullptr est une valeur valide
│
└── NON → Le type est-il petit et trivial à copier ?
          │
          ├── OUI → Passer par valeur (Type)
          │
          └── NON → Passer par référence constante (const Type&)
```

---

## Le cas particulier des pointeurs

Le passage par pointeur est le mode historique hérité du C. En C++ moderne, il est **largement remplacé** par les références pour la plupart des usages. Cependant, le pointeur reste pertinent dans deux situations précises.

La première est quand **la valeur `nullptr` a un sens** — c'est-à-dire quand l'absence de donnée est un cas valide que la fonction doit gérer. Une référence ne peut pas être « nulle » (lier une référence à `nullptr` est un comportement indéfini), tandis qu'un pointeur peut légitimement valoir `nullptr`.

La seconde est l'**interopérabilité avec du code C** ou avec des API système (POSIX, bibliothèques legacy) qui attendent des pointeurs.

En dehors de ces cas, préférez systématiquement les références. Les C++ Core Guidelines le formalisent ainsi :

- **F.60** — Préférez `T*` quand « pas d'argument » est une option valide (signalée par `nullptr`).  
- **F.16** — Pour les paramètres en entrée, utiliser `const T&` pour les types « coûteux à copier ».  
- **F.17** — Pour les paramètres en entrée-sortie, utiliser `T&`.

---

## Résumé des conventions modernes

Le tableau suivant synthétise les recommandations des C++ Core Guidelines et de la majorité des guides de style professionnels :

| Intention | Type petit/trivial | Type gros/complexe |  
|-----------|-------------------|---------------------|  
| Lecture seule (in) | `T` (par valeur) | `const T&` |  
| Modification (in-out) | `T&` | `T&` |  
| Optionnel (nullable) | `T*` ou `std::optional<T>` | `const T*` ou `T*` |  
| Transfert de propriété | `T` (par valeur, avec move) | `std::unique_ptr<T>` |

La dernière ligne (transfert de propriété) fait intervenir la **sémantique de mouvement**, couverte en détail au chapitre 10. Pour l'instant, retenez que passer un `std::unique_ptr` par valeur transfère la propriété de la ressource à la fonction appelée.

---

## Ce qui suit

Les quatre sous-sections suivantes examinent chaque mode en détail, avec des exemples concrets, les pièges à éviter et les cas d'usage précis :

| Sous-section | Mode | Ce que vous y trouverez |  
|--------------|------|------------------------|  
| **4.3.1** | Par valeur | Copie implicite, types primitifs, idiome *sink parameter* |  
| **4.3.2** | Par référence (`&`) | Modification de l'original, alias, restrictions |  
| **4.3.3** | Par référence constante (`const &`) | Lecture sans copie, compatibilité avec les temporaires |  
| **4.3.4** | Par pointeur (`*`) | Nullable, interopérabilité C, arithmétique d'adresses |

---

> 💡 **Conseil pédagogique** — Si vous ne devez retenir qu'une seule règle de cette section, c'est celle-ci : **`const T&` est le mode par défaut pour les types non primitifs en lecture seule**. C'est la convention la plus utilisée en C++ moderne, et elle vous évitera la grande majorité des copies inutiles tout en protégeant la donnée d'origine.

⏭️ [Par valeur](/04-structures-controle-fonctions/03.1-par-valeur.md)
