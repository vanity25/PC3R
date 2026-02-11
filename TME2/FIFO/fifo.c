#include <fthread.h>
#include <stdio.h>
#include <stdlib.h>   
#include <string.h>   



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
    char *nom_produit = args_p->nom_produit;
    int cible = args_p->cible;
    tapis *tapis_prod = args_p->tapis_prod;

    for(int i = 0; i<cible;i++){
        paquet *p = malloc(sizeof(paquet));
        p->contenu = malloc(strlen(nom_produit) +1);

        
        tapis_enfiler(tapis_prod, p);//les paquets sont attache a meme tapis donc meme ordonnanceur
        
        //ecrire dans le fichier du journal producteur
        fprintf(args_p->f_prod,"%s + %d\n",args_p->nom_produit,i);

        fflush(args_p->f_prod);


        ft_thread_cooperate( );
    }
}

/*
partie consommateur
*/

typedef struct {
    char *nom_consommateur;
    int cible;               
    tapis *tapis_cons;       
    FILE *f_cons;            
} args_consommateur;

void consommateur(void *args){
    args_consommateur *args_c = (args_consommateur*) args;
    tapis *tapis_cons = args_c->tapis_cons;
    int cible = args_c->cible;

    for(int i = 0; i < cible; i++){
        paquet *p = tapis_defiler(tapis_cons);
        fprintf(args_c->f_cons,"%s + %d\n",args_c->nom_consommateur,i);
        fflush(args_c->f_cons);
        ft_thread_cooperate();
    }
}

/*
partie messagers
*/

typedef struct {
    int valeur;   
} compteur;



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

    while (m->cpt->valeur > 0) {
        ft_thread_link(m->sched_prod);
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

        ft_thread_cooperate();
    }
}





int main(void) {

    /* 1️⃣ 创建调度器 */
    ft_scheduler_t sched = ft_scheduler_create();

    /* 2️⃣ 创建共享的 tapis */
    tapis *t = tapis_create(5, sched);  // 容量 5

    /* 3️⃣ 打开日志文件 */
    FILE *f = fopen("journal_producteur.txt", "w");
    if (!f) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    /* 4️⃣ 创建 producteur 参数 */
    args_producteur *args = malloc(sizeof(args_producteur));
    args->nom_produit = "X";
    args->cible = 10;         // 生产 10 个
    args->tapis_prod = t;
    args->f_prod = f;

    /* 5️⃣ 创建线程 */
    ft_thread_t th_prod =
        ft_thread_create(sched, producteur, NULL, args);

    /* 6️⃣ 启动调度器 */
    ft_scheduler_start(sched);

    /* 7️⃣ 等待线程结束 */
    ft_thread_join(th_prod);

    /* 8️⃣ 清理 */
    fclose(f);
ft_scheduler_stop((ft_thread_t) sched);


    return 0;
}


