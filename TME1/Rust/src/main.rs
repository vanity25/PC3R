use std::collections::VecDeque;
use std::sync::{Arc, Mutex, Condvar};
use std::thread;


struct Paquet{
    nom:String,
}//les paquets encapsulent une chaine de caractere

struct Tapis{
    file:VecDeque<Paquet>,
    capacite:usize,
}//tapis contient un VecDeque de paquets

struct FinalTapis{
    tapis:Mutex<Tapis>,
    cond_vide:Condvar,
    cond_complet:Condvar,
}//utilise Mutex<> pour realiser la gestion des threads

impl FinalTapis{
    fn new(cap:usize)->Self{
        FinalTapis {
            tapis:Mutex::new(Tapis{
                file:VecDeque::new(),
                capacite:cap,
            }),
            cond_vide:Condvar::new(),
            cond_complet:Condvar::new(),
        }
    }

    fn enfiler(&self, p:Paquet){
        let mut verrou=self.tapis.lock().unwrap();
        while verrou.file.len()==verrou.capacite{
            verrou=self.cond_vide.wait(verrou).unwrap();
        }
        verrou.file.push_back(p);//enfiler a la fin de file
        self.cond_vide.notify_one();
    }

    fn defiler(&self)->Paquet{
        let mut verrou=self.tapis.lock().unwrap();
        while verrou.file.len()==0{
            verrou=self.cond_complet.wait(verrou).unwrap();
        }
        let p=verrou.file.pop_front().unwrap();//return le premier element de file
        self.cond_complet.notify_one();
        return p;
    }
}

fn producteur(name:String, nombre:usize, tapis:Arc<FinalTapis>){
    for i in 0..nombre{
        let p=Paquet{
            nom:format!("{}{}",name,i),
        };
        tapis.enfiler(p);
    }
}

fn consommateur(compteur:Arc<Mutex<u32>>,tapis:Arc<FinalTapis>){
    while true {
        let mut count = compteur.lock().unwrap();
        if *count<=0{break;}
        *count-=1;//verifier une operation a la meme temp
        let p=tapis.defiler();
    }
}

fn main(){
    let n=3;//nombre des producteurs
    let m=2;//nombre des consommateurs
    let cible=6;//cible de production pour chaque producteur
    let capacite=10;
    let produits=vec![
        "cigarette", "bierre", "banana","porc"
    ];
    let tapis=Arc::new(FinalTapis::new(capacite));
    let total_production=n*cible;
    let compteur=Arc::new(Mutex::new(total_production as u32));
    let mut handles=vec![];

    for i in 0..n {
        let t=Arc::clone(&tapis);
        let nom=produits[i].to_string();
        handles.push(thread::spawn(move||{
            producteur(nom,cible,t);
        }));
    }//producteur

    for i in 0..m {
        let t=Arc::clone(&tapis);
        let count=Arc::clone(&compteur);
        handles.push(thread::spawn(move||{
            consommateur(count,t);
        }));
    }//consommateur

    for h in handles {
        h.join().unwrap();
    }//wait pour chaque threads
}