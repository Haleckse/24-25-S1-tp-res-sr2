#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"

#define CAPA_SEQUENCE 16
#define paquetRecu 0
#define maxRetransmit 25

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
    
    int compt = 0; 
    int curseur = 0; // Prochain numéro de séquence à envoyer
    int borneInf = 0; // Borne inférieure de la fenêtre glissante (premier paquet non acquitté)
    int acquitte[CAPA_SEQUENCE] = {0}; // Tableau pour garder trace des paquets acquittés
    int retransmis[CAPA_SEQUENCE] = {0}; 
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
    while ( (retransmis[borneInf] < maxRetransmit) && ( (taille_msg != 0) || (borneInf != curseur)) ) {  
        if (dans_fenetre(borneInf, curseur, tailleFenetre) && taille_msg > 0) {
            if (compt > 500) break; 
            compt++; 

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

            curseur = incrementer(curseur, CAPA_SEQUENCE); // Avancer modulo capacité de séquence

            // Lire le message suivant
            de_application(message, &taille_msg);

        } 
        else {
            // Plus de crédit, attente obligatoire
            evenement = attendre();
            if (evenement == -1) {
                de_reseau(&reponse); // Récupérer l'ACK

                // Vérifier le checksum de l'ACK et s'assurer qu'il est dans la fenêtre
                if (verifierControle(reponse) && dans_fenetre(borneInf, reponse.num_seq, tailleFenetre)) {
                    retransmis[reponse.num_seq] = 0; // reset des nombre de retranslission (maxRetransmission)
                    arret_temporisateur_num(reponse.num_seq); 
                    printf("ACK %d reçu\n", reponse.num_seq);
                    if(reponse.num_seq == borneInf){ //on peut decaler la fenetre
                    do {
                        acquitte[borneInf] = 0; 
                        borneInf = incrementer(borneInf, CAPA_SEQUENCE); 
                    } while (acquitte[borneInf] == 1); 

                }

                else {
                    acquitte[reponse.num_seq] = 1; 
                }
                
            }  
            
            } 
            else {
                depart_temporisateur_num(evenement, 100); 
                vers_reseau(&tabp[evenement]);  // retransmission du paquet dont le timeout a expire
                retransmis[evenement]++;  // on increment le nombre de retransmission du paquet en question
            }
        }  
    }

    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
