#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"

#define CAPA_SEQUENCE 16

/* =============================== */
/* Programme principal - émetteur  */
/* =============================== */
int main(int argc, char* argv[]) {

    int tailleFenetre; // Taille de la fenêtre glissante

    // Vérification du nombre de paramètres
    if(argc > 2){
        perror("Nombre de paramètres incorrect");
        return 1;
    }

    unsigned char message[MAX_INFO]; /* message de l'application */
    int taille_msg, evenement; /* taille du message */
    paquet_t reponse; /* paquet utilisé par le protocole */
    
    int curseur = 0; // Prochain numéro de séquence à envoyer
    int borneInf = 0; // Borne inférieure de la fenêtre glissante (premier paquet non acquitté)
    int acquitte[CAPA_SEQUENCE] = {0}; // Tableau pour garder trace des paquets acquittés
    paquet_t tabp[CAPA_SEQUENCE]; // Tableau de paquets envoyés mais non encore acquittés

    // Initialisation de la taille de la fenêtre glissante
    if(argc == 2){
        tailleFenetre = atoi(argv[1]);
    } else {
        tailleFenetre = 4;
    }

    if (tailleFenetre >= CAPA_SEQUENCE) {
        perror("Erreur : la taille de la fenêtre ne peut pas être supérieure à la capacité de séquence.");
        return 1;
    }

    init_reseau(EMISSION);
    printf("[TRP] Initialisation réseau : OK.\n");
    printf("[TRP] Début exécution protocole transport.\n");

    // Lecture des données provenant de la couche application
    de_application(message, &taille_msg);

    // Tant que l'émetteur a des données à envoyer ou des paquets non acquittés
    while ((taille_msg != 0) || (borneInf != curseur)) {
        if (dans_fenetre(borneInf, curseur, tailleFenetre) && taille_msg > 0) {

            // Construction du paquet
            memcpy(tabp[curseur].info, message, taille_msg);
            tabp[curseur].num_seq = curseur;
            tabp[curseur].type = DATA;
            tabp[curseur].lg_info = taille_msg;
            tabp[curseur].somme_ctrl = genererControle(tabp[curseur]);

            // Envoi du paquet vers le réseau
            vers_reseau(&tabp[curseur]);
            depart_temporisateur_num(curseur, 100); // Démarrer un temporisateur pour chaque paquet
            printf("PAQUET ENVOYÉ %d \n", curseur);

            // Lire le message suivant
            de_application(message, &taille_msg);
            curseur = incrementer(curseur, CAPA_SEQUENCE); // Avancer modulo capacité de séquence
        } else {
            // Plus de credit, attente obligatoire
            evenement = attendre();
            if (evenement == -1) {
                de_reseau(&reponse); // Récupérer l'ACK

                // Vérifier le checksum de l'ACK et s'assurer qu'il est dans la fenêtre
                if (verifierControle(reponse) && dans_fenetre(borneInf, reponse.num_seq, tailleFenetre)) {
                    acquitte[reponse.num_seq] = 1; // Marquer le paquet comme acquitté
                    printf("ACK %d reçu\n", reponse.num_seq);

                    // Décaler la fenêtre si le premier paquet non acquitté est acquitté
                    while (acquitte[borneInf] == 1) {
                        acquitte[borneInf] = 0; // Réinitialiser l'état du paquet
                        borneInf = incrementer(borneInf, CAPA_SEQUENCE); // Décaler la borne inférieure
                    }

                    // Arrêter le temporisateur si tous les paquets de la fenêtre sont acquittés
                    if (borneInf == curseur) {
                        arret_temporisateur(); // Aucun paquet en attente
                    }
            
                }
            } else {
                // Timeout : retransmettre le paquet qui a expiré
                printf("Timeout pour le paquet %d, retransmission.\n", evenement);
                vers_reseau(&tabp[evenement]);
                depart_temporisateur_num(evenement, 100); // Redémarrer le temporisateur pour ce paquet
            }
        }
    }

    printf("[TRP] Fin exécution protocole transfert de données (TDD).\n");
    return 0;
}