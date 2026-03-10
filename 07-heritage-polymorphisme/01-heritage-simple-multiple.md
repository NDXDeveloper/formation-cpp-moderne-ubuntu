🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 7.1 — Héritage simple et multiple

## Chapitre 7 : Héritage et Polymorphisme · Module 3 : Programmation Orientée Objet

---

## Introduction

L'héritage est le mécanisme par lequel une classe — appelée **classe dérivée** — peut acquérir les membres (données et méthodes) d'une autre classe — appelée **classe de base**. C'est l'un des outils fondamentaux de la modélisation orientée objet : il permet d'exprimer des relations **"est-un"** (*is-a*) entre types, de factoriser du code commun et de construire des hiérarchies de types extensibles.

En C++, l'héritage ne se limite pas à une relation simple entre deux classes. Le langage supporte l'**héritage multiple**, qui permet à une classe dérivée d'hériter simultanément de plusieurs classes de base. Cette capacité, absente de langages comme Java ou C# (qui la restreignent aux interfaces), offre une grande expressivité mais introduit des problématiques spécifiques — en particulier le célèbre **problème du diamant** — qui nécessitent une compréhension précise de la disposition mémoire des objets.

Cette section pose les bases théoriques et syntaxiques de l'héritage avant de plonger dans les sous-sections dédiées. Elle clarifie les concepts fondamentaux, les différences entre les modes d'héritage et les raisons pour lesquelles l'héritage multiple demande une attention particulière en C++.

---

## Pourquoi hériter ?

L'héritage répond à deux besoins distincts qui sont souvent confondus :

**La réutilisation de code.** Une classe `Vehicule` définit des attributs comme la vitesse, le poids et des méthodes comme `accelerer()`. Les classes `Voiture` et `Camion` héritent de `Vehicule` pour ne pas dupliquer cette logique commune. C'est l'héritage **d'implémentation**.

**La substitution de type.** Un pointeur ou une référence vers `Vehicule` peut désigner n'importe quel objet dérivé (`Voiture`, `Camion`). Le code client manipule l'abstraction sans connaître le type concret. C'est l'héritage **d'interface**, qui prend tout son sens avec le polymorphisme dynamique (section 7.3).

En C++ moderne, la recommandation est de **favoriser l'héritage d'interface** (classes abstraites, fonctions virtuelles pures) et de recourir à la **composition** plutôt qu'à l'héritage d'implémentation quand la relation n'est pas naturellement un "est-un". Ce principe — souvent résumé par *"prefer composition over inheritance"* — est un fil conducteur des C++ Core Guidelines.

---

## Les trois modes d'héritage

C++ distingue trois modes d'héritage qui contrôlent la **visibilité des membres hérités** dans la classe dérivée et pour le code extérieur. Le mode est spécifié par le mot-clé placé devant le nom de la classe de base :

```cpp
class Derivee : public Base { };      // Héritage public  
class Derivee : protected Base { };   // Héritage protégé  
class Derivee : private Base { };     // Héritage privé  
```

Le comportement de chaque mode se résume ainsi :

| Membre dans `Base` | Héritage `public` | Héritage `protected` | Héritage `private` |  
|---|---|---|---|  
| `public` | reste `public` | devient `protected` | devient `private` |  
| `protected` | reste `protected` | reste `protected` | devient `private` |  
| `private` | inaccessible | inaccessible | inaccessible |

