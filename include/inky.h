#ifndef INKY_H
#define INKY_H
#include <inttypes.h>

typedef struct{
    uint16_t width;
    uint16_t height;
}resolution_t;


typedef enum {
    BLACK = 0,
    WHITE,
    GREEN,
    BLUE,
    RED,
    YELLOW,
    ORANGE,
    CLEAN
} Colour_7_t;

class Inky{

};

#endif
