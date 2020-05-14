/*
    This file is part of move-chain.

    move-chain is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    move-chain is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with move-chain.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file PeerWhitelist.cpp
 * Whitelist of peer connection
 * @author jimmyshi
 * @date: 2019-08-06
 */
#include "PeerWhitelist.h"
#include <sstream>
using namespace std;
using namespace dev;

PeerWhitelist::PeerWhitelist(std::vector<std::string> _strList, bool _enable) : m_enable(_enable)
{
    for (auto str : _strList)
    {
        m_whitelist.insert(NodeID(str));
    }
}

bool PeerWhitelist::has(NodeID _peer) const
{
    if (!m_enable)
    {
        // If not enable, all peer is in whitelist
        return true;
    }

    auto itr = m_whitelist.find(_peer);
    return itr != m_whitelist.end();
}

bool PeerWhitelist::has(const std::string& _peer) const
{
    return has(NodeID(_peer));
}

std::string PeerWhitelist::dump(bool _isAbridged)
{
    stringstream ret;
    ret << LOG_KV("enable", m_enable) << LOG_KV("size", m_whitelist.size()) << ",list[";
    for (auto nodeID : m_whitelist)
    {
        if (_isAbridged)
        {
            ret << nodeID.abridged();
        }
        else
        {
            ret << nodeID;
        }
        ret << ",";  // It's ok to tail with ",]"
    }
    ret << "]";

    return ret.str();
}

bool PeerWhitelist::isNodeIDOk(NodeID _nodeID)
{
    return NodeID() != _nodeID;
}

bool PeerWhitelist::isNodeIDOk(const string& _nodeID)
{
    try
    {
        NodeID nodeID = NodeID(_nodeID);
        return isNodeIDOk(nodeID);
    }
    catch (...)
    {
        return false;
    }

    return false;
}