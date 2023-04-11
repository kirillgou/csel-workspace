---
title       : Rapport CSEL
author      : "Kirill Goundiaev & Tanguy Dietrich"
date        : \today
papersize   : A4
geometry    : "left=2cm,right=2cm,top=1.5cm,bottom=2cm"
colorlinks  : urlcolor
fontsize    : 11pt
lang        : fr-CH
---

<!--
## TODO
 NO Comment
--> 

# Environnement Linux embarqué et programmation noyau Linux

# Préambule
Ce travail repose sur la théorie et les instructions données lors du cours CSEL faisant partie du cursus MES de la HES-SO. Ces informations sont disponible sur le [site du cours](https://mse-csel.github.io/website/). Une base de code est également donnée sur le [git du cours](https://github.com/mse-csel/csel-workspace).

# Introduction
Le but de ce travail est de mettre en place un environnement de travail pour une cible embarquée, la compréhension de différentes zones de mémoire, développement de module pour le kernel linux, ainsi que d'application.

# 1. Environnement Linux embarqué
## Mise en place de la machine hôte
Pour mener à bien ce travail, nous avons besion d'une machine hôte sous Linux, Windows ou OSx et des logiciels suivants: [Docker Desktop](https://www.docker.com/products/docker-desktop), [Git](https://github.com/), [Visual Studio Code](https://code.visualstudio.com/), [Balena Etcher](https://www.balena.io/etcher/)

Nous commençons par forker le repos principal, puis le pull sur notre machine. Nous ouvrons le repos dans VSCode et acceptons de le réouvrir dans un Container (Reopen in COntainer). Si VSCode ne nous propose pas cette option, il faut verifer que le module 'Dev Containers' soit bien installé.

Une fois ouvert dans un Container, nous executons le scripte `get-buildroot.sh` dans le terminal de VSCode. Cet scripte nous permet de télécharger buildroot et faire la configuration nécessaire pour ce projet. [Buildroot](http://buildroot.uclibc.org/) est un outil qui nous permettera de générer une distribution Linux pour notre cible à partir de zéro. Cette outil se compose :\
1. D'une chaine de compilation croisée nous permetant de compiler pour notre cible dont l'architecture est différente de notre hôte.\
2. Image du noyeau (Kernel) Linux, code souce (niveau Kernel) de l'OS que nous déployerons sur la cible\
3. RootFS, le système fichier racine est le répertroire principal contenant des répertoires, logiciels de base du système et des logiciel d'applications.\
4. Bootloader, dans notre cas U-Boot qui est le premier programme à être exécuté au lancement du système. Il se chargera d'effectuer certaines verification, pius de démarrer le système principal sous Linux.\

## Compilation et lancement de la cible
Le noyeau et le rootfs peuvent être configuer dans buildroot avec les commandes:

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

    # ou le script :
    /usr/local/bin/sync-images.sh
```

En utilisant Balena Etcher, nous flashons notre catre SD avec l'image `buildroot-images/sdcard.img`.

En insérant la carte SD dans la cible et la démmarant, nous pouovons observer la séquance de lancement de U-Boot avec une comminucation série (cable série USB). Enfin, nous pourrons nous connecté une fois le boot terminé avec le login `root` _**sans mot de passe**_
![First Strart](./img_rapport/first_start.png "First Start")

## Configuration pour la communication réseau
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
> Les fichiers `/workspace/boot-scripts/boot_cifs.cmd` et `/workspace/config/board/friendlyarm/nanopi-neo-plus2/rootfs_overlay/etc/network/interfaces` contiennent les informations dont nous avons besoin. \
> Il suffira de recréer une image (sans recompiler le kernel) et de la déployer.

La commande `uname -a` nous permet de voir le système d'exploitation de la cible.

## Mise en place de l’espace de travail (workspace) sous CIFS/SMB
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

Il est possible d'automatiser le processus en éditant le fichier `/etc/fstab` en ajoutant la ligne:
```sh
//192.168.0.4/workspace /workspace cifs vers=1.0,username=root,password=toor,port=1445,noserverino
```
Ensuite il est possible d'utiliser la commande `mount -a` pour effectuer tout les montage qui ont été paramétré. Sinon le montage se fera automatiquement au démarrage de la cible.

## Génération d’applications sur la machine de développement hôte
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


## Debugging de l’application sur la cible (VS-Code)
Pour plus d'[infos](https://mse-csel.github.io/website/assignments/environnement/#debugging-de-lapplication-sur-la-cible-vs-code)


## Mise en place de l’environnement pour le développement du noyau sous CIFS/SMB
Executer la [marche a suivre](https://mse-csel.github.io/website/assignments/environnement/#debugging-de-lapplication-sur-la-cible-vs-code)


## Questions
1. Comment faut-il procéder pour générer l’U-Boot ?
    
    On peut se déplacer dans le dossier `/builroot` et executer la commande `make` pour compiler complétement le builroot qui contient également U-Boot. `make` utilise la fichier `.config` dans lequel se touvent toutes les informations nécessaire à la compilation.

2. Comment peut-on ajouter et générer un package supplémentaire dans le Buildroot ?

    Dans le dossier `/buildroot` nous effectuons la commande `make menuconfig`  qui ouvre une fenêtre de parametrag de de buildroot. Puis, dans `Target packages` nous pouvons donc y ajouter des packages supplémentaires. Pour la génération, il suffira de refaire un `make`.

3. Comment doit-on procéder pour modifier la configuration du noyau Linux ?

    Denouveau dans `/buildroot` avec la commande `make menuconfig`, nous selectionnons les paramètres dans `Kernel`.    

4. Comment faut-il faire pour générer son propre rootfs ?

    Les paramètres de rootfs peuvent également être modifié dans `make menuconfig` de `/buildroot` sous la rubrique `Filesystem images`. Puis, il faudra faire une compilation, après quoi effacer l'ancien rootfs et y extraire le nouveau :

    ```sh
    rm -Rf /rootfs/*
    tar xf /buildroot/output/images/rootfs.tar -C /rootfs
    ```

5. Comment faudrait-il procéder pour utiliser la carte eMMC en lieu et place de la carte SD ?

    Il nous faudrait transferé tout ce qui se trouve sur la carte SD dans la carte eMMC, à savoir l'U-Boot, le Kernel Linux, ainsi que les differents système de fichier. Il nous faudra créer une partition par entité à mettre sur la carte eMMC.

    > Il faudra peut-être préciser au _sunxi-spl_, le booteur initial de NanoPi, l'emplacement d'U-Boot.


6. Dans le support de cours, on trouve différentes configurations de l’environnement de développement. Qu’elle serait la configuration optimale pour le développement uniquement d’applications en espace utilisateur ?

    Pour le développement d'application en espace utilisateur, nous aurons besion d'une flexibilité au niveau de l'usrfs, car c'est à cette endroit que sera déployé l'application. De ce fait, nous pouvons déployer dés le début l'U-Boot, le kernel, ainsi que le rootfs sur la carte SD ou eMMC, afin de ne plus intéragir physiquement avec notre cible. Puis, nous attacherons l'usrfs se trouvant sur la machine hôte en utilisant CIFS/SMB, ce qui nous permettrait de faire la compilation sur la machine hôte directement dans l'usrfs et de tester avec la cible en accedant au même usrfs.

## Todo ce qui a été apris 
## Todo remarque et chose a retenir
## feedback personnel


---
# 2. Programmation Noyau
# 2.1 Modules noyaux
## Exercice 1
Le premier exercice nous propose de consevoir et générer un module noyau [out of tree](https://mse-csel.github.io/website/lecture/programmation-noyau/modules/module-gen/#generation-out-of-tree). C'est donc un module qui est à l'exterieure de l'arboraissance du noyau. Cela nous permet de généré le module indépendament du kernel, mais il ne pourra pas être linké statiquement à ce dernier. En s'inspirant de l'[example](https://mse-csel.github.io/website/lecture/programmation-noyau/modules/module/#squelette-dun-module), nous avons créer notre module qui nous dis bonjour et aurevoir. Pour instancier le module il nous faut nous rendre dans le répertoire oú se trouve notre module au format `*.ko` et executer la commande `insmod *.ko`. Pour le retirer, nous utilisons la commande `rmmod <module>`. Le affichage effectué par le module ne sont pas visible sur le terminal dans l'espace utilisateur. Pour les visioner, nous devons utiliser la commande `dmesg`. Les commande `lsmod` et `cat /proc/modules` nous permettent de visualiser les modules installés. 

Pour ajouter aux modules référencé, nous devons installer le module en ajoutant la comande `make install` aux Makefile de notre module. Cela ajoutera notre mode dans `/lib/modules/<kernel_version>/modules.dep`, qui référance tout les modes et leurs dépendances.
```makefile
    #besion de l'expor PATH pour faire modules_install
    MODPATH := /rootfs # production mode install:
    install:
        $(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_PATH=$(MODPATH) modules_install
```
Pour l'installer, nous effectuons la commande `sudo make install` sur la machine hôte. Voici les parametres de modprobe :
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
Le [makefile d'example](https://mse-csel.github.io/website/lecture/programmation-noyau/modules/module-gen/#generation-out-of-tree) propose d'utiliser `TOOLS := /buildroot/output/host/usr/bin/aarch64-linux-gnu-`, qui est incorrect, nous avons utiliser `TOOLS := /buildroot/output/host/usr/bin/aarch64-buildroot-linux-gnu-` afin de compiler.\
\
Il faut également ajouter `clean` pour la commande `make clean`

### Apris, remarque, feedback
Utilisation de make de facons récursive, extrêmement utile et puissant. Manipulation avec `modprobe` facilite l'installation et le retrait des modules, ainsi que la gestion des dépendances. La gestion de dépendance n'a pas encore été testé, serait un plus.

## Exercice 2
Cette exercice à pour but de [passer des parammetres au module](https://mse-csel.github.io/website/lecture/programmation-noyau/modules/parameters/) lors de son initialisation. Nous réutilisons le code de l'exercice précédent.\
Utilisation de macro [module_param](https://mse-csel.github.io/website/lecture/programmation-noyau/modules/parameters/).\ 
Code ajouté :
```c
    /*my_module.c*/
    #include <linux/moduleparam.h>  /* needed for module parameters */
    static char* name = "Module ex 2";
    module_param(name, charp, 0);
    static int elements = 1;
    module_param(elements, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    static int __init my_module_init(void){
        pr_info("Name: %s\telement: %d\n", name, elements);
        ...
    }
```
```sh
#passage de paramètres :
insmod mod_ex_noyau_2.ko elements=1024 name="test_1"
#chage param
vi /sys/module/mod_ex_noyau_2/parameters/elements

# avec modprobe ajouter fichier /etc/modprobe.conf et dedans:
options mod_ex_noyau_2 elements=12 name="From modprobe"
```
### Apris, remarque, feedback
Utilisation des paramètres avec les modules, donner les drois de modificaition des paramètre.

## Exercice 3
Trouvez la signification des 4 valeurs affichées lorsque l’on tape la commande `cat /proc/sys/kernel/printk`.
[Message logging with printk](https://www.kernel.org/doc/html/latest/core-api/printk-basics.html) nous apprend que les valeurs retournées par cette commande nous informe du _console_loglevel_ courrant.
Elles corespondent au niveau courrant, par défaut, minimum et du boot-time. Le niveau peut être modifié avec la commande `echo 8 > /proc/sys/kernel/printk` (8 = print all messages to the console).
La corespondance des niveaux: [Source](https://www.kernel.org/doc/html/latest/core-api/printk-basics.html)
![log Level](img_rapport/Kernel_Log_level.png "Log Level")

Le _log level_ définit l'importance du message de `printk(`_`log level`_` "Message: %s\n", arg);`. Si le niveau est supprérieure au niveau courrant, le message sera affiché directement dans le terminal.

> Note pas sur d'avoir compris comment changer et surtout afficher..

> Remarque. Nous avons remarqué que le print se fait dans le terminal série et pas le ssh. Donc fonctionne. Cependant, il faudrait voir s'il est possible de printer dans le terminal qui lance la commande pour le module.


### Apris, remarque, feedback
Pas reussi a utiliser correctement les niveaux de message..

## Exercice 4
Cette exercice nous demande de faire de l'allocation dynamique de memoire au niveau du kernel. Pour cela, nous devrons utiliser la fonction `kmalloc()`.\
Nous aurons besoin de :\
1. [l'allocation dynamique](https://mse-csel.github.io/website/lecture/programmation-noyau/modules/malloc/#allocateur-kmalloc)\
2. Traitement des strings\
3. [Gestion de list](https://mse-csel.github.io/website/lecture/programmation-noyau/modules/bibliotheques/#exemple-de-liste-chainee)\

> Attention à bien libérer l'éspace lors du exit

### Apris, remarque, feedback
Nous avons revu comment faire une allocation de mémoire au niveau kernel. Nous avons également appris à utiliser les list avec la librairie list.h.

## Exercice 5: Accès aux entrées/sorties
Cette exercice nous demande de récupérer les informations se trouvant dans des registres sur notre cible. Pour ce faire, nous devons acceder à des zones précises dans la mémoire. On utilise alors le MMIO memory-mapped I/O et un remapping dans la memoire virtuelle.

> Note\
L'utilisation de `request_mem_region` pose problème. La réservation ne se passe pas correctement. Nous avons également testé avec le code de correction fourni, et l'erreure est également présente.\
//TODO (ca sonne mal) : Sans la reservation peut aussi fonctionner.\
Avec la commande `cat /proc/iomem`, nous avons pu observer que cette espace est déjà reservé par :\
    01c30000-01c3ffff : 1c30000.ethernet ethernet@1c30000

> attention les données sont en BigEndian
> Note\
Les valeurs retournées pour la température sembles cohérente, mais sont pas exactement les même qu'avec la commande `cat /sys/class/thermal/thermal_zone0/temp`

### Apris, remarque, feedback
Nous avons appris la reservation de mémoire, ainsi que le fait qu'elle ne bloque pas le bon exécution de programme si elle échoue. Il faut également faire attention avec le Big et Little Endean.


## Exercice 6: Threads du noyau
Dans cette exercice, nous devons implémenté un module qui [lancera un thread](https://mse-csel.github.io/website/lecture/programmation-noyau/modules/threads/#creation-de-threads-dans-le-noyau) à son instantiation. Ce thread devra afficher un message toutes les 5 secondes. 

Nous avons utiliser les fonctions proposé dans le cours pour faire cette exercice. A noter que le paramettre `namefmt` donné lors de l'initialisation `struct task_struct* kthread_run(int (*threadfn)(void *data), data namefmt,...);` peut être retrouvé en effectuant la commande `ps -aux` qui nous liste les processes en éxecution.

### Apris, remarque, feedback
Nous avons apprit à lancer les threads au niveau kernel. Tout comment en C, ils peuvent récupérer des données à leurs lancement à travers des structures de données. 


## Exercice 7: Mise en sommeil
Le but de cette exercice est de synchroniser deux thread du noyau à l'aide d'events. Pour ce faire, nous aurons besion d'une queue d'attente, qui nous permettera d'informer un thread qu'une modification à été effectuée. Une fois le thread informé, il se reveillera et effectuera une verification sur la condition qui lui est donnée. Si la condition est remplie, il effectuera l'action qui lui est demandé. Sinon, il se remettra en sommeil.\
Dans notre cas, la condition est une valeur de variable atomique. Cette variable nous permet d'effectuer des opérations atomiques et donc nous autorise un accès concurrent.\
Le thread qui fera la notification (le reveil) incrémentera la variable atomique. Le thread qui fera l'attente (le sommeil) verifera la variable et la décrémentera. Si elle est positive, le thread qui attend effectuera l'action.\
Afin de partager l'information entre deux threads, nous avons deux solutions :\
1. utiliser une variable globale\
2. utiliser une variable partagée\

Nous avons opter pour les varables partagées, même si elles sont plus complexes à mettre en place, nous trouvons que c'est une solution plus propre.\
Pour partager les données entre les threads, nous avons utilisé une structure de données qui nous permet de stocker les données et de les passer en paramettre à nos fonctions.\
Nos deux fonctions foctionnent correctement et donne les résultats attendu. A noter qu'il ne faut pas oublier le ```\n``` à la fin de la chaine de caractère pour que le message soit afficher imédiatement, sinon celà pour porter confusion lors des observations.

### Apris, remarque, feedback
Nous avons appris à utiliser les variables atomiques, ainsi que les queues d'attente. Il était particulierement intéressant de metre en place une execution concurrente avec une communication inter thread. C'est une notion que nous pensons peut être souvant utile. Nous avons également mis en place un affichage coloré que facilite la lecture des messages.
 
## Exercice 8: Interruptions
Cette exercice nous demande de créer un module qui va nous permettre de récupérer les interruptions des switchs. Pour ce faire, nous devons utiliser [les fonctions](https://mse-csel.github.io/website/lecture/programmation-noyau/modules/interruptions/#installation-des-routines-de-traitement-des-interruptions) `request_irq`, `gpio_to_irq` et `free_irq`.\
Pour la gestion des GPIO nous avons utilisé la librairie gpio.h, plus particulierement les fonctions `gpio_is_valid` `gpio_request`, `gpio_direction_input` et `gpio_free`.\
Après initialisation, si nous appuyons sur un des boutons, le terminal nous affiche le numéro d'interruption du bouton correspondant.\

### Apris, remarque, feedback
Le numéro du GPIO à utiliser peut être retrouvé dans [le schéma](https://mse-csel.github.io/website/documentation/assets/Schematic_NanoHat_OLED_v1.4_1804.pdf).\
La commande `cat /proc/interrupts` nous permet de voir les interruptions en cours:\
```sh
    88:         31          0          0          0  sunxi_pio_edge   0 Edge      irq_k1
    90:         42          0          0          0  sunxi_pio_edge   2 Edge      irq_k2
    91:         36          0          0          0  sunxi_pio_edge   3 Edge      irq_k3
```
Attention à garder l'interruption en cours.\


# 2.2 Pilotes de périphériques
Quatre types de pilotes de périphériques sont disponibles dans le noyau Linux :\
- pilotes orientés caractère (char device driver)
- pilotes orientés bloc (block device driver)
- pilotes orientés réseaux (network device driver)
- pilotes orientés mémoire (uio device driver)


## Exercice 1: Pilotes orientés mémoire
Cette exercice nous demande de créer un [pilote orienté mémoire](https://mse-csel.github.io/website/lecture/programmation-noyau/pilotes/uio-driver/). C'est le plus simple des pilotes à créer qui nous permet de mapper dans l'espace virtuel du processus les registres et zones mémoires.\
Ce mapping se fait avec la fonction `mmap` qui prend en paramettre le fichier `/dev/mem` qui offre ce srvice par défaut sous Linux.\
Cette methode permet d'acceder à la mémoire uniquement depuis l'application dévelopée, ne partageant pas cette information les autres parties du système à travers des fichiers partagés.\
Il faut faire attention avec les adresses. Nous pouvons adresser uniquement depuis de début de la page d'adresse. De ce fait nous devons savoir à quelle offset de la page se trouve notre donnée.\
Pour le faire, nous pouvons utiliser la fonction `getpagesize()` qui nous retourne la taille d'une page.\
Pour avoir l'offset de notre donnée, nous devons faire `offset = adresse % getpagesize()`.\
Puis nous pouvons obtenir l'adresse de la page avec `adresse_page = adresse - offset`.\
De plus lorsque nous voulons acceder à une donnée, nous devons faire `regs + (offset + position * 4 )/sizeof(uint32_t)` ou `regs + offset / sizeof(uint32_t) + (position)`.\

### Apris, remarque, feedback
Nous avons appris à utiliser mmap au niveau utilisateur. Nous avons recontré des problèmes avec les adressage, que nous avons pu réger en nous aidant du code de correction fourni. De plus, il était compliqué de créer un Makefile pour compiler notre programme. Nous avons donc utilisé le Makefile fourni dans le dossier de correction. Nous pensons que ça serait une bonne idée d'avoir un petit explicatif sur les differents Makefile kernel et userspace.\

## Exercice 2: Pilotes orientés caractère
Cette exercice nous demande de créer un (pilote orienté caractère)[https://mse-csel.github.io/website/lecture/programmation-noyau/pilotes/char] nous permetant de stocker et récupérer une valeur à l'aide des commandes `read` et `write`.\ 
Ce pilote est un module et donc est dévellopé au niveau du noyau. Il va nous permettre d'intéragir à travers le fichier `/dev/[char_driver]`.\
Le pilote doit comporter les fonctions `open`, `release`, `read` et `write`. Il doit aussi être enregistré avec la fonction `alloc_chrdev_region` et déregistré avec `unregister_chrdev_region` et associé à un fichier avec `cdev_init` et `cdev_add`.\
Une fois le pilote chargé, nous devons créer le fichier `/dev/[char_driver]` avec la commande `mknod /dev/[char_driver] c [MAJOR] [MINOR]`. L'affichage de ces valeur est pratique lors de l'initalisation avec `MAJOR(dev_t)` et `MINOR(dev_t)`. Pour retirer le fichier, une fois que le module est retiré, nous pouvons utiliser la commande `rm /dev/[char_driver]`.\

### Apris, remarque, feedback
Nous nous sommes rafraichis la mémoire sur la création de pilote de type caractère et avons apris à utiliser la commande `mknod` pour rendre accecible par à travers le fichier notre pilote.\

## Exercice 3: Pilotes orientés caractère
Dans cette exercice, nous devons reprendre le pilote de l'exercice 2 et le modifier pour que nous puissions spécifier le nombre d'instances que nous voulons créer.\
Les fonctions `static inline unsigned iminor(const struct inode *inode)` et `static inline unsigned imajor(const struct inode *inode)` permettent de récupérer le numéro major et minor du fichier qui est utilisé. De cette manière, nous pouvons savoir vers quel buffer adresser l'opération. Nous pouvons aussi retrouver cette information à partir du descripteur de fichier avec la fonction `iminor(file_inode(f))` ou `iminor(f->f_inode);` et `imajor(file_inode(f))` ou `imajor(f->f_inode)`.\
Ne pas oublier d'ajouter le paramettre dans `vi /etc/modprobe.conf ` : `options mod_ex_pilotes_3 instances=5`
Puis il faut refaire la commande `mknod /dev/[char_driver] c [MAJOR] [MINOR]` pour créer les fichiers.\












## Ajout personnel
Nous avons jouter un script de lancement pour savoir plus facilement quand notre cible est prette pour que nous puissions s'y connecter en ssh. 
```sh
    # in /etc/init.d/S90kirillEasy 
    #!/bin/sh
    cd /sys/class/gpio/
    echo 10 > export 
    echo out > gpio10/direction 
    echo 1 > gpio10/value
```



# 31.03.23 on commance: sysfs (Exercice #5: Développer un pilote)