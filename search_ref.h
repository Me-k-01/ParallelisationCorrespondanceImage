#ifndef SEARCH_REF
#define SEARCH_REF

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include "utils.h"

//#pragma once 

// Passage en niveau de gris
unsigned char * greyScaleRef( unsigned char * img,  unsigned int width,  unsigned int height);



//SSD evaluator
uint64_t evaluatorRef(
        unsigned int xOffset ,  unsigned int yOffset, 
        unsigned char * img,  unsigned int imgWidth,  unsigned int imgHeight,
        unsigned char * imgToSearch,  unsigned int imgSearchWidth,  unsigned int imgSearchHeight);

// Recherche exaustive dans l'image
struct point searchRef(unsigned char * img ,  unsigned int imgWidth,  unsigned int imgHeight, 
             unsigned char * imgToSearch,  unsigned int imgSearchWidth,  unsigned int imgSearchHeight) ;


//tracer le carré rouge 
void traceRef(unsigned char * img, unsigned int imgWidth, unsigned int imgHeight,  struct point pos , unsigned int imgSearchWidth, unsigned int imgSearchHeight);



unsigned char * locateImgRef(unsigned char * inputImg,int inputImgWidth, int inputImgHeight,unsigned char *searchImg, int searchImgWidth, int searchImgHeight );

#endif // SEARCH_REF