/*Definition des status du feu*/
mtype={ROUGE,VERT,ORANGE,PANNE};

/*Defitnition de la canal de feu->observeur*/
chan observer=[0] of {mtype};

/*Definition de la canal pour le cas en panne,type de bit:soit le valeur 1.soit le valeur 0*/
/*On prends 1 si en panne, 0 si normal*/
chan panne=[0] of {bit};
                        
proctype feu (chan obs;chan pne){

/*Commence le cycle*/    
initial:
    observer!ORANGE;
    goto cycle_orange;/*Status initial est en orange*/

cycle_orange:
    if
        ::pne?1->goto cas_panne;/*Si en panne, passe au cas en panne*/
        ::true->obs!ORANGE;goto cycle_rouge;/*Cas normal:passe au cycle rouge*/
    fi;

cycle_rouge:
    if
        ::pne?1->goto cas_panne;
        ::true->obs!ROUGE;goto cycle_vert;
    fi;

cycle_vert:
    if
        ::pne?1->goto cas_panne;
        ::true->obs!VERT;goto cycle_orange;
    fi;


cas_panne:/*Cas si en panne:toujours ORANGE*/
    do
        ::obs!ORANGE;
    od
}