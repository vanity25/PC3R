#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct {
    char *s; 
} paquet;

typedef struct {
    int capacite;
    paquet* paquets;
    int nombre_paquet;
    
    pthread_mutex_t mutc;//un pour ecrire un pour lire better?
    pthread_cond_t cond_enfiler;
    pthread_cond_t cond_defiler;
} tapis;

//on renvoie un bool instead?
void enfiler(tapis *t, paquet p){
    pthread_mutex_lock(&(t->mutc));
    while(t->nombre_paquet == t->capacite){
        pthread_cond_wait(&t->cond_enfiler,&t->mutc);
    }
    t->paquets[t->nombre_paquet] = p;
    t->nombre_paquet++;
    pthread_cond_signal(&t->cond_defiler);
    pthread_mutex_unlock(&(t->mutc));
}

paquet defiler(tapis *t, paquet p){
    pthread_mutex_lock(&(t->mutc));
    while(t->nombre_paquet == 0){
        //on attend un signal de enfiler pour avoir un
        //nouvelle truc
        pthread_cond_wait(&t->cond_defiler,&t->mutc);
    }
    paquet tmp = t->paquets[0];
    //on decaler tous les restes
    //probleme: je ne pourrais pas modifier les 
    //parties enfiler et defiler en meme temps parceque
    //quand je modifier enfiler, il touch le tail de tapis
    //aussi.
    for(int i = 0; i < t->nombre_paquet;i++){
        t->paquets[i] = t->paquets[i+1];
    }
    t->nombre_paquet--;
    pthread_cond_signal(&t->cond_enfiler);
    pthread_mutex_lock(&(t->mutc));
    return tmp;
}

typedef struct {
    tapis *tapis;
    char *nom_produit; 
    int cible;               
} producteur_arg;

void* producteur(void* arg){
    producteur_arg *p = (producteur_arg*) arg;

    for(int i = 0; i<p->cible;i++){
        char tmp[100];
        snprintf(tmp,sizeof(tmp), "%s %d" ,p->nom_produit, i);

        paquet p;
        p.s = tmp;//?
        


    }
}

