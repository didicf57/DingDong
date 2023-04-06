# DingDong

Cette sonnette permet de recevoir une notification sur son Smartphone (Android ou Apple) lorsque quelqu'un sonne à votre porte.

Pour cela, la sonnette doit etre connecté au reseau WiFi et permettre d'enregistrer son identifiant SimplePush.

Pour la connexion WiFi, l'ESP32 se connecte grace au WPS.
Pour enregistrer l'identifiant (2 possibles), l'ESP32 dispose d'une page Web embarquée accessible grace à l'utilisation de mDNS.
De plus, les mises à jour sont possibles grace à une page web specifique.

Il y a bien sur une partie electronique à réaliser afin d'alimenter l'ESP32 et de detecter l'appui sur le bouton.
Cette partie depend donc du type de sonnette utilisée tout en conservant la fonction d'origine.

Voir la notice PDF pour le mode d'emploi et les crédits.
