//////////////////////////////////
#define STB_IMAGE_IMPLEMENTATION
#include "lib_stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib_stb_image/stb_image_write.h"
//////////////////////////////////
#include <mpi.h>
#include <stdio.h> 
#include <omp.h>
//#include <stdlib.h>
#include "search_ref.h"  
#include "utils.h"


int main(int argc, char *argv[]) {
    // On verifie que nous avons les chemins des images.
    if (argc != 3) {
        printf("Invalid arguments !\n");
        return EXIT_FAILURE;
    }

    // Initialize the MPI environment
    MPI_Init(NULL, NULL);
    // Find out rank, size
    int worldRank; int worldSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank); 
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize); 
    unsigned int sizes[4];
 
    ////////////////////// Chargement de l'image sur la machine principale //////////////////////
    unsigned char * greyInputImg; unsigned char * greySearchImg;
    if (worldRank == 0) {  
        printf("Starting...\n");

        // Get image paths from arguments.
        char * inputImgPath = argv[1];
        char * searchImgPath = argv[2];

        // ==================================== Loading input image.
        int inputImgWidth;
        int inputImgHeight;
        int dummyNbChannels; // number of channels forced to 3 in stb_load.
        unsigned char * inputImg = stbi_load(inputImgPath, &inputImgWidth, &inputImgHeight, &dummyNbChannels, 3);
        if (inputImg == NULL)
        {
            printf("Cannot load image %s", inputImgPath);
            return EXIT_FAILURE;
        }
        printf("Input image %s: %dx%d\n", inputImgPath, inputImgWidth, inputImgHeight);

        // ====================================  Loading search image.
        int searchImgWidth;
        int searchImgHeight;
        unsigned char * searchImg = stbi_load(searchImgPath, &searchImgWidth, &searchImgHeight, &dummyNbChannels, 3);
        if (searchImg == NULL)
        {
            printf("Cannot load image %s", searchImgPath);
            return EXIT_FAILURE;
        }
        printf("Search image %s: %dx%d\n", searchImgPath, searchImgWidth, searchImgHeight);

        
        // ====================================  Passage en noir et blanc.
        unsigned char * greyInputImg  = greyScaleRef(inputImg,  inputImgWidth, inputImgHeight);
        unsigned char * greySearchImg = greyScaleRef(searchImg, searchImgWidth , searchImgHeight);            
    
        sizes[0] = inputImgWidth;
        sizes[1] = inputImgHeight;
        sizes[2] = searchImgWidth;
        sizes[3] = searchImgHeight; 

        // Plus besoin des images originaux. 
        stbi_image_free(inputImg); 
        stbi_image_free(searchImg); 
    } 
    
    ////////////////////// Broadcast //////////////////////
    printf("Broadcast\n");
    // On envoie de la part du master
    MPI_Bcast(sizes, 4, MPI_UNSIGNED, 0, MPI_COMM_WORLD); 
    unsigned int inputWidth   = sizes[0];
    unsigned int inputHeight  = sizes[1];
    unsigned int searchWidth  = sizes[2];
    unsigned int searchHeight = sizes[3]; 

    printf("sizes: (%i, %i), (%i, %i)\n",inputWidth, inputHeight, searchWidth, searchHeight);
    // On prépare les machines autres que le master a recevoir l'image
    if (worldRank != 0) {
        greyInputImg  = (unsigned char *)malloc(inputWidth * inputHeight * sizeof(unsigned char));
        greySearchImg = (unsigned char *)malloc(searchWidth * searchHeight * sizeof(unsigned char));
    } 
      
    // On broadcast les deux images
    MPI_Bcast(greyInputImg, inputWidth * inputHeight, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    printf("MPI_Bcast\n");
    MPI_Bcast(greySearchImg, searchWidth * searchHeight, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD); 
    
    // Démarage du timer avant le traitement
    double time = omp_get_wtime();
 

    ////////////////////// Traitement //////////////////////
    printf("Traitement\n");
    uint64_t minSSD = UINT64_MAX ;  
    struct point bestPosition;  
   
    // On choisit l'axe primaire (x)
    // TODO 

    // Découpe de l'image suivant le world rank (par rapport à l'axe primaire)
    unsigned int offset = worldRank * worldSize;
    for (unsigned int x = offset; x < offset + (inputWidth - searchWidth) / worldSize; x++) {
        // On fait la recherche en séquentielle sur l'axe secondaire
        for (unsigned int y = 0; y < inputHeight - searchHeight; y++) { 

            uint64_t currSSD = evaluatorRef(x, y, 
                greyInputImg, inputWidth, inputHeight, 
                greySearchImg, searchWidth, searchHeight
            ) ;           
            //printf("x: %i, y: %i, currSSD : %li \n", x, y, currSSD);
            if (currSSD <= minSSD) {  
                bestPosition.x = x;
                bestPosition.y = y;
                minSSD         = currSSD;
            }
        }
    } 

    ////////////////////// Récuperation des données //////////////////////
    printf("Reduction\n");
    // Reduction pour le obtenir le min
    uint64_t localRes[2] = {minSSD, worldRank}; 
    int globalRes[2]; 
    MPI_Reduce(localRes, globalRes, 1, MPI_UNSIGNED_LONG, MPI_MINLOC, 0, MPI_COMM_WORLD);
    // On renvoit la position au master
    /*
    if (worldRank == globalRes[1]) {
        uint64_t bestPos[2] = {bestPosition.x, bestPosition.y};
        MPI_Send(bestPos, 2, MPI_UNSIGNED_LONG, 1, 0, MPI_COMM_WORLD); 
    } else 
    */
    uint64_t bestPos[2] = {bestPosition.x, bestPosition.y};
    MPI_Send(bestPos, 2, MPI_UNSIGNED_LONG, 1, 0, MPI_COMM_WORLD); // Le master s'enverra l'info a lui même dans le cas ou c'est lui qui possède la meilleurs réponse
    
    // Le master recupère la meilleur position uniquement de celui qui est meilleurs
    if (worldRank == 0) {
        uint64_t bestPos[2];
        MPI_Recv(
            bestPos,          // data
            2,                // count
            MPI_UNSIGNED_LONG,// datatype
            globalRes[1],     // source
            0,                // tag 
            MPI_COMM_WORLD,   // communicator
            MPI_STATUS_IGNORE // status
        );
        printf("Received best position : %li, %li\n", bestPos[0], bestPos[1]);
        // Fin 
        unsigned char * resultImg = (unsigned char *)malloc(inputWidth * inputHeight * 3 * sizeof(unsigned char));
        memcpy(resultImg, greyInputImg, inputWidth * inputHeight * 3 * sizeof(unsigned char) );
        traceRef(resultImg, inputWidth, inputHeight, bestPosition, searchWidth, searchHeight);
 
        stbi_write_png("img/save_example.png", inputWidth, inputHeight, 3, resultImg, inputWidth * 3);
        printf("Time taken : %f s \n", omp_get_wtime()-time);
    }

     
    free(greyInputImg);
    free(greySearchImg);
    
    //MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
     
    return EXIT_SUCCESS;
}