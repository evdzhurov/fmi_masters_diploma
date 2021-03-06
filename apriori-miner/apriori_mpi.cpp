
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cctype>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <iomanip>

#include <unistd.h>
#include <mpi.h>

///////////////////////////////////////////////////////////////////////////////////////////
struct Params
{
    Params(int argc, char** argv)
        : m_ArgC(argc)
        , m_ArgV(argv)
    {}

    bool WaitAttach()
    {
        for (int i = 1; i < m_ArgC; ++i)
        {
            if (strcmp(m_ArgV[i], "--wait_attach") == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool Parse()
    {
        // Read command line arguments
        for (int i = 1; i < m_ArgC; ++i)
        {
            if (strcmp(m_ArgV[i], "--input") == 0 && i + 1 < m_ArgC)
            {
                m_InputFile = m_ArgV[i+1];
                ++i;
            }
            else if (strcmp(m_ArgV[i], "--max_k") == 0 && i + 1 < m_ArgC)
            {
                m_MaxK = std::atoi(m_ArgV[i + 1]);
                ++i;
            }
            else if (strcmp(m_ArgV[i], "--min_conf") == 0 && i + 1 < m_ArgC)
            {
                m_MinConf = std::atof(m_ArgV[i + 1]);
                ++i;
            }
            else if (strcmp(m_ArgV[i], "--min_sup") == 0 && i + 1 < m_ArgC)
            {
                m_MinSup = std::atof(m_ArgV[i + 1]);
                ++i;
            }
        }

        if (m_InputFile.empty())
        {
            std::cout << "Usage: --input <file>\n";
            return false;
        }

        return true;
    }

    void Print()
    {
        std::cout << "Params: (";
        std::cout << "max_k = " << m_MaxK << ", ";
        std::cout << "min_sup = " << m_MinSup << ", ";
        std::cout << "min_conf = " << m_MinConf << ")\n";
    }

    int             m_MaxK = 2;
    float           m_MinSup = 0.05f;
    float           m_MinConf = 0.8f;
    std::string     m_InputFile;

    private:
    int     m_ArgC = 0;
    char**  m_ArgV = nullptr;
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
            std::cout << pair.first << ": " << pair.second << '\n';
        }
        std::cout << '\n';
    }

    int m_NextId = 0;
    std::map<std::string, int> m_ItemToId;
    std::map<int, std::string> m_IdToItem;
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
            if (m_Items[i] != other.m_Items[i])
            {
                return m_Items[i] < other.m_Items[i];
            }
        }

        return false; // same
    }

    std::vector<Itemset> Subsets() const
    {
        std::vector<Itemset> result;
        int size = m_Items.size();
        for (int i = 0; i < size; ++i)
        {
            Itemset subset;
            subset.m_Items.reserve(size-1);
            auto first = std::begin(m_Items);
            auto leaveOneOut = first + i;
            auto last = std::end(m_Items);
            std::copy(first, leaveOneOut, std::back_inserter(subset.m_Items));
            std::copy(leaveOneOut + 1, last, std::back_inserter(subset.m_Items));
            result.emplace_back(std::move(subset));
        }
        return result;
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

    std::string ToString(const ItemMap& imap, char delim = ',') const
    {
        if (m_Items.empty()) return "";
        std::stringstream ss;
        ss << '<';

        auto getItemName = [&](int itemId) -> const char*
        {
            auto str = imap.GetItem(itemId);
            return str ? str->c_str() : "MISSING";
        };

        for (int i = 0; i < m_Items.size() - 1; ++i)
        {
            auto str = imap.GetItem(m_Items[i]);
            ss << getItemName(m_Items[i]) << delim;
        }
        ss << getItemName(m_Items.back()) << '>';
        
        return ss.str();
    }

    std::vector<int> m_Items;
};

using Itemsets = std::vector<Itemset>;
using ItemsetCounts = std::map<Itemset, int>;

