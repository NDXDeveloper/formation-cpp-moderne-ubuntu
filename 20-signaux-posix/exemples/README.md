# Chapitre 20 — Signaux POSIX : exemples

Compilation : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -o <nom> <nom>.cpp`

---

## 20.1 — Comprendre les signaux Unix (01-signaux-unix.md)

| Fichier | Description | Sortie attendue |
|---|---|---|
| `01_sigpipe_ignore.cpp` | Ignorer SIGPIPE — bonne pratique pour les programmes réseau | `SIGPIPE ignoré avec succès` |
| `01_sigsegv_handler.cpp` | Handler minimaliste pour SIGSEGV — diagnostic puis terminaison | `FATAL: Segmentation fault — arrêt du programme` (stderr), exit code 139 |
| `01_kill_signal.cpp` | Envoi de signaux avec `kill()` et vérification d'existence | `kill: No such process` puis `Le processus 1234 n'existe pas` |
| `01_child_status.cpp` | Détecter comment un processus enfant a terminé (`waitpid`) | `Processus <pid> terminé normalement, code 42` |
| `01_signal_disposition.cpp` | Consulter la disposition actuelle d'un signal avec `sigaction` | `action par défaut` → `ignoré` → `action par défaut` |

## 20.2 — Installation de handlers (02-handlers.md)

| Fichier | Description | Sortie attendue |
|---|---|---|
| `02_sigaction_handler.cpp` | Installation d'un handler simple avec `sigaction()` | `Signal SIGTERM reçu — arrêt propre` (stderr) |
| `02_sa_siginfo.cpp` | Handler étendu `SA_SIGINFO` avec informations `siginfo_t` | `Signal 10 reçu de PID <pid> (UID <uid>), code=-6` (stderr) |
| `02_flag_atomique.cpp` | Pattern 1 — Flag atomique `volatile sig_atomic_t` pour arrêt propre | `Traitement en cours...` → `Arrêt propre en cours...` → `Arrêt terminé.` |
| `02_self_pipe.cpp` | Pattern 2 — Self-pipe trick pour boucle d'événements `poll` | `Signal 15 reçu via self-pipe` → `Arrêt propre terminé` |
| `02_signalfd.cpp` | Pattern 3 — `signalfd` (Linux) pour lecture synchrone des signaux | `Signal 15 reçu de PID <pid> (UID <uid>)` → `Arrêt propre terminé` |
| `02_graceful_shutdown.cpp` | Graceful shutdown — squelette complet de serveur avec arrêt propre | Séquence d'arrêt : refus connexions → finalisation → fermeture → terminé |
| `02_scoped_handler.cpp` | `ScopedSignalHandler` RAII — sauvegarder et restaurer les dispositions | `SIGINT ignoré dans la section critique` → `SIGINT restauré` |
| `02_sigprocmask.cpp` | Masquage temporaire de signaux avec `sigprocmask` | `Mise à jour atomique en cours...` → `Mise à jour terminée` |
| `02_scoped_block.cpp` | `ScopedSignalBlock` RAII — masquage temporaire de signaux | `Dans la zone protégée par ScopedSignalBlock` |

## 20.3 — Signaux et threads (03-signaux-threads.md)

| Fichier | Description | Sortie attendue |
|---|---|---|
| `03_sigwait_thread.cpp` | Thread dédié aux signaux avec `sigwait()` — pattern recommandé | Workers + `SIGTERM reçu — arrêt demandé` → arrêt propre de tous les threads |
| `03_signalfd_epoll.cpp` | `signalfd` + `epoll` — boucle d'événements sans handler | `Signal 15 — arrêt demandé` → workers terminés → arrêt propre |
| `03_jthread_stop.cpp` | `std::jthread` + `std::stop_source` — arrêt coopératif C++20 | Workers + `Terminated reçu — demande d'arrêt` → arrêt coopératif de chaque worker |
| `03_condvar_stop.cpp` | `condition_variable_any` avec stop_token — attente interruptible | Consumers traitent 6 items → `arrêt demandé` pour chaque consumer |
