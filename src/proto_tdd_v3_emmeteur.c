#include <stdio.h>
#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"
#define CAPA_SEQUENCE 16


/* =============================== */
/* Programme principal - émetteur  */
/* =============================== */
int main(int argc, char* argv[])
{

    if(argc > 2){
        perror("Nombre de parametre incorect"); 
        return 1; 
    }

    
    unsigned char message[MAX_INFO]; /* message de l'application */
    int taille_msg, evenement; /* taille du message */
    paquet_t paquet, reponse; /* paquet utilisé par le protocole */
    int curseur = 0; 
    int borneInf = 0; 
    int tailleFenetre = atoi(argv[1]); 
    paquet_t tabp[CAPA_SEQUENCE]; 


    if(argv[1] != NULL){
        if(argv[1] >= CAPA_SEQUENCE){
        perror("W > N erreur"); 
        }
        else tailleFenetre = 4; 
    }
    

    init_reseau(EMISSION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /* lecture de donnees provenant de la couche application */
    de_application(message, &taille_msg);

    /* tant que l'émetteur a des données à envoyer */
    while ( taille_msg != 0 ) {
        if(dans_fenetre(borneInf, curseur, tailleFenetre)){
            de_application(message, &taille_msg);
            tabp[curseur].info = message; 
            tabp[curseur].type = DATA; 
            tabp[curseur].ctrl = genererControle(tabp[curseur]); 
            vers_reseau(&tabp[curseur]); 
            if(curseur == borneInf){
                depart_temporisateur(100); 
            }
            incrementer(curseur, CAPA_SEQUENCE);
        }
        else{
            //Plus de credit, attente obligatoire
            evenement = attendre(); 
            if(evenement == -1){
                de_reseau(&reponse); 
                if (verifierControle(reponse) && dans_fenetre(borneInf, reponse.num_seq, tailleFenetre)){
                    //On decale la fenetre
                    borneInf = incrementer(reponse.num_seq, CAPA_SEQUENCE); 
                    if(borneInf == curseur){
                        arret_temporisateur(); 
                    }
                }
            }
            else{
                //timeout
                int i = borneInf; 
                depart_temporisateur(100); 
                while(i != curseur){
                    vers_reseau(&tabp[i]); 
                    i = incrementer(i, CAPA_SEQUENCE); 
                }
            }   
            
        }
    }



    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
