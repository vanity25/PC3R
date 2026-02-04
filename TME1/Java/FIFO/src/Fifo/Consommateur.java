package Fifo;

import java.util.concurrent.atomic.AtomicInteger;

class Consommateur implements Runnable {
    private final int id;
    private final Tapis tapis;
    private final AtomicInteger compteur;

    public Consommateur(int id, Tapis tapis, AtomicInteger compteur) {
        this.id = id;
        this.tapis = tapis;
        this.compteur = compteur;
    }

    @Override
    public void run() {
        try {
            while (true) {
                int restant = compteur.getAndDecrement();
                if (restant <= 0) {
                    break;
                }

                String p = tapis.defiler();
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }
}
