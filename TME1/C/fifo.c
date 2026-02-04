#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct {
    char *s; 
} paquet;

typedef struct {
    int valeur;
    pthread_mutex_t mutex;
} compteur;

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

paquet defiler(tapis *t){
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
    for(int i = 0; i < t->nombre_paquet - 1;i++){
        t->paquets[i] = t->paquets[i+1];
    }
    t->nombre_paquet--;
    pthread_cond_signal(&t->cond_enfiler);
    pthread_mutex_unlock(&(t->mutc));
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

        paquet pa;
        pa.s = strdup(tmp);// une nouvelle chaîne sur le tas
        

        enfiler(p->tapis, pa);
    }
    return NULL;
}


typedef struct {
    int id;
    tapis *tapis;
    compteur *cpt; 

}consommateur_arg;

void* consommateur(void* arg){
    consommateur_arg *a = (consommateur_arg*) arg;

    while (1) {
        //on utilise pas atomic ici parceque 
        pthread_mutex_lock(&a->cpt->mutex);// on ajout mutex sur la valeur de compteur,donc tous les threads voir 0 a la fin et termine correctement
        if (a->cpt->valeur <= 0) { // on consome tous
            pthread_mutex_unlock(&a->cpt->mutex);
            break;
        }

        a->cpt->valeur--;
        pthread_mutex_unlock(&a->cpt->mutex);

        paquet p = defiler(a->tapis);
    }
    return NULL;
}


int main(void) {
    int capacite_tapis = 100;
    int nb_prod = 16;
    int nb_cons = 16;
    int cible = 100;

    tapis t;
    t.capacite = capacite_tapis;
    t.paquets = malloc(sizeof(paquet) * t.capacite);
    t.nombre_paquet = 0;

    pthread_mutex_init(&t.mutc, NULL);
    pthread_cond_init(&t.cond_enfiler, NULL);
    pthread_cond_init(&t.cond_defiler, NULL);
    compteur cpt;
    cpt.valeur = cible * nb_prod;
    pthread_mutex_init(&cpt.mutex, NULL);

    pthread_t prod_th[nb_prod];
    pthread_t cons_th[nb_cons];

    producteur_arg pargs[nb_prod];
    consommateur_arg cargs[nb_cons];


    for (int i = 0; i < nb_prod; i++) {
        pargs[i].tapis = &t;
        pargs[i].nom_produit = "Pomme";
        pargs[i].cible = cible;
    }

    for (int i = 0; i < nb_cons; i++) {
        cargs[i].id = i + 1;
        cargs[i].tapis = &t;
        cargs[i].cpt = &cpt;
    }
    //on attend tous les threads terminent
    for (int i = 0; i < nb_prod; i++) {
        pthread_join(prod_th[i], NULL);
    }
    // compteur-> valeur == 0, tous les threads return.
    for (int i = 0; i < nb_cons; i++) {
        pthread_join(cons_th[i], NULL);
    }

    pthread_mutex_destroy(&cpt.mutex);
    pthread_mutex_destroy(&t.mutc);
    pthread_cond_destroy(&t.cond_enfiler);
    pthread_cond_destroy(&t.cond_defiler);


    return 0;
}

