# Chapitre 23 — Processus et IPC : Exemples

Tous les exemples se compilent avec `g++-15 -std=c++23 -Wall -Wextra`.  
Certains nécessitent des flags supplémentaires (`-lrt`, `-pthread`).  

---

## Section 23.1 : fork, exec et gestion de processus

| Fichier | Description | Compilation | Comportement attendu |
|---------|-------------|-------------|----------------------|
| `ex01_fork_basic.cpp` | fork() basique avec identification parent/enfant | `g++-15 -std=c++23 -o ex01 ex01_fork_basic.cpp` | Affiche les PID parent et enfant, enfant termine avec code 0 |
| `ex02_fork_exec.cpp` | execvp et execlp après fork() | `g++-15 -std=c++23 -o ex02 ex02_fork_exec.cpp` | Lance `ls -la /tmp` deux fois (via execvp puis execlp) |
| `ex03_waitpid_status.cpp` | Analyse du status retourné par waitpid() | `g++-15 -std=c++23 -o ex03 ex03_waitpid_status.cpp` | Test 1: code 0, Test 2: signal 9 (SIGKILL), Test 3: code 42 |
| `ex04_posix_spawn.cpp` | Lancement de processus avec posix_spawn | `g++-15 -std=c++23 -o ex04 ex04_posix_spawn.cpp` | Lance `ls -la /tmp` via posix_spawnp, affiche PID et code retour |
| `ex05_process_raii.cpp` | Classe Process RAII (posix_spawn, wait, timeout, signaux) | `g++-15 -std=c++23 -o ex05 ex05_process_raii.cpp` | Lance echo (succès), puis sleep 30 avec timeout 2s → SIGTERM |
| `ex06_capture_output.cpp` | Capturer stdout via pipe + posix_spawn | `g++-15 -std=c++23 -o ex06 ex06_capture_output.cpp` | Capture sortie de `uname -a` et `ls -1 /tmp` dans des strings |

## Section 23.2 : Pipes et communication inter-processus

| Fichier | Description | Compilation | Comportement attendu |
|---------|-------------|-------------|----------------------|
| `ex07_pipe_basic.cpp` | Pipe anonyme : écriture/lecture dans le même processus | `g++-15 -std=c++23 -o ex07 ex07_pipe_basic.cpp` | Affiche "Lu: Hello depuis le pipe!" |
| `ex08_pipe_parent_child.cpp` | Communication parent → enfant via pipe | `g++-15 -std=c++23 -o ex08 ex08_pipe_parent_child.cpp` | Enfant reçoit "Message du parent", parent confirme terminaison |
| `ex09_pipe_raii.cpp` | Classe Pipe RAII + communication fork | `g++-15 -std=c++23 -o ex09 ex09_pipe_raii.cpp` | Enfant reçoit 32 octets via RAII Pipe |
| `ex10_pipe_capture_stdout.cpp` | Capturer stdout avec Pipe RAII + dup2 | `g++-15 -std=c++23 -o ex10 ex10_pipe_capture_stdout.cpp` | Capture "Sortie capturée via dup2!" dans une string |
| `ex11_pipe_feed_stdin.cpp` | Alimenter stdin d'un enfant via Pipe + dup2 | `g++-15 -std=c++23 -o ex11 ex11_pipe_feed_stdin.cpp` | wc compte 4 lignes envoyées par le parent |
| `ex12_pipeline.cpp` | Pipeline cat\|grep\|wc en C++ | `g++-15 -std=c++23 -o ex12 ex12_pipeline.cpp` | Affiche le nombre de lignes contenant "root" dans /etc/passwd |
| `ex13_fifo.cpp` | FIFO (pipe nommé) producteur/consommateur | `g++-15 -std=c++23 -o ex13 ex13_fifo.cpp` | 5 messages échangés via FIFO, nettoyage automatique |

## Section 23.3 : Shared memory et mmap

| Fichier | Description | Compilation | Comportement attendu |
|---------|-------------|-------------|----------------------|
| `ex14_mmap_anonymous.cpp` | mmap MAP_SHARED\|MAP_ANONYMOUS parent↔enfant | `g++-15 -std=c++23 -o ex14 ex14_mmap_anonymous.cpp` | Enfant écrit, parent lit "Hello depuis l'enfant!" |
| `ex15_shm_posix.cpp` | shm_open + mmap avec atomic ready flag | `g++-15 -std=c++23 -o ex15 ex15_shm_posix.cpp -lrt` | Écrivain met counter=42, lecteur les lit correctement |
| `ex16_mmap_file.cpp` | Lecture de fichier via mmap (accès par pointeur) | `g++-15 -std=c++23 -o ex16 ex16_mmap_file.cpp` | Affiche les 5 premières lignes de /etc/passwd |
| `ex17_pthread_mutex_shared.cpp` | pthread_mutex PROCESS_SHARED + ROBUST | `g++-15 -std=c++23 -pthread -o ex17 ex17_pthread_mutex_shared.cpp` | Counter final = 2000 (1000 par processus, synchronisé) |
| `ex18_shm_raii.cpp` | Classe SharedMemory RAII + métriques atomiques | `g++-15 -std=c++23 -o ex18 ex18_shm_raii.cpp -lrt` | Serveur écrit 100 requêtes, monitoring lit requests=100, errors=10, latency=12.5ms |

## Section 23.4 : Message queues POSIX

| Fichier | Description | Compilation | Comportement attendu |
|---------|-------------|-------------|----------------------|
| `ex19_mq_basic.cpp` | Création de queue, envoi/réception, priorités | `g++-15 -std=c++23 -o ex19 ex19_mq_basic.cpp -lrt` | Envoie/reçoit "Hello, queue!", puis 4 messages par priorité décroissante (10, 5, 1, 0) |
| `ex20_mq_raii.cpp` | Classe MessageQueue RAII + producteur/consommateur | `g++-15 -std=c++23 -o ex20 ex20_mq_raii.cpp -lrt` | Producteur envoie 5 tâches + STOP, consommateur les reçoit par priorité |