L'héritage **`public`** est de loin le plus courant. C'est le seul qui préserve la relation "est-un" : un pointeur vers la classe de base peut pointer vers un objet dérivé. Les héritages `protected` et `private` servent à l'héritage d'implémentation (réutilisation interne sans exposer l'interface de la classe de base).

> ⚠️ **Attention au comportement par défaut.** Lorsqu'on utilise le mot-clé `class`, l'héritage par défaut est **`private`**. Avec `struct`, il est **`public`**. Oublier le `public` est une erreur fréquente chez les débutants :  
>
> ```cpp
> class Voiture : Vehicule { };          // Héritage PRIVATE (implicite avec class)
> class Voiture : public Vehicule { };   // Héritage PUBLIC  (intention habituelle)
> ```

---

## Héritage simple vs héritage multiple : vue d'ensemble

### Héritage simple

L'héritage simple est la forme la plus directe : une classe dérivée possède **une seule classe de base directe**. La hiérarchie forme un arbre.

```
    Animal
      │
   ┌──┴──┐
  Chat  Chien
```

La disposition mémoire est prévisible : l'objet dérivé contient un sous-objet de la classe de base, suivi de ses propres membres. C'est le cas couvert en détail dans la section 7.1.1.

### Héritage multiple

L'héritage multiple permet à une classe de dériver de **plusieurs classes de base** simultanément :

```cpp
class AmphibienVehicule : public VehiculeTerrestre, public VehiculeMarin {
    // Hérite des deux
};
```

La hiérarchie n'est plus un arbre mais un **graphe orienté acyclique** (DAG). Cela offre une modélisation plus riche mais introduit des questions que l'héritage simple ne pose pas :

- **Ambiguïté de noms** : si `VehiculeTerrestre` et `VehiculeMarin` définissent tous deux une méthode `demarrer()`, laquelle est appelée ? Le compilateur refusera l'appel ambigu et exigera une qualification explicite (`VehiculeTerrestre::demarrer()`).  
- **Duplication de sous-objets** : si les deux classes de base héritent elles-mêmes d'une classe commune `Vehicule`, l'objet `AmphibienVehicule` contiendra **deux copies** du sous-objet `Vehicule`. C'est le problème du diamant.

```
     Vehicule         Vehicule
        │                 │
  VehiculeTerrestre  VehiculeMarin
        │                 │
        └──────┬──────────┘
         AmphibienVehicule
         (2 copies de Vehicule — problème !)
```

L'**héritage virtuel** résout ce problème en garantissant qu'une seule instance du sous-objet commun est partagée. Ce mécanisme, ses implications mémoire et ses contraintes sur les constructeurs sont traités en section 7.1.3.

---

## Ce que l'héritage ne fait pas

Quelques points de vigilance importants pour éviter les malentendus :

**Les constructeurs ne sont pas hérités automatiquement.** Une classe dérivée doit définir ses propres constructeurs. Elle peut invoquer le constructeur de la classe de base via la liste d'initialisation, et depuis C++11 utiliser `using Base::Base;` pour importer explicitement les constructeurs de la base. Mais ce n'est jamais implicite.

**Les opérateurs d'affectation ne sont pas hérités de manière transparente.** L'opérateur `operator=` généré par défaut dans la classe dérivée appelle celui de la base, mais si vous avez défini un `operator=` personnalisé dans la base (Rule of Five, section 6.5), vous devez vous assurer que la version de la classe dérivée appelle correctement la version de la base.

**L'amitié (`friend`) ne se transmet pas.** Si `Base` déclare une fonction `friend`, cette amitié ne s'étend pas aux classes dérivées.

**Le destructeur doit être virtuel dans une classe de base polymorphique.** C'est un point si critique qu'il mérite d'être rappelé ici même s'il est couvert en profondeur en section 7.2. Supprimer un objet dérivé via un pointeur de base avec un destructeur non virtuel est un **comportement indéfini** :

```cpp
Base* ptr = new Derivee();  
delete ptr;  // ⚠️ Comportement indéfini si ~Base() n'est pas virtual  
```

---

## Guide de lecture des sous-sections

Cette section se décompose en trois parties progressives :

| Sous-section | Contenu | Prérequis |  
|---|---|---|  
| **7.1.1 — Héritage simple** | Syntaxe, construction/destruction, ordre d'appel, slicing, `using` pour les constructeurs hérités. | Chapitre 6 (classes, constructeurs) |  
| **7.1.2 — Héritage multiple et problème du diamant** | Syntaxe, ambiguïtés de noms, duplication de sous-objets, diagnostic du problème du diamant. | 7.1.1 |  
| **7.1.3 — Héritage virtuel** | Résolution du diamant avec `virtual`, impact sur la disposition mémoire, contraintes sur les constructeurs, coût et recommandations d'usage. | 7.1.2 |

---

## Sommaire de la section

- [7.1.1 — Héritage simple](/07-heritage-polymorphisme/01.1-heritage-simple.md)  
- [7.1.2 — Héritage multiple et problème du diamant](/07-heritage-polymorphisme/01.2-heritage-multiple.md)  
- [7.1.3 — Héritage virtuel](/07-heritage-polymorphisme/01.3-heritage-virtuel.md)

⏭️ [Héritage simple](/07-heritage-polymorphisme/01.1-heritage-simple.md)
