### Tanguy Dietrich & Kirill Goundiaev - printemps 2023
# Rapport CSEL

## Préambule
Ce travail repose sur la théorie et les instructions données lors du cours CSEL faisant partie du cursus MES de la HES-SO. Ces informations sont disponible sur le [site du cours](https://mse-csel.github.io/website/). Une base de code est également donnée sur le [git du cours](https://github.com/mse-csel/csel-workspace).

## Introduction
Le but de ce travail est de mettre en place un environnement de travail pour une cible embarquée, la compréhension de différentes zones de mémoire, développement de module pour le kernel linux, ainsi que d'application.

## Environnement Linux embarqué
### Mise en place de la machine hôte
Pour mener à bien ce travail, nous avons besion d'une machine hôte sous Linux, Windows ou OSx et des logiciels suivants:
- [Docker Desktop](https://www.docker.com/products/docker-desktop)
- git
- Visual Studio Code
- [Balena Etcher](https://www.balena.io/etcher/)

Nous commençons par forker le repos principal, puis le pull sur notre machine. Nous ouvrons le repos dans VSCode et acceptons de le réouvrir dans un Container (Reopen in COntainer). Si VSCode ne nous propose pas cette option, il faut verifer que le module 'Dev Containers' soit bien installé.

Une fois ouvert dans un Container, nous executons le scripte `get-buildroot.sh` dans le terminal de VSCode. Cet scripte nous permet de télécharger buildroot et faire la configuration nécessaire pour ce projet. [Buildroot](http://buildroot.uclibc.org/) est un outil qui nous permettera de générer une distribution Linux pour notre cible à partir de zéro. Cette outil se compose :
- d'une chaine de compilation croisée nous permetant de compiler pour notre sible dont l'architecture est différente de notre hôte.
- Image du noyeau (Kernel) Linux, code souce (niveau Kernel) de l'OS que nous déployerons sur la cible
- RootFS, le système fichier racine est le répertroire principal contenant des répertoires, logiciels de base du système et des logiciel d'applications.
- Bootloader, dans notre cas U-Boot qui est le premier programme a être exécuté au lancement du système. Il se chargera d'effectuer certaines verification, pius de démarrer le système principal sous Linux.

### Compilation et lancement de la cible
Le noyeau et le rootfs peut être configuer dans buildroot avec les commandes:

```sh
    cd /buildroot
    make menuconfig
```
Enfin, la compilation se fait avec les commandes:
```sh
    cd /buildroot
    make
```
Une fois que la compilation est faite, nous pouvons enlever l'ancien rootfs s'il est présent et extraire le nouveau:
```sh
    rm -Rf /rootfs/*
    tar xf /buildroot/output/images/rootfs.tar -C /rootfs
```
> Note\
>Vous pouvez aussi utiliser les script `/usr/local/bin/delete-rootfs.sh` et `/usr/local/bin/extract-rootfs.sh` présents dans l’image Docker.

Maintenant que nous disposons de nos images, nous devons les extraire du container vers notre machine hôte afin de graver une carte SD. Pour ce faire, nous pouvons utiliser la commande:
```sh 
    rsync -rlt --progress --delete /buildroot/output/images/ /workspace/buildroot-images

    # ou le scripte
    /usr/local/bin/sync-images.sh
```

En utilisant Balena Etcher, nous flaschons notre catre SD avec l'image `buildroot-images/sdcard.img`.

En insérant la carte SD dans la cible et la démmarant, nous pouovons observer la séquance de lancement de U-Boot avec une comminucation série. Enfin, nous pourrons nous connecté une fois le boot terminé avec le login `root` sans mot de passe???

# TODO ajouter image de lancement?..

### Configuration pour la communication réseau
Notre sible a été configuré avec l'adresse 192.168.0.14. Il nous est proposé de configurer notre machine hôte avec l'adresse 192.168.0.4, afin de pouvoir communique avec cette derniere à travers le réseau.  
> Notes
>- IP :   192.168.0.4
>- Mask : 255.255.255.0
>
>- sous Windows:
>   - Mask : 24
>   - Permettre les connexion entrante dans le firewall poour le réseau 192.168.0.0/24 

Une fois ces manipulations faites, nous pouvons tester la connexion avec un ping, puis se connecter par ssh à la cible:
```sh
    ssh root@192.168.0.14
```

> Note à verifier\
> L'adresse IP de la cible est défini au moment du boot. Elle est écrite en dur, mais nous pouvons la modifier. \
> Les fichiers `/workspace/boot-scripts/boot_cifs.cmd` et `/workspace/config/board/friendlyarm/nanopi-neo-plus2/rootfs_overlay/etc/network/interfaces` \
> Une autre possibilité serait de changer la variable d'environnement dans l'U-Boot pour donner une nouvelle adresse IP.

La commande `uname -a` nous permet de voir le système d'exploitation de la cible.

### Mise en place de l’espace de travail (workspace) sous CIFS/SMB

### Génération d’applications sur la machine de développement hôte

### Debugging de l’application sur la cible (VS-Code)

### Mise en place de l’environnement pour le développement du noyau sous CIFS/SMB

### Questions
1. Comment faut-il procéder pour générer l’U-Boot ?
2. Comment peut-on ajouter et générer un package supplémentaire dans le Buildroot ?
3. Comment doit-on procéder pour modifier la configuration du noyau Linux ?
4. Comment faut-il faire pour générer son propre rootfs ?
5. Comment faudrait-il procéder pour utiliser la carte eMMC en lieu et place de la carte SD ?
6. Dans le support de cours, on trouve différentes configurations de l’environnement de développement. Qu’elle serait la configuration optimale pour le développement uniquement d’applications en espace utilisateur ?