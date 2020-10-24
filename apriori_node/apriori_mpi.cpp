#include <mpi.h>
#include <iostream>
#include <cstring>
#include <cstdlib>

#include <vector>
#include <map>
#include <unordered_map>
#include <sstream>
#include <algorithm>

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
struct MPIContext
{
    int m_Size = 0;
    int m_Rank = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////
struct ItemMap
{
    int GetOrCreateId(const std::string& item)
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

    void Print() const
    {
        std::cout << "ItemMap:\n";
        for (const auto& pair : m_ItemToId)
        {
            std::cout << '\t' << pair.first << " -> " << pair.second << '\n';
        }
        std::cout << '\n';
    }

    int m_NextId = 0;
    std::unordered_map<std::string, int> m_ItemToId;
    std::unordered_map<int, std::string> m_IdToItem;
};

///////////////////////////////////////////////////////////////////////////////////////////
struct Itemset
{
    Itemset() = default;
    explicit Itemset(int item) : m_Items{item} {}; // Single item initialization

    std::size_t Size() const { return m_Items.size(); }

    bool operator<(const Itemset& other) const
    {
        if (Size() != other.Size())
            return Size() < other.Size();

        for (int i = 0; i < Size(); ++i)
        {
            if (m_Items[i] < other.m_Items[i])
            {
                return true;
            }
        }

        return false;
    }

    void FromString(const std::string& str)
    {
        m_Items.clear();
        std::stringstream ss(str);
        std::string token;
        while(std::getline(ss, token, '|'))
        {
            m_Items.push_back(std::stoi(token));
        }
    }

    std::string ToString() const
    {
        if (m_Items.empty()) return "";
        std::stringstream ss;
        for (int i = 0; i < m_Items.size() - 1; ++i)
            ss << m_Items[i] << '|';
        ss << m_Items.back();
        return ss.str();
    }

    std::vector<int> m_Items;
};

using Itemsets = std::vector<Itemset>;
using ItemsetCounts = std::map<Itemset, int>;

///////////////////////////////////////////////////////////////////////////////////////////
struct FrequentItemsts
{
    float GetSupport(const Itemset& itemset) const
    {
        if (m_NumTrans == 0) return 0.f;

        auto itCounts = m_KthItemsetCounts.find(itemset.Size());
        if (itCounts == m_KthItemsetCounts.end()) return 0.f;

        auto itItemset = itCounts->second.find(itemset);
        if (itItemset == itCounts->second.end()) return 0.f;
        
        return itItemset->second / (float)m_NumTrans;
    }

    ItemMap m_ItemMap;
    std::unordered_map<int, ItemsetCounts> m_KthItemsetCounts;
    int m_NumTrans = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////
struct Rule
{
    std::string ToString() const
    {
        std::stringstream ss;
        ss << '{' << m_Antidecent.ToString().c_str();
        ss << "} => {" << m_Consequent.ToString().c_str() << '}';
        return ss.str();
    }

