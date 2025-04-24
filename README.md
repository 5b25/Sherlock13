# 🕵️ Sherlock13

Ce projet est une version multijoueur en ligne du jeu **Sherlock13**, avec une interface graphique développée en SDL2, SDL_ttf et SDL_image.

> 🌐 Prend en charge **jusqu'à 4 joueurs connectés simultanément** sur un réseau local, chaque joueur utilisant un client distinct pour participer au jeu.

---

## 📦 1. Environnement et installation des dépendances

Ce projet utilise des bibliothèques BSD étendues et des bibliothèques de threads réseau POSIX, donc **il est uniquement compatible avec les systèmes Linux** (comme Ubuntu).

> ✅ Environnement de test : **Ubuntu 22.04 LTS**

### 1. Installer les dépendances (exemple pour Ubuntu)

```bash
sudo apt update
sudo apt install build-essential libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libbsd-dev git
```

### 2. Télécharger le projet

- **Méthode 1 : utiliser git clone**

```bash
git clone https://github.com/your-username/Sherlock13.git
cd Sherlock13
```

> ⚠️ Assurez-vous que `git` est installé (`sudo apt install git` si besoin)

- **Méthode 2 : télécharger l'archive zip et la décompresser**

```bash
unzip Sherlock13.zip
cd Sherlock13
```

### 3. Compiler le projet

Dans le répertoire racine du projet, exécutez :

```bash
make
```

> La compilation génère deux exécutables : `serveur` et `client`

---

## 🚀 2. Méthodes de lancement

### ✅ Lancer le serveur (`serveur`)

Il faut lancer le serveur en premier. Deux méthodes sont possibles :

| Mode | Commande | Description |
|------|----------|-------------|
| 1️⃣  | `./serveur 40000` | Spécifie manuellement le port (ici 40000) |
| 2️⃣  | `./serveur`       | Utilise le port par défaut **32000** |

---

### ✅ Lancer un client (`client`)

Chaque joueur doit lancer un client. Plusieurs options sont possibles :

| Mode | Commande | Description |
|------|----------|-------------|
| 1️⃣  | `./client`                       | Mode graphique par défaut, entrez un nom et cliquez sur GO |
| 2️⃣  | `./client Alice`                 | IP par défaut 127.0.0.1 |
| 3️⃣  | `./client 127.0.0.1 Alice`       | Port est attribué automatiquement par le serveur, par défaut 32000 + nbClient |
| 4️⃣  | `./client 127.0.0.1 32001 Alice` | Connexion IP:port, nom = Alice |

> ⚠️ Le serveur doit être lancé avant que les clients ne se connectent.

---

## 🎮 3. Utilisation et règles du jeu

### 1. Déroulement du jeu

- Chaque joueur reçoit 3 cartes personnages
- Une carte est mise de côté (le "coupable")
- Les joueurs déduisent qui est le coupable en posant des questions

### 2. Interface graphique

- **GO** : soumettre le nom et rejoindre le jeu
- **O** : demander si **n'importe qui** a un objet donné
- **S** : demander à un **joueur spécifique** s'il possède un objet
- **G** : deviner le coupable (perdre en cas d'erreur)
- **Zone en haut à gauche** : icônes des objets + quantité
- **Tableau à gauche** : noms des joueurs + objets associés
- **Barre d'état** : joueur actuel et résultat de la dernière action

### 3. Règles principales

- Posez des questions pour éliminer des suspects
- Trois types d'actions par tour : O / S / G
- Devinez juste pour gagner, une erreur mène à l'élimination

> 📘 Voir le fichier PDF fourni pour les règles détaillées

---
![Interface du jeu](./Captures%20d’écran/Interface.png)
![Interface du jeu](./Captures%20d’écran/Button%20O.png)
![Interface du jeu](./Captures%20d’écran/Button%20S.png)
![Interface du jeu](./Captures%20d’écran/Button%20G.png)
