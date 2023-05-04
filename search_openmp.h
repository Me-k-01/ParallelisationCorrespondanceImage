#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
//#pragma once

struct point{
    unsigned int x;
    unsigned int y;
};

// Passage en niveau de gris
unsigned char * greyScaleOpenMP( unsigned char * img,  unsigned int width,  unsigned int height);



//SSD evaluator
uint64_t evaluatorOpenMP(
        unsigned int xOffset ,  unsigned int yOffset, 
        unsigned char * img,  unsigned int imgWidth,  unsigned int imgHeight,
        unsigned char * imgToSearch,  unsigned int imgSearchWidth,  unsigned int imgSearchHeight);

// Recherche exaustive dans l'image
struct point searchOpenMP(unsigned char * img ,  unsigned int imgWidth,  unsigned int imgHeight, 
             unsigned char * imgToSearch,  unsigned int imgSearchWidth,  unsigned int imgSearchHeight) ;


//tracer le carré rouge 
void traceOpenMP(unsigned char * img, unsigned int imgWidth, unsigned int imgHeight,  struct point pos , unsigned int imgSearchWidth, unsigned int imgSearchHeight);