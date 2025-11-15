# TPReseaux_Client
Fonctions implémentées:
- Bio des joueurs, modifiable, affichable.
- Ajout/retrait/affichage des Amis (pas de comportement spécial implémenté)
- Chat global, chat privé avec un autre joueur et chat de partie
- Challenge des autres joueurs (pour lancer une partie)
- Jeu de Awalé


Notre UI est séparé en trois zones :
- Une zone BOARD où est affiché la partie en cours
- Une zone CHAT où sont affichés les commandes disponibles dans l'état actuel, l'historique du chat et les print autres que la partie active.
- une zone INPUT où l'on peut écrire les commandes à executer.


Guide:

Lancer l'application:
    - Utiliser les commandes "make" ou "make re" pour compiler. ("make clean" pour nettoyer les fichiers compilés)
    - Lancer le serveur dans un terminal avec "./serveur"
    - Dans un autre terminal vous pouvez alors lancer un client avec "./client [adresse_serveur] [pseudo]"
    - Vous arrivez alors dans le menu

Dans le menu :
    Dans le menu vous pouvez :
    - Voir les joueurs connectés avec "/list"
    - Ajouter ou modifier sa bio avec "/bio"
      Vous allez alors dans le mode saisie de la bio -> voir partie Dans la bio
    - Voir la bio d'un joueur connecté avec "/viewbio [user]".
      Un message est affiché si le joueur n'est pas trouvé ou qu'il n'a pas de bio.
    - Challenger un joueur avec "/challenge [user]" pour faire une partie avec lui.
      Un message est affiché si le joueur n'est pas trouvé ou qu'il n'est pas disponible (déjà en partie).
      Si le joueur existe et est disponible vous allez dans le mode challenge -> voir partie Dans le challenge
    - Ajouter un ami avec "/addfriend [user]"
      Le nombre maximum d'amis est 10.
      Un message est affiché si le joueur n'est pas trouvé, qu'il est déjà votre amis, si vous avez trop d'amis ou qu'il s'agit de vous-même.
    - Retirer un ami avec "/removefriend [user]"
      Un message est affiché si e joueur n'est pas trouvé dans votre liste d'amis.
    - Afficher les amis avec "/friends"
    - Envoyer un message dans le chat global en tapant n'importe quel entrée qui ne commance pas par l'une des commandes.
    - Envoyer un message privé à un joueur avec "/chat [user] [message]"
    - Observer la partie d'un autre joueur en partie avec "/spectate [user]" (fonction en développement)
    - Vider la partie CHAT de l'UI avec "/clearchat"


Dans la bio :
    Une fois dans le mode saisie de la bio vous pouvez:
    - Ajouter une ligne à votre bio en tapant n'importe quel ligne ne commençant pas par une commande.
    - Supprimer la bio actuelle avec "/clearbio"
    - Quitter le mode saisie de la bio avec "/endbio".
      Vous retournez alors dans le menu -> Voir la partie Dans le menu
    La taille de la bio est limitée à 10 lignes de 80 caractères. Au dela, vous n'aurrez pas de message d'erreur, mais les lignes ne seront plus enregistrées/coupées.

Dans le challenge :
    Une fois dans le mode challenge en tant que challenger vous pouvez :
    - Attendre que votre adversaire réponde à votre challenge
    - Annuler votre challenge avec "/cancelchallenge"
    Vous retournez alors dans le menu -> Voir la partie Dans le menu
    En challengeant un autre joueur, vous le faites aussi rentrer dans le mode challenge, il peut alors :
    - Accepter le challenge avec "/accept [user]"
    Vous entrez allors tous les deux en partie -> voir la partie Dans la partie
    Un message est affiché si le joueur n'est pas celui qui a lancé le challenge.
    - Refuser le challenge avec "/refuse [user]"
    Vous retournez alors tous les deux dans le menu -> Voir la partie Dans le menu
    Un message est affiché si le joueur n'est pas celui qui a lancé le challenge.

Dans la partie :
    Une fois dans la partie vous pouvez voir le board ainsi que les scores des joueurs et le joueur qui doit jouer. Si votre score est affiché à gauche vous êtes le joueur 1, votre côté du board est celui avec les lettres majuscules et vous commencez la partie. Si votre score est affiché à droite, vous êtes le joueur 2 et votre côté du board est celui avec les lettres minuscules.
    Le sens de semis est anti-horaire.
    En partie, vous pouvez :
    - Ecrire dans le chat de la partie en tapant un texte ne commençant pas par une commande
    - Jouer un move avec "/move [pit]" (pit la lettre du trou que vous voulez semer)
    Un message s'affiche si vous ne pouvez pas jouer ce pit vous expliquant la raison.
    Si vous avez joué un move valide, un message est affiché pour indiquer à vous et à votre adversaire le pit choisis.
    Les règles sont celles du Awalé de tournoi. Les cas où vous ne pouvez pas jouer sont donc : ce n'est pas votre tour, ce n'est pas un trou à vous, vous affamez l'adversaire en jouant ce trou ou le trou est vide.
    - Quitter la partie avec "/quitgame"
    Vous retournez alors dans le menu (avec l'autre joueur) -> Voir la partie Dans le menu
    Les joueurs reçoivent un message quand la partie est finie, mais ils restent dans la partie pour pouvoir voir le board final, discuter avec leur adversaire plus longtemps ou tenter de trouver un résultat différent dans le cas d'une fin par boucle (retour dans une situation déjà rencontrée). Les joueurs doivent donc quitter d'eux-même une fois qu'ils sont satisfaits.


Les utilisateurs (pseudo, amis, bio) sont sauvegardés dans un fichier .dat lorsque l'utilisateur se déconnecte. Attention, ils ne sont pas sauvegardés si c'est le serveur qui s'éteint avant la déconnexion de l'utilisateur. De plus, les infos ne sont pas visibles par "/list" et "/viewbio [user]" tant que les utilisateurs ne se sont pas reconnectés.



Organisation de notre application:

  - client2.c/h          # Client contenant toutes les fonctions permettant de communiquer avec le serveur et de traiter ses réponses.
  - server2.c/h          # Serveur contenant la communication avec le client mais aussi la logique de l'application avec les différents modes et commandes possibles.
  - game.c/h             # Fichiers contenant la struct game qui contient les informations d'une partie. Ces fichiers contiennent aussi une partie de la logique du jeu d'Awalé
  - board.c/h            # Fichiers comprenant les fonctions de gestion du plateau (représentant un état d'une partie) et la deuxième moitié de la logique du jeu
  - player.c/h           # Fichiers comprenant les fonctions de gestion des joueurs
  - simple_ui.c/h        # Fichiers contenant les fonctions permettant d'afficher notre UI client
  - common.h             # Définitions partagées et utilisées par plusieurs autres fichiers
  - Makefile             # Configuration de compilation