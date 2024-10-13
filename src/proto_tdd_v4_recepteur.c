/*************************************************************
* proto_tdd_v0 -  récepteur                                  *
* TRANSFERT DE DONNEES  v2                                   *
*                                                            *
* Protocole sans contrôle de flux, sans reprise sur erreurs  *
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
    
    paquet_t paquet, reponse; /* paquet utilisé par le protocole */
    int fin = 0; /* condition d'arrêt */

    int curseur = 0; // Pointeur pour le prochain numéro de séquence à envoyer
    int borneInf = 0; // Borne inférieure de la fenêtre glissante (premier paquet non acquitté)
    int taille_fenetre = 4; 
    int paquets_bufferise[CAPA_SEQUENCE] = {0};
    paquet_t buffer[CAPA_SEQUENCE]; 


    init_reseau(RECEPTION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");


    /* tant que le récepteur reçoit des données */
    while ( !fin ) {
        
        // attendre(); /* optionnel ici car de_reseau() fct bloquante */
        de_reseau(&paquet);
        
        
        if (verifierControle(paquet)){
            printf("VERIF CTRL OK\n");
            if(dans_fenetre(borneInf, paquet.num_seq, taille_fenetre)){
                printf("[TCP] pack recu sans erreurs.\n");
                //Constructuon de l'ack
                reponse.num_seq = paquet.num_seq; 
                reponse.type = ACK; 
                reponse.lg_info = 0; 
                reponse.somme_ctrl = genererControle(reponse); 
                 /* extraction des donnees du paquet recu */
                
                //Si le paquet recu est hors sequence, on le bufferise
                if( !(paquet.num_seq == borneInf) ){
                    printf("Paquet %d recu hors sequence, bufferisé\n", paquet.num_seq);
                    buffer[paquet.num_seq] = paquet; 
                    paquets_bufferise[paquet.num_seq] = 1;
                }
                
                //Si le paquet recu est en sequence, on le transmet a l'app ainsi que le contenu du buffer.
                else {
                    unsigned char mess[MAX_INFO]; /* message pour l'application */
                    for (int i=0; i<paquet.lg_info; i++) {
                            mess[i] = paquet.info[i];
                        }   
                    fin = vers_application(mess, paquet.lg_info);
                    paquets_bufferise[borneInf] = 0; // Libérer le tampon

                    borneInf = incrementer(borneInf, CAPA_SEQUENCE); // Avancer la borne inférieure
                

                    //On transmet a l'app le contenu du buffer
                    while (paquets_bufferise[borneInf]){
                        
                        paquet_t p = buffer[borneInf];

                        unsigned char message[MAX_INFO]; /* message pour l'application */
                        for (int i=0; i<p.lg_info; i++) {
                            message[i] = p.info[i];
                        }   
                        fin = vers_application(message, p.lg_info);
                        paquets_bufferise[borneInf] = 0; // Libérer le tampon
                        borneInf = incrementer(borneInf, CAPA_SEQUENCE); // Avancer la borne inférieure
                    }
                }

                // else{
                //     buffer[paquet.num_seq] = paquet; 
                // }
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