    Itemset m_Antidecent;
    Itemset m_Consequent;
};

///////////////////////////////////////////////////////////////////////////////////////////
using InputData = std::vector<std::vector<std::string>>; // Raw transactions
using OutputData = std::vector<std::string>;

static const InputData k_SampleData = {
    {"milk", "cheese", "oats", "carrots"},
    {"cheese", "oats", "onions" },
    {"milk", "cheese", "bread"}
};

OutputData Apriori(const InputData& data, const Params& params, const MPIContext& ctx)
{
    FrequentItemsts fsets;
    fsets.m_NumTrans = data.size();

    std::vector<std::vector<int>> transactions;

    // Map input data to transactions
    for (const auto& row : data)
    {
        transactions.emplace_back();
        for (const auto& item : row)
        {
            transactions.back().emplace_back(fsets.m_ItemMap.GetOrCreateId(item));
        }
    }

    fsets.m_ItemMap.Print();

    ///////////////////////////////////////////////////////////////////////////////////////////
    auto count = [&](const Itemsets& itemsets, int k) 
    {
        //first = mpi_rank * t_count // mpi_size
        //last = (mpi_rank + 1) * t_count // mpi_size
        auto counts = fsets.m_KthItemsetCounts[k];
        for (const auto& t : transactions/*[first:last]*/)
        {
            if (t.size() < k) continue; // Transaction cannot contain itemset
            for (const auto& itemset : itemsets)
            {
                bool found = true;
                for (const auto& item : itemset.m_Items)
                {
                    if (std::find(t.begin(), t.end(), item) == t.end())
                    {
                        found = false;
                        break;
                    }
                }

                if (found)
                {
                    counts[itemset] += 1;
                }
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////
    auto gather_1 = [&]()
    {
        auto& counts = fsets.m_KthItemsetCounts[1];

        std::vector<int> globalCountSizes(ctx.m_Size);
        std::vector<int> countOffsets;
        countOffsets.reserve(ctx.m_Size);
        
        // Calculate total size of counts across processes
        const int localCountSize = counts.size() * 2;

        MPI_Allgather(
            &localCountSize,
            1,
            MPI_INT,
            globalCountSizes.data(),
            globalCountSizes.size(),
            MPI_INT,
            MPI_COMM_WORLD);

        // Create offsets from running sum of global counts size
        int globalCountsSize = 0;
        for (int size : globalCountSizes)
        {
            countOffsets.push_back(globalCountsSize);
            globalCountsSize += size;
        }

        // Gather all counts
        std::vector<int> localCounts;
        localCounts.reserve(localCountSize);

        for (const auto& kvp : counts)
        {
            localCounts.push_back(kvp.first.m_Items.front());
            localCounts.push_back(kvp.second);
        }

        std::vector<int> globalCounts(globalCountsSize);

        MPI_Allgatherv(
            localCounts.data(),
            localCounts.size(),
            MPI_INT,
            globalCounts.data(),
            globalCountSizes.data(),
            countOffsets.data(),
            MPI_INT,
            MPI_COMM_WORLD);
        

        // Merge counts from all processes into local data
        counts.clear();
        for (int i = 1; i < globalCountsSize; ++i)
        {
            counts[Itemset(globalCounts[i-1])] += globalCounts[i];
        }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////
    auto gather_k = [](int k)
    {
    };

    ///////////////////////////////////////////////////////////////////////////////////////////
    auto prune = [&](const std::vector<Itemset>& itemsets, int k)
    {
        std::vector<Itemset> result;
        auto& counts = fsets.m_KthItemsetCounts[k];
        for (const auto& itemset : itemsets)
        {
            if (counts[itemset] / (float)fsets.m_NumTrans < params.m_MinSup)
            {
                counts.erase(itemset);
            }
            else
            {
                result.push_back(itemset);
            }
        }
        
        return result;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////
    auto gen_L1 = [&]() -> Itemsets
    {
        std::vector<Itemset> c1;
        c1.reserve(fsets.m_ItemMap.m_ItemToId.size());
        for (const auto& pair: fsets.m_ItemMap.m_ItemToId)
        {
            c1.emplace_back(pair.second);
        }
        count(c1, 1);
        gather_1();
        return prune(c1, 1);
    };

    ///////////////////////////////////////////////////////////////////////////////////////////
    auto gen_Lk = [](const Itemsets& itemsets, int k) -> Itemsets
    {
        return Itemsets();
    };

    ///////////////////////////////////////////////////////////////////////////////////////////
    int k = 2;
    Itemsets L = gen_L1();
    while (L.size() > 0)
    {
        if (params.m_MaxK > 0 and k > params.m_MaxK) break;
        Itemsets C = gen_Lk(L, k);
        count(C, k);
        gather_k(k);
        L = prune(C, k);
        ++k;
    }

    return OutputData();
}

///////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    MPIContext ctx;

    MPI_Comm_size(MPI_COMM_WORLD, &ctx.m_Size);
	MPI_Comm_rank(MPI_COMM_WORLD, &ctx.m_Rank);

    std::cout << "\n======================= MPI INITIALIZED =======================\n";
    std::cout << "MPI rank " << ctx.m_Rank << '/' << ctx.m_Size << '\n';

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

    Apriori(k_SampleData, params, ctx);

    MPI_Finalize();
    std::cout << "\n======================= MPI FINALIZED =======================\n\n";

    return 0;
}