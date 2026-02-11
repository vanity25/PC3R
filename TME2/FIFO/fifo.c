#include <fthread.h>
#include <stdio.h>
#include <stdlib.h>   
#include <string.h> 
#include <unistd.h>  



typedef struct paquet {
    char *contenu;
} paquet;

typedef struct cellule {
    paquet *p;
    struct cellule *suiv;
} cellule;

typedef struct tapis {
    paquet **buffer;          //boucle file
    int capacite;             // taille max

    int tete;                 
    int queue;                
    int taille;               
    ft_event_t evt_non_vide;  // events
    ft_event_t evt_non_plein;  

    ft_scheduler_t sched;     // ordonnanceur 
} tapis;

tapis *tapis_create(int capacite, ft_scheduler_t sched) {
    tapis *t = malloc(sizeof(tapis));

    t->buffer = malloc(sizeof(paquet*) * capacite);
    t->capacite = capacite;

    t->tete = 0;
    t->queue = 0;
    t->taille = 0;

    t->sched = sched;

    t->evt_non_vide  = ft_event_create(sched);
    t->evt_non_plein = ft_event_create(sched);

    return t;
}

int tapis_est_vide(tapis *t) {
    return t->taille == 0;
}

int tapis_est_plein(tapis *t) {
    return t->taille == t->capacite;
}

void tapis_enfiler(tapis *t, paquet *p) {

    while (tapis_est_plein(t)) {
        ft_thread_await(t->evt_non_plein);
    }
    //inserer dans le tapis
    t->buffer[t->queue] = p;
    t->queue = (t->queue + 1) % t->capacite;
    t->taille++;

    ft_thread_generate(t->evt_non_vide);
}

paquet* tapis_defiler(tapis *t){
    while(tapis_est_vide(t)){
        ft_thread_await(t->evt_non_vide);
    }
    paquet *p = t->buffer[t->tete];
    t->buffer[t->tete] = NULL;                 
    t->tete = (t->tete + 1) % t->capacite;
    t->taille--;
    ft_thread_generate(t->evt_non_plein);

    return p;

}

/*
partie producer

*/

typedef struct args_producteur {
    char *nom_produit;      
    int cible;              // nombre total à produire
    tapis *tapis_prod;      // tapis attache a un ordonnanceur
    FILE *f_prod;
} args_producteur;


void producteur(void * args){
    args_producteur* args_p = (args_producteur*) args;
    int cible = args_p->cible;
    tapis *tapis_prod = args_p->tapis_prod;

    for(int i = 0; i<cible;i++){
        paquet *p = malloc(sizeof(paquet));
        p->contenu = malloc(100);
        sprintf(p->contenu,"%s %d",args_p->nom_produit, i);
        
        tapis_enfiler(tapis_prod, p);//les paquets sont attache a meme tapis donc meme ordonnanceur
        
        //ecrire dans le fichier du journal producteur
        fprintf(args_p->f_prod,"%s\n", p->contenu);

        fflush(args_p->f_prod);


        ft_thread_cooperate( );
    }
}

/*
partie consommateur
*/
typedef struct {
    int valeur;   
} compteur;

typedef struct {
    char *nom_consommateur;
    int cible;               
    tapis *tapis_cons;       
    FILE *f_cons;
    compteur *cpt;            
} args_consommateur;

void consommateur(void *args){
    args_consommateur *args_c = (args_consommateur*) args;
    tapis *tapis_cons = args_c->tapis_cons;

    while(1){
        if (args_c->cpt->valeur <= 0){break;}
        paquet *p = tapis_defiler(tapis_cons);
        int i=0;
        fprintf(args_c->f_cons,"%s+%s+%d\n",args_c->nom_consommateur,p->contenu,i);
        fflush(args_c->f_cons);
        args_c->cpt->valeur--;
        free(p->contenu);
        free(p);
        ft_thread_cooperate();
    }
}

