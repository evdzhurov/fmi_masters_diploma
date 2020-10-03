#include <mpi.h>
#include <iostream>
#include <cstring>
#include <cstdlib>

///////////////////////////////////////////////////////////////////////////////////////////
struct Params
{
    int     m_MaxK = 2;
    float   m_MinSup = 0.05f;
    float   m_MinConf = 0.8f;

    void Print()
    {
        std::cout << "Params: (";
        std::cout << "max_k = " << m_MaxK << ", ";
        std::cout << "min_sup = " << m_MinSup << ", ";
        std::cout << "min_conf = " << m_MinConf << ")\n";
    }
};

///////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int size = 0;
    int rank = 0;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::cout << "\n======================= MPI INITIALIZED =======================\n";
    std::cout << "MPI rank " << rank << '/' << size << '\n';

    Params params;

    // Read command line arguments
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--max_k") == 0 && i + 1 < argc)
        {
            params.m_MaxK = std::atoi(argv[i + 1]);
            ++i;
        }
        else if (strcmp(argv[i], "--min_conf") == 0 && i + 1 < argc)
        {
            params.m_MinConf = std::atof(argv[i + 1]);
            ++i;
        }
        else if (strcmp(argv[i], "--min_sup") == 0 && i + 1 < argc)
        {
            params.m_MinSup = std::atof(argv[i + 1]);
        }
    }

    params.Print();

    MPI_Finalize();
    std::cout << "\n======================= MPI FINALIZED =======================\n\n";

    return 0;
}