#include <soundDriver.h>

// Codigo de https://wiki.osdev.org/PC_Speaker

//Play sound using built-in speaker
void play_sound(uint32_t nFrequence) {
    uint32_t Div;
    uint8_t tmp;

    //Set the PIT to the desired frequency
    Div = 1193180 / nFrequence;
    outb(0x43, 0xb6);
    outb(0x42, (uint8_t) (Div) );
    outb(0x42, (uint8_t) (Div >> 8));

    //And play the sound using the PC speaker
    tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }
}
 
//make it shut up
void nosound() {
    uint8_t tmp = inb(0x61) & 0xFC;
        
    outb(0x61, tmp);
}
 
void playSoundForDuration(uint32_t nFrequence, uint32_t duration) {
    play_sound(nFrequence);
    sleep(duration);
    nosound();
}

//Make a beep
void beep() {
    play_sound(1000);
    sleep(10);
    nosound();
    //set_PIT_2(old_frequency);
}

void game_thud_sound() {
    play_sound(500);
    sleep(4);
    nosound();
}

void play_boot_sound() {
    playSoundForDuration(100, 100);
	sleep(100);
	playSoundForDuration(200, 100);
	sleep(100);
	playSoundForDuration(300, 100);
	sleep(100);
	playSoundForDuration(400, 100);
	sleep(100);
	playSoundForDuration(500, 100);
}