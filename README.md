## Commande de compilation
- Openmp :
```sh
gcc -fopenmp -Wall -o main main.c search_openmp.c search_ref.c -lm
```

## Commande d'execution
- Openmp :
```sh
./main ./img/beach.png ./img/goat.png
```