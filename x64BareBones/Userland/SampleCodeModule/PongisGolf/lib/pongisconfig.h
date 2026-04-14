#ifndef _PONGISCONFIG_H_
#define _PONGISCONFIG_H_
 
#include <color.h>

#define BACKGROUND_COLOR            GREEN

#define MIP_DEFAULT_COLOR           WHITE
#define MIP_INITIAL_RADIUS          20
#define MIP_INITIAL_SPEED           10
#define MIP_INITIAL_X               100
#define MIP_INITIAL_Y               100
#define MIP_EYE_RADIUS              5           // Porcentaje del radio del mip
#define MIP_EYE_COLOR               BLACK


#define BALL_COLOR                  BLUE
#define BALL_RADIUS                 10
#define BALL_INITIAL_SPEED          MIP_INITIAL_SPEED + 15
#define BALL_INITIAL_X              MIP_INITIAL_X + 50
#define BALL_INITIAL_Y              MIP_INITIAL_Y + 50

#define HOLE_COLOR                  BLACK
#define HOLE_DEFAULT_RADIUS         30
#define HOLE_DEFAULT_X              500
#define HOLE_DEFAULT_Y              500

#define LEVEL_COUNT                 1

#define MAX_PLAYERS                 2

#endif