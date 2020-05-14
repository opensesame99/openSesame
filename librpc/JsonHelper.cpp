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
 * (c) 2016-2018 fisco-dev contributors.
 *
 * @file JsonHelper.cpp
 * @author: caryliao
 * @date 2018-10-26
 */

#include "JsonHelper.h"
#include <jsonrpccpp/common/exception.h>
#include <libethcore/CommonJS.h>
#include <libethcore/Transaction.h>

using namespace std;
using namespace dev::eth;

namespace dev
{
namespace rpc
{
Json::Value toJson(
    Transaction const& _t, std::pair<h256, unsigned> _location, BlockNumber _blockNumber)
{
    Json::Value res;
    if (_t)
    {
        res["hash"] = toJS(_t.sha3());
        res["input"] = toJS(_t.data());
        res["to"] = _t.isCreation() ? Json::Value() : toJS(_t.receiveAddress());
        res["from"] = toJS(_t.safeSender());
        res["gas"] = toJS(_t.gas());
        res["gasPrice"] = toJS(_t.gasPrice());
        res["nonce"] = toJS(_t.nonce());
        res["value"] = toJS(_t.value());
        res["blockHash"] = toJS(_location.first);
        res["transactionIndex"] = toJS(_location.second);
        res["blockNumber"] = toJS(_blockNumber);
    }
    return res;
}

TransactionSkeleton toTransactionSkeleton(Json::Value const& _json)
{
    TransactionSkeleton ret;
    if (!_json.isObject() || _json.empty())
        return ret;

    if (!_json["from"].empty())
        ret.from = jsToAddress(_json["from"].asString());
    if ((!_json["to"].empty()) && (!_json["to"].asString().empty()) &&
        _json["to"].asString() != "0x")
        ret.to = jsToAddress(_json["to"].asString());
    else
        ret.creation = true;

    if (!_json["value"].empty())
        ret.value = jsToU256(_json["value"].asString());

    if (!_json["gas"].empty())
        ret.gas = jsToU256(_json["gas"].asString());

    if (!_json["gasPrice"].empty())
        ret.gasPrice = jsToU256(_json["gasPrice"].asString());

    if (!_json["data"].empty() && _json["data"].isString())
        ret.data = jsToBytes(_json["data"].asString(), OnFailed::Throw);

    if (!_json["code"].empty())
        ret.data = jsToBytes(_json["code"].asString(), OnFailed::Throw);

    if (!_json["randomid"].empty())
        ret.nonce = jsToU256(_json["randomid"].asString());

    // add blocklimit params
    if (!_json["blockLimit"].empty())
        ret.blockLimit = jsToU256(_json["blockLimit"].asString());

    return ret;
}

}  // namespace rpc

}  // namespace dev
