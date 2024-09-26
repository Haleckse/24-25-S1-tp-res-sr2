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

    int tailleFenetre; 

    if(argc > 2){
        perror("Nombre de parametre incorect"); 
        return 1; 
    }

    
    unsigned char message[MAX_INFO]; /* message de l'application */
    int taille_msg, evenement; /* taille du message */
    paquet_t reponse; /* paquet utilisé par le protocole */
    int curseur = 0; 
    int borneInf = 0; 
    paquet_t tabp[CAPA_SEQUENCE];
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
    while ( taille_msg != 0 ) {
        if(dans_fenetre(borneInf, curseur, tailleFenetre)){
            
            memcpy(tabp[curseur].info, message, taille_msg);
            tabp[curseur].num_seq = curseur; 
            tabp[curseur].type = DATA; 
            tabp[curseur].somme_ctrl = genererControle(tabp[curseur]); 
            tabp[curseur].lg_info = taille_msg; 
            vers_reseau(&tabp[curseur]); 
            if(curseur == borneInf){
                arret_temporisateur(); 
                depart_temporisateur(100); 
            }
            incrementer(curseur, 16);
            de_application(message, &taille_msg);
        }
        else{
            //Plus de credit, attente obligatoire
            evenement = attendre(); 
            if(evenement == -1){
                de_reseau(&reponse); 
                if (verifierControle(reponse) && dans_fenetre(borneInf, reponse.num_seq, tailleFenetre)){
                    //On decale la fenetre
                    borneInf = incrementer(reponse.num_seq, 16); 
                    if(borneInf == curseur){
                        arret_temporisateur(); 
                    }
                }
            }
            else{
                //timeout
                int i = borneInf; 
                arret_temporisateur();
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
