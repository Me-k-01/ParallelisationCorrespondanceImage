## Commande de compilation
- Openmp :
```sh
gcc -fopenmp -Wall -o main main.c search_openmp.c search_ref.c -lm
```
- MPI :
```
mpicc -o ./main_mpi ./main_mpi_v1
```

## Commande d'execution
- Openmp :
```sh
./main ./img/beach.png ./img/goat.png
```

- MPI :
```sh
mpirun -np 1 -host fst-o-i-212-02.unilim.fr ./main_mpi
```