///////////////////////////////////////////////////////////////////////////////////////////
struct FrequentItemsets
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

    void Print()
    {
        m_ItemMap.Print();
        std::cout << "\nNum transactions: " << m_NumTrans << '\n';
        std::cout << "Itemset counts:\n";

        for (const auto& pair : m_KthItemsetCounts)
        {
            for (const auto& itemsetCount : pair.second)
            {
                std::cout << std::left << std::setw(20) << itemsetCount.first.ToString(m_ItemMap);
                std::cout << std::setprecision(5) << itemsetCount.second / (float)m_NumTrans << '\n';
            }
        }
    }

    bool ToCsv(const std::string& file)
    {
        std::ofstream ofs(file, std::ios::trunc);
        if (!ofs.is_open()) return false;

        ofs << "Itemset, Frequency\n"; // Column Names

        for (const auto& pair : m_KthItemsetCounts)
        {
            for (const auto& itemsetCount : pair.second)
            {
                ofs << itemsetCount.first.ToString(m_ItemMap, ':') << ", ";
                ofs << std::setprecision(5) << itemsetCount.second / (float)m_NumTrans << '\n';
            }
        }
        
        return true;
    }

    ItemMap m_ItemMap;
    std::map<int, ItemsetCounts> m_KthItemsetCounts;
    int m_NumTrans = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////
struct Rule
{
    std::string ToString(const FrequentItemsets& fsets) const
    {
        std::stringstream ss;
        ss << std::left << std::setw(20) << m_Antidecent.ToString(fsets.m_ItemMap).c_str() << " => ";
        ss << std::left << std::setw(20) << m_Consequent.ToString(fsets.m_ItemMap).c_str();
        ss << " | " << std::left << std::setprecision(5) << std::setw(10) << m_Confidence;
        ss << " | " << std::left << std::setprecision(5) << std::setw(10) << m_Lift;
        return ss.str();
    }

    Itemset m_Antidecent;
    Itemset m_Consequent;
    float m_Confidence = 0.f;
    float m_Lift = 0.f;
};
using Rules = std::vector<Rule>;

///////////////////////////////////////////////////////////////////////////////////////////
using InputData = std::vector<std::vector<std::string>>; // Raw transactions
using OutputData = std::vector<std::string>;

