//
// PRL projekt 3: Mesh Multiplication
// Autor: Jakub Pastuszek (xpastu00)
// Description: Product of two matrices with using OpenMPI
//

#include <mpi.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <math.h> 

using namespace std;

const char * input1 = "mat1";
const char * input2 = "mat2";

#define REG_EMPTY (-1)
#define REG_INIT 0

#define TAG_REG_A 0
#define TAG_REG_B 1
#define TAG_REG_C 2

int main(int argc, char *argv[])
{
    int procs;
    int myId;
	
	int value;
	vector<int> arrA, arrB;
	int *arrC;

	int numRows;
	int numCols;
	int n;

	int i, j, k;
	bool first=true;
	
	int reg_a = REG_EMPTY;
    int reg_b = REG_EMPTY;
	int reg_c = REG_INIT;
	
    MPI_Status stat;
	MPI_Request request;

    //MPI INIT
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myId);

	int distribProc = procs-1;
	
	//Load both files by distrib proc
	if ( myId == distribProc ) 
	{
        fstream fin;
		string line;

        fin.open(input1, ios::in);

		while( getline( fin, line ) )
		{
			istringstream iss( line );

			while ( iss >> value )
	   		{
				if ( first )
				{
					//get first num from input1 as number of rows
					numRows = value;
					first = false;
					break;
				}

				//make array of values by rows value by value
				arrA.push_back(value);
	        }
	    }

        fin.close();

        first = true;

        fin.open(input2, ios::in);

		while( getline( fin, line ) )
		{
			istringstream iss( line );

			while ( iss >> value )
	   		{
				if ( first )
				{
					//get first num from input1 as number of cols
					numCols = value;
					first = false;
					break;
				}

				//make array of values by rows value by value
				arrB.push_back(value);
	        }       	
	    }

        fin.close();

		//calculate N and verify if both matrices has that value same
        n = arrA.size()/numRows;

		if ( (float)arrA.size()/numRows != (float)arrB.size()/numCols ) 
		{ 
			cerr << "Wrong input matrix" << endl; 
			MPI_Abort(MPI_COMM_WORLD, 1); 
		}
    }

	/*
    //Měření času
    double start, end;
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();
	*/

	//send all processors loaded metadata
	MPI_Bcast(&numRows, 1, MPI_INT, distribProc, MPI_COMM_WORLD);
	MPI_Bcast(&numCols, 1, MPI_INT, distribProc, MPI_COMM_WORLD);
	MPI_Bcast(&n, 1, MPI_INT, distribProc, MPI_COMM_WORLD);

	//distrib proc has to distribute values
	if ( myId == distribProc ) 
	{
		//new array for final gather
		arrC = new (nothrow) int[procs];

		k=0;

		//send A values
		for ( i=0; i < numRows; ++i )
		{
			for ( j=0; j < n; ++j )
			{
				//cout << "A " << i << ": " << arrA[k++] << endl;

				MPI_Send(&arrA[k++], 1, MPI_INT, i, TAG_REG_A, MPI_COMM_WORLD);	
			}
	    }

		k=0;

		//send B values
		for ( j=0; j < n; ++j )
		{
			for ( i=0; i < numCols; ++i )
			{
				//cout << "B " << i*numRows << ": " << arrB[k++] << endl;

				MPI_Send(&arrB[k++], 1, MPI_INT, i*numRows, TAG_REG_B, MPI_COMM_WORLD);	    	
			}
	    }
    }

	//get know which processor send me data
	int recvA = myId-numRows;
	int recvB = myId-1;

	if ( myId < numRows )
	{
		recvA = distribProc;
	}
	if ( myId % numRows == 0 )
	{
		recvB = distribProc;
	}

	//loop for all processors
	for ( i = 0; i < n; ++i )
	{
		//receive both values A, B
        MPI_Recv(&reg_a, 1, MPI_INT, recvA, TAG_REG_A, MPI_COMM_WORLD, &stat);
		MPI_Recv(&reg_b, 1, MPI_INT, recvB, TAG_REG_B, MPI_COMM_WORLD, &stat);
		
		//cout << "RecA " << myId << ": " << reg_a << endl;
		//cout << "RecB " << myId << ": " << reg_b << endl;

		//make a product of A and B and add to C
		reg_c += reg_a*reg_b;
		
		//send A, B to appropriate processor
		if ( myId < procs - numRows )
		{
			//cout << "SendA " << myId << " to: " << myId+numRows << endl;
			MPI_Send(&reg_a, 1, MPI_INT, myId+numRows, TAG_REG_A, MPI_COMM_WORLD);
		}
		
		if ( myId % numRows != numRows-1 )
		{
			//cout << "SendB " << myId << " to: " << myId+1 << endl;
			MPI_Send(&reg_b, 1, MPI_INT, myId+1, TAG_REG_B, MPI_COMM_WORLD);
		}
	}	

	//gather result from all processors
	MPI_Gather(&reg_c,1,MPI_INT,arrC,1,MPI_INT,distribProc,MPI_COMM_WORLD);

	/*
    //Měření času
    MPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();
    if (myId == 0) 
	{
        //cerr << "Exec time: " << end-start << endl;
        cerr << end-start << endl;
    }
	*/
	
	//////////////////
	//Print out result

	if ( myId == distribProc ) 
	{
		cout << numRows << ":" << numCols << endl;

		for ( i = 0; i < numRows; ++i )
		{
			for ( j = 0; j < numCols; ++j )
			{
				if ( j != 0 ) { cout << " "; }

				cout << arrC[i+j*numRows];
			}

			cout << endl;
		}
	}

	//End print

	//cout << myId << ": " << reg_c << endl;

    MPI_Finalize();

    return 0;
}