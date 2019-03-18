#ifndef __ALL_TO_ALL_LIB_
#define __ALL_TO_ALL_LIB_

#include <stdint.h>

// This function is from kuroi neko on
// https://stackoverflow.com/questions/21442088/computing-the-floor-of-log-2x-using-only-bitwise-operators-in-c
int log2_floor ( unsigned x )
{
	#define MSB_HIGHER_THAN(n) (x &(~((1<<n)-1)))
	int res = 0;
	if MSB_HIGHER_THAN(16) {res+= 16; x >>= 16;}
	if MSB_HIGHER_THAN( 8) {res+=  8; x >>=  8;}
	if MSB_HIGHER_THAN( 4) {res+=  4; x >>=  4;}
	if MSB_HIGHER_THAN( 2) {res+=  2; x >>=  2;}
	if MSB_HIGHER_THAN( 1) {res+=  1;}
	return res;
}

// This function is from dgobbi on
// https://stackoverflow.com/questions/3272424/compute-fast-log-base-2-ceiling
int log2_ceil ( unsigned long long x )
{
	static const unsigned long long t[6] = {
		0xFFFFFFFF00000000ull,
		0x00000000FFFF0000ull,
		0x000000000000FF00ull,
		0x00000000000000F0ull,
		0x000000000000000Cull,
		0x0000000000000002ull
  	};

	int y = (((x & (x - 1)) == 0) ? 0 : 1);
	int j = 32;
	int i;

	for ( i = 0; i < 6; ++i )
	{
		int k = ( ( (x & t[i]) == 0 ) ? 0 : j );
		y += k;
		x >>= k;
		j >>= 1;
	}

	return y;
}

int alltoallf
(
	uint64_t* data, uint64_t* data_count, uint64_t data_size,	// Send/Recv Data Buffer
	uint64_t* buffer, uint64_t buffer_size,						// Extra Buffer
	int (*filter)( uint64_t, uint64_t* ),						// Filter function
	uint64_t rank, uint64_t nproc								// Process Information
)
{
	int depth = log2_ceil( (uint64_t) nproc );
	int shift = 1 << ( log2_floor( (uint32_t) nproc ) );

	for ( uint32_t i = 0; i < depth; ++i )
	{
		// Calculate rank of partner
		uint32_t partner = (rank >> i) % 2 == 0 ? rank + (1 << i) : rank - (1 << i);

		if ( partner >= nproc )			// If my partner doesn't exist
			partner = partner % shift;	// map to proc playing role of partner

		if ( partner == rank )			// If I play the role of my partner
			continue;					// I am done

		// Split data up
		uint32_t nmine = 0;  // Number of data points for me
		uint32_t nprtn = 0;  // Number of data points for my partner

		if ( ( rank >> i ) % 2 == 0 ) // TODO REMOVE on left side
		{
		//	printf("Rank: %d Data_count: %lld\n", rank, *data_count );
			for ( uint64_t data_index = 0; data_index < *data_count; ++data_index )
			{
				uint8_t accounted_for = 0; // Bit field

				for ( uint64_t process_index = 0; process_index < nproc && accounted_for != 0b11; ++process_index )
				{
					//printf( "Rank: %d, Filter_Value: %d\n", rank, (*filter)( process_index, data + data_index ) );
					if ( (*filter)( process_index, data + data_index ) )
					{
		//printf("Rank: %d, Data_Count: %lld, NMINE: %lld, NPRTN: %lld\n", rank, *data_count, nmine, nprtn );
						if ( ( accounted_for & 0b10 ) == 0 && (process_index >> i) % 2 == 0 ) // TODO REMOVE also on left side
						{
							accounted_for |= 0b10;
							data[ nmine++ ] = data[ data_index ];
						}
						if ( ( accounted_for & 0b01 ) == 0 && (process_index >> i) % 2 == 1 )
						{
							accounted_for |= 0b01;
							buffer[ nprtn++ ] = data[ data_index ];
						}
					}
				}
			}
		}
		else	// TODO REMOVE on right side
		{
			for ( uint64_t data_index = 0; data_index < *data_count; ++data_index )
			{
				uint8_t accounted_for = 0; // Bit field

				for ( uint64_t process_index = 0; process_index < nproc && accounted_for != 0b11; ++process_index )
				{
					if ( (*filter)( process_index, data + data_index ) )
					{
						if ( ( accounted_for & 0b10 ) == 0 && (process_index >> i) % 2 == 0 ) // TODO REMOVE on left side
						{
							accounted_for |= 0b10;
							buffer[ nprtn++ ] = data[ data_index ];
						}
						if ( ( accounted_for & 0b01 ) == 0 && (process_index >> i) % 2 == 1 )
						{
							accounted_for |= 0b01;
							data[ nmine++ ] = data[ data_index ];
						}
					}
				}
			}
		}

		MPI_Status status;
		uint32_t recv_count;
		MPI_Sendrecv( buffer, nprtn, MPI_UNSIGNED_LONG,
					  partner, 0, (data + nmine), (data_size - nmine), MPI_UNSIGNED_LONG,
					  partner, 0, MPI_COMM_WORLD, &status );

		MPI_Get_count( &status, MPI_UNSIGNED_LONG, &recv_count );
		*data_count = nmine + recv_count;

		// If I play the role of another processor
		if ( rank >= nproc % shift && rank+shift < (1 << depth) )
		{
			int new_rank    = rank + shift;
			int new_partner = (new_rank >> i) % 2 == 0 ? new_rank + (1 << i) : new_rank - (1 << i);

			// If new partner exists
			if ( new_partner < nproc )
			{
				// Receive additional data, send nothing
				MPI_Status status;
				int recv_count;
				MPI_Sendrecv( buffer, 0, MPI_UNSIGNED_LONG, new_partner, 0,
							  (data + *data_count), (data_size - *data_count), MPI_UNSIGNED_LONG,
							  new_partner, 0, MPI_COMM_WORLD, &status );

				MPI_Get_count( &status, MPI_UNSIGNED_LONG, &recv_count );
				*data_count += recv_count;
			}
		}
	}
}

#endif