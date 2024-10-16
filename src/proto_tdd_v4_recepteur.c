/*************************************************************
* proto_tdd_v0 -  récepteur                                  *
* TRANSFERT DE DONNEES  v2                                   *
*                                                            *
* Protocole sans contrôle de flux, sans reprise sur erreurs  *
*                                                            *
* E. Lavinal - Univ. de Toulouse III - Paul Sabatier         *
**************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"
#define CAPA_SEQUENCE 16

/* =============================== */
/* Programme principal - récepteur */
/* =============================== */
int main(int argc, char* argv[])
{

    int tailleFenetre; 
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
    
    paquet_t paquet, reponse; /* paquet utilisé par le protocole */
    int fin = 0; /* condition d'arrêt */

    int borneInf = 0; // Borne inférieure de la fenêtre glissante (premier paquet non acquitté)
    int acquitte[CAPA_SEQUENCE] = {0}; 
    paquet_t buffer[CAPA_SEQUENCE]; 
    unsigned char mess[MAX_INFO]; /* message pour l'application */



    init_reseau(RECEPTION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");


    /* tant que le récepteur reçoit des données */
    while ( !fin ) {
        
        // attendre(); /* optionnel ici car de_reseau() fct bloquante */
        de_reseau(&paquet);
        
        
        if (verifierControle(paquet)){ //verif checksum 
            printf("VERIF CTRL OK\n");
            if(dans_fenetre(borneInf, paquet.num_seq, tailleFenetre)){ //verif que le paquet est dans la fenetre
                printf("[TCP] pack recu sans erreurs.\n");
                //Constructuon de l'ack
                reponse.num_seq = paquet.num_seq; 
                reponse.type = ACK; 
                reponse.lg_info = 0; 
                reponse.somme_ctrl = genererControle(reponse); 
                 /* extraction des donnees du paquet recu */
                
                //Si le paquet recu est hors sequence, on le bufferise et on l'ajoute a la liste des prochain paquet a acquiter
                if( !(paquet.num_seq == borneInf) ){ 
                    printf("Paquet %d recu hors sequence, bufferisé\n", paquet.num_seq);
                    buffer[paquet.num_seq] = paquet; 
                    acquitte[paquet.num_seq] = 1;
                }
                
                //Si le paquet recu est en sequence, on le transmet a l'app ainsi que le contenu du buffer.
                else {
                    buffer[borneInf] = paquet; 
                    do{
                    for (int i=0; i<buffer[borneInf].lg_info; i++) { 
                            mess[i] = buffer[borneInf].info[i];
                        }   
                    fin = vers_application(mess, buffer[borneInf].lg_info);
                    acquitte[borneInf] = 0; // Libérer le tampon

                    borneInf = incrementer(borneInf, CAPA_SEQUENCE); // Avancer la borne inférieure
                   
                    } while( acquitte[borneInf] != 0 ); 
                   
                }

            }
            reponse.num_seq = paquet.num_seq; 
            reponse.type = ACK; 
            reponse.somme_ctrl = genererControle(reponse); 
            vers_reseau(&reponse); 
        }
        
    }

    //Si le dernier ack se perd, on attend une retransmission du dernier paquet de part du recepteur, et on l'acquitte
    
    depart_temporisateur(1000); 
    int evt = attendre(); 
    if (evt == -1){
        vers_reseau(&reponse); 
    }

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}
