#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include "utils.h"

//#pragma once


// Passage en niveau de gris
unsigned char * greyScaleMPI( unsigned char * img,  unsigned int width,  unsigned int height);



//SSD evaluator
uint64_t evaluatorMPI(
        unsigned int xOffset ,  unsigned int yOffset, 
        unsigned char * img,  unsigned int imgWidth,  unsigned int imgHeight,
        unsigned char * imgToSearch,  unsigned int imgSearchWidth,  unsigned int imgSearchHeight);

// Recherche exaustive dans l'image
struct point searchMPI(unsigned char * img ,  unsigned int imgWidth,  unsigned int imgHeight, 
             unsigned char * imgToSearch,  unsigned int imgSearchWidth,  unsigned int imgSearchHeight) ;


//tracer le carré rouge 
void traceMPI(unsigned char * img, unsigned int imgWidth, unsigned int imgHeight,  struct point pos , unsigned int imgSearchWidth, unsigned int imgSearchHeight);