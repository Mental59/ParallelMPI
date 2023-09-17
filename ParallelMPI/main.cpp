#include <iostream>
#include <array>
#include "mpi.h"

struct SConnectedNodes
{
	int Left, Right, Up, Down;

	void Print(const int WorldRank)
	{
		std::cout << "WorldRank=" << WorldRank << ": " << "Left=" << Left << ", " << "Right=" << Right << ", " << "Up=" << Up << ", " << "Down=" << Down << std::endl;
	}
};

bool IsFirst(const int WorldRank)
{
	return WorldRank == 0;
}

SConnectedNodes GetConnectedNodes(const int WorldRank, const int WorldSize)
{
	int Left = WorldRank - 1;
	int Right = WorldRank + 1;
	int Up = WorldRank - WorldSize / 2;
	int Down = WorldRank + WorldSize / 2;

	Left = Left >= 0 && Left < WorldSize ? Left : -1;
	Right = Right >= 0 && Right < WorldSize ? Right : -1;
	Up = Up >= 0 && Up < WorldSize ? Up : -1;
	Down = Down >= 0 && Down < WorldSize ? Down : -1;

	return  { Left, Right, Up, Down };
}

void CombineResults(int* Results1, const int* Results2, const int WorldSize)
{
	for (int i = 0; i < WorldSize; i++)
	{
		Results1[i] = Results1[i] != -1 ? Results1[i] : Results2[i];
	}
}

int main(int Argc, char* Argv[])
{
	int WorldRank, WorldSize;

	MPI_Status Statuses[2];

	MPI_Init(&Argc, &Argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &WorldRank);
	MPI_Comm_size(MPI_COMM_WORLD, &WorldSize);

	SConnectedNodes ConnectedNodes = GetConnectedNodes(WorldRank, WorldSize);

	int* ReceivedResults = new int[WorldSize * 2];
	memset(ReceivedResults, -1, sizeof(int) * WorldSize * 2);

	// Receiving Messages
	if (ConnectedNodes.Right != -1)
	{
		MPI_Recv(ReceivedResults, WorldSize, MPI_INT, ConnectedNodes.Right, 1, MPI_COMM_WORLD, Statuses);
	}
	if (ConnectedNodes.Down != -1)
	{
		MPI_Recv(ReceivedResults + WorldSize, WorldSize, MPI_INT, ConnectedNodes.Down, 1, MPI_COMM_WORLD, Statuses + 1);
	}
	ReceivedResults[WorldRank] = WorldRank;
	CombineResults(ReceivedResults, ReceivedResults + WorldSize, WorldSize);

	// Sending Messages
	if (ConnectedNodes.Left != -1)
	{
		MPI_Send(ReceivedResults, WorldSize, MPI_INT, ConnectedNodes.Left, 1, MPI_COMM_WORLD);
	}
	if (ConnectedNodes.Up != -1)
	{
		MPI_Send(ReceivedResults, WorldSize, MPI_INT, ConnectedNodes.Up, 1, MPI_COMM_WORLD);
	}

	if (IsFirst(WorldRank))
	{
		for (int i = 0; i < WorldSize; i++)
		{
			std::cout << ReceivedResults[i] << " ";
		}
		std::cout << std::endl;
	}

	MPI_Finalize();
	delete[] ReceivedResults;

	return 0;
}