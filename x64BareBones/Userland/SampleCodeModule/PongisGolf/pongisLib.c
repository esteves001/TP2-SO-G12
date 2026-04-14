#include <pongisLib.h>

static uint8_t playersCounter = 0;

// Helpers

/*
    Esta forma no nos gusta mucho, proque lo ideal seria hacerlo con un ADT
    pero no tenemos malloc
*/

// Devuelve la cantidad de jugadores
uint8_t startMenu(MipP mip1, MipP mip2)
{
    uint32_t colores[] = {WHITE, ORANGE, TURQUOISE, OLIVE};

    uint16_t linesWritten = 0;

    clearScreen();
    printf("Welcome to Pongis Golf!\n");
    printf("Slect 1 or 2 players\n");
    do
    {
        playersCounter = getchar() - '0';
    }while (playersCounter < 0 && playersCounter > MAX_PLAYERS);
    clearScreen();

    for (unsigned int player = 1; player <= playersCounter; player++)
    {
        printf("Select Mip%d color:\n", player);
        printf("1. White\n");
        printf("2. Orange\n");
        printf("3. Turquoise\n");
        printf("4. Olive\n");

        uint8_t colorChoice;
        do
        {
            colorChoice = getchar() - '0';
        } while (colorChoice < 1 || colorChoice > 4);

        if (player == 1) {
            mip1->color = colores[colorChoice - 1];
        } else {
            mip2->color = colores[colorChoice - 1];
        }
        clearScreen();
    }

    return playersCounter;
}

void endMenu(uint8_t winner)
{
    clearScreen();

    /*
        TODO: Habria que cambiar drawString para evitar esto
        TODO: Arreglar esto
    */


    if (winner == 1)
    {
        printf("Player 1 wins!!!\n");
        //drawString(getScreenWidth() / 2, getScreenHeight() / 2, "Player 1 wins!!!", WHITE);
    }

    else if (winner == 2)
    {
        printf("Player 2 wins!!!\n");
        //drawString(getScreenWidth() / 2, getScreenHeight() / 2, "Player 2 wins!!!", WHITE);
    }

    playHoleMusic();
}

void drawLevel(uint16_t level, MipP mip1, MipP mip2, BallP ball, HoleP hole)
{   
    paintScreen(BACKGROUND_COLOR);
    
    switch (level)
    {
    case 1:
        // Mip1
        mip1->x = MIP_INITIAL_X;
        mip1->y = MIP_INITIAL_Y;
        drawMip(mip1);

        // Mip2
        if (playersCounter == 2) 
        {
            mip2->x = MIP_INITIAL_X;
            mip2->y = MIP_INITIAL_Y + 100;
            drawMip(mip2);
        }

        // Pelota
        ball->x = BALL_INITIAL_X;
        ball->y = BALL_INITIAL_Y;
        drawBall(ball);

        // Agujero
        hole->x = HOLE_DEFAULT_X;
        hole->y = HOLE_DEFAULT_Y;
        hole->radius = HOLE_DEFAULT_RADIUS + 30;
        drawHole(hole);
        break;

    default:
        break;
    }

    playStartMusic();
}

uint8_t checkColisionMipBall(const MipP mip, const BallP ball)
{
    // Calcular la distancia entre los centros de ambos círculos
    int64_t dx = (int64_t)mip->x - (int64_t)ball->x;
    int64_t dy = (int64_t)mip->y - (int64_t)ball->y;
    
    // Pitagoras
    uint64_t distanceSquared = (dx * dx) + (dy * dy);
    
    // Calcular la suma de los radios
    uint64_t radiiSum = mip->radius + ball->radius;
    
    // Calcular la suma de radios al cuadrado
    uint64_t radiiSumSquared = radiiSum * radiiSum;
    
    // Si la distancia al cuadrado es menor o igual que la suma de radios al cuadrado,
    // entonces hay colisión
    return distanceSquared <= radiiSumSquared;
}

uint8_t checkValidScreenPosition(uint32_t x, uint32_t y, uint32_t radius)
{
    uint32_t screenWidth = getScreenWidth();
    uint32_t screenHeight = getScreenHeight();

    return (x + radius < screenWidth && x >= radius && y + radius < screenHeight && y >= radius);
}


// Seccion Mip
void drawMip(MipP mip)
{
    // Dibujar cuerpo
    drawCircle(mip->radius, mip->color, mip->x, mip->y);

    uint64_t eye_radius = (mip->radius * MIP_EYE_RADIUS) / 100;
    uint64_t eye_distance = mip->radius / 2;  // Distancia del centro al ojo
    
    // Posición del ojo según la dirección
    int64_t eye_x, eye_y;
    
    switch(mip->degree) {
        case 0: // Arriba
            eye_x = mip->x;
            eye_y = mip->y - eye_distance;
            break;
            
        case 1: // Derecha
            eye_x = mip->x + eye_distance;
            eye_y = mip->y;
            break;
            
        case 2: // Abajo
            eye_x = mip->x;
            eye_y = mip->y + eye_distance;
            break;
            
        case 3: // Izquierda
            eye_x = mip->x - eye_distance;
            eye_y = mip->y;
            break;
            
        default: // Por defecto, mirando arriba
            eye_x = mip->x;
            eye_y = mip->y - eye_distance;
            break;
    }
    
    // Dibujar el ojo
    drawCircle(eye_radius, MIP_EYE_COLOR, eye_x, eye_y);
}

