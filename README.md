## Commande de compilation
- Openmp :
```sh
gcc -fopenmp -Wall -o main main.c search_openmp.c search_ref.c -lm
```
- MPI :
```
mpicc -o ./main_mpi ./main_mpi_v1.c ./search_ref.c -fopenmp -lm
```

## Commande d'execution
- Openmp :
```sh
./main ./img/beach.png ./img/goat.png
```

- MPI :
```sh
mpirun -np 1 -host fst-o-i-212-02.unilim.fr ./main_mpi ./img/beach.png ./img/goat.png
```



mpicc -O3 -o ./main_mpi ./main_openmp_mpi.c ./search_openmp.c ./search_ref.c -fopenmp -lm && mpirun -np 2 -host fst-o-i-212-02.unilim.fr,fst-o-i-212-05.unilim.fr ./main_mpi ./img/space.png ./img/goat.png
