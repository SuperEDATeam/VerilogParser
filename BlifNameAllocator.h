#ifndef BLIF_NAME_ALLOCATOR_H
#define BLIF_NAME_ALLOCATOR_H

#include <string>
#include <unordered_map>

// 允许“多 key→同 alias”，但禁止“一 key→多 alias”
class BlifNameAllocator {
public:
    bool              registerName(const std::string& key, const std::string& alias);
    std::string       allocateName(const std::string& key);
    const std::string& getName(const std::string& key) const;
    void              clear();

    bool hasKey(const std::string& key)   const { return keyToAlias_.count(key); }
    bool aliasUsed(const std::string& alias) const { return aliasCount_.count(alias); }

private:
    static std::string indexToAlias(std::size_t idx);

    std::size_t nextIdx_ = 0;                          // 自动分配计数器
    std::unordered_map<std::string, std::string> keyToAlias_;   // key → alias
    std::unordered_map<std::string, std::size_t> aliasCount_;   // alias → 使用次数
};

#endif
