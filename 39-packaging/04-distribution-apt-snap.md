🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 39.4 — Distribution via gestionnaires de paquets (apt, snap)

## Fermer la boucle : de l'artefact au `apt install`

---

## Introduction

Les sections précédentes de ce chapitre ont couvert la création de paquets — DEB, RPM, AppImage. Mais un paquet `.deb` posé sur un serveur de fichiers ou attaché à une release GitHub n'est qu'un fichier à télécharger manuellement. Pour que vos utilisateurs puissent installer votre logiciel avec `sudo apt install syswatch` et recevoir les mises à jour avec `sudo apt upgrade`, il faut que ce paquet soit accessible via un **dépôt APT** — un serveur structuré que le gestionnaire de paquets sait interroger, vérifier et synchroniser.

Cette section couvre les différentes méthodes pour publier et distribuer vos paquets C++ : hébergement d'un dépôt APT privé, publication sur un Snap Store, et utilisation de services d'hébergement d'artefacts. L'objectif est de transformer votre pipeline CI/CD en une chaîne de distribution complète, du commit au `apt install` sur la machine cible.

---

## Comment fonctionne un dépôt APT

Avant de créer un dépôt, il est utile de comprendre ce qu'`apt` attend quand il contacte un serveur.

### Structure d'un dépôt

Un dépôt APT est un serveur HTTP (ou HTTPS, ou un simple répertoire local) contenant une arborescence standardisée. Quand vous ajoutez une entrée dans `/etc/apt/sources.list` :

```
deb https://packages.exemple.com/apt stable main
```

`apt` interprète cette ligne comme suit : aller sur `https://packages.exemple.com/apt`, chercher la distribution `stable`, dans le composant `main`. Concrètement, il télécharge le fichier d'index situé à :

```
https://packages.exemple.com/apt/dists/stable/main/binary-amd64/Packages
```

Ce fichier `Packages` est un index texte qui liste tous les paquets disponibles avec leurs métadonnées (nom, version, dépendances, checksum, chemin du `.deb`). `apt` le compare à sa base locale, identifie les paquets à installer ou mettre à jour, puis télécharge les `.deb` correspondants depuis le sous-répertoire `pool/`.

L'arborescence complète d'un dépôt ressemble à ceci :

```
apt-repo/
├── dists/
│   └── stable/
│       ├── InRelease              # Index signé (GPG, inline)
│       ├── Release                # Métadonnées de la distribution
│       ├── Release.gpg            # Signature détachée
│       └── main/
│           └── binary-amd64/
│               ├── Packages       # Index des paquets (texte)
│               ├── Packages.gz    # Index compressé
│               └── Packages.xz    # Index compressé (alternatif)
└── pool/
    └── main/
        └── s/
            └── syswatch/
                └── syswatch_1.2.0-1_amd64.deb
```

### Signature GPG : pourquoi et comment

APT refuse par défaut d'installer des paquets provenant d'un dépôt non signé — et il a raison. Sans signature cryptographique, rien ne garantit que le paquet n'a pas été altéré en transit ou remplacé par un attaquant sur le serveur.

La signature fonctionne ainsi : vous générez une paire de clés GPG, vous signez les fichiers d'index (`Release` → `InRelease` et `Release.gpg`), et vous distribuez la clé publique aux machines clientes. APT vérifie la signature avant de faire confiance aux checksums listés dans le fichier `Release`, et ces checksums garantissent l'intégrité de chaque `.deb`.

Génération d'une clé dédiée au dépôt :

```bash
# Générer une clé GPG sans passphrase (pour usage CI)
gpg --batch --gen-key << 'EOF'
%no-protection
Key-Type: RSA  
Key-Length: 4096  
Subkey-Type: RSA  
Subkey-Length: 4096  
Name-Real: Syswatch APT Repository  
Name-Email: sre@exemple.com  
Expire-Date: 3y  
%commit
EOF

# Exporter la clé publique
gpg --armor --export sre@exemple.com > syswatch-archive-keyring.gpg
```

La clé publique sera installée sur les machines clientes dans `/usr/share/keyrings/` (approche moderne) ou `/etc/apt/trusted.gpg.d/` (approche legacy). Nous y reviendrons lors de la configuration côté client.

