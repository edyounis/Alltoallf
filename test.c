#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "alltoallf.h"

#define BUFFER_SIZE 400000 // TODO make command line argument
#define NUM_DATA    50000 // TODO make command line argument
#define NUM_ITERS   100 // TODO "" ""

double read_timer ( void )
{
	static int initialized = 0;
	static struct timeval start;
	struct timeval end;
	if( !initialized )
	{
		gettimeofday( &start, NULL );
		initialized = 1;
	}
	gettimeofday( &end, NULL );
	return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}

int filter ( uint64_t rank, uint64_t* data )
{
	return *data == rank;
}

uint64_t filter2 ( uint64_t* data )
{
	return *data;
}

int filter_invert ( uint64_t rank, uint64_t* data )
{
	return *data == rank + 2;
}

uint64_t filter2_invert ( uint64_t* data )
{
	return lrand48();
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

	// Reset Data
	for ( uint32_t i = 0; i < NUM_DATA; ++i )
		data[i] = lrand48() % nproc;

	// Start Test alltoallf
	double simulation_time = read_timer();

	for ( uint32_t i = 0; i < NUM_ITERS; ++i )
	{
		alltoallf( data, &data_count, BUFFER_SIZE, buff, BUFFER_SIZE, &filter2, rank, nproc );
		alltoallf( data, &data_count, BUFFER_SIZE, buff, BUFFER_SIZE, &filter2_invert, rank, nproc );
	}

	simulation_time = read_timer( ) - simulation_time;

	printf( "Alltoallf: Rank: %lld, Average Time: %lf seconds\n", rank, simulation_time / (2 * NUM_ITERS) );

	// Reset Data
	data_count = NUM_DATA;

	for ( uint32_t i = 0; i < NUM_DATA; ++i )
		data[i] = lrand48() % nproc;

	// Start Test alltoallv
	double simulation_time2 = read_timer();

	int sendcounts[ nproc ];
	int sdisplps  [ nproc ];
	int recvcounts[ nproc ];

	for ( uint32_t p = 0; p < nproc; ++p )
	{
		recvcounts[p] = BUFFER_SIZE / nproc;
		sdisplps[p]   = BUFFER_SIZE / nproc * rank;
	}

	for ( uint32_t i = 0; i < NUM_ITERS; ++i )
	{
		// Equivalent to filter alltoallf call
		for ( uint32_t p = 0; p < nproc; ++p )
			sendcounts[p] = 0;

		for ( uint32_t j = 0; j < data_count; ++j )
			buff[ sdisplps[ data[j] % nproc ] + sendcounts[ data[j] % nproc ]++ ] = data[j];

		MPI_Alltoallv( buff, sendcounts, sdisplps, MPI_UNSIGNED_LONG, data, recvcounts, sdisplps, MPI_UNSIGNED_LONG, MPI_COMM_WORLD );

		// Equivalent to filter_invert alltoallf call
		for ( uint32_t p = 0; p < nproc; ++p )
			sendcounts[p] = 0;

		for ( uint32_t j = 0; j < data_count; ++j )
			buff[ sdisplps[ (data[j] + 2) % nproc ] + sendcounts[ (data[j] + 2) % nproc ]++ ] = data[j];

		MPI_Alltoallv( buff, sendcounts, sdisplps, MPI_UNSIGNED_LONG, data, recvcounts, sdisplps, MPI_UNSIGNED_LONG, MPI_COMM_WORLD );
	}

	simulation_time2 = read_timer( ) - simulation_time2;

	printf( "Alltoallv: Rank: %lld, Average Time: %lf seconds\n", rank, simulation_time2 / (2 * NUM_ITERS) );

	free( data );
	free( buff );
	MPI_Finalize();
	return 0;
}