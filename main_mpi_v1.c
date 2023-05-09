//////////////////////////////////
#define STB_IMAGE_IMPLEMENTATION
#include "lib_stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib_stb_image/stb_image_write.h"
//////////////////////////////////
#include <mpi.h>
#include <stdio.h> 
//#include <stdlib.h>
#include "search_ref.h"
#include "search_openmp.h"
#include "utils.h"

enum msgType { DATA, END }; 

void master(int world_size) {
    if (argc != 3) {
        printf("Invalid arguments !\n");
        return EXIT_FAILURE;
    }

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


    // Démarage du timer
    time = omp_get_wtime();
    
    // ==================================== Envoie du travail
    unsigned char * greyInputImg  = greyScaleRef(inputImg,  inputImgWidth, inputImgHeight);
    unsigned char * greySearchImg = greyScaleRef(searchImg, searchImgWidth , searchImgHeight);            


    unsigned int * sizes = (unsigned int) malloc( 4*sizeof(unsigned int) );
    sizes[0]=inputImgWidth;
    sizes[1]=inputImgHeight;
    sizes[2]=searchImgWidth;
    sizes[3]=searchImgHeight;
     
    MPI_Bcast(sizes, 4, MPI_UNSIGNED_INT, 0, MPI_COMM_WRLD);
    // Broadcast des deux images
    MPI_Bcast(greyInputImg, inputImgWidth * inputImgHeight, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WRLD);
    MPI_Bcast(greySearchImg, searchImgWidth * searchImgHeight, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WRLD);
    
    // On envoie un travail initial à tout le monde.
    unsigned int x = 0;
    unsigned int y = 0;
    uint64_t min = UINT64_MAX;
    unsigned int xBest = 0;
    unsigned int yBest = 0;

    
    // On entre dans la boucle de travail
    // Tant qu'il y a du travail a effectuer 
        // On effectue le travail sur le master
        // On regarde s'il y a un client qui a terminer son travail (non bloquant)
        // Et on lui envoie un nouveau travail
    // Lorsqu'on a pu de travail, on a finit
    // Donc on envoie un signal de fin à tout le monde, 
        // Et on attends leurs reponse individuel
        // A chaque réponse, on compare et stocke le meilleurs minimum
     
    MPI_Status status;

    while (true) {  
        // Pour chaque client
        for (int i = 1; i < world_size; i++){
            MPI_Request * req; 
            // On regarde si le client nous a demander un travail
            MPI_Irecv(
            /* data         = */ &_, 
            /* count        = */ 1, 
            /* datatype     = */ MPI_INT, 
            /* source       = */ i, 
            /* tag          = */ MPI_ANY_TAG, 
            /* communicator = */ MPI_COMM_WORLD,  
                                & req
            );
            int askForTask;
            MPI_Test(&req, &askForTask, &status);
            // Si on a pas de demande de la par du client, on skip cette machine
            if (! askForTask) { 
                continue;
            }

            // On verifie qu'il nous reste du travail
            if (x < inputImgWidth - searchImgWidth && y < inputImgHeight - searchImgHeight) { 
                break; // Ça nous sort de toutes les boucles quand on y pense
            }
            x ++;
            if (x > inputImgWidth - searchImgWidth) {
                y ++;
                x = 0;
            }
            // S'il nous reste du travail on l'envoie au client
            unsigned int data = malloc(2 * sizeof(unsigned int));
            MPI_Send(&data, 2, MPI_INT, i, msgType.DATA, MPI_COMM_WORLD);
        }   
        // On reverifie qu'il nous reste du travail
        if (x < inputImgWidth - searchImgWidth && y < inputImgHeight - searchImgHeight) 
            break;

        // On effectue le travail
        
        uint64_t currMin = evaluatorRef(img , imgWidth, imgHeight, imgToSearch, imgSearchWidth, imgSearchHeight);
        if (min > currMin) {
            min = currMin;
            xBest = x;
            yBest = y;
        }
    }  

    
    // On recupère tout les resultat

    //void traceRef(unsigned char * img, unsigned int imgWidth, unsigned int imgHeight,  struct point pos , unsigned int imgSearchWidth, unsigned int imgSearchHeight);

    stbi_write_png("img/save_example.png", inputImgWidth, inputImgHeight, 3, saveExample, inputImgWidth*3);
    
    stbi_image_free(inputImg); 
    stbi_image_free(searchImg); 
    
    time = omp_get_wtime()-time;

    printf("Time taken : %f s \n",time);
 
}

void client() {
    ////////////////// On reçoit les tailles des images
    int * sizes;
    MPI_Recv(
    /* data         = */ &sizes, 
    /* count        = */ 4, 
    /* datatype     = */ MPI_INT, 
    /* source       = */ 0, 
    /* tag          = */ MPI_ANY_TAG, 
    /* communicator = */ MPI_COMM_WORLD, 
    /* status       = */ MPI_STATUS_IGNORE
    );
    unsigned int inputImgWidth   = sizes[0];
    unsigned int inputImgHeight  = sizes[1];
    unsigned int searchImgWidth  = sizes[2];
    unsigned int searchImgHeight = sizes[3];

    ////////////////// On reçoit les images 
    unsigned char * imageInput, imageSearch; 
    MPI_Recv(
    /* data         = */ &imageInput, 
    /* count        = */ inputImgWidth * inputImgHeight, 
    /* datatype     = */ MPI_INT, 
    /* source       = */ 0, 
    /* tag          = */ MPI_ANY_TAG, 
    /* communicator = */ MPI_COMM_WORLD, 
    /* status       = */ MPI_STATUS_IGNORE
    );
    MPI_Recv(
    /* data         = */ &imageSearch, 
    /* count        = */ searchImgWidth * searchImgHeight, 
    /* datatype     = */ MPI_INT, 
    /* source       = */ 0, 
    /* tag          = */ MPI_ANY_TAG, 
    /* communicator = */ MPI_COMM_WORLD, 
    /* status       = */ MPI_STATUS_IGNORE
    ); 
    
    
    MPI_Status status; 
    unsigned int * coords = (unsigned int *)malloc(2 * sizeof(unsigned int));
    uint64_t * result = (uint64_t *)malloc();  // [x, y, minSSD]
    result[2] = UINT64_MAX;

    MPI_Recv(coords, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &status); // Reçoit au plus deux coordonnées.    
    // Lorsqu'on reçoit du travail du master
    if (status.MPI_TAG == msgType.DATA) { 
        // On traite le travail
        int x = coords[0]; int y = coords[1];
        uint64_t currMin = evaluatorRef(img , imgWidth, imgHeight, imgToSearch, imgSearchWidth, imgSearchHeight);
        // On stocke l'évaluation du SSD dans un minimum
        if (currMin < result[2] ) {
          result[0] = x;
          result[1] = y;
          result[2] = currMin;
        }
        // Et on demande au master le reste du travail
        MPI_Send(0, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); 
    } else { // status.MPI_TAG == msgType.END
        // Ou lorsque l'on reçoit le signal de fin, on envoie notre résultat (les machines qui n'ont pas travailler renvoit FLOAT_MAX)
        MPI_Send(&result, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);

    }
}


int main(int argc, char *argv[]) {

    // Initialize the MPI environment
    MPI_Init(NULL, NULL);
    // Find out rank, size
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    double time = 0.0 ; 

    // Traitement initial de l'image, et repartition du travail
    if (world_rank = 0) { 
        master(world_size);   
    } else { 
        client();
    }
    char direction;
 
    

    // Calcul 
    // send
 
    return EXIT_SUCCESS;
}