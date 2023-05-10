#include "search_ref.h"

// Passage en niveau de gris
unsigned char * greyScaleRef( unsigned char * img,  unsigned int width,  unsigned int height){

    unsigned char * greyScaleImg = (unsigned char *)malloc(width * height * 1 * sizeof(unsigned char));
    
    for(unsigned int y= 0 ; y<height ; y++){         
        for(unsigned int x=0 ; x<width ; x++) {

            unsigned int posGrey = x + y * width ;
            unsigned int posImg = 3 * (x + y * width);

            greyScaleImg[posGrey]= (0.299f*img[posImg])+(0.587f*img[posImg+1])+(0.114f*img[posImg+2]);
                  
        }
    }

    return greyScaleImg;
}



//SSD evaluator
uint64_t evaluatorRef( unsigned int xOffset ,  unsigned int yOffset, 
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
struct point searchRef(unsigned char * img ,  unsigned int imgWidth,  unsigned int imgHeight, unsigned char * imgToSearch,  unsigned int imgSearchWidth,  unsigned int imgSearchHeight){

    // Comparer "imgToSearch" -> C avec l'ensemble des sous images de "img" -> Q

    uint64_t minSSD = UINT64_MAX ; 
    uint64_t currSSD;
    struct point position;  

    for (unsigned int x = 0; x < imgWidth - imgSearchWidth; x++) {
        for (unsigned int y = 0; y < imgHeight - imgSearchHeight; y++) { 

            currSSD = evaluatorRef(x, y, 
                img, imgWidth, imgHeight, 
                imgToSearch, imgSearchWidth, imgSearchHeight
            ) ;           
            //printf("x: %i, y: %i, currSSD : %li \n", x, y, currSSD);
            if (currSSD <= minSSD) { 
                minSSD = currSSD;
                position.x = x;
                position.y = y;
            }
        }
    }
    return position;
}


//tracer le carré rouge (image d'entrée sur 3 canneaux)
void traceRef(unsigned char * img, unsigned int imgWidth, unsigned int imgHeight,  struct point pos , unsigned int imgSearchWidth, unsigned int imgSearchHeight){
 
    for (unsigned int y = 0; y < imgSearchHeight; y++) { 
        unsigned int i =  3 * ((pos.x) + (y+pos.y) * imgWidth);
        img[i]   = 255;
        img[i+1] = 0;
        img[i+2] = 0;
        unsigned int j =  3 * ((pos.x + imgSearchWidth) + (y+pos.y) * imgWidth); 
        img[j]   = 255;
        img[j+1] = 0;
        img[j+2] = 0;
    }
   
    for (unsigned int x = 0; x < imgSearchWidth; x++) {
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



unsigned char * locateImgRef(unsigned char * inputImg,int inputImgWidth, int inputImgHeight,unsigned char *searchImg, int searchImgWidth, int searchImgHeight ){


    printf("%i , %i \n", inputImgWidth, inputImgHeight);
    unsigned char *greyScaleImg       = greyScaleRef(inputImg, inputImgWidth, inputImgHeight);
    unsigned char *greyScaleSearchImg = greyScaleRef(searchImg, searchImgWidth, searchImgHeight);
    

    struct point position = searchRef(
        greyScaleImg, inputImgWidth, inputImgHeight, 
        greyScaleSearchImg, searchImgWidth, searchImgHeight
    );
   

    printf("x: %i, y: %i \n", position.x, position.y);
    printf("valeur SSD : %li\n", evaluatorRef(position.x, position.y, 
        greyScaleImg, inputImgWidth, inputImgHeight, 
        greyScaleSearchImg, searchImgWidth, searchImgHeight
    ));
    
    unsigned char *saveExample = (unsigned char *)malloc(inputImgWidth * inputImgHeight * 3 * sizeof(unsigned char));
    memcpy( saveExample, inputImg, inputImgWidth * inputImgHeight * 3 * sizeof(unsigned char) );
    traceRef(saveExample,inputImgWidth, inputImgHeight, position, searchImgWidth, searchImgHeight);


    free(greyScaleImg);
    free(greyScaleSearchImg);

    return saveExample;
    
    


}