/**
 * @CopyRight:
 * opensesame-chain is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * opensesame-chain is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with opensesame-chain.  If not, see <http://www.gnu.org/licenses/>
 * (c) 2016-2019 fisco-dev contributors.
 *
 * @brief: calc trie hash with merkle tree
 *
 * @file: TrieHash2.h
 * @author: darrenyin
 * @date 2019-09-24
 */
#pragma once

#include <libdevcore/FixedHash.h>
#include <vector>

namespace dev
{
h256 getHash256(const std::vector<dev::bytes>& bytesCaches);

void getMerkleProof(const std::vector<dev::bytes>& bytesCaches,
    std::shared_ptr<std::map<std::string, std::vector<std::string>>> _parent2ChildList);

}  // namespace dev
