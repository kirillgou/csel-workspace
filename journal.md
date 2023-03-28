- pull git
- launche docker throw Dev Containers
- downlaod buidroot with get-buildroot.sh

make buildroot:
    make -j8

actualisation du rootfs:
    rm -Rf /rootfs/*
    tar xf /buildroot/output/images/rootfs.tar -C /rootfs
    ou
    delete-rootfs.sh extract-rootfs.sh

gravure carte SD
copier les images dans le répretoire synchronisé avec l'ordinateur:
    rsync -rlt --progress --delete /buildroot/output/images/ /workspace/buildroot-images
    ou 
    sync-images.sh

avec Balena Etcher graver l'image buildroot-images/sdcard.img sur SD
se connecter avec putty sur le port COM en 115200 pour observer le bon démarrage du boot
se logger avec root

configurer ip pc a 192.168.0.4 dans paramètre réseau

tester les deux ping dans les deux sences, 192.168.0.4 et 192.168.0.14
connexion par ssh : 
    ssh root@192.168.0.14

Dans ce cas, effacez les entrées dans votre fichier known_hosts avec la commande suivante:
    ssh-keygen -R  192.168.0.14

uname -a, pour voir le système d’exploitation de la cible

----

Pour attacher manuellement l’espace de travail de la machine hôte sur la cible via CIFS/SMB, il faut :
- Se logger sur la cible (username : « root » et password : lasser vide)
- Créer la première fois un point d’attachement sur la cible :
    mkdir -p /workspace
- Attacher le workspace de la machine hôte sur la cible :
    mount -t cifs -o vers=1.0,username=root,password=toor,port=1445,noserverino //192.168.0.4/workspace /workspace
- Détacher le workspace de la cible :
    umount /workspace

Pour attacher automatiquement l’espace de travail de la machine hôte sur la cible via CIFS/SMB, 
il suffit d’effectuer une seule fois les instructions ci-dessous :
- Créer la première fois un point d’attachement sur la cible, par exemple :
    mkdir -p /workspace
- Editer le ficher “/etc/fstab” (avec vi) et ajouter la ligne ci-dessous :
    //192.168.0.4/workspace /workspace cifs vers=1.0,username=root,password=toor,port=1445,noserverino
- Activer la configuration :
    mount -a
----

```sh
    #avant le lancement, ouvrir vscode avec docker, puis après le lancement  
    mount -a
    # pour monter le workspace

```




