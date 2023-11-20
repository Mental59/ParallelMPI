#include <iostream>
#include <chrono>
#include "mpi.h"

constexpr int MASTER_TAG = 1;
constexpr int WORKER_TAG = 2;
constexpr int MATRIX_SIZES[]{ 64, 128, 256, 512, 1024, 2048 };

struct Matrix
{
	int m_Rows, m_Columns;
	int* m_Data;

	Matrix(int Rows, int Columns) : m_Rows(Rows), m_Columns(Columns)
	{
		m_Data = new int[Rows * Columns];
	}

	void Fill();

	void Print();

	virtual ~Matrix() { delete[] m_Data; }
};

void Master(int WorldRank, int WorldSize);
void Worker(int WorldRank, int WorldSize);
void MatrixMul(const Matrix& A, const Matrix& B, Matrix& C);

int main(int Argc, char* Argv[])
{
	srand(time(0));
	int WorldRank, WorldSize;

	MPI_Init(&Argc, &Argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &WorldRank);
	MPI_Comm_size(MPI_COMM_WORLD, &WorldSize);

	if (WorldRank == 0)
	{
		Master(WorldRank, WorldSize);
	}
	else
	{
		Worker(WorldRank, WorldSize);
	}

	MPI_Finalize();
}

void Matrix::Fill()
{
	for (int i = 0; i < m_Rows * m_Columns; i++)
	{
		m_Data[i] = rand() % 10 - 5;
	}
}

void Matrix::Print()
{
	for (int i = 0; i < m_Rows; i++)
	{
		for (int j = 0; j < m_Columns; j++)
		{
			std::cout << m_Data[m_Columns * i + j] << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

void Master(int WorldRank, int WorldSize)
{
	MPI_Status Status;
	std::chrono::steady_clock::time_point Begin;
	std::chrono::steady_clock::time_point End;

	for (int MatrixSize : MATRIX_SIZES)
	{
		Begin = std::chrono::steady_clock::now();
		int RowsToSend = MatrixSize / (WorldSize - 1);

		Matrix A(MatrixSize, MatrixSize);
		Matrix B(MatrixSize, MatrixSize);
		Matrix C(MatrixSize, MatrixSize);

		A.Fill();
		B.Fill();

		int Offset = 0;
		for (int i = 1; i < WorldSize; i++)
		{
			MPI_Send(A.m_Data + Offset, RowsToSend * MatrixSize, MPI_INT, i, MASTER_TAG, MPI_COMM_WORLD);
			MPI_Send(B.m_Data, MatrixSize * MatrixSize, MPI_INT, i, MASTER_TAG, MPI_COMM_WORLD);

			Offset += RowsToSend * MatrixSize;
		}

		Offset = 0;
		for (int i = 1; i < WorldSize; i++)
		{
			MPI_Recv(C.m_Data + Offset, RowsToSend * MatrixSize, MPI_INT, i, WORKER_TAG, MPI_COMM_WORLD, &Status);
			Offset += MatrixSize * RowsToSend;
		}
		End = std::chrono::steady_clock::now();

		const long long nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds> (End - Begin).count();
		const long double ms = nanoseconds * 1e-6;

		std::cout << "NumWorkerProcesses=" << WorldSize - 1 << " MatrixSize=" << MatrixSize << " RunTime=" << ms << "ms" << std::endl;
	}
}

void Worker(int WorldRank, int WorldSize)
{
	MPI_Status Status;

	for (int MatrixSize : MATRIX_SIZES)
	{
		int RowsToReceive = MatrixSize / (WorldSize - 1);

		Matrix A(RowsToReceive, MatrixSize);
		Matrix B(MatrixSize, MatrixSize);
		Matrix C(RowsToReceive, MatrixSize);

		MPI_Recv(A.m_Data, RowsToReceive * MatrixSize, MPI_INT, 0, MASTER_TAG, MPI_COMM_WORLD, &Status);
		MPI_Recv(B.m_Data, MatrixSize * MatrixSize, MPI_INT, 0, MASTER_TAG, MPI_COMM_WORLD, &Status);

		MatrixMul(A, B, C);

		MPI_Send(C.m_Data, MatrixSize * RowsToReceive, MPI_INT, 0, WORKER_TAG, MPI_COMM_WORLD);
	}
}

void MatrixMul(const Matrix& A, const Matrix& B, Matrix& C)
{
	for (int i = 0; i < A.m_Rows; i++)
	{
		for (int j = 0; j < B.m_Columns; j++)
		{
			C.m_Data[i * C.m_Columns + j] = 0;
			for (int k = 0; k < A.m_Columns; k++)
			{
				C.m_Data[i * C.m_Columns + j] += A.m_Data[i * A.m_Columns + k] * B.m_Data[k * B.m_Columns + j];
			}
		}
	}
}
