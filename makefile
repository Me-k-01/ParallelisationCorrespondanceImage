all: main main_mpi_v1 main_mpi_v2 

main: main.c
	gcc -O3 -fopenmp -Wall -o main main.c search_openmp.c search_ref.c -lm 

main_mpi_v1: main_mpi_v1.c
	mpicc -O3 -o ./main_mpi_v1 ./main_mpi_v1.c ./search_ref.c ./search_openmp.c -fopenmp -lm

main_mpi_v2: main_mpi_v2.c
	mpicc -O3 -o ./main_mpi_v2 ./main_mpi_v2.c ./search_ref.c ./search_openmp.c -fopenmp -lm

clean: 
	rm -f main main_mpi_v1 main_mpi_v2