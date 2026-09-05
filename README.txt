####### Prérequis pour travailler sur le projet: 
    - Installer QT Creator 12.0.2 (avec QT 6.11.2, je ne sais plus si QTCreator installe QT 6.11.2 lors de son installation)
    - Installer mysql 8.0.35 https://dev.mysql.com/downloads/installer/
    - Installer Mysql workbench
    - Installer github desktop
    - Installer Liquibase (dernière version)
    
####### Pour lancer le server Mysql(service windows): 
	Il existe un service windows. Le retrouver et le lancer.
	service name: Mysql@localhost:3306
	Pour lancer un service dans un invite de cmd: 
	sc start mysql80
	sc stop mysql80
	ce service ne permet pas par défaut d'autoriser des connections autres que localement. 
	pour autoriser des connections externes, il faut ajouter la ligne bind_address=0.0.0.0 dans le fichier C:\ProgramData\MySQL\MySQL Server 8.0\my.ini

####### Pour configurer une première connection:
	ALTER USER 'root'@'localhost' IDENTIFIED BY 'MOT_DE_PASSE'; --(change mdp root)
	create user 'adrien'@'localhost' identified by 'adrien'; --(créer un utilisteur)
	grant all on dbtest.* to 'adrien'@'localhost'; --(Donne tous les privilèges à l'utilisateur)
    
    
####### Pour se connecter au server:
    utiliser Mysql Workbench, en tant que "root" sur le port localhost:3306
    
####### Installation de QT 
    Pour que QTCreator fonctionne avec mysql il faut obtenir les bonnes DLL:

    Il faut récupérer le driver qt correspondant à la version de QT présent sur la machine et au compilateur présent sur le PC (mingw probablement)
    Le lien suivant propose de télécharger le driver https://github.com/thecodemonkey86/qt_mysql_driver/releases
    Une fois le dossier zippé téléchargé, déziper, puis: 
        copier coller les 3 DLL libcrypto-3-x64, libmysql, libssl-3-x64 dans le dossier C:\Qt\QT\6.11.2\mingw_64\bin
        copier coller les libs contenues dans le dossier sqldrivers ici: C:\QT\6.11.2\mingw_64\plugins\sqldrivers

####### Liquibase

- Intro :
    Liquibase est un outil qui permet de suivre de façon atomique les modifications apportées à une base de données par tous les développeurs.
    Chaque ajout/modification de table, de colonne, de ligne dans une table ou de propriétés d'une colonne sont consignées dans un fichier texte remonté en Gconf comme n'importe quel fichier source. Ce fichier s'appelle "microbrico-changelog.mysql.sql". 
    C'est le point d'entrée de toute modification que l'on souhaite opérer sur la DB.
    Chaque Dev doit renseigner les modifications qu'il souhaite faire à la DB dans ce fichier. 
    L’outil liquibase se charge de vérifier quelles modifications ont déjà été déployées sur une DB, et quelles modifications doivent être remontées. 
    La commande liquibase update fait tout cela sans effort : 
    •	Elle parse le fichier "microbrico-changelog.mysql.sql"
    •	Elle détecte les changeset qui y sont contenus
    •	Elle détermine quels changeset ne sont pas encore appliqués à la DB "target" (plus d'info sur la configuration d'une DB target dans le chapitre "property file") 
    •	Elle applique les nouveaux changeset à la DB target
    
- Avantages : 
        1 - Cet outil rend simple le déploiement de modifications faites par d'autres développeurs 
        2- Cet outil permet de faciliter les « merges » de modifications éventuelles, en centralisant tous les changesets dans un fichier texte éditable. 

- Le fichier de propriété
    Pour configurer les accès de Liquibase à la DB de développement (ou aussi appelée db target ), il est possible de créer un fichier de propriété pour chaque utilisateur.
    Chaque utilisateur dispose ainsi de sa propre configuration qu’il est libre de remonter ou non dans Git. 
    Par exemple dans le fichier de propriété « liquibase_AGO.properties » Il y a l’intégralité de la configuration de la DB de dev qui permet à liquibase de s’y connecter
    A chaque fois que l’utilisateur AGO souhaite remonter dans sa DB de dev les modifications de ses collègues, il doit ouvrir une invite de commande dans le dossier liquibase à la racine du projet, et entrer la commande suivante : 
    liquibase --defaults-file=liquibase_AGO.properties update
    En revanche s’il veut remonter les modifications dans la DB de production, il suffit simplement de taper : 
    liquibase update
    car liquibase lit par défaut le fichier de propriété qui s’appelle « liquibase.properties » dans lequel sont contenues les informations de connexion au serveur de production.
    
- Modification du fichier de Changelog 
    Très simplement, un fichier changelog contient plusieurs changeset (voir schéma ici : https://docs.liquibase.com/concepts/introduction-to-liquibase.html)
    Chaque changeset contient/décrit une opération atomique sur la DB target.
    Chaque changeset contient dés informations sur l’auteur de la modification. 
    Il est préférable d’ajouter un commentaire à chaque changeset pour donner du contexte : 
    Voici un exemple de changeset : 
    --changeset Adrien:1 comment:blablabla
    create table company (
        id int primary key,
        address varchar(255)
    );

    Ici il y a des informations sur le format des changesets : https://docs.liquibase.com/concepts/changelogs/changeset.html

    