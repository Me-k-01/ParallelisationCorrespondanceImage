//////////////////////////////////
#define STB_IMAGE_IMPLEMENTATION
#include "lib_stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib_stb_image/stb_image_write.h"
//////////////////////////////////
#include <mpi.h>
#include <stdio.h> 
#include <stdlib.h>
#include <omp.h>   
#include "search_ref.h"  
#include "utils.h"
#include "search_openmp.h"  

#define USE_OPENMP

typedef struct pointSSDStruct {
    uint64_t value; 
    unsigned int x;
    unsigned int y;
} pointSSD;


void minPointSSD(void *in, void *inout, int *len, MPI_Datatype *type){ 
    pointSSD *invals    = in;
    pointSSD *inoutvals = inout;

    for (int i=0; i<*len; i++) {
        if (invals[i].value < inoutvals[i].value) {
            inoutvals[i].value  = invals[i].value;
            inoutvals[i].x = invals[i].x;
            inoutvals[i].y = invals[i].y;
        }
    }

    return;
}

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
    unsigned char * inputImg; unsigned char * greyInputImg; unsigned char * greySearchImg; 
    if (worldRank == 0) {  
        printf("Starting...\n");

        // Get image paths from arguments.
        char * inputImgPath = argv[1];
        char * searchImgPath = argv[2];

        // ==================================== Loading input image.
        int inputImgWidth;
        int inputImgHeight;
        int dummyNbChannels; // number of channels forced to 3 in stb_load.
        inputImg = stbi_load(inputImgPath, &inputImgWidth, &inputImgHeight, &dummyNbChannels, 3);
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
        #ifdef USE_OPENMP
            greyInputImg  = greyScaleOpenMP(inputImg,  inputImgWidth, inputImgHeight);
            greySearchImg = greyScaleOpenMP(searchImg, searchImgWidth , searchImgHeight);            
        #else
            greyInputImg  = greyScaleRef(inputImg,  inputImgWidth, inputImgHeight);
            greySearchImg = greyScaleRef(searchImg, searchImgWidth , searchImgHeight);            
        #endif 
        sizes[0] = inputImgWidth;
        sizes[1] = inputImgHeight;
        sizes[2] = searchImgWidth;
        sizes[3] = searchImgHeight; 

        // Plus besoin de l'image de recherche original.  
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
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(greyInputImg, inputWidth * inputHeight, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD); 
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(greySearchImg, searchWidth * searchHeight, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD); 
    
    // Démarage du timer avant le traitement
    double time = omp_get_wtime();
 

    ////////////////////// Traitement ////////////////////// 
    uint64_t minSSD = UINT64_MAX ;  
    struct point bestPosition;  
   
    // On choisit l'axe primaire (x)
    // TODO 

    // Découpe de l'image suivant le world rank (par rapport à l'axe primaire)
    unsigned int chunk = (inputWidth - searchWidth) / worldSize;
    unsigned int offset = worldRank * chunk;
    for (unsigned int x = offset; x <  offset + chunk; x++) {
        // On fait la recherche en séquentielle sur l'axe secondaire
        for (unsigned int y = 0; y < inputHeight - searchHeight; y++) { 

            #ifdef USE_OPENMP
                uint64_t currSSD = evaluatorOpenMP(x, y, 
                    greyInputImg, inputWidth, inputHeight, 
                    greySearchImg, searchWidth, searchHeight
                ) ;           
            #else
                uint64_t currSSD = evaluatorRef(x, y, 
                    greyInputImg, inputWidth, inputHeight, 
                    greySearchImg, searchWidth, searchHeight
                ) ;     
            #endif
            
            //printf("x: %i, y: %i, currSSD : %li \n", x, y, currSSD);
            if (currSSD <= minSSD) {  
                bestPosition.x = x;
                bestPosition.y = y;
                minSSD         = currSSD;
            }
        }
    } 

    ////////////////////// Récuperation des données ////////////////////// 
    // Reduction pour le obtenir le min 

    // Thanks to https://stackoverflow.com/questions/9285442/mpi-get-processor-with-minimum-value
    /* create our new data type */
    MPI_Datatype mpiPointSSD;
    MPI_Datatype types[3] = { MPI_UNSIGNED_LONG, MPI_UNSIGNED, MPI_UNSIGNED };
    MPI_Aint disps[3] = { 
        offsetof(pointSSD, value),
        offsetof(pointSSD, x),
        offsetof(pointSSD, y),  
    };
    int lens[3] = {1,1,1};
    MPI_Type_create_struct(3, lens, disps, types, &mpiPointSSD);
    MPI_Type_commit(&mpiPointSSD);

    /* create our operator */
    MPI_Op mpiMinPointSSD;
    MPI_Op_create(minPointSSD, 1, &mpiMinPointSSD);


    pointSSD localRes;
    pointSSD globalRes;
    localRes.value = minSSD;
    localRes.x = bestPosition.x;
    localRes.y = bestPosition.y;

    //printf("localRes.value : %li, localRes.x,y : %i, %i\n", localRes.value, localRes.x, localRes.y);
    //MPI_Barrier(MPI_COMM_WORLD);  // synchronize all processes
    MPI_Reduce(&localRes, &globalRes, // Reduction des locaux vers le global
        1,             // count
        mpiPointSSD,   // datatype
        mpiMinPointSSD,     // operation
        0,             // destination
        MPI_COMM_WORLD // root
    ); // Min_Loc n'existe pas pour unsigned long
 
 
    if (worldRank == 0) {  
        printf("Received best position : %li, %i, %i\n", globalRes.value, globalRes.x, globalRes.y);
        bestPosition.x = globalRes.x;
        bestPosition.y = globalRes.y;
 

        // Et il calcul l'image final
        unsigned char * resultImg = (unsigned char *)malloc(inputWidth * inputHeight * 3 * sizeof(unsigned char));
        memcpy(resultImg, inputImg, inputWidth * inputHeight * 3 * sizeof(unsigned char) );
        #ifdef USE_OPEN_MP
            traceOpenMP(resultImg, inputWidth, inputHeight, bestPosition, searchWidth, searchHeight);
        #else
            traceRef(resultImg, inputWidth, inputHeight, bestPosition, searchWidth, searchHeight);
        #endif 
        stbi_write_png("img/save_example.png", inputWidth, inputHeight, 3, resultImg, inputWidth * 3);
        printf("Time taken : %f s \n", omp_get_wtime()-time);
        stbi_image_free(inputImg); 
    }

     
    MPI_Op_free(&mpiMinPointSSD);
    MPI_Type_free(&mpiPointSSD); 
    free(greyInputImg);
    free(greySearchImg);
    
    //MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
     
    return EXIT_SUCCESS;
}