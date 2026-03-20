# Chapitre 40 — Observabilite et Monitoring : Exemples

## Compilation

Tous les exemples se compilent avec un seul CMakeLists.txt. Les dependances (spdlog, nlohmann/json, cpp-httplib) sont telechargees automatiquement via FetchContent.

```bash
cd exemples/  
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release  
cmake --build build --parallel $(nproc)  
```

## Exemples

### 01_basic_spdlog.cpp

| | |
|---|---|
| **Section** | 40.1.1 — Installation et configuration de spdlog |
| **Fichier source** | `01.1-installation-spdlog.md` |
| **Description** | Premier programme spdlog : niveaux de severite, formatage positionnel, logger par defaut |
| **Sortie attendue** | 6 lignes avec horodatage. Le `debug` initial est filtre (niveau `info` par defaut). Apres `set_level(debug)`, les messages debug apparaissent. |

```
[...] [info] Application démarrée
[...] [debug] Maintenant ce message s'affiche
[...] [info] Serveur en écoute sur 0.0.0.0:8080
[...] [warning] Fichier de config absent, utilisation des valeurs par défaut
[...] [error] Connexion à la base de données échouée: timeout après 5000ms
[...] [critical] Espace disque insuffisant, arrêt du service
```

---

### 02_named_loggers.cpp

| | |
|---|---|
| **Section** | 40.1.1 — Installation et configuration de spdlog |
| **Fichier source** | `01.1-installation-spdlog.md` |
| **Description** | Loggers nommes par composant (`network`, `database`) avec niveaux independants. Recuperation via `spdlog::get()`. |
| **Sortie attendue** | 4 lignes. Le debug de `database` est filtre (niveau `warn`). Le debug de `network` s'affiche (niveau `debug`). |

```
[...] [network] [info] Connection accepted from 10.0.1.42
[...] [database] [warning] Query slow: 342ms
[...] [network] [debug] Packet received: 1024 bytes
[...] [network] [info] Retrieved via spdlog::get()
```

---

### 03_multi_sink.cpp

| | |
|---|---|
| **Section** | 40.1.2 — Niveaux de log et sinks |
| **Fichier source** | `01.2-niveaux-sinks.md` |
| **Description** | Multi-sink : console (`info`+) et fichier rotatif (`debug`+) avec formats differents par sink. |
| **Sortie attendue** | Console : 3 lignes (`info`, `warning`, `error`). Fichier `syswatch.log` : 4 lignes incluant `debug`. Le debug n'apparait pas sur la console. |

Console :
```
[HH:MM:SS.mmm] [info] Server started on port 8080
[HH:MM:SS.mmm] [warning] Config file missing, using defaults
[HH:MM:SS.mmm] [error] Connection to database failed: timeout after 5000ms
```

Fichier syswatch.log :
```
[date time] [syswatch] [debug] [tid:N] This debug goes to file only
[date time] [syswatch] [info] [tid:N] Server started on port 8080
[date time] [syswatch] [warning] [tid:N] Config file missing, using defaults
[date time] [syswatch] [error] [tid:N] Connection to database failed: timeout after 5000ms
```

---

### 04_patterns.cpp

| | |
|---|---|
| **Section** | 40.1.3 — Pattern de formatage |
| **Fichier source** | `01.3-pattern-formatage.md` |
| **Description** | Quatre patterns recommandes : developpement (source location), production (pid/tid), Docker (ISO 8601), JSON structure. Necessite `SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE`. |
| **Sortie attendue** | Quatre groupes de lignes avec des formats differents. Le pattern dev inclut `[fichier:ligne]`. Le pattern JSON produit des objets JSON valides. |

---

### 05_custom_flag.cpp

| | |
|---|---|
| **Section** | 40.1.3 — Pattern de formatage |
| **Fichier source** | `01.3-pattern-formatage.md` |
| **Description** | Custom flag formatter `%*` qui injecte le hostname de la machine dans chaque ligne de log. |
| **Sortie attendue** | 3 lignes avec le hostname de la machine entre crochets (ex: `[prod-web-03]`). |

```
[date time] [<hostname>] [info] Server started on port 8080
[date time] [<hostname>] [warning] High memory usage detected
[date time] [<hostname>] [error] Connection refused by upstream
```

---

### 06_healthcheck.cpp

| | |
|---|---|
| **Section** | 40.4 — Health checks et readiness probes |
| **Fichier source** | `04-health-checks.md` |
| **Description** | Serveur HTTP minimal (cpp-httplib) exposant `/healthz` (liveness) et `/readyz` (readiness). Simule un demarrage long : readyz retourne 503 pendant le demarrage, puis 200 une fois pret. |
| **Comportement attendu** | Pendant le demarrage : `curl :8081/healthz` → 200 `{"status":"ok"}`, `curl :8081/readyz` → 503 `{"status":"not_ready"}`. Apres `set_ready(true)` : `/readyz` → 200 `{"status":"ready"}`. Arret propre apres 2 secondes. |

Test :
```bash
./build/06_healthcheck &
sleep 0.5  
curl -s http://localhost:8081/healthz   # {"status":"ok"}  
curl -s http://localhost:8081/readyz    # {"status":"not_ready"}  
sleep 1  
curl -s http://localhost:8081/readyz    # {"status":"ready"}  
```

---

### 07_json_sink.cpp

| | |
|---|---|
| **Section** | 40.5 — Structured logging — JSON logs pour agregation |
| **Fichier source** | `05-json-logs.md` |
| **Description** | Custom sink spdlog qui produit du JSON valide via nlohmann/json. Teste l'echappement correct des guillemets, retours a la ligne et backslashes. Ajoute des champs statiques (service, version, environment). |
| **Sortie attendue** | 4 lignes JSON valides. Les guillemets dans le message sont echappe `\"`. Les `\n` et `\\` sont echappe correctement. Chaque ligne contient `time`, `level`, `logger`, `msg`, `tid`, `pid`, `service`, `version`, `environment`. |

```json
{"environment":"production","level":"info","logger":"syswatch","msg":"Server started on port 8080",...}
{"environment":"production","level":"error","logger":"syswatch","msg":"Failed to parse file \"config.yaml\": unexpected token at line 42","source":{...},...}
{"environment":"production","level":"warning","logger":"syswatch","msg":"Multi-line\nerror message",...}
{"environment":"production","level":"info","logger":"syswatch","msg":"Path: C:\\Users\\test\\file.txt",...}
```

Validation JSON :
```bash
./build/07_json_sink | python3 -c "import sys,json; [json.loads(l) for l in sys.stdin]; print('OK')"
```

---

## Nettoyage

```bash
rm -rf build syswatch.log
```
