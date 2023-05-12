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
    
    //unsigned char * saveExample = locateImgRef(inputImg,inputImgWidth,inputImgHeight,searchImg,searchImgWidth,searchImgHeight);
    unsigned char * saveExample = locateImgOpenMP(inputImg,inputImgWidth,inputImgHeight,searchImg,searchImgWidth,searchImgHeight);
    

    stbi_write_png("img/save_example.png", inputImgWidth, inputImgHeight, 3, saveExample, inputImgWidth*3);
    
    stbi_image_free(inputImg); 
    stbi_image_free(searchImg); 
    
    time = omp_get_wtime()-time;

    printf("Time taken : %f s \n",time);

    return EXIT_SUCCESS;
}