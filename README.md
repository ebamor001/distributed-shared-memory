Ce projet implémente un système de mémoire partagée distribuée (DSM) entre plusieurs processus exécutés sur différentes machines, en utilisant :
    mmap, mprotect
    gestion de SIGSEGV
    transferts réseau TCP
    un démon de communication par thread
    un programme maître dsmexec

📦 Structure
.
├── Phase1/                      # dsmexec et services (spawn, I/O, réseau…)
│   ├── bin/                     # contient dsmexec + exécutables DSM
│   ├── binaries/                # utilitaires fournis (non nécessaires au DSM)
│   ├── include/                 # headers Phase1
│   ├── services/
│   │   ├── file_management.c
│   │   ├── cleanup_management.c
│   │   ├── childproc_management.c     # gestion des processus enfants (fork, SSH, signaux)
│   │   ├── io_management.c            # poll(), multiplexage stdout/stderr + socket maître
│   │   ├── spawn_management.c         # construction des variables DSMEXEC_* + exécution SSH
│   │   └── network_management.c       # création + écoute du socket maître
│   ├── examples/
│   └── Makefile
│
├── Phase2/                      # implémentation DSM (libdsm.a + handlers)
│   ├── include/
│   ├── examples/                # double_touch
│   └── Makefile
│
└── machine_file                 # liste des machines distantes (1 par ligne)


⚙️ Compilation
1. Compiler Phase1 (dsmexec)
cd Phase1
make install

Génère :

Phase1/bin/
    ├── dsmexec
    └── data_exchange

2. Compiler Phase2 (bibliothèque DSM + exemples)
cd Phase2
make

Génère libdsm.a et les exemples DSM (ex: double_touch) puis les déplace automatiquement dans :
Phase1/bin/
    ├── double_touch
    ├── dsmexec
    └── data_exchange

🚀 Exécution

Depuis Phase1 :

./bin/dsmexec machine_file double_touch


Où machine_file contient une machine par ligne 

🧹 Nettoyage
Phase1 :
cd Phase1
make clean

Phase2 :
cd Phase2
make clean

✔️ Dépendances

gcc

ssh fonctionnel vers les machines listées

pthread

Linux (utilisation de mmap, mprotect, sigaction, poll, etc.)

📌 Notes

Tous les exécutables DSM vont dans Phase1/bin/

dsmexec doit toujours être lancé depuis Phase1

La communication inter-processus se fait automatiquement via TCP