---

## Option 1 : Dépôt APT avec reprepro

**reprepro** est l'outil le plus utilisé pour gérer un dépôt APT privé. Il gère l'ajout et la suppression de paquets, la génération des fichiers d'index, et la signature GPG de manière transparente.

### Installation et configuration

```bash
sudo apt install reprepro
```

Créez la structure du dépôt :

```bash
mkdir -p /var/www/apt-repo/conf
```

Le fichier de configuration principal est `conf/distributions` :

```bash
cat > /var/www/apt-repo/conf/distributions << 'EOF'  
Origin: Exemple SRE  
Label: Syswatch Repository  
Codename: stable  
Architectures: amd64 arm64  
Components: main  
Description: Depot interne pour les outils SRE  
SignWith: sre@exemple.com  
EOF  
```

Chaque champ a un rôle. **`Codename`** est le nom de la distribution (`stable`, `jammy`, `noble` — c'est ce qui apparaît dans le `sources.list`). **`Architectures`** liste les architectures supportées. **`Components`** définit les composants (`main`, `contrib`, etc.). **`SignWith`** indique l'identifiant de la clé GPG utilisée pour signer le dépôt.

Le fichier optionnel `conf/options` contrôle le comportement global :

```bash
cat > /var/www/apt-repo/conf/options << 'EOF'  
verbose  
basedir /var/www/apt-repo  
EOF  
```

### Ajout d'un paquet

```bash
reprepro -b /var/www/apt-repo includedeb stable syswatch_1.2.0-1_amd64.deb
```

Cette commande copie le `.deb` dans `pool/`, met à jour le fichier `Packages`, recalcule les checksums dans `Release`, et signe le tout. Vérifions :

```bash
$ reprepro -b /var/www/apt-repo list stable
stable|main|amd64: syswatch 1.2.0-1
```

### Mise à jour d'un paquet

Quand une nouvelle version est prête, il suffit de l'ajouter :

```bash
reprepro -b /var/www/apt-repo includedeb stable syswatch_1.3.0-1_amd64.deb
```

`reprepro` remplace automatiquement l'ancienne version. Si vous voulez conserver les anciennes versions (pour permettre un rollback), ajoutez `Tracking: keep` dans `conf/distributions`.

### Suppression d'un paquet

```bash
reprepro -b /var/www/apt-repo remove stable syswatch
```

### Servir le dépôt

Le dépôt est un répertoire statique — n'importe quel serveur HTTP peut le servir. La configuration Nginx minimale :

```nginx
server {
    listen 443 ssl;
    server_name packages.exemple.com;

    root /var/www/apt-repo;
    autoindex off;

    location / {
        try_files $uri =404;
    }
}
```

Pour un usage interne, un simple `python3 -m http.server` ou un bucket S3 avec CloudFront conviennent également.

### Configuration côté client

Sur chaque machine qui doit accéder au dépôt, deux fichiers sont nécessaires : la clé publique GPG et la source APT.

```bash
# Installer la clé publique (approche moderne signed-by)
sudo cp syswatch-archive-keyring.gpg /usr/share/keyrings/

# Ajouter la source APT
cat | sudo tee /etc/apt/sources.list.d/syswatch.list << 'EOF'  
deb [signed-by=/usr/share/keyrings/syswatch-archive-keyring.gpg] https://packages.exemple.com/apt stable main  
EOF  

# Mettre à jour et installer
sudo apt update  
sudo apt install syswatch  
```

L'attribut `signed-by` dans la source APT lie explicitement ce dépôt à une clé spécifique. C'est plus sûr que l'ancienne approche qui ajoutait la clé au trousseau global `trusted.gpg` — une clé dans le trousseau global est de confiance pour **tous** les dépôts, ce qui élargit inutilement la surface d'attaque.

---

## Option 2 : Dépôt APT simplifié (flat repository)

Pour les cas les plus simples — un ou deux paquets, peu de versions — un **dépôt plat** évite la complexité de `reprepro`. Il s'agit d'un répertoire contenant les `.deb` et un fichier `Packages` généré manuellement.

```bash
mkdir -p /var/www/apt-simple

# Copier les paquets
cp syswatch_1.2.0-1_amd64.deb /var/www/apt-simple/

# Générer l'index
cd /var/www/apt-simple  
dpkg-scanpackages --multiversion . /dev/null > Packages  
gzip -k Packages  
```

Côté client :

```bash
deb [trusted=yes] https://packages.exemple.com/apt-simple ./
```

L'attribut `[trusted=yes]` désactive la vérification GPG — acceptable sur un réseau interne de confiance, inacceptable pour tout dépôt accessible depuis Internet. Pour un dépôt plat signé, il faut signer le fichier `Release` manuellement avec `gpg --clearsign`.

Cette approche fonctionne mais ne gère ni les versions multiples, ni la suppression propre, ni les architectures multiples. Elle est utile pour le prototypage et les petites équipes, pas pour la production.

---

## Option 3 : Services d'hébergement d'artefacts

Plutôt que de gérer votre propre infrastructure, plusieurs services hébergent des dépôts APT (et RPM) clé en main.

### Cloudsmith

Cloudsmith est un service SaaS qui héberge des dépôts pour de nombreux formats (DEB, RPM, PyPI, npm, Docker, etc.). L'intégration CI est directe :

```bash
# Depuis votre pipeline CI
pip install cloudsmith-cli  
cloudsmith push deb exemple/syswatch/ubuntu/noble \  
    syswatch_1.2.0-1_amd64.deb
```

Côté client, Cloudsmith fournit la source APT et la clé GPG prêtes à l'emploi.

### Artifactory (JFrog)

Artifactory est la solution d'entreprise dominante pour l'hébergement d'artefacts. Il supporte nativement les dépôts APT et RPM, avec gestion des permissions, réplication, et promotion entre environnements (dev → staging → production).

L'upload se fait via l'API REST :

```bash
curl -u "${ARTIFACTORY_USER}:${ARTIFACTORY_TOKEN}" \
     -XPUT "https://artifactory.exemple.com/apt-local/pool/syswatch_1.2.0-1_amd64.deb;deb.distribution=stable;deb.component=main;deb.architecture=amd64" \
     -T syswatch_1.2.0-1_amd64.deb
```

Artifactory régénère automatiquement les index et signe le dépôt.

### GitHub Releases + script d'installation

Pour les projets open source ou les outils distribués à une communauté de développeurs, GitHub Releases est souvent suffisant. Le `.deb` est attaché comme artefact de release, et un script d'installation automatise le téléchargement :

```bash
#!/bin/sh
# install.sh — Télécharge et installe la dernière version de syswatch
set -eu

REPO="exemple/syswatch"  
LATEST=$(curl -s "https://api.github.com/repos/${REPO}/releases/latest" \  
         | grep -o '"tag_name": *"[^"]*"' | cut -d'"' -f4)
DEB_URL="https://github.com/${REPO}/releases/download/${LATEST}/syswatch_${LATEST#v}-1_amd64.deb"

TMPFILE=$(mktemp)  
curl -sL "$DEB_URL" -o "$TMPFILE"  
sudo dpkg -i "$TMPFILE"  
sudo apt install -f -y  
rm -f "$TMPFILE"  

echo "syswatch ${LATEST} installe avec succes"
```

L'utilisateur installe avec :

```bash
curl -sSL https://raw.githubusercontent.com/exemple/syswatch/main/install.sh | sh
```

Cette approche est simple et courante, mais elle ne fournit pas de mises à jour automatiques. Chaque upgrade nécessite de relancer le script ou de télécharger manuellement le nouveau `.deb`. Pour une distribution sérieuse, un dépôt APT (même hébergé sur GitHub Pages ou S3) est préférable.

---

## Publication sur le Snap Store

Si vous avez construit un snap (voir section 39.3), la publication passe par le **Snap Store**, le dépôt centralisé géré par Canonical.

### Prérequis

Créez un compte sur [snapcraft.io](https://snapcraft.io) et authentifiez-vous en CLI :

```bash
snapcraft login
```

### Enregistrement du nom

Chaque snap a un nom unique réservé dans le store :

```bash
snapcraft register syswatch
```

### Publication

Le workflow de publication utilise les **canaux** du Snap Store, qui permettent un déploiement progressif :

```bash
# Publier sur le canal edge (développement, instable)
snapcraft upload syswatch_1.2.0_amd64.snap --release edge

# Après validation, promouvoir vers beta puis stable
snapcraft release syswatch 42 beta  
snapcraft release syswatch 42 stable  
```

Le chiffre `42` est le numéro de révision attribué automatiquement par le store lors de l'upload. La promotion entre canaux ne nécessite pas de re-upload — c'est la même révision qui est rendue disponible sur un canal plus stable.

### Canaux et stratégie de déploiement

Les quatre canaux standards suivent une progression de stabilité croissante :

- **`edge`** — Builds automatiques depuis la branche principale. Potentiellement instables. Destinés aux contributeurs et testeurs.  
- **`beta`** — Versions fonctionnelles en cours de validation. Destinées aux early adopters.  
- **`candidate`** — Release candidates. Dernière étape avant la publication stable. Destinées aux utilisateurs qui veulent tester avant la masse.  
- **`stable`** — Version de production. C'est ce que `snap install syswatch` installe par défaut.

L'utilisateur choisit son canal :

```bash
# Installer la version stable (défaut)
sudo snap install syswatch

# Suivre le canal beta
sudo snap install syswatch --channel=beta
```

### Intégration CI

L'upload automatique depuis un pipeline CI nécessite un token d'authentification :

```bash
# Générer un token (une seule fois, en local)
snapcraft export-login snapcraft-credentials.txt
```

Ce fichier de credentials est stocké comme secret dans votre CI. Exemple GitHub Actions :

```yaml
jobs:
  publish-snap:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Build snap
        uses: snapcore/action-build@v1.3
        id: build

      - name: Publish to edge
        uses: snapcore/action-publish@v1.1
        env:
          SNAPCRAFT_STORE_CREDENTIALS: ${{ secrets.SNAPCRAFT_TOKEN }}
        with:
          snap: ${{ steps.build.outputs.snap }}
          release: edge
```

La promotion de `edge` vers `stable` reste une action manuelle — c'est une décision humaine, pas un automatisme CI.

---

## Publication d'un dépôt APT sur GitHub Pages

Pour les projets open source qui ne veulent pas gérer d'infrastructure, GitHub Pages peut servir de dépôt APT statique. Le dépôt est un répertoire dans une branche dédiée (`gh-pages`) du projet, mis à jour par la CI à chaque release.

Le workflow complet :

```yaml
jobs:
  update-apt-repo:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          ref: gh-pages
          path: repo

      - name: Download DEB artifact
        uses: actions/download-artifact@v4
        with:
          name: syswatch-deb
          path: new-packages/

      - name: Update repository
        run: |
          # Importer la clé GPG depuis les secrets
          echo "${{ secrets.GPG_PRIVATE_KEY }}" | gpg --batch --import
          
          cp new-packages/*.deb repo/pool/main/s/syswatch/
          
          cd repo
          dpkg-scanpackages pool/ /dev/null > dists/stable/main/binary-amd64/Packages
          gzip -k dists/stable/main/binary-amd64/Packages
          
          # Générer et signer Release
          cd dists/stable
          apt-ftparchive release . > Release
          gpg --batch --yes --armor --detach-sign -o Release.gpg Release
          gpg --batch --yes --clearsign -o InRelease Release

      - name: Push to gh-pages
        run: |
          cd repo
          git config user.name "CI Bot"
          git config user.email "ci@exemple.com"
          git add -A
          git commit -m "Update repo: syswatch $(date +%Y%m%d)"
          git push
```

L'utilisateur ajoute le dépôt :

```bash
deb [signed-by=/usr/share/keyrings/syswatch.gpg] https://exemple.github.io/syswatch/apt stable main
```

Cette solution est gratuite, ne nécessite aucune infrastructure, et offre un CDN mondial (GitHub Pages est servi via Fastly). Sa limite principale est l'absence de gestion avancée des versions — pour cela, `reprepro` ou un service comme Cloudsmith reste nécessaire.

---

## Automatisation complète : du commit au `apt install`

En combinant les outils vus dans ce chapitre et au chapitre 38, un pipeline de distribution complet ressemble à ceci :

1. **Commit et tag** — Le développeur pousse un tag `v1.3.0` sur le dépôt Git.
2. **CI : build** — GitHub Actions compile le projet en mode Release avec CMake + Ninja, accéléré par ccache (section 38.3).
3. **CI : tests** — Les tests unitaires (Google Test, chapitre 33) et l'analyse statique (clang-tidy, chapitre 32) s'exécutent.
4. **CI : packaging** — Le pipeline produit en parallèle un `.deb`, un `.rpm`, et un AppImage via des matrix builds (section 38.7).
5. **CI : publication** — Le `.deb` est poussé vers le dépôt APT (reprepro, Cloudsmith, ou GitHub Pages). Le `.rpm` est poussé vers un dépôt DNF. L'AppImage est attachée à la release GitHub. Le snap est uploadé sur le canal `edge`.
6. **Promotion** — Après validation, le snap est promu vers `stable` manuellement.
7. **Machine cible** — L'administrateur lance `sudo apt update && sudo apt upgrade` et reçoit la nouvelle version.

Le temps entre le tag Git et la disponibilité sur `apt` dépend de la durée du pipeline CI — typiquement entre 5 et 15 minutes pour un projet C++ de taille moyenne avec cache actif. C'est un ordre de grandeur comparable aux pratiques de déploiement continu des écosystèmes interprétés.

---

## Bonnes pratiques de distribution

**Signez toujours vos dépôts.** Un dépôt APT non signé est un vecteur d'attaque. Même pour un dépôt interne, la signature GPG protège contre les compromissions de serveur et les attaques man-in-the-middle. Le surcoût de mise en place est négligeable par rapport au risque.

**Séparez les canaux de stabilité.** Utilisez des distributions distinctes dans votre dépôt (`stable`, `testing`, `nightly`) ou les canaux Snap (`stable`, `beta`, `edge`). Ne poussez jamais un build non validé directement vers le canal de production.

**Versionnez correctement.** APT et DNF comparent les numéros de version pour déterminer si une mise à jour est disponible. Si votre versioning est incohérent (par exemple `1.2.0` suivi de `1.2.0-hotfix`), les gestionnaires de paquets peuvent refuser la mise à jour ou la proposer dans le mauvais ordre. Suivez le Semantic Versioning et utilisez le champ révision (`-1`, `-2`) pour les corrections de packaging.

**Testez l'installation sur un système vierge.** Un conteneur Docker minimal (`ubuntu:24.04` sans rien d'autre) est le meilleur test. Si `apt install` fonctionne sur un système vierge avec uniquement votre dépôt, les dépendances sont correctement déclarées.

**Documentez l'ajout du dépôt.** Fournissez à vos utilisateurs un script ou une commande copier-coller pour ajouter la clé GPG et la source APT. Moins il y a d'étapes manuelles, moins il y a d'erreurs.

**Planifiez la rotation des clés GPG.** Les clés GPG expirent (et c'est souhaitable). Prévoyez un processus de rotation qui distribue la nouvelle clé publique *avant* l'expiration de l'ancienne. Le paquet `syswatch-archive-keyring` — un paquet DEB qui ne contient que la clé publique — est le mécanisme standard pour distribuer et mettre à jour les clés de dépôt.

---

## Résumé du chapitre

Le chapitre 39 a couvert le parcours complet d'un binaire C++ depuis la sortie du compilateur jusqu'à la commande `apt install` sur la machine de production. Les paquets DEB (39.1) et RPM (39.2) offrent l'intégration la plus profonde avec leurs distributions respectives. AppImage (39.3) élimine la dépendance à une distribution spécifique. Et la publication via des dépôts APT, le Snap Store, ou des services d'hébergement d'artefacts (39.4) ferme la boucle en rendant ces paquets accessibles aux gestionnaires de paquets que les utilisateurs connaissent déjà.

Combiné aux chapitres 37 (Docker), 38 (CI/CD), et 40 (Observabilité), ce chapitre complète la boîte à outils DevOps du développeur C++ : du code source au binaire en production, surveillé et maintenable.

⏭️ [Observabilité et Monitoring](/40-observabilite/README.md)
