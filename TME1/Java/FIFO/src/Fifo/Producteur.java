package Fifo;

public class Producteur implements Runnable {
    private final Tapis tapis;
    private final String nomProduit;
    private final int cible;

    public Producteur(Tapis tapis, String nomProduit, int cible) {
        this.tapis = tapis;
        this.nomProduit = nomProduit;
        this.cible = cible;
    }

    @Override
    public void run() {
        try {
            for (int i = 0; i < cible; i++) {
                String paquet = nomProduit + " " + i;
                tapis.enfiler(paquet);
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }
}

