# Projet de mise en correspondance d'images
Ce projet a été réalisé dans le cadre de l'UE de Parallélisme et Application.

## Commande de compilation
Pour compiler le programme en version optimisé (-O3) : 
```sh
make
```
3 Fichiers seront généré :
./main -> Pour la version OpenMP ou séquentiel du projet
./main_mpi_v1.c et ./main_mpi_v2.c -> Pour les différentes itérations de la parallélisation avec MPI

## Comment changer les versions des fichiers ?

### Pour les versions séquentielles et OpenMP :
Dans le fichier main.c 
- Pour la version OpenMP -> macro *USE_OPENMP* non commentée
- Pour la version séquentielle -> macro *USE_OPENMP* commentée  

### Pour les versions MPI :
Dans le fichier main_mpi_v1.c 
- Pour la version classique -> macro *USE_OPENMP* commentée  
- Pour la version hybride avec OpenMP -> macro *USE_OPENMP* non commentée 
De même pour main_mpi_v2.c 

## Commande d'execution
Pour le main :
```sh
./main ./img/beach.png ./img/goat.png
```

Pour les versions MPI :
```sh
mpirun -np 1 -hostfile ./hostfile ./main_mpi_v1 ./img/beach.png ./img/goat.png
```


 
