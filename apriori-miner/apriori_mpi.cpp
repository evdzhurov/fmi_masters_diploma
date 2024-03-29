
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
// LOGGING
///////////////////////////////////////////////////////////////////////////////////////////
enum class LogLevel : uint8_t
{
    Debug = 0,
    Info = 1,
    Warn = 2,
    Error = 3
};
const char* const s_logLevelStr[] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

#define LOG_LEVEL LogLevel::Debug

#define LOG_COMMON(level, x) if (level >= LOG_LEVEL) std::cout << "[" << s_logLevelStr[(std::uint8_t)level] << "] " << x << '\n'
#define LOG_DEBUG(x) LOG_COMMON(LogLevel::Debug, x)
#define LOG_INFO(x) LOG_COMMON(LogLevel::Info, x)
#define LOG_WARN(x) LOG_COMMON(LogLevel::Warn, x)
#define LOG_ERROR(x) LOG_COMMON(LogLevel::Error, x)

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

    bool Empty() const { return m_NextId == 0; }

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

    std::string ToString(const ItemMap& imap = ItemMap{}, char delim = ',') const
    {
        if (m_Items.empty()) return "";
        std::stringstream ss;
        ss << '<';

        if (imap.Empty())
        {
            for (int i = 0; i < m_Items.size() - 1; ++i)
            {
                ss << m_Items[i] << delim;
            }
            ss << m_Items.back() << '>';
        }
        else
        {
            auto getItemName = [&](int itemId) -> const char*
            {
                auto str = imap.GetItem(itemId);
                return str ? str->c_str() : "MISSING";
            };

            for (int i = 0; i < m_Items.size() - 1; ++i)
            {
                ss << getItemName(m_Items[i]) << delim;
            }
            ss << getItemName(m_Items.back()) << '>';
        }
        
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

    LOG_INFO("Transactions: " << transactions.size());

    ///////////////////////////////////////////////////////////////////////////////////////////
    auto count = [&](const Itemsets& itemsets, int k) 
    {
        int first = ctx.m_Rank * fsets.m_NumTrans / ctx.m_Size;
        int last = (ctx.m_Rank + 1) * fsets.m_NumTrans / ctx.m_Size;
        auto& counts = fsets.m_KthItemsetCounts[k];

        // Init all itemsets with count 0
        // Note that counts will be later gathered across all processes 
        // and all itemsets should exist in m_KthItemsetCounts
        for (const auto& itemset : itemsets)
            counts[itemset] = 0;

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
                    counts[itemset] += 1;
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////
    auto gather_1 = [&]()
    {
        LOG_DEBUG("Gather K=1 ...");

        auto& counts = fsets.m_KthItemsetCounts[1];
        std::vector<int> localCountsDataSizeForRank(ctx.m_Size);
        std::vector<int> countOffsets;
        countOffsets.reserve(ctx.m_Size);
        
        // Sum the number of non-zero counts we will be sending
        int localCountsDataSize = 0;
        for (const auto& kvp : counts)
        {
            if (kvp.second > 0) ++localCountsDataSize;
        }

        // Each count is a pair of an item id and count
        localCountsDataSize = localCountsDataSize * 2;

        int err = MPI_SUCCESS;

        // Exchange local counts data sizes 
        // to calculate the global counts data size
        err = MPI_Allgather(
            &localCountsDataSize,
            1,
            MPI_INT,
            localCountsDataSizeForRank.data(),
            1,
            MPI_INT,
            MPI_COMM_WORLD);

        if (err != MPI_SUCCESS)
        {
            LOG_ERROR("MPI_Allgather failed with err: " << err);
            exit(1);
        }

        // Create offsets from running sum of global counts size
        int globalCountsDataSize = 0;
        for (int size : localCountsDataSizeForRank)
        {
            countOffsets.push_back(globalCountsDataSize);
            globalCountsDataSize += size;
        }

        // Gather counts data
        std::vector<int> localCountsData;
        localCountsData.reserve(localCountsDataSize);

        LOG_DEBUG("Local counts data:");
        for (const auto& kvp : counts)
        {
            if (kvp.second > 0)
            {
                localCountsData.push_back(kvp.first.m_Items.front());
                localCountsData.push_back(kvp.second);

                LOG_DEBUG(kvp.first.ToString() << ": " << kvp.second);
            }
        }

        std::vector<int> globalCountsData(globalCountsDataSize);

        err = MPI_Allgatherv(
            localCountsData.data(),
            localCountsData.size(),
            MPI_INT,
            globalCountsData.data(),
            localCountsDataSizeForRank.data(),
            countOffsets.data(),
            MPI_INT,
            MPI_COMM_WORLD);

        if (err != MPI_SUCCESS)
        {
            LOG_ERROR("MPI_Allgatherv failed with err: " << err);
            exit(1);
        }

        // Merge counts from all processes into local data
        counts.clear();
        for (int i = 1; i < globalCountsDataSize; i+=2)
        {
            counts[Itemset(globalCountsData[i-1])] += globalCountsData[i];
        }

        LOG_DEBUG("Global counts data for k=1:");
        for (const auto& kvp : counts)
        {
            LOG_DEBUG(kvp.first.ToString() << ": " << kvp.second);
        }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////
    auto gather_k = [&](int k)
    {
        LOG_DEBUG("Gather k=" << k << " ...");
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

        LOG_DEBUG("MPI_Reduce_scatter start");
        LOG_DEBUG("counts size =" << size);
        LOG_DEBUG("size part = " << size_part);
        MPI_Reduce_scatter(
            localCounts.data(), /*sendbuf*/
            globalCountsForRank.data(), /*recvbuf*/
            sizes.data(), /*recvcounts*/
            MPI_INT,
            MPI_SUM,
            MPI_COMM_WORLD
        );
        LOG_DEBUG("MPI_Reduce_scatter end");

        // All gather
        std::vector<int> globalCounts(size);
        std::vector<int> globalCountsOffsets;

        int offsetSum = 0;
        for (const auto& size : sizes)
        {
            globalCountsOffsets.push_back(offsetSum);
            offsetSum += size;
        }

        LOG_DEBUG("MPI_Allgatherv start");
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
        LOG_DEBUG("MPI_Allgatherv end");

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
        LOG_DEBUG("Generating L=1 ...");
        
        std::vector<Itemset> c1;
        c1.reserve(fsets.m_ItemMap.m_ItemToId.size());
        for (const auto& pair: fsets.m_ItemMap.m_IdToItem) 
            c1.emplace_back(pair.first);

        count(c1, 1);
        gather_1();
        return prune(c1, 1);
    };

    ///////////////////////////////////////////////////////////////////////////////////////////
    auto gen_Lk = [&](const Itemsets& itemsets, int k) -> Itemsets
    {
        LOG_DEBUG("Generating L=" << k << "...");
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
        if (params.m_MaxK > 0 && k > params.m_MaxK) break;
        Itemsets C = gen_Lk(L, k);
        count(C, k);
        LOG_DEBUG("k=" << k << " candidate itemsets:");
        for (const auto& itemset : C)
            LOG_DEBUG(itemset.ToString());

        gather_k(k);
        L = prune(C, k);

        LOG_DEBUG("k=" << k << " pruned itemsets:");
        for (const auto& itemset : L)
            LOG_DEBUG(itemset.ToString());
        
        ++k;
    }

    LOG_DEBUG("Done building frequent itemsets. k=" << k << " max_k=" << params.m_MaxK);

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
    LOG_INFO("GenerateRules ...");
    std::vector<Rule> rules;

     // Use the actual computed k rounds (could be less than max_k param)
    int k = fsets.m_KthItemsetCounts.size();

    // Rules are meaningles for k=1 (single item itemsets)
    if (k == 1)
    {
        LOG_WARN("Rules can't be generated from single item itemsets(k=1)!");
        return rules;
    }

    for (int i = 2; i <= k; ++i)
    {
        // Partition itemsets equally among processes
        auto it = fsets.m_KthItemsetCounts.find(i);
        if (it == fsets.m_KthItemsetCounts.end())
        {
            LOG_ERROR("k=" << i << " itemsets not found!");
            exit(1);
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

    LOG_INFO("GenerateRules done.");

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
    LOG_DEBUG("Wating to attach debugger...");

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
        LOG_ERROR("Failed to parse arguments!");
        return 1;
    }
    else
    {
        LOG_INFO("Params (input=" << params.m_InputFile
                    << " max_k=" << params.m_MaxK 
                    << " min_sup=" << params.m_MinSup
                    << " min_conf=" << params.m_MinConf);
    }

    InputData samples;
    if (!ReadInputData(params.m_InputFile, samples))
    {
        LOG_ERROR("Failed to read input data from file! fileName=" << params.m_InputFile);
        return 1;
    }

    LOG_DEBUG("Initializing MPI...");
    MPI_Init(&argc, &argv);

    MPIContext ctx;
    MPI_Comm_size(MPI_COMM_WORLD, &ctx.m_Size);
	MPI_Comm_rank(MPI_COMM_WORLD, &ctx.m_Rank);

    LOG_INFO("MPI Initialized" << " rank=" << ctx.m_Rank << "/" << ctx.m_Size);

    auto fsets = Apriori(samples, params, ctx);

    std::cout << "Frequent Itemsets:\n";
    fsets.Print();
    //fsets.ToCsv("frequent_itemsets.csv");

    auto rules = GenerateRules(fsets, params, ctx);
    std::cout << "\nRule | Confidence | Lift\n";
    for (const auto& rule : rules)
       std::cout << rule.ToString(fsets) << '\n';
    //RulesToCsv(fsets, rules, "rules.csv");

    MPI_Finalize();
    LOG_INFO("MPI finalized");

    return 0;
}