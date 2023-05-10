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

int master(int world_size, int argc, char *argv[]) { 

    

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
    double time = omp_get_wtime();
    
    // ==================================== Envoie du travail
    unsigned char * greyInputImg  = greyScaleRef(inputImg,  inputImgWidth, inputImgHeight);
    unsigned char * greySearchImg = greyScaleRef(searchImg, searchImgWidth , searchImgHeight);            


    unsigned int * sizes = (unsigned int *)malloc( 4*sizeof(unsigned int) );
    sizes[0] = inputImgWidth;
    sizes[1] = inputImgHeight;
    sizes[2] = searchImgWidth;
    sizes[3] = searchImgHeight;
     
    MPI_Bcast(sizes, 4, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    // Broadcast des deux images
    MPI_Bcast(greyInputImg, inputImgWidth * inputImgHeight, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    MPI_Bcast(greySearchImg, searchImgWidth * searchImgHeight, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    
    // On envoie un travail initial à tout le monde.
    unsigned int x = 0;
    unsigned int y = 0;
    uint64_t min = UINT64_MAX;
    unsigned int xBest = 0;
    unsigned int yBest = 0;
 
     
    MPI_Status status;
    ////////////////////////////////////////////////////
    // Pseudocode:

    // On entre dans la boucle de travail
    // Tant qu'il y a du travail a effectuer 
        // On effectue le travail sur le master
        // On regarde s'il y a un client qui a terminer son travail (non bloquant)
        // Et on lui envoie un nouveau travail
    // Lorsqu'on a pu de travail, on a finit
    // Donc on envoie un signal de fin à tout le monde, 
        // Et on attends leurs reponse individuel
        // A chaque réponse, on compare et stocke le meilleurs minimum
    
    while (1) {  
        // Pour chaques clients
        for (int i = 1; i < world_size; i++) {
            MPI_Request req; 
            int _;
            // On regarde si le client nous a demander un travail
            printf("machine %i \n",i);
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
            if (! askForTask){
                
                printf(" machine %i is busy \n",i);
                continue;

            }
            

            // On verifie qu'il nous reste du travail
            if (x >= inputImgWidth - searchImgWidth && y >= inputImgHeight - searchImgHeight) 
                break; // Ça nous sort de toutes les boucles quand on y pense
            
            
            // S'il nous reste du travail on l'envoie au client
            unsigned int * coords = (unsigned int *)malloc(2 * sizeof(unsigned int));
            coords[0] = x; coords[1] = y;
            
            printf("send to machine %i \n",i);
            MPI_Send(&coords, 2, MPI_INT, i, DATA, MPI_COMM_WORLD);
            
            x ++;
            if (x > inputImgWidth - searchImgWidth) {
                y ++;
                x = 0;
            } 
        }   
        // On revérifie qu'il nous reste du travail
        if (x >= inputImgWidth - searchImgWidth && y >= inputImgHeight - searchImgHeight) 
            break;

        // On effectue le travail        
        uint64_t currMin = evaluatorRef(x, y, greyInputImg , inputImgWidth, inputImgHeight, greySearchImg, searchImgWidth, searchImgHeight);
        if (min > currMin) {
            min = currMin;
            xBest = x;
            yBest = y;
        }
        // On passe au prochain travail
        x ++;
        if (x > inputImgWidth - searchImgWidth) {
            y ++;
            x = 0;
        }
    }  

    ////////////////////////////////////////////////////
    // On recupère tout les resultat
    // Pour chaque client
    for (int i = 1; i < world_size; i++){
        int n;
        // On envoie un signal de fin a tout le monde.
        MPI_Send(
        /* data         = */ &n, 
        /* count        = */ 1, 
        /* datatype     = */ MPI_INT, 
        /* source       = */ i, 
        /* tag          = */ END, 
        /* communicator = */ MPI_COMM_WORLD
        );
        
        uint64_t * result = (uint64_t *)malloc(3 * sizeof(uint64_t));  // [x, y, minSSD]
        // Et on attends leurs réponse individuel
        MPI_Recv(
        /* data         = */ &result, 
        /* count        = */ 3, 
        /* datatype     = */ MPI_INT, 
        /* source       = */ i, 
        /* tag          = */ MPI_ANY_TAG, 
        /* communicator = */ MPI_COMM_WORLD, 
        /* status       = */ MPI_STATUS_IGNORE
        );

        // À chaque réponse, on compare et stocke le meilleurs minimum
        if (result[2] < min) {
            xBest = result[0];
            yBest = result[1];
            min = result[2];
        }
    }
     

    struct point position;
    position.x = xBest;
    position.y = yBest;
    unsigned char *saveExample = (unsigned char *)malloc(inputImgWidth * inputImgHeight * 3 * sizeof(unsigned char));
    memcpy(saveExample, inputImg, inputImgWidth * inputImgHeight * 3 * sizeof(unsigned char) );
    traceRef(saveExample,inputImgWidth, inputImgHeight, position, searchImgWidth, searchImgHeight);


    free(greyInputImg);
    free(greySearchImg);


    //void traceRef(unsigned char * img, unsigned int imgWidth, unsigned int imgHeight,  struct point pos , unsigned int imgSearchWidth, unsigned int imgSearchHeight);

    stbi_write_png("img/save_example.png", inputImgWidth, inputImgHeight, 3, saveExample, inputImgWidth*3);
    
    stbi_image_free(inputImg); 
    stbi_image_free(searchImg); 
    
    time = omp_get_wtime()-time;

    printf("Time taken : %f s \n",time);
  
}

void client(int world_rank) {
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
    unsigned char * inputImg;
    unsigned char * searchImg; 
    MPI_Recv(
    /* data         = */ &inputImg, 
    /* count        = */ inputImgWidth * inputImgHeight, 
    /* datatype     = */ MPI_INT, 
    /* source       = */ 0, 
    /* tag          = */ MPI_ANY_TAG, 
    /* communicator = */ MPI_COMM_WORLD, 
    /* status       = */ MPI_STATUS_IGNORE
    );
    MPI_Recv(
    /* data         = */ &searchImg, 
    /* count        = */ searchImgWidth * searchImgHeight, 
    /* datatype     = */ MPI_INT, 
    /* source       = */ 0, 
    /* tag          = */ MPI_ANY_TAG, 
    /* communicator = */ MPI_COMM_WORLD, 
    /* status       = */ MPI_STATUS_IGNORE
    ); 
    
    printf("Image receive on client : %i\n", world_rank);
    MPI_Status status; 
    unsigned int * coords = (unsigned int *)malloc(2 * sizeof(unsigned int));
    uint64_t * result = (uint64_t *)malloc(3 * sizeof(uint64_t));  // [x, y, minSSD]
    result[2] = UINT64_MAX;

    MPI_Recv(&coords, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &status); // Reçoit au plus deux coordonnées. 
    printf("client : %i has received a coordinate\n", world_rank);   
    // Lorsqu'on reçoit du travail du master
    if (status.MPI_TAG == DATA) { 
        // On traite le travail
        unsigned int x = coords[0]; unsigned int y = coords[1];
        uint64_t currMin = evaluatorRef(x, y, inputImg , inputImgWidth, inputImgHeight, searchImg, searchImgWidth, searchImgHeight);
        // On stocke l'évaluation du SSD dans un minimum
        if (currMin < result[2] ) {
          result[0] = x;
          result[1] = y;
          result[2] = currMin;
        }
        // Et on demande au master le reste du travail
        MPI_Send(0, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); 
    } else { // status.MPI_TAG == END
        // Ou lorsque l'on reçoit le signal de fin, on envoie notre résultat (les machines qui n'ont pas travailler renvoit FLOAT_MAX)
        MPI_Send(&result, 3, MPI_INT, 0, 0, MPI_COMM_WORLD); 
    }
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Invalid arguments !\n");
        return EXIT_FAILURE;
    }
    // Initialize the MPI environment
    MPI_Init(NULL, NULL);
    // Find out rank, size
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    double time = 0.0 ;  
    int status = EXIT_SUCCESS;
    printf("Starting...\n");
    // Traitement initial de l'image, et repartition du travail
    if (world_rank == 0) {  
        status = master(world_size, argc, argv);   
    } else { 
        client(world_rank);
    } 
 
    MPI_Finalize();
    

    // Calcul 
    // send
 
    return EXIT_SUCCESS;
}