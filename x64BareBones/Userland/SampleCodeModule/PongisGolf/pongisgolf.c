#include <color.h>
#include <pongisLib.h>
#include <pongisconfig.h>

static uint8_t playersCounter = 1;
static uint8_t winner;

void startPongisGolf() 
{
    MipT mip1 = {
        .x = MIP_INITIAL_X,
        .y = MIP_INITIAL_Y,
        .speed = MIP_INITIAL_SPEED,
        .radius = MIP_INITIAL_RADIUS,
        .color = MIP_DEFAULT_COLOR,
        .degree = 0
    };
    
    MipT mip2 = {
        .x = MIP_INITIAL_X + 100,
        .y = MIP_INITIAL_Y + 100,
        .speed = MIP_INITIAL_SPEED,
        .radius = MIP_INITIAL_RADIUS,
        .color = MIP_DEFAULT_COLOR,
        .degree = 0
    };

    BallT ball = {
        .x = BALL_INITIAL_X,
        .y = BALL_INITIAL_Y,
        .speed = BALL_INITIAL_SPEED,
        .radius = BALL_RADIUS,
        .color = BALL_COLOR
    };

    HoleT hole = {
        .x = HOLE_DEFAULT_X,
        .y = HOLE_DEFAULT_Y,
        .radius = HOLE_DEFAULT_RADIUS,
        .color = HOLE_COLOR
    };

    playersCounter = startMenu(&mip1, &mip2);
    drawLevel(1 ,&mip1, &mip2, &ball, &hole);

    // TODO: Falta chequear que no colisionen los mips

    uint8_t mipDegree;

    char c;
    int active = 1;
    while (active) {
        c = getchar();
        switch (c) {

            // Jugador 1
            case 'w': // Arriba
                walkMip(&mip1);
                if (checkColisionMipBall(&mip1, &ball)) {
                    moveBall(&ball, &mip1);
                    drawMip(&mip1);
                    playHitSound();
                }

                if (checkHoleCollision(&ball, &hole)) {
                    winner = 1;
                    endMenu(winner);
                    active = 0;
                }
                break;
            case 'd': // Derecha
                changeMipDir(&mip1,(getMipDegree(&mip1) + 1) % 4);
                break;
            case 'a': // Izquierda
                mipDegree = getMipDegree(&mip1) - 1;
                changeMipDir(&mip1,(mipDegree < 0 ? mipDegree + 4 : mipDegree) % 4);
                break;
        }

        if (playersCounter == 2)
        {
            switch (c)
            {
                // Jugador 2
                case 'i': // Arriba
                    walkMip(&mip2);
                    if (checkColisionMipBall(&mip2, &ball)) {
                        moveBall(&ball, &mip2);
                        drawMip(&mip2);
                        playHitSound();
                    }
                    if (checkHoleCollision(&ball, &hole)) {
                        winner = 2;
                        endMenu(winner);
                        active = 0;
                    }
                    break;
                case 'l': // Derecha
                    changeMipDir(&mip2,(getMipDegree(&mip2) + 1) % 4);
                    break;
                case 'j': // Izquierda
                    mipDegree = getMipDegree(&mip2) - 1;
                    changeMipDir(&mip2,(mipDegree < 0 ? mipDegree + 4 : mipDegree) % 4);
                    break;

                default:
                    continue; // Ignorar otras teclas
            }
        }
    }

}