#include "../include/thread.h"
#include "vga.h"

// ---------------------------------------------------------------------------
// Race Condition Demo
// ---------------------------------------------------------------------------
static volatile int myglobal = 0;
static mutex_t mymutex;

static void race_thread_unsafe(void *arg) {
    (void)arg;
    for (int i = 0; i < 100000; i++) {
        int temp = myglobal;
        // Introduce artificial delay to force context switches
        for (volatile int j = 0; j < 10; j++); 
        myglobal = temp + 1;
    }
}

static void race_thread_safe(void *arg) {
    (void)arg;
    for (int i = 0; i < 100000; i++) {
        mutex_lock(&mymutex);
        int temp = myglobal;
        for (volatile int j = 0; j < 10; j++);
        myglobal = temp + 1;
        mutex_unlock(&mymutex);
    }
}

void demo_race_unsafe(void) {
    myglobal = 0;
    vga_puts("Starting unsafe race demo with 2 threads (target: 200000)...\n");
    thread_create(race_thread_unsafe, 0);
    thread_create(race_thread_unsafe, 0);
    // Note: We'd normally wait for them to finish, but for this simple OS we can just let them run.
    // However, we want to see the result. So maybe just run them and print intermediate results.
}

void demo_race_safe(void) {
    myglobal = 0;
    mutex_init(&mymutex);
    vga_puts("Starting safe race demo with 2 threads (target: 200000)...\n");
    thread_create(race_thread_safe, 0);
    thread_create(race_thread_safe, 0);
}

// ---------------------------------------------------------------------------
// Producer-Consumer Demo
// ---------------------------------------------------------------------------
#define BUFFER_SIZE 5
static int buffer[BUFFER_SIZE];
static int in = 0, out = 0;
static semaphore_t empty, full;
static mutex_t pc_mutex;

static void producer(void *arg) {
    (void)arg;
    for (int i = 1; i <= 10; i++) {
        sem_wait(&empty);
        mutex_lock(&pc_mutex);
        
        buffer[in] = i;
        vga_puts_color("Produced: ", VGA_LIGHT_GREEN, VGA_BLACK);
        // Quick poor-man's print int
        char buf[4];
        buf[0] = (i / 10) ? '0' + (i / 10) : ' ';
        buf[1] = '0' + (i % 10);
        buf[2] = '\n'; buf[3] = 0;
        vga_puts(buf);
        in = (in + 1) % BUFFER_SIZE;
        
        mutex_unlock(&pc_mutex);
        sem_signal(&full);
        
        // Delay
        for(volatile int d=0; d<500000; d++);
    }
}

static void consumer(void *arg) {
    (void)arg;
    for (int i = 1; i <= 10; i++) {
        sem_wait(&full);
        mutex_lock(&pc_mutex);
        
        int item = buffer[out];
        vga_puts_color("Consumed: ", VGA_LIGHT_RED, VGA_BLACK);
        char buf[4];
        buf[0] = (item / 10) ? '0' + (item / 10) : ' ';
        buf[1] = '0' + (item % 10);
        buf[2] = '\n'; buf[3] = 0;
        vga_puts(buf);
        out = (out + 1) % BUFFER_SIZE;
        
        mutex_unlock(&pc_mutex);
        sem_signal(&empty);
        
        // Delay
        for(volatile int d=0; d<1000000; d++);
    }
}

void demo_prodcons(void) {
    in = 0; out = 0;
    mutex_init(&pc_mutex);
    sem_init(&empty, BUFFER_SIZE);
    sem_init(&full, 0);
    
    vga_puts("Starting Producer-Consumer (Buffer size 5, items 10)\n");
    thread_create(producer, 0);
    thread_create(consumer, 0);
}
