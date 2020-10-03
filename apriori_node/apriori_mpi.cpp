#include <mpi.h>
#include <iostream>
#include <cstring>
#include <cstdlib>

#include <unordered_map>

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
struct ItemMap
{
    int GetId(const std::string& item)
    {
        auto it = m_ItemToId.find(item);
        if (it == m_ItemToId.end())
        {
            it = m_ItemToId.emplace(item, m_NextId++).first;
            m_IdToItem.emplace(it->second, item);
        }
        return it->second; 
    }

    const std::string* GetItem(int id) const
    {
        auto it = m_IdToItem.find(id);
        return it != m_IdToItem.end() ? &(it->second) : nullptr;
    }

    int m_NextId = 0;
    std::unordered_map<std::string, int> m_ItemToId;
    std::unordered_map<int, std::string> m_IdToItem;
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