# 🧠 Distributed Shared Memory (DSM)

Implémentation d’un système de mémoire partagée distribuée en C : gestion de pages distribuées, transferts TCP, mmap/mprotect, SIGSEGV handler, thread de communication, protocole DSM minimaliste.

---

## 📂 Project Structure

### **Phase1 — dsmexec (launcher)**  
Gestion du lancement distant via SSH, redirection stdout/stderr, distribution de la table de connexions.

- `dsmexec` — programme maître  
- `services/childproc_management.c` — gestion des processus enfants (fork, SSH, signaux)  
- `services/io_management.c` — poll(), multiplexage des pipes + sockets  
- `services/spawn_management.c` — génération des variables DSMEXEC_*, exécution SSH  
- `services/network_management.c` — création + écoute du socket maître  
- `include/` — headers Phase1  
- `bin/` — contient dsmexec + double_touch

---

### **Phase2 — DSM Runtime Library**

Implémentation complète du DSM (gestion des pages, SIGSEGV, protocole, transferts réseau).

- `dsm.c` — gestion des pages, handler SIGSEGV, protocole DSM  
- `network.c` — wrappers robustes send/recv  
- `include/dsm_impl.h` — structures internes + API DSM  
- `libdsm.a` — bibliothèque statique générée (non versionnée)  
- `examples/double_touch.c` — exemple minimal utilisant la DSM

---

## ⚙️ Compilation

### **1️⃣ Phase 1 : construire dsmexec**

Depuis `Phase1/` :

make install

Génère :

Phase1/bin/  
 ├── dsmexec  
 └── data_exchange  

---

### **2️⃣ Phase 2 : construire la bibliothèque DSM**

Depuis `Phase2/` :

make

Génère :

- libdsm.a  
- double_touch (déplacé automatiquement dans `Phase1/bin/`)

---

## 🚀 Exécution

Depuis `Phase1/bin/` :

./dsmexec machine_file double_touch

Le fichier `machine_file` contient une machine par ligne :

trombone.pedago.ipb.fr  
trompette.pedago.ipb.fr  
txistu.pedago.ipb.fr  

⚠️ Les machines doivent accepter SSH sans mot de passe (clé publique).

---

## 🔄 DSM Protocol Overview

### 🧱 Gestion des pages
- Allocation en tourniquet : owner = index % nb_processus  
- Protection mémoire :  
  - PROT_NONE → déclenche SIGSEGV → demande de page  
  - PROT_READ | PROT_WRITE → page possédée  

### ⚡ Sur SIGSEGV (faute de page)
1. Envoi de DSM_REQ au propriétaire  
2. Le propriétaire envoie DSM_PAGE  
3. Tous les processus reçoivent DSM_UPDATE  

### 🔚 Finalisation
- Chaque processus envoie DSM_FINALIZE  
- Barrière d’attente  
- Fermeture propre des sockets  
- Fin du thread daemon  

---

## 🧪 Exemple : double_touch

Programme simple montrant un accès concurrent à la même page DSM.

Sortie typique :

[Proc 0] integer value: 4  
[Proc 1] integer value: 12  

---

## 🧹 Nettoyage

### Phase 1 :
make distclean

### Phase 2 :
make clean

---

## 🛠 Dépendances

- gcc  
- pthreads  
- Linux (mprotect, mmap, poll, sockets…)  
- SSH sans mot de passe  

---

## 📚 À propos

Le projet implémente :

- gestion distribuée des pages DSM  
- transferts réseau TCP  
- gestion SIGSEGV → handler DSM  
- communication asynchrone (poll)  
- synchronisation mutex + condition variable  
- protocole DSM simple mais robuste  

