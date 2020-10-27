
#include <cstring>
#include <cstdlib>
#include <cassert>

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <sstream>
#include <algorithm>

#include <mpi.h>

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
        ss << '{' << m_Antidecent.c_str() << "} => {" << m_Consequent.c_str() << '}';
        return ss.str();
    }

    std::string m_Antidecent;
    std::string m_Consequent;
    float m_Confidence = 0.f;
    float m_Lift = 0.f;
};
using Rules = std::vector<Rule>;

///////////////////////////////////////////////////////////////////////////////////////////
using InputData = std::vector<std::vector<std::string>>; // Raw transactions
using OutputData = std::vector<std::string>;

static const InputData k_SampleData = {
    {"milk", "cheese", "oats", "carrots"},
    {"cheese", "oats", "onions" },
    {"milk", "cheese", "bread"}
};

FrequentItemsts Apriori(const InputData& data, const Params& params, const MPIContext& ctx)
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
        int first = ctx.m_Rank * fsets.m_NumTrans / ctx.m_Size;
        int last = (ctx.m_Rank + 1) * fsets.m_NumTrans / ctx.m_Size;
        auto counts = fsets.m_KthItemsetCounts[k];

        for (int i = first; i < last; ++i)
        {
            const auto& t = transactions[i];

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
    auto gather_k = [&](int k)
    {
        auto& counts = fsets.m_KthItemsetCounts[k];
        
        // Reduce scatter
        const int size = counts.size();
        const int size_part = size / ctx.m_Size;
        std::vector<int> sizes(ctx.m_Size, size_part);
        
        // Distribute remainder
        for (int i = 0; i < size % ctx.m_Size; ++i) 
            sizes[i] += 1;

        std::vector<int> localCounts;
        localCounts.reserve(size);

        for (const auto& kvp : counts)
            localCounts.push_back(kvp.second);

        std::vector<int> globalCountsForRank(sizes[ctx.m_Rank]);

        MPI_Reduce_scatter(
            localCounts.data(), /*sendbuf*/
            globalCountsForRank.data(), /*recvbuf*/
            sizes.data(), /*recvcounts*/
            MPI_INT,
            MPI_SUM,
            MPI_COMM_WORLD
        );

        // All gather
        std::vector<int> globalCounts(size);
        std::vector<int> globalCountsOffsets;

        int offsetSum = 0;
        for (const auto& size : sizes)
        {
            globalCountsOffsets.push_back(offsetSum);
            offsetSum += size;
        }

        MPI_Allgatherv(
            globalCountsForRank.data(), /*sendbuf*/
            globalCountsForRank.size(), /*sendcount*/
            MPI_INT,
            globalCounts.data(), /*recvbuf*/
            sizes.data(), /*recvcounts*/
            globalCountsOffsets.data(), /*displacements*/
            MPI_INT,
            MPI_COMM_WORLD
        );

        // Update final counts assuming dict is ordered
        int i = 0;
        for (auto it = counts.begin(); it != counts.end(); ++it, ++i)
        {
            it->second = globalCounts[i];
        }
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

    return fsets;
}

///////////////////////////////////////////////////////////////////////////////////////////
void GenerateRulesR(const Itemset& lhs, const Itemset& rhs, Rules& rules, float support, const FrequentItemsts& fsets, const Params& params)
{
    auto subsetsOf = [](const Itemset& itemset) -> Itemsets
    {
        Itemsets result;
        int size = itemset.Size();
        for (int i = 0; i < size; ++i)
        {
            Itemset subset;
            subset.m_Items.reserve(size-1);
            auto first = std::begin(itemset.m_Items);
            auto leaveOneOut = first + i;
            auto last = std::end(itemset.m_Items);
            std::copy(first, leaveOneOut, std::back_inserter(subset.m_Items));
            std::copy(leaveOneOut + 1, last, std::back_inserter(subset.m_Items));
            result.emplace_back(std::move(subset));
        }
        return result;
    };

    auto subtract = [](const Itemset& lhs, const Itemset& rhs) -> Itemset
    {
        Itemset result;
        std::set_difference(
            std::begin(lhs.m_Items), std::end(lhs.m_Items),
            std::begin(rhs.m_Items), std::end(rhs.m_Items),
            std::back_inserter(result.m_Items));

        return result;
    };

    for (const auto& itemset : subsetsOf(rhs))
    {
        float conf = support / fsets.GetSupport(itemset);
        if (conf >= params.m_MinConf)
        {
            Rule rule;
            rule.m_Antidecent = itemset.ToString();
            Itemset consequent = subtract(lhs, itemset);
            rule.m_Consequent = consequent.ToString();
            rule.m_Confidence = conf;
            rule.m_Lift = conf / fsets.GetSupport(consequent);

            if (itemset.m_Items.size() > 1)
                GenerateRulesR(lhs, itemset, rules, support, fsets, params);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
std::vector<Rule> GenerateRules(const FrequentItemsts& fsets, const Params& params, const MPIContext& ctx)
{
    std::vector<Rule> rules;

    for (int i = 1; i <= params.m_MaxK; ++i)
    {
        // Partition itemsets equally
        auto it = fsets.m_KthItemsetCounts.find(i);
        if (it == fsets.m_KthItemsetCounts.end())
        {
            assert(!"k-th itemsets not found!");
            return rules;
        }

        const auto& itemsets = it->second;

        int numItemsets = itemsets.size();
        int first = ctx.m_Rank * numItemsets / ctx.m_Size;
        int last = (ctx.m_Rank + 1) * numItemsets / ctx.m_Size;

        auto firstIt = itemsets.begin();
        std::advance(firstIt, first);

        auto lastIt = firstIt;
        std::advance(firstIt, last - first);
        
        for (;firstIt != lastIt; ++firstIt)
        {
            const auto& itemset = firstIt->first;
            float support = fsets.GetSupport(itemset);
            GenerateRulesR(itemset, itemset, rules, support, fsets, params);
        }
    }

    return rules;
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

    auto fsets = Apriori(k_SampleData, params, ctx);
    auto rules = GenerateRules(fsets, params, ctx);

    MPI_Finalize();
    std::cout << "\n======================= MPI FINALIZED =======================\n\n";

    return 0;
}