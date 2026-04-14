#include <soundLib.h>

void playSound(uint32_t frequency, uint32_t duration) {
    sys_playSound(frequency, duration);
}

void playHitSound()
{
    playSound(300, 100);
    playSound(250, 50);
}

void playStartMusic()
{
    playSound(262, 150);  // Do
    playSound(294, 150);  // Re
    playSound(330, 150);  // Mi
    playSound(349, 150);  // Fa
    playSound(392, 200);  // Sol
    playSound(440, 150);  // La
    playSound(523, 300);  // Do alto
}

void playHoleMusic()
{
    playSound(523, 200);  // Do alto
    playSound(659, 200);  // Mi alto
    playSound(784, 200);  // Sol alto
    playSound(1047, 400); // Do súper alto
    
    playSound(294, 150);
    
    // Acorde final
    playSound(523, 600);
}