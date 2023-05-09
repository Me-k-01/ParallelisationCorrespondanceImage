#ifndef SEARCH_OPENMP
#define SEARCH_OPENMP

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include "utils.h"

//#pragma once


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



unsigned char * locateImgOpenMP(unsigned char * inputImg,int inputImgWidth, int inputImgHeight,unsigned char *searchImg, int searchImgWidth, int searchImgHeight );

#endif // SEARCH_OPENMP