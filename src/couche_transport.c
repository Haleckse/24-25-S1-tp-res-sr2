#include <stdio.h>
#include "couche_transport.h"
#include "services_reseau.h"
#include "application.h"

/* ************************************************************************** */
/* *************** Fonctions utilitaires couche transport ******************* */
/* ************************************************************************** */

// RAJOUTER VOS FONCTIONS DANS CE FICHIER...



/*--------------------------------------*/
/* Fonction d'inclusion dans la fenetre */
/*--------------------------------------*/
int dans_fenetre(unsigned int inf, unsigned int pointeur, int taille) {

    unsigned int sup = (inf+taille-1) % SEQ_NUM_SIZE;

    return
        /* inf <= pointeur <= sup */
        ( inf <= sup && pointeur >= inf && pointeur <= sup ) ||
        /* sup < inf <= pointeur */
        ( sup < inf && pointeur >= inf) ||
        /* pointeur <= sup < inf */
        ( sup < inf && pointeur <= sup);
}

//fonctions maison

uint8_t genererControle(paquet_t paquet){
    uint8_t s = 0;
    s = paquet.type^paquet.num_seq^paquet.lg_info; 
    for (int i = 0; i < paquet.lg_info; i++){
         s = s^paquet.info[i]; 
        }
    return s; 
}

int verifierControle(paquet_t paquet){
    return paquet.somme_ctrl == genererControle(paquet); 
}

int incrementer(int n, int m){
    return (n+1)%m; 
}

