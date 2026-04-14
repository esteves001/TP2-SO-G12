#ifndef _PONGISLIB_H_
#define _PONGISLIB_H_

#include <usrio.h>
#include <color.h>
#include <shell.h>
#include <stdint.h>
#include <soundLib.h>
#include <videoLib.h>
#include <soundLib.h>
#include <pongisconfig.h>
#include <pongisLevels.h>

/*
    Estaria bueno hacer un ADT pero no podemos porque
    no tenemos malloc
*/

typedef struct MipT
{
    uint64_t x;
    uint64_t y;
    uint64_t speed;
    uint64_t radius;
    uint32_t color;
    int16_t degree; // 0 arriba, 1 derecha, 2 abajo, 3 izquierda  
}MipT;

typedef struct BallT
{
    uint64_t x;
    uint64_t y;
    uint64_t speed;
    uint64_t radius;
    uint32_t color;
}BallT;

typedef struct HoleT
{
    uint64_t x;
    uint64_t y;
    uint64_t radius;
    uint32_t color;
}HoleT;

typedef MipT* MipP;
typedef BallT* BallP;
typedef HoleT* HoleP;

/*
    Esto al final no lo usamos, pero lo dejamos porque es lo que vamos a usar
    cuando implementemos malloc en SO.
*/
typedef struct LevelT
{
    uint16_t level;
    MipP mip1;
    MipP mip2;
    BallP ball;
    HoleP hole;
}LevelT;

typedef LevelT* LevelP;

// Helpers
void drawLevel(uint16_t level, MipP mip1, MipP mip2, BallP ball, HoleP hole);
uint8_t checkColisionMipBall(MipP mip, BallP ball);
uint8_t checkValidScreenPosition(uint32_t x, uint32_t y, uint32_t radius);
uint8_t startMenu(MipP mip1, MipP mip2);
void endMenu(uint8_t winner);

// Pelota
void drawBall(BallP ball);
void eraseBall(BallP ball);
void moveBall(BallP ball, MipP mip);

// Mip
void startPongisGolf();
void drawMip(MipP mip);
void walkMip(MipP mip);
void eraseMip(MipP mip);
void changeMipDir(MipP mip, uint16_t degree);
uint16_t getMipDegree(MipP mip);
uint8_t checkColisionMipMip(MipP mip1, MipP mip2);

// Agujero
void eraseHole(HoleP hole);
void drawHole(HoleP hole);
uint8_t checkHoleCollision(BallP ball, HoleP hole);

#endif