/*
partie messagers
*/




typedef struct {
    FILE *f_message;
    ft_scheduler_t sched_prod;
    ft_scheduler_t sched_cons;
    tapis *tapis_prod;
    tapis *tapis_cons;
    compteur *cpt;   
} args_messager;

void messager(void *args)
{
    args_messager *m = (args_messager*) args;
    paquet *p = NULL;

    while (1) {
        //ajouter dans la pthread un partie de instant, ensuite apres un instant il 
        //execute messager
        // ft_thread_link(m->sched_prod);
        // compteur valeur doit etre atomic??? on regard valeur dans la pthread prod mais il est
        //modifie dans pthread con(data race?)
        if ( m->cpt->valeur <= 0) {
        ft_thread_unlink();
        break;
        }// 

        p = tapis_defiler(m->tapis_prod);
        ft_thread_unlink();

        fprintf(m->f_message, "transfer start %s\n", p->contenu);
        fflush(m->f_message);

        ft_thread_link(m->sched_cons);
        tapis_enfiler(m->tapis_cons, p);
        ft_thread_unlink();

        fprintf(m->f_message, "transfer end %s\n", p->contenu);
        fflush(m->f_message);
        p = NULL;

        ft_thread_link(m->sched_prod);
    }
}




// quand on execute le main, il bloque. Seulement il a ecrit 5 lignes dans la journal_producteurs.
//on suppose que quand notre tapis est plein, il bloque, parceque 5 est la taille max de tapis.
int main(void) {

    ft_scheduler_t sched_prod = ft_scheduler_create();
    ft_scheduler_t sched_cons = ft_scheduler_create();
    //creation des deux ordonnanceurs

    tapis *t1 = tapis_create(5, sched_prod);
    tapis *t2 = tapis_create(5, sched_cons);
    //creation des deux tapis de capacite 5

    int nb_prod=2;//2 producteurs
    int nb_paquets=8;//8 paquets pour chaque producteur
    compteur *c=malloc(sizeof(compteur));
    c->valeur=nb_prod*nb_paquets;//initialisation de compteur
    

    FILE *f_prod = fopen("journal_producteurs.txt", "w");
    FILE *f_cons = fopen("journal_consommateurs.txt", "w");
    FILE *f_msg  = fopen("journal_messagers.txt", "w");

    //creation des producteurs
    char* nom_prod[]={"orange","pomme"};
    for(int i=0;i<nb_prod;i++){
        args_producteur* pros=malloc(sizeof(args_producteur));
        pros->nom_produit=nom_prod[i];
        pros->cible=nb_paquets;
        pros->tapis_prod=t1;
        pros->f_prod=f_prod;
        ft_thread_create(sched_prod, producteur, NULL, pros);
    }

    //creation des consommateurs
    char* nom_consom[]={"A","B"};
    for (int i=0;i<nb_prod;i++){
        args_consommateur* cons=malloc(sizeof(args_consommateur));
        cons->nom_consommateur=nom_consom[i];
        cons->tapis_cons=t2;
        cons->cible=(nb_prod*nb_paquets)%2==0?nb_prod*nb_paquets/2:(nb_prod*nb_paquets+1)/2;
        cons->f_cons=f_cons;
        cons->cpt=c;
        ft_thread_create(sched_cons, consommateur, NULL, cons);
    }

    //creation des messagers
    for (int i=0;i<nb_prod;i++){
        args_messager* mes=malloc(sizeof(args_messager));
        mes->sched_cons=sched_cons;
        mes->sched_prod=sched_prod;
        mes->tapis_prod=t1;
        mes->tapis_cons=t2;
        mes->f_message=f_msg;
        mes->cpt=c;
        ft_thread_create(sched_prod, messager, NULL, mes);
    }

    ft_scheduler_start(sched_prod);
    ft_scheduler_start(sched_cons);

    ft_exit();
    return 0;
}


