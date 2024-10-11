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
        perror("Nombre de parametres incorrect");
        return 1;
    }

    unsigned char message[MAX_INFO]; /* message de l'application */
    int taille_msg, evenement; /* taille du message */
    paquet_t reponse; /* paquet utilisé par le protocole */

    int curseur = 0; 
    int borneInf = 0; 
    paquet_t tabp[CAPA_SEQUENCE]; 

    int compteur_ack[CAPA_SEQUENCE] = {0}; // Tableau pour compter les acks
    int fast_retransmit_done[CAPA_SEQUENCE] = {0}; // nbr fast retransmit deja effectue

    // Initialisation de la taille de la fenêtre glissante à partir de l'argument, sinon par défaut à 4
    if(argc == 2){
        tailleFenetre = atoi(argv[1]);
    }
    else{
        tailleFenetre = 4;
    }

    if (tailleFenetre >= CAPA_SEQUENCE) {
        perror("Erreur : la taille de la fenêtre ne peut pas être supérieure à la capacité de séquence.");
        return 1;
    }

    init_reseau(EMISSION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /* lecture de donnees provenant de la couche application */
    de_application(message, &taille_msg);

    /* tant que l'émetteur a des données à envoyer */
    while ((taille_msg != 0) || (curseur != borneInf)) {
        if ((dans_fenetre(borneInf, curseur, tailleFenetre)) && (taille_msg > 0)) {

            // Construction du paquet à partir des données de l'application
            memcpy(tabp[curseur].info, message, taille_msg);
            tabp[curseur].num_seq = curseur;
            tabp[curseur].type = DATA;
            tabp[curseur].lg_info = taille_msg;
            tabp[curseur].somme_ctrl = genererControle(tabp[curseur]);

            // Envoi du paquet vers le réseau
            vers_reseau(&tabp[curseur]);
            printf("PAQUET ENVOYE %d \n", curseur);

            // Si c'est le premier paquet de la fenêtre, démarrer le temporisateur
            if (curseur == borneInf) {
                depart_temporisateur(100);
            }
            // Incrémenter le curseur (modulo capacité de séquence)
            curseur = incrementer(curseur, 16);

            de_application(message, &taille_msg);
        }
        else {
            // Plus de crédit, attente obligatoire
            evenement = attendre();
            if (evenement == -1) {
                de_reseau(&reponse); // Recuperation de l'ack depuis le réseau

                //verif checksum ACK et on s'assure que celui ci est dans la fenetre
                if (verifierControle(reponse)) {
                    // Détection des ACKs dupliqués : le paquet acquitté est celui avant la borne inférieure
                    if (reponse.num_seq == (borneInf - 1 + CAPA_SEQUENCE) % CAPA_SEQUENCE) {
                        compteur_ack[reponse.num_seq]++;

                        // Si 3 ACKs duplicatas sont reçus, Fast Retransmit
                        if (compteur_ack[reponse.num_seq] == 3 && fast_retransmit_done[reponse.num_seq] == 0) {
                            printf("Fast Retransmit pour le paquet %d\n", reponse.num_seq);
                            vers_reseau(&tabp[reponse.num_seq]); // fast restransmit
                            fast_retransmit_done[reponse.num_seq] = 1; //fast restransmit effectue 
                            depart_temporisateur(100); 
                        }
                    } else if (dans_fenetre(borneInf, reponse.num_seq, tailleFenetre)) {
                        // Nouveau ACK reçu decaler la fenetre
                        printf("ACK %d recu \n", reponse.num_seq);
                        borneInf = incrementer(reponse.num_seq, 16); //decalage de la borne inf
                        if (borneInf == curseur) {
                            arret_temporisateur();
                        }
                        // reinitialiser les dettes du compteur car acquite
                        compteur_ack[reponse.num_seq] = 0;
                        fast_retransmit_done[reponse.num_seq] = 0; 
                    }
                }
            }
            else {
                // Timeout : on retransmet tous les paquets de la fenêtre
                int i = borneInf;
                depart_temporisateur(100);
                while (i != curseur) {
                    vers_reseau(&tabp[i]);
                    i = incrementer(i, 16);
                }
            }
        }
    }

    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
