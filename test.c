#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>

#include "alltoallf.h"

#define BUFFER_SIZE 10000 // TODO make command line argument
#define NUM_DATA    8000 // TODO make command line argument

int filter ( uint64_t rank, uint64_t* data )
{
	return *data == rank;
}

int main ( int argc, char** argv )
{
	int nproc;
	int rank;

	MPI_Init( &argc, &argv );
	MPI_Comm_size( MPI_COMM_WORLD, &nproc );
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );

	// Initialize and Scatter Data
	uint64_t* data = (uint64_t*) malloc( BUFFER_SIZE * sizeof(uint64_t) );
	uint64_t* buff = (uint64_t*) malloc( BUFFER_SIZE * sizeof(uint64_t) );

	uint64_t data_count = NUM_DATA;

	for ( uint32_t i = 0; i < NUM_DATA; ++i )
		data[i] = lrand48() % nproc;

	alltoallf( data, &data_count, BUFFER_SIZE, buff, BUFFER_SIZE, &filter, rank, nproc );

	printf( "Rank: %lld, data_count: %lld\n", rank, data_count );

	// if ( rank == 3 )
	// 	for ( int i = 0; i < data_count; ++i )
	// 		printf("%lld\n", data[i]);

	free( data );
	free( buff );
	MPI_Finalize();
	return 0;
}