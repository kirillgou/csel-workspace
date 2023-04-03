### Tanguy Dietrich & Kirill Goundiaev
### Printemps 2023
# Rapport CSEL
# Environnement Linux embarqué et programmation noyau Linux

## Préambule
Ce travail repose sur la théorie et les instructions données lors du cours CSEL faisant partie du cursus MES de la HES-SO. Ces informations sont disponible sur le [site du cours](https://mse-csel.github.io/website/). Une base de code est également donnée sur le [git du cours](https://github.com/mse-csel/csel-workspace).

## Introduction
Le but de ce travail est de mettre en place un environnement de travail pour une cible embarquée, la compréhension de différentes zones de mémoire, développement de module pour le kernel linux, ainsi que d'application.

## 1. Environnement Linux embarqué
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
> Nous pouvons aussi utiliser les script `/usr/local/bin/delete-rootfs.sh` et `/usr/local/bin/extract-rootfs.sh` présents dans l’image Docker.

Maintenant que nous disposons de nos images, nous devons les extraire du container vers notre machine hôte afin de graver une carte SD. Pour ce faire, nous pouvons utiliser la commande:
```sh 
    rsync -rlt --progress --delete /buildroot/output/images/ /workspace/buildroot-images

    # ou le scripte
    /usr/local/bin/sync-images.sh
```

En utilisant Balena Etcher, nous flashons notre catre SD avec l'image `buildroot-images/sdcard.img`.

En insérant la carte SD dans la cible et la démmarant, nous pouovons observer la séquance de lancement de U-Boot avec une comminucation série (cable série USB). Enfin, nous pourrons nous connecté une fois le boot terminé avec le login `root` _**sans mot de passe**_
![First Strart](./img_rapport/first_start.png "First Start")

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
# TODO modiyf : l'ip doit être changer dans le script pour l'U-Boot ainsi que dans l'overlay pour le linux 
> _Note / supposition à verifier_\
> L'adresse IP de la cible est défini au moment du boot, puis au démarrage de Linux. Elle est écrite en dur, mais nous pouvons la modifier. \
> Les fichiers `/workspace/boot-scripts/boot_cifs.cmd` et `/workspace/config/board/friendlyarm/nanopi-neo-plus2/rootfs_overlay/etc/network/interfaces` contiennent les informations dont nous avons besion. \
> Il suffira de recréer une image (sans recompiler le kernel) et de la déployer.

La commande `uname -a` nous permet de voir le système d'exploitation de la cible.

### Mise en place de l’espace de travail (workspace) sous CIFS/SMB
Cette étape nous permettra de partager notre répertoire de travail avec la cible. Ce que donnera un access directe depuis la cible et nous évitera les transferts de fichiers.\
Pour effectuer l'attachement du workspace nous devons disposer du dossier `/workspace` sur la cible.
```sh 
mkdir -p /workspace
# -p, --parents
#       no error if existing,\
#       make parent directories as needed
```
Pour attacher et détacher le dossier manuellement:
```sh
mount -t cifs -o vers=1.0,username=root,password=toor,port=1445,noserverino //192.168.0.4/workspace /workspace

umount /workspace
```

Il est possible d'automatiser le processus éditant le fichier `/etc/fstab` en ajoutant la ligne:
```sh
//192.168.0.4/workspace /workspace cifs vers=1.0,username=root,password=toor,port=1445,noserverino
```
Cela permet d'utiliser la commande `mount -a` pour effectuer tout les montage qui ont été paramétré.

_possibilité d'ajouter `mount -a` dans un scritpt de démarage? (nécessaire?)_

### Génération d’applications sur la machine de développement hôte
L'example de Makefile ci-dessous donne les paths utile pour la compilation croisée.

```Makefile
# Makefile toolchain part
TOOLCHAIN_PATH=/buildroot/output/host/usr/bin/
TOOLCHAIN=$(TOOLCHAIN_PATH)aarch64-linux-

# Makefile common part
CC=$(TOOLCHAIN)gcc
LD=$(TOOLCHAIN)gcc
AR=$(TOOLCHAIN)ar
CFLAGS+=-Wall -Wextra -g -c -mcpu=cortex-a53 -O0 -MD -std=gnu11
```


### Debugging de l’application sur la cible (VS-Code)
Pour plus d'[infos](https://mse-csel.github.io/website/assignments/environnement/#debugging-de-lapplication-sur-la-cible-vs-code)

### Mise en place de l’environnement pour le développement du noyau sous CIFS/SMB

### Questions
1. Comment faut-il procéder pour générer l’U-Boot ?
    
    On peut se déplacer dans le dossier `/builroot` et executer la commande `make` pour compiler complétement le builroot qui contient également U-Boot. `make` utilise la fichier `.config` dans lequel se touvent toutes les informations nöcessaire à la compilation.

2. Comment peut-on ajouter et générer un package supplémentaire dans le Buildroot ?

    Dans le dossier `/buildroot` nous effectuons la commande `make menuconfig`  qui nous ouvre fenêtre de parametragde de buildroot. Puis, dans `Target packages` nous pouvons donc y ajouter des packages supplémentaires. Pour la génération, il suffira de refaire un `make`.

3. Comment doit-on procéder pour modifier la configuration du noyau Linux ?

    Denouveau dans `/buildroot` avec la commande `make menuconfig`, nous selectionons les paramètres dans `Kernel`.    

4. Comment faut-il faire pour générer son propre rootfs ?

    Les paramètres de rootfs peuvent également être modifié dans `make menuconfig` de `/buildroot` sous la rubrique `Filesastem images`. Puis, il faudra faire une compilation, après quoi effacer l'ancien rootfs et y extraire le nouveau :

    ```sh
    rm -Rf /rootfs/*
    tar xf /buildroot/output/images/rootfs.tar -C /rootfs
    ```

5. Comment faudrait-il procéder pour utiliser la carte eMMC en lieu et place de la carte SD ?

    Il nous faudrait transferet tout ce qui se trouve sur la carte SD dans la carte eMMC, à savoit l'U-Boot, le Kernel Linux, ainsi que les differents système de fichier. Il nous faura créer une partition par entité à mettre sur la carte eMMC.

    > Il faudra peut-être préciser au _sunxi-spl_, le booteur initial de NanoPi, l'emplacement d'U-Boot.


6. Dans le support de cours, on trouve différentes configurations de l’environnement de développement. Qu’elle serait la configuration optimale pour le développement uniquement d’applications en espace utilisateur ?

    Pour déveleppement d'application en espace utilisateur, nous aurons besion d'une flexibilité au niveau de l'usrfs, car c'est à cette endroit que sera déployé l'application. De ce fait, nous pouvons déployer dés le début l'U-Boot, le kernel, ainsi que le rootfs sur la carte SD ou eMMC, afin de ne plus intéragir phisyquement avec notre cible. Puis, nous attacherons l'usrfs se trouvant sur la machine hôte en utilisant CIFS/SMB, ce qui nous permettrait de faire la compilation sur la mchine hôte directement dans l'usrfs et de tester avec la cible en accedant au même usrfs.

### Todo ce qui a été apris 
### Todo remarque et chose a retenir
### feedback personnel


---
## 2. Programmation Noyau
## 2.1 Modules noyaux

Le premier exersice nous propose de consevoir et générer un module noyau [out of tree](https://mse-csel.github.io/website/lecture/programmation-noyau/modules/module-gen/#generation-out-of-tree). C'est donc un module qui est à l'exterieure de l'arboraissance du noyau. Cela nous permet de généré le module indépendament du kernel, mais il ne pourra pas être linké statiquement à ce dernier. En s'inspirant de l'[example](https://mse-csel.github.io/website/lecture/programmation-noyau/modules/module/#squelette-dun-module), nous avons créer notre module qui nous dis bonjour et aurevoir. Pour instancier le module il nous faut nous rendre dans le répertoire oú se trouve notre module au format `*.ko` et executer la commande `insmod *.ko`. Pour le retirer, nous utilisons la commande `rmmod <module>`. Le affichage effectué par le module ne sont pas visible sur le terminal dans l'espace utilisateur. Pour les visioner, nous devons utiliser la commande `dmesg`. Les commande `lsmod` et `cat /proc/modules` nous permettent de visualiser les modules installés. 

Pour ajouter aux modules référencé, nous devons installer le module en ajoutant la comande `make install` aux Makefile de notre module. Cela ajoutera notre mode dans `/lib/modules/<kernel_version>/modules.dep`, qui référance tout les modes et leurs dépendances.
```c
    MODPATH := /rootfs # production mode install:
    install:
        $(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_PATH=$(MODPATH) modules_install
```
Pour l'installer, nous effectuons la commande `sudo make install` sur la machine hôte. Avec modprobe, nous pouvons :
```sh
Usage: modprobe [-alrqvsD] MODULE [SYMBOL=VALUE]...

        -a      Load multiple MODULEs
        -l      List (MODULE is a pattern)
        -r      Remove MODULE (stacks) or do autoclean
        -q      Quiet
        -v      Verbose
        -s      Log to syslog
        -D      Show dependencies
```


> Note.\
Le [makefile d'example](https://mse-csel.github.io/website/lecture/programmation-noyau/modules/module-gen/#generation-out-of-tree) propose d'utiliser `TOOLS := /buildroot/output/host/usr/bin/aarch64-linux-gnu-`, qui est incorrect, nous avons utiliser `TOOLS := /buildroot/output/host/usr/bin/aarch64-buildroot-linux-gnu-` afin dde compiler.\
\
Il faut également ajouter `clean` pour la commande `make clean`

# Apris 
Utilisation de make de facons récursive, extrainement utile et puissant.





# 31.03.23 on commance: sysfs (Exercice #5: Développer un pilote)