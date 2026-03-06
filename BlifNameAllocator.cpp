#include "BlifNameAllocator.h"
#include <algorithm>
#include <stdexcept>

/*-----------------------------------------------------------
 * 1. 注册 (多 key 可共享同一 alias；一个 key 只能有唯一 alias)
 *----------------------------------------------------------*/
bool BlifNameAllocator::registerName(const std::string& key,
    const std::string& alias)
{
    if (key.empty() || alias.empty()) return false;

    /*--- 若 key 已绑定旧 alias，先撤销旧绑定 ---*/
    auto it = keyToAlias_.find(key);
    if (it != keyToAlias_.end()) {
        std::string oldAlias = it->second;
        if (--aliasCount_[oldAlias] == 0) {
            aliasCount_.erase(oldAlias);   // 旧 alias 无人使用
        }
    }

    /*--- 建立新绑定（允许 alias 已被其它 key 使用） ---*/
    keyToAlias_[key] = alias;
    ++aliasCount_[alias];
    return true;
}

/*-----------------------------------------------------------
 * 2. 自动分配：若 key 未注册，分配一个“未被任何 key 使用”的新 alias
 *----------------------------------------------------------*/
std::string BlifNameAllocator::allocateName(const std::string& key)
{
    // 已有映射直接返回
    auto it = keyToAlias_.find(key);
    if (it != keyToAlias_.end()) return it->second;

    // 生成全局唯一的新 alias
    while (true) {
        std::string cand = indexToAlias(nextIdx_++);
        if (!aliasCount_.count(cand)) {          // cand 尚未被任何 key 使用
            keyToAlias_[key] = cand;
            aliasCount_[cand] = 1;
            return cand;
        }
    }
}

/*-----------------------------------------------------------*/
const std::string& BlifNameAllocator::getName(const std::string& key) const
{
    auto it = keyToAlias_.find(key);
    if (it == keyToAlias_.end())
        throw std::out_of_range("BlifNameAllocator: key not registered");
    return it->second;
}

/*-----------------------------------------------------------*/
void BlifNameAllocator::clear()
{
    keyToAlias_.clear();
    aliasCount_.clear();
    nextIdx_ = 0;
}

/* 0→a, 25→z, 26→aa, ... */
std::string BlifNameAllocator::indexToAlias(std::size_t idx)
{
    std::string s;
    ++idx;                       // 1-based
    while (idx) {
        --idx;
        s.push_back(static_cast<char>('a' + (idx % 26)));
        idx /= 26;
    }
    std::reverse(s.begin(), s.end());
    return s;
}
