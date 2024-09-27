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
        perror("Nombre de parametre incorect"); 
        return 1; 
    }

    
    unsigned char message[MAX_INFO]; /* message de l'application */
    int taille_msg, evenement; /* taille du message */
    paquet_t reponse; /* paquet utilisé par le protocole */

    int curseur = 0; // Pointeur pour le prochain numéro de séquence à envoyer
    int borneInf = 0; // Borne inférieure de la fenêtre glissante (premier paquet non acquitté)
    paquet_t tabp[CAPA_SEQUENCE]; // Tableau de paquets envoyés mais non encore acquittés

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
    while ( (taille_msg != 0) || (curseur != borneInf)) {
        if( (dans_fenetre(borneInf, curseur, tailleFenetre)) && (taille_msg>0) ){
            
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
            if(curseur == borneInf){
                depart_temporisateur(100); 
            }
            // Incrémenter le curseur (modulo capacite de sequence)
            curseur = incrementer(curseur, 16);
            de_application(message, &taille_msg);

            
        }
        else{
            //Plus de credit, attente obligatoire
            evenement = attendre(); 
            if(evenement == -1){
                de_reseau(&reponse); // Récupération du paquet ACK depuis le réseau
                printf("ACK %d recu avant verif\n", reponse.num_seq);

                //verif checksum ACK et on s'assure que celui ci est dans la fenetre
                if (verifierControle(reponse)  && dans_fenetre(borneInf, reponse.num_seq, tailleFenetre)){
                    //On decale la fenetre
                    printf("ACK %d recu \n", reponse.num_seq); 
                    borneInf = incrementer(reponse.num_seq, 16); //on decale la borne inf de la fenetre
                    if(borneInf == curseur){
                        arret_temporisateur(); 
                    }
                }
            }
            else{
                //timeout : on restransler tous les paquets de la fenetre
                int i = borneInf; 
                depart_temporisateur(100); 
                while(i != curseur){
                    vers_reseau(&tabp[i]); 
                    i = incrementer(i, 16); 
                }
            }   
            
        }
    }
    
    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
