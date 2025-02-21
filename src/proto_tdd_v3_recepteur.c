/*************************************************************
* proto_tdd_v3 -  récepteur                                  *
* TRANSFERT DE DONNEES  v3                                   *
*                                                            *
* protocole go back n                                        *
*                                                            *
* E. Lavinal - Univ. de Toulouse III - Paul Sabatier         *
**************************************************************/

#include <stdio.h>
#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"
#define CAPA_SEQUENCE 16

/* =============================== */
/* Programme principal - récepteur */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message pour l'application */
    paquet_t paquet, reponse; /* paquet utilisé par le protocole */
    int fin = 0; /* condition d'arrêt */

    

    init_reseau(RECEPTION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    uint8_t paquetAttendu = 0; 

    /* tant que le récepteur reçoit des données */
    while ( !fin ) {
        
        // attendre(); /* optionnel ici car de_reseau() fct bloquante */
        de_reseau(&paquet);
        
        
        if (verifierControle(paquet)){
            printf("VERIF CTRL OK\n");
            if(paquet.num_seq == paquetAttendu){
                printf("paquet attendu : %d paquet recu : %d\n", paquetAttendu, paquet.num_seq); 
                printf("[TCP] pack recu sans erreurs.\n");
                //Constructuon de l'ack
                reponse.num_seq = paquetAttendu; 
                reponse.type = ACK; 
                reponse.lg_info = 0; 
                reponse.somme_ctrl = genererControle(reponse); 
                 /* extraction des donnees du paquet recu */
                for (int i=0; i<paquet.lg_info; i++) {
                    message[i] = paquet.info[i];
                }

                 /* remise des données à la couche application */
                fin = vers_application(message, paquet.lg_info);
                
                //On incremente le paquet attendu suivant
                paquetAttendu = incrementer(paquetAttendu, 16); 
            }
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
