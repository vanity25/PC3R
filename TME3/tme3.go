package main

import (
	"fmt"
	"strings"
	"time"
)

func leteur(f string, out chan string) {
	lines := strings.Split(f, "\n") //split ressource par ligne
	for _, line := range lines[1:] {
		out <- line
	}
}

type paquet struct {
	arrive string
	depart string
	arret  int
	reply  chan paquet
}

func calcul(url chan calcul_cont) {
	for {
		contenu := url
		go func(cont calcul_cont) {

			adate, _ := time.Parse("", cont.p.arrive)
			ddate, _ := time.Parse("", cont.p.depart)
			time.Sleep(10 * time.Millisecond)
			duree := int(ddate.Sub(adate.Second()))
			new_p := paquet{arrive: cont.p.arrive, depart: cont.p.depart, arret: duree, reply: cont.p.reply}
			cont.c <- new_p
		}(contenu)
	}
}

func travailleur(in1 chan string, out1 chan paquet, out2 chan paquet, id int) {
	for {
		ressource := <-in1
		fmt.Println("travailleur", id, "a recu une ligne de donnee")
		partie := strings.Split(ressource, ",")
		local := make(chan paquet)
		p := paquet{arrive: partie[1], depart: partie[1], arret: 0, reply: local}
		out1 <- p
		resultat := <-local
		out2 <- resultat
	}
}

func serveur(in chan paquet, out chan int) {
	ressource := <-in

}

func reducteur(in chan paquet, out chan int, moyenne chan float32) {

}

func main()
