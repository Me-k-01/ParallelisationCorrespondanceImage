#include "search_openmp.h"

// Passage en niveau de gris
unsigned char * greyScaleOpenMP( unsigned char * img,  unsigned int width,  unsigned int height){

    unsigned char *greyScaleImg = (unsigned char *)malloc(width * height * 1* sizeof(unsigned char));
    
    #pragma omp parallel for collapse(2)
    for(unsigned int y= 0 ; y<height ; y++){         
        for(unsigned int x=0 ; x<width ; x++) {

            unsigned int posGrey = x + y * width ;
            unsigned int posImg = 3 * (x + y * width);

            greyScaleImg[posGrey]= .299f * img[posImg] + .587f * img[posImg+1] + .114f * img[posImg+2];
                  
        }
    }
    return greyScaleImg;
}



//SSD evaluator
//lui pas parallélisse car 
uint64_t evaluatorOpenMP( unsigned int xOffset ,  unsigned int yOffset, 
        unsigned char * img,  unsigned int imgWidth,  unsigned int imgHeight,
        unsigned char * imgToSearch,  unsigned int imgSearchWidth,  unsigned int imgSearchHeight 
) {
    uint64_t sum = 0;
    long int d;
   
    for (unsigned int x = 0; x < imgSearchWidth; x++) {
        for (unsigned int y = 0; y < imgSearchHeight; y++) { 

            // Index dans img Q
            unsigned int indexQ = (x + xOffset) + (y + yOffset) * imgWidth; 
            // Index dans imgToSearch C
            unsigned int indexC = x + y * imgSearchWidth; 
            
            // Calcul de la distance
            d = img[indexQ] - imgToSearch[indexC];
            sum += d*d;
        }
    }
    
    return sum;
}

// Recherche exaustive dans l'image
struct point searchOpenMP(unsigned char * img ,  unsigned int imgWidth,  unsigned int imgHeight, unsigned char * imgToSearch,  unsigned int imgSearchWidth,  unsigned int imgSearchHeight){

    // Comparer "imgToSearch" -> C avec l'ensemble des sous images de "img" -> Q

    uint64_t minSSD = UINT64_MAX ; 
    uint64_t currSSD;
    struct point position;  

    #pragma omp parallel for collapse(2)
    for (unsigned int x = 0; x < imgWidth - imgSearchWidth; x++) {
        for (unsigned int y = 0; y < imgHeight - imgSearchHeight; y++) { 

            currSSD = evaluatorOpenMP(x, y, 
                img, imgWidth, imgHeight, 
                imgToSearch, imgSearchWidth, imgSearchHeight
            ) ;           
            //printf("x: %i, y: %i, currSSD : %li \n", x, y, currSSD);
             #pragma omp critical
            { 
                if (currSSD <= minSSD) { 
                    minSSD = currSSD;
                    position.x = x;
                    position.y = y;
                }
            } 
        }
    }
    return position;
}
/*
 //un poil moin bien sur space
// Recherche exaustive dans l'image
struct point searchOpenMP(unsigned char * img ,  unsigned int imgWidth,  unsigned int imgHeight, unsigned char * imgToSearch,  unsigned int imgSearchWidth,  unsigned int imgSearchHeight){

    // Comparer "imgToSearch" -> C avec l'ensemble des sous images de "img" -> Q

    uint64_t minSSD = UINT64_MAX ; 
    uint64_t currSSD;
    struct point position;  

    #pragma omp parallel
    #pragma omp single
    {   
    for (unsigned int x = 0; x < imgWidth - imgSearchWidth; x++) {
        for (unsigned int y = 0; y < imgHeight - imgSearchHeight; y++) { 
            
           #pragma omp task
          {  
            currSSD = evaluatorOpenMP(x, y, 
                img, imgWidth, imgHeight, 
                imgToSearch, imgSearchWidth, imgSearchHeight
            ) ;           
            //printf("x: %i, y: %i, currSSD : %li \n", x, y, currSSD);
             #pragma omp critical
            { 
                if (currSSD <= minSSD) { 
                    minSSD = currSSD;
                    position.x = x;
                    position.y = y;
                }
            } 
          } 
        }
    }
    } 
    return position;
}
*/

//tracer le carré rouge (image d'entrée sur 3 canneaux)
void traceOpenMP(unsigned char * img, unsigned int imgWidth, unsigned int imgHeight,  struct point pos , unsigned int imgSearchWidth, unsigned int imgSearchHeight){
 
    #pragma omp parallel for 
    for (unsigned int y = 0; y <= imgSearchHeight; y++) { 
        unsigned int i =  3 * ((pos.x) + (y+pos.y) * imgWidth);
        img[i]   = 255;
        img[i+1] = 0;
        img[i+2] = 0;
        unsigned int j =  3 * ((pos.x + imgSearchWidth) + (y+pos.y) * imgWidth); 
        img[j]   = 255;
        img[j+1] = 0;
        img[j+2] = 0;
    }
   
    #pragma omp parallel for 
    for (unsigned int x = 0; x <=imgSearchWidth; x++) {
        unsigned int i =  3 * ((x+pos.x) + (pos.y) * imgWidth);
        img[i]   = 255;
        img[i+1] = 0;
        img[i+2] = 0;
        
        unsigned int j =  3 * ((x+pos.x) + (pos.y+imgSearchHeight) * imgWidth);
        img[j]   = 255;
        img[j+1] = 0;
        img[j+2] = 0;
        
    }

    
}