void eraseMip(MipP mip)
{
    drawCircle(mip->radius, BACKGROUND_COLOR, mip->x, mip->y);
}

void changeMipDir(MipP mip, uint16_t degree)
{
    mip->degree = degree;
    drawMip(mip);
}

void walkMip(MipP mip)
{
    /*
        TODO: Cambiar la forma en que lo borra para
        no repetir codigo. Tiene que ser antes
        de que actualice la posicion
    */

    eraseMip(mip);
    switch(mip->degree) {
        case 0: // Arriba
            if (checkValidScreenPosition(mip->x, mip->y - mip->speed, mip->radius)) {
                mip->y -= mip->speed;
            }
            break;
        case 1: // Derecha
            if (checkValidScreenPosition(mip->x + mip->speed, mip->y, mip->radius)) {
                mip->x += mip->speed;
            }
            break;
        case 2: // Abajo
            if (checkValidScreenPosition(mip->x, mip->y + mip->speed, mip->radius)) {
                mip->y += mip->speed;
            }
            break;
        case 3: // Izquierda
            if (checkValidScreenPosition(mip->x - mip->speed, mip->y, mip->radius)) {
                mip->x -= mip->speed;
            }
            break;
        default:
            break;
    }
    drawMip(mip);
}

uint16_t getMipDegree(MipP mip)
{
    return mip->degree;
}

uint8_t checkColisionMipMip(MipP mip1, MipP mip2)
{
    // Calcular la distancia entre los centros de ambos Mips
    int64_t dx = (int64_t)mip1->x - (int64_t)mip2->x;
    int64_t dy = (int64_t)mip1->y - (int64_t)mip2->y;
    
    // Pitagoras
    uint64_t distanceSquared = (dx * dx) + (dy * dy);
    
    // Calcular la suma de los radios
    uint64_t radiiSum = mip1->radius + mip2->radius;
    
    // Calcular la suma de radios al cuadrado
    uint64_t radiiSumSquared = radiiSum * radiiSum;
    
    // Si la distancia al cuadrado es menor o igual que la suma de radios al cuadrado,
    // entonces hay colisión
    return distanceSquared <= radiiSumSquared;
}


// Seccion Pelota
void drawBall(BallP ball)
{
    drawCircle(ball->radius, ball->color, ball->x, ball->y);
}

void eraseBall(BallP ball)
{
    drawCircle(ball->radius, BACKGROUND_COLOR, ball->x, ball->y);
}

void moveBall(BallP ball, MipP mip)
{
    // Borrar la pelota antes de moverla
    eraseBall(ball);

    // Calcular la nueva posición de la pelota
    switch(mip->degree) {
        case 0: // Arriba
            if (checkValidScreenPosition(ball->x, ball->y - BALL_INITIAL_SPEED, ball->radius)) {
                ball->y -= BALL_INITIAL_SPEED;
            }
            break;
        case 1: // Derecha
            if (checkValidScreenPosition(ball->x + BALL_INITIAL_SPEED, ball->y, ball->radius)) {
                ball->x += BALL_INITIAL_SPEED;
            }
            break;
        case 2: // Abajo
            if (checkValidScreenPosition(ball->x, ball->y + BALL_INITIAL_SPEED, ball->radius)) {
                ball->y += BALL_INITIAL_SPEED;
            }
            break;
        case 3: // Izquierda
            if (checkValidScreenPosition(ball->x - BALL_INITIAL_SPEED, ball->y, ball->radius)) {
                ball->x -= BALL_INITIAL_SPEED;
            }
            break;
        default:
            break;
    }

    // Dibujar la pelota en la nueva posición
    drawBall(ball);
}


// Seccion Agujero
void drawHole(HoleP hole)
{
    drawCircle(hole->radius, HOLE_COLOR, hole->x, hole->y);
}

void eraseHole(HoleP hole)
{
    drawCircle(hole->radius, BACKGROUND_COLOR, hole->x, hole->y);
}

uint8_t checkHoleCollision(BallP ball, HoleP hole)
{
    // Calcular la distancia entre el centro de la pelota y el centro del agujero
    int64_t dx = (int64_t)ball->x - (int64_t)hole->x;
    int64_t dy = (int64_t)ball->y - (int64_t)hole->y;
    
    // Pitagoras
    uint64_t distanceSquared = (dx * dx) + (dy * dy);
    
    // Calcular el radio del agujero al cuadrado
    uint64_t holeRadiusSquared = hole->radius * hole->radius;
    
    // Si la distancia al cuadrado es menor o igual que el radio del agujero al cuadrado,
    // entonces la pelota ha caído en el agujero
    return distanceSquared <= holeRadiusSquared;
}
