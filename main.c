#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "lib_stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib_stb_image/stb_image_write.h"

#include "search_ref.h"
#include "search_openmp.h"

int main (int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Invalid arguments !\n");
        return EXIT_FAILURE;
    }

    // Get image paths from arguments.
     char *inputImgPath = argv[1];
     char *searchImgPath = argv[2];

    // ==================================== Loading input image.
    int inputImgWidth;
    int inputImgHeight;
    int dummyNbChannels; // number of channels forced to 3 in stb_load.
    unsigned char *inputImg = stbi_load(inputImgPath, &inputImgWidth, &inputImgHeight, &dummyNbChannels, 3);
    if (inputImg == NULL)
    {
        printf("Cannot load image %s", inputImgPath);
        return EXIT_FAILURE;
    }
    printf("Input image %s: %dx%d\n", inputImgPath, inputImgWidth, inputImgHeight);

    // ====================================  Loading search image.
    int searchImgWidth;
    int searchImgHeight;
    unsigned char *searchImg = stbi_load(searchImgPath, &searchImgWidth, &searchImgHeight, &dummyNbChannels, 3);
    if (searchImg == NULL)
    {
        printf("Cannot load image %s", searchImgPath);
        return EXIT_FAILURE;
    }
    printf("Search image %s: %dx%d\n", searchImgPath, searchImgWidth, searchImgHeight);



    // ====================================  Save example: save a copy of 'inputImg'


    double time = omp_get_wtime();
   
    printf("%i , %i \n", inputImgWidth, inputImgHeight);
    unsigned char *greyScaleImg= greyScaleRef(inputImg, inputImgWidth, inputImgHeight);
    unsigned char *greyScaleSearchImg= greyScaleRef(searchImg, searchImgWidth, searchImgHeight);
    

    struct point position = searchRef(
        greyScaleImg, inputImgWidth, inputImgHeight, 
        greyScaleSearchImg, searchImgWidth, searchImgHeight
    );
    // 29214668.000000
    // 41969804.000000
    printf("x: %i, y: %i \n", position.x, position.y);
    printf("valeur SSD : %li\n", evaluatorRef(position.x, position.y, 
        greyScaleImg, inputImgWidth, inputImgHeight, 
        greyScaleSearchImg, searchImgWidth, searchImgHeight
    ));
    
    unsigned char *saveExample = (unsigned char *)malloc(inputImgWidth * inputImgHeight * 3 * sizeof(unsigned char));
    memcpy( saveExample, inputImg, inputImgWidth * inputImgHeight * 3 * sizeof(unsigned char) );
    traceRef(saveExample,inputImgWidth, inputImgHeight, position, searchImgWidth, searchImgHeight);
    stbi_write_png("img/save_example.png", inputImgWidth, inputImgHeight, 3, saveExample, inputImgWidth*3);

    free(greyScaleImg);
    free(greyScaleSearchImg);
    stbi_image_free(inputImg); 
    stbi_image_free(searchImg); 

    time = omp_get_wtime()-time;

    printf("Good bye! time taken : %f s \n",time);

    return EXIT_SUCCESS;
}