FrequentItemsets Apriori(const InputData& data, const Params& params, const MPIContext& ctx)
{
    FrequentItemsets fsets;
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

    ///////////////////////////////////////////////////////////////////////////////////////////
    auto count = [&](const Itemsets& itemsets, int k) 
    {
        int first = ctx.m_Rank * fsets.m_NumTrans / ctx.m_Size;
        int last = (ctx.m_Rank + 1) * fsets.m_NumTrans / ctx.m_Size;
        auto& counts = fsets.m_KthItemsetCounts[k];

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
        for (int i = 1; i < globalCountsSize; i+=2)
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
        for (const auto& pair: fsets.m_ItemMap.m_IdToItem)
        {
            c1.emplace_back(pair.first);
        }
        count(c1, 1);
        gather_1();
        return prune(c1, 1);
    };

    ///////////////////////////////////////////////////////////////////////////////////////////
    auto gen_Lk = [&](const Itemsets& itemsets, int k) -> Itemsets
    {
        Itemsets result;

        const auto& counts = fsets.m_KthItemsetCounts[k-1]; // Counts for k-1 subsets
        
        auto same_prefix = [](const Itemset& lhs, const Itemset& rhs)
        {
            if (lhs.m_Items.size() != rhs.m_Items.size()) return false;
            for (int i = 0; i < lhs.m_Items.size() - 1; ++i)
            {
                if (lhs.m_Items[i] != rhs.m_Items[i]) return false; // Mismatch
            }
            return true; // Same (size - 1) prefix
        };

        for (int i = 0; i < itemsets.size(); ++i)
        {
            const auto& lhs = itemsets[i];

            for (int j = i+1; j < itemsets.size(); ++j)
            {    
                const auto& rhs = itemsets[j];

                // Require same prefix
                if (!same_prefix(lhs, rhs)) break;

                // Create new itemset (lhs + last element of rhs)
                Itemset itemset = lhs;
                itemset.m_Items.push_back(rhs.m_Items.back());

                // Check if all k-1 subsets are frequent
                bool valid = true;
                for (const auto& subset : itemset.Subsets())
                {
                    if (counts.find(subset) == counts.end())
                    {
                        valid = false;
                        break;
                    }
                }

                if (valid) 
                    result.emplace_back(std::move(itemset));
            }
        }

        return result;
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
void GenerateRulesR(const Itemset& lhs, const Itemset& rhs, Rules& rules, float support, const FrequentItemsets& fsets, const Params& params)
{
    auto subtract = [](const Itemset& lhs, const Itemset& rhs) -> Itemset
    {
        Itemset result;
        std::set_difference(
            std::begin(lhs.m_Items), std::end(lhs.m_Items),
            std::begin(rhs.m_Items), std::end(rhs.m_Items),
            std::back_inserter(result.m_Items));

        return result;
    };

    for (const auto& itemset : rhs.Subsets())
    {
        float conf = support / fsets.GetSupport(itemset);
        if (conf >= params.m_MinConf)
        {
            Rule rule;
            rule.m_Antidecent = itemset;
            rule.m_Consequent = subtract(lhs, itemset);
            rule.m_Confidence = conf;
            rule.m_Lift = conf / fsets.GetSupport(rule.m_Consequent);

            rules.push_back(std::move(rule));

            if (itemset.m_Items.size() > 1)
                GenerateRulesR(lhs, itemset, rules, support, fsets, params);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
std::vector<Rule> GenerateRules(const FrequentItemsets& fsets, const Params& params, const MPIContext& ctx)
{
    std::vector<Rule> rules;

    for (int i = 2; i <= params.m_MaxK; ++i)
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
        std::advance(lastIt, last - first);
        
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
bool RulesToCsv(const FrequentItemsets& fsets, const std::vector<Rule> rules, const std::string& file)
{
    if (rules.empty()) return false;

    std::ofstream ofs(file, std::ios::trunc);
    if (!ofs.is_open()) return false;

    ofs << "Antidecent, Consequent, Confidence, Lift\n"; // Columns names
    for (const auto& rule : rules)
    {
        ofs << rule.m_Antidecent.ToString(fsets.m_ItemMap, ':').c_str() << ", ";
        ofs << rule.m_Consequent.ToString(fsets.m_ItemMap, ':').c_str() << ", ";
        ofs << std::setprecision(5) << rule.m_Confidence << ", ";
        ofs << std::setprecision(5) << rule.m_Lift << '\n';
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////
void DebugAttachWait()
{
    volatile int i = 0;
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    printf("PID %d on %s is ready for attach\n", getpid(), hostname);
    fflush(stdout);
    while(i == 0) 
        sleep(5);
}

///////////////////////////////////////////////////////////////////////////////////////////
bool ReadInputData(const std::string& file, InputData& outData)
{
    std::ifstream ifs(file);
    if (!ifs.is_open()) return false;

    for (std::string line; std::getline(ifs, line);)
    {
        outData.emplace_back();
        std::stringstream ss(line);
        for (std::string item; std::getline(ss, item, ',');)
        {
            // Trim leading whitespace
            item.erase(item.begin(), std::find_if(item.begin(), item.end(), [](unsigned char c){
                return !std::isspace(c);
            }));

            // Trim trailing whitespace
            item.erase(std::find_if(item.rbegin(), item.rend(), [](unsigned char c){
                return !std::isspace(c);
            }).base(), item.end());
            
            if (!item.empty())
                outData.back().emplace_back(item);
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    Params params(argc, argv);

    if (params.WaitAttach())
    {
        DebugAttachWait();
    }

    if (!params.Parse())
    {
        std::cout << "Error parsing arguments!\n";
        return 1;
    }
    else
    {
        params.Print();
    }

    InputData samples;
    if (!ReadInputData(params.m_InputFile, samples))
    {
        std::cout << "Error reading input data from file!\n";
        return 1;
    }

    MPI_Init(&argc, &argv);

    MPIContext ctx;
    MPI_Comm_size(MPI_COMM_WORLD, &ctx.m_Size);
	MPI_Comm_rank(MPI_COMM_WORLD, &ctx.m_Rank);

    std::cout << "\n======================= MPI INITIALIZED =======================\n";
    std::cout << "MPI rank " << ctx.m_Rank << '/' << ctx.m_Size << '\n';

    auto fsets = Apriori(samples, params, ctx);

    std::cout << "Frequent Itemsets:\n";
    fsets.Print();
    fsets.ToCsv("frequent_itemsets.csv");

    auto rules = GenerateRules(fsets, params, ctx);

    std::cout << "\nRule | Confidence | Lift\n";
    for (const auto& rule : rules)
    {
        std::cout << rule.ToString(fsets) << '\n';
    }

    RulesToCsv(fsets, rules, "rules.csv");

    MPI_Finalize();
    std::cout << "\n======================= MPI FINALIZED =======================\n\n";

    return 0;
}