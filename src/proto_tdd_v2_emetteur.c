/*************************************************************
* proto_tdd_v2 -  émetteur                                   *
* TRANSFERT DE DONNEES  v2                                   *
*                                                            *
* E. Lavinal - Univ. de Toulouse III - Paul Sabatier         *
**************************************************************/

// Un contrôle de flux en mode « Stop-and-Wait
// Une détection d'erraeurs basée sur une somme de contrôle

#include <stdlib.h>
#include <stdio.h>
#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"

/* =============================== */
/* Programme principal - émetteur  */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message de l'application */
    int taille_msg; /* taille du message */
    paquet_t paquet, reponse; /* paquet utilisé par le protocole */

   
    init_reseau(EMISSION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /* lecture de donnees provenant de la couche application */
    de_application(message, &taille_msg);

    uint8_t prochainPaquet = 0; 

    /* tant que l'émetteur a des données à envoyer */
    while ( taille_msg != 0 ) {

        /* construction paquet */
        for (int i=0; i<taille_msg; i++) {
            paquet.info[i] = message[i];
        }
        paquet.lg_info = taille_msg;
        paquet.type = DATA;

        paquet.num_seq = prochainPaquet; 
        
        //generer somme controle
        paquet.somme_ctrl = genererControle(paquet); 

        /* remise à la couche reseau */
        vers_reseau(&paquet);


        
        //depart du temporisateur x
        depart_temporisateur(100);

        int evt = attendre(); 

        int iter = 0; 
       // La variable iter est utilisée pour éviter des boucles infinies notamment lors de la perte du dernier ack si le compteur atteint 25, le programme se termine.
        while(evt != -1 ){
                if (iter >= 25) exit(0); 
                vers_reseau(&paquet); 
                arret_temporisateur();                
                depart_temporisateur(100); 
                evt = attendre(); 
                iter++; 
        }
        
        arret_temporisateur(); 
        
        //On recupere l'ack envoye par le recpeteur depuis le reseau
        de_reseau(&reponse); 

        //Le numero de sequence est incrementé de facon circulaire (dans le cas present de 0 a 7)
        prochainPaquet = incrementer(prochainPaquet, 8); 

        /* lecture des donnees suivantes de la couche application */
        de_application(message, &taille_msg);
    }

    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
