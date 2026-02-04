package Fifo;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;

class Tapis {
    private final BlockingQueue<String> queue;

    public Tapis(int capacite) {
        this.queue = new ArrayBlockingQueue<>(capacite);
    }

    public void enfiler(String s) throws InterruptedException {
        queue.put(s); 
    }

    public String defiler() throws InterruptedException {
        return queue.take(); 
    }
}