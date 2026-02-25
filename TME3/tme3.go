package main

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"strings"
	"time"
)

/*
Cette fonction reçoit le nom d’un fichier, l’ouvre et le lit, puis transmet
son contenu ligne par ligne au travailleur via le chan out
*/
func lecteur(titre string, out chan string) {
	/*
		lines := strings.Split(f, "\n") //split ressource par ligne
		for _, line := range lines[1:] {
			out <- line
		}
	*/
	fichier, err := os.Open(titre)
	if err != nil {
		fmt.Printf("Error opening file: %v\n", err)
		close(out)
		return
	}

	scanner := bufio.NewScanner(fichier)
	continu := true    //definie et affectation
	_ = scanner.Scan() //abandonner la premier ligne

	for continu {
		resultat := scanner.Scan()
		if resultat == false {
			err = scanner.Err()
			if err == nil {
				continu = false
			} else {
				log.Fatal(err)
			}
		} else {
			out <- scanner.Text()
		}
	}
}

type paquet struct {
	arrive string
	depart string
	arret  int
}

type calcul_cont struct {
	p paquet
	c chan paquet
}

/*
Cette fonction encapsule les données dans un paquet,
crée pour chaque paquet un canal privé, l’envoie au serveur,
attend que le serveur termine le traitement
, puis transmet le résultat à channel ch_red pour le reducteur
*/
func travailleur(in1 chan string, ch_serv chan calcul_cont, ch_red chan int, id int) {
	for ressource := range in1 {
		fmt.Println("travailleur", id, "a recu une ligne de donnee")
		partie := strings.Split(ressource, ",")
		local := make(chan paquet)                                  //private channel
		p := paquet{arrive: partie[1], depart: partie[2], arret: 0} //construire paquet
		req := calcul_cont{p: p, c: local}
		ch_serv <- req //envoyer a serveur
		resultat := <-local
		ch_red <- resultat.arret //envoyer a reducteur
	}
}

/*
Calculer les durees.
*/
func serveur(url chan calcul_cont) {
	for {
		contenu := <-url
		go func(cont calcul_cont) {
			adate, _ := time.Parse("15:04:03", cont.p.arrive)
			ddate, _ := time.Parse("15:04:03", cont.p.depart)
			time.Sleep(10 * time.Millisecond)
			duree := int(ddate.Sub(adate).Seconds())
			new_p := paquet{arrive: cont.p.arrive, depart: cont.p.depart, arret: duree}
			cont.c <- new_p
		}(contenu)
	}
}

/*
Avant que main n’envoie le message stop,
cette fonction reçoit en continu des valeurs de durée.
Lorsque le signal stop arrive,
elle cesse de recevoir des messages et commence à calculer la moyenne.
*/
func reducteur(in chan int, stop chan bool, moyenne chan float64) {
	sum := 0
	count := 0
	for {
		select { //Permet d’écouter simultanément deux canaux.
		case val := <-in:
			fmt.Println("Ajoute ", val)
			sum += val
			count++
		case <-stop: //si main signal=true
			if count > 0 {
				moyenne <- float64(sum) / float64(count)
			} else {
				moyenne <- 0
			}
			return
		}
	}
}

func main() {
	//Un canal sans tampon est utilisé pour assurer la synchronisation.
	ch_lines := make(chan string)
	ch_serv := make(chan calcul_cont)
	ch_red := make(chan int)
	ch_stop := make(chan bool)
	ch_moy := make(chan float64)

	go lecteur("stop_times.txt", ch_lines)

	go serveur(ch_serv)

	go reducteur(ch_red, ch_stop, ch_moy)

	nbTravailleurs := 4
	for i := 0; i < nbTravailleurs; i++ {
		go travailleur(ch_lines, ch_serv, ch_red, i)
	}

	time.Sleep(3 * time.Second)

	ch_stop <- true

	moyenne := <-ch_moy

	fmt.Println("Moyenne des durées :", moyenne)
}
