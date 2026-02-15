package main

import (
	"fmt"
	"strings"
	"time"
)

func lecteur(f string, out chan string) {
	lines := strings.Split(f, "\n") //split ressource par ligne
	for _, line := range lines[1:] {
		out <- line
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

func travailleur(in1 chan string, ch_serv chan calcul_cont, ch_red chan int, id int) {
	for ressource := range in1 {
		fmt.Println("travailleur", id, "a recu une ligne de donnee")
		partie := strings.Split(ressource, ",")
		local := make(chan paquet)
		p := paquet{arrive: partie[1], depart: partie[2], arret: 0} //construire paquet
		req := calcul_cont{p: p, c: local}
		ch_serv <- req //envoyer a serveur
		resultat := <-local
		ch_red <- resultat.arret //envoyer a reducteur
	}
}

func serveur(url chan calcul_cont) {
	for {
		contenu := <-url
		go func(cont calcul_cont) {
			adate, _ := time.Parse("", cont.p.arrive) //a completer
			ddate, _ := time.Parse("", cont.p.depart) //a completer
			time.Sleep(10 * time.Millisecond)
			duree := int(ddate.Sub(adate).Seconds())
			new_p := paquet{arrive: cont.p.arrive, depart: cont.p.depart, arret: duree}
			cont.c <- new_p
		}(contenu)
	}
}

func reducteur(in chan int, stop chan bool, moyenne chan float64) {
	sum := 0
	count := 0
	for {
		select {
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
	ch_in := make(chan string)           //lecteur des donnees
	ch_tra_ser := make(chan calcul_cont) //travailleurs a serveur
	ch_tra_red := make(chan int)         //travailleurs a reducteur
	stop := make(chan bool)
	resultat := make(chan float64)

	go serveur(ch_tra_ser)

	go reducteur(ch_tra_red, stop, resultat)

	numtravailleurs := 2 //nombre des travailleurs
	for i := 0; i < numtravailleurs; i++ {
		//i=id des travailleurs
		go travailleur(ch_in, ch_tra_ser, ch_tra_red, i)
	}

	go lecteur("", ch_in)
	time.Sleep(2 * time.Second)

	fmt.Println("************************")
	fmt.Println("Fin")
	fmt.Println("************************")

	stop <- true
	moyenne := <-resultat
	fmt.Printf("Moyen arret: %.2f secondes\n", moyenne)
}
