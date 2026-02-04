package Fifo;
import java.util.concurrent.atomic.AtomicInteger;

public class Main {
	public static void main(String[] args) throws InterruptedException {

        int capaciteTapis = 100;
        int nbProd = 16;
        int nbCons = 16;
        int cible = 100;


        Tapis tapis = new Tapis(capaciteTapis);

        AtomicInteger compteur = new AtomicInteger(nbProd * cible);
        Thread[] producteurs = new Thread[nbProd];
        Thread[] consommateurs = new Thread[nbCons];

        for (int i = 0; i < nbProd; i++) {
            producteurs[i] = new Thread(new Producteur(tapis, "Pomme", cible));
            producteurs[i].start();
        }
        for (int i = 0; i < nbCons; i++) {
            consommateurs[i] = new Thread(new Consommateur(i + 1, tapis, compteur));
            consommateurs[i].start();
        }

        for (Thread t : producteurs) {
            t.join();
        }
        for (Thread t : consommateurs) {
            t.join();
        }
    }
}
