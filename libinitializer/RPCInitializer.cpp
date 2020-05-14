/*
 * @CopyRight:
 * move-chain is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * move-chain is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with move-chain.  If not, see <http://www.gnu.org/licenses/>
 * (c) 2016-2018 fisco-dev contributors.
 */
/** @file RPCInitializer.h
 *  @author chaychen
 *  @modify first draft
 *  @date 20181022
 */

#include "RPCInitializer.h"
#include "libblockchain/BlockChainInterface.h"
#include "libchannelserver/ChannelRPCServer.h"
#include "libchannelserver/ChannelServer.h"
#include "libdevcore/Common.h"                    // for byte
#include "libeventfilter/EventLogFilterParams.h"  // for eventfilter
#include "libinitializer/Common.h"
#include "libledger/LedgerManager.h"
#include "librpc/Rpc.h"  // for Rpc
#include "librpc/SafeHttpServer.h"

using namespace dev;
using namespace dev::initializer;

void RPCInitializer::initChannelRPCServer(boost::property_tree::ptree const& _pt)
{
    std::string listenIP = _pt.get<std::string>("rpc.listen_ip", "0.0.0.0");
    int listenPort = _pt.get<int>("rpc.channel_listen_port", 20200);
    int httpListenPort = _pt.get<int>("rpc.jsonrpc_listen_port", 8545);

    if (!isValidPort(listenPort) || !isValidPort(httpListenPort))
    {
        ERROR_OUTPUT << LOG_BADGE("RPCInitializer")
                     << LOG_DESC(
                            "initConfig for RPCInitializer failed! Invalid ListenPort for RPC!")
                     << std::endl;
        exit(1);
    }

    m_channelRPCServer.reset(new ChannelRPCServer(), [](ChannelRPCServer* p) { (void)p; });
    m_channelRPCServer->setListenAddr(listenIP);
    m_channelRPCServer->setListenPort(listenPort);
    m_channelRPCServer->setSSLContext(m_sslContext);
    m_channelRPCServer->setService(m_p2pService);

    auto ioService = std::make_shared<boost::asio::io_service>();

    auto server = std::make_shared<dev::channel::ChannelServer>();
    server->setIOService(ioService);
    server->setSSLContext(m_sslContext);
    server->setEnableSSL(true);
    server->setBind(listenIP, listenPort);
    server->setMessageFactory(std::make_shared<dev::channel::ChannelMessageFactory>());

    m_channelRPCServer->setChannelServer(server);

    // start channelServer before initialize ledger, because amdb-proxy depends on channel
    auto rpcEntity = new rpc::Rpc(nullptr, nullptr);
    m_channelRPCHttpServer = new ModularServer<rpc::Rpc>(rpcEntity);
    m_rpcForChannel.reset(rpcEntity, [](rpc::Rpc*) {});
    m_channelRPCHttpServer->addConnector(m_channelRPCServer.get());
    try
    {
        if (!m_channelRPCHttpServer->StartListening())
        {
            BOOST_THROW_EXCEPTION(ListenPortIsUsed());
        }
    }
    catch (std::exception& e)
    {
        INITIALIZER_LOG(ERROR) << LOG_BADGE("RPCInitializer")
                               << LOG_KV("check channel_listen_port", listenPort)
                               << LOG_KV("check listenIP", listenIP)
                               << LOG_KV("EINFO", boost::diagnostic_information(e));
        ERROR_OUTPUT << LOG_BADGE("RPCInitializer")
                     << LOG_KV("check channel_listen_port", listenPort)
                     << LOG_KV("check listenIP", listenIP) << std::endl;
        BOOST_THROW_EXCEPTION(
            ListenPortIsUsed() << errinfo_comment(
                "Please check channel_listenIP and channel_listen_port are valid"));
    }
    INITIALIZER_LOG(INFO) << LOG_BADGE("RPCInitializer")
                          << LOG_DESC("ChannelRPCHttpServer started.");
    m_channelRPCServer->setCallbackSetter(std::bind(&rpc::Rpc::setCurrentTransactionCallback,
        rpcEntity, std::placeholders::_1, std::placeholders::_2));
}

void RPCInitializer::initConfig(boost::property_tree::ptree const& _pt)
{
    std::string listenIP = _pt.get<std::string>("rpc.listen_ip", "0.0.0.0");
    int listenPort = _pt.get<int>("rpc.channel_listen_port", 20200);
    int httpListenPort = _pt.get<int>("rpc.jsonrpc_listen_port", 8545);
    if (!isValidPort(listenPort) || !isValidPort(httpListenPort))
    {
        ERROR_OUTPUT << LOG_BADGE("RPCInitializer")
                     << LOG_DESC(
                            "initConfig for RPCInitializer failed! Invalid ListenPort for RPC!")
                     << std::endl;
        exit(1);
    }

    try
    {
        // m_rpcForChannel is created in initChannelRPCServer, now complete m_rpcForChannel
        m_rpcForChannel->setLedgerManager(m_ledgerManager);
        m_rpcForChannel->setService(m_p2pService);
        // event log filter callback
        m_channelRPCServer->setEventFilterCallback(
            [this](const std::string& _json, uint32_t _version,
                std::function<bool(
                    const std::string& _filterID, int32_t _result, const Json::Value& _logs)>
                    _respCallback,
                std::function<bool()> _activeCallback) -> int32_t {
                auto params =
                    dev::event::EventLogFilterParams::buildEventLogFilterParamsObject(_json);
                if (!params)
                {  // json parser failed
                    return dev::event::ResponseCode::INVALID_REQUEST;
                }

                auto ledger = getLedgerManager()->ledger(params->getGroupID());
                if (!ledger)
                {
                    return dev::event::ResponseCode::GROUP_NOT_EXIST;
                }

                return ledger->getEventLogFilterManager()->addEventLogFilterByRequest(
                    params, _version, _respCallback, _activeCallback);
            });

        for (auto it : m_ledgerManager->getGroupList())
        {
            auto groupID = it;
            auto blockChain = m_ledgerManager->blockChain(it);
            auto channelRPCServer = std::weak_ptr<dev::ChannelRPCServer>(m_channelRPCServer);
            auto handler = blockChain->onReady([groupID, channelRPCServer](int64_t number) {
                LOG(INFO) << "Push block notify: " << std::to_string(groupID) << "-" << number;
                auto channelRpcServer = channelRPCServer.lock();
                if (channelRpcServer)
                {
                    channelRpcServer->blockNotify(groupID, number);
                }
            });

            m_channelRPCServer->addHandler(handler);
        }

        auto channelRPCServerWeak = std::weak_ptr<dev::ChannelRPCServer>(m_channelRPCServer);
        m_p2pService->setCallbackFuncForTopicVerify(
            [channelRPCServerWeak](const std::string& _1, const std::string& _2) {
                auto channelRPCServer = channelRPCServerWeak.lock();
                if (channelRPCServer)
                {
                    channelRPCServer->asyncPushChannelMessageHandler(_1, _2);
                }
            });

        // Don't to set destructor, the ModularServer will destruct.
        auto rpcEntity = new rpc::Rpc(m_ledgerManager, m_p2pService);
        m_safeHttpServer.reset(
            new SafeHttpServer(listenIP, httpListenPort), [](SafeHttpServer* p) { (void)p; });
        m_jsonrpcHttpServer = new ModularServer<rpc::Rpc>(rpcEntity);
        m_jsonrpcHttpServer->addConnector(m_safeHttpServer.get());
        // TODO: StartListening() will throw exception, catch it and give more specific help
        if (!m_jsonrpcHttpServer->StartListening())
        {
            INITIALIZER_LOG(ERROR) << LOG_BADGE("RPCInitializer")
                                   << LOG_KV("check jsonrpc_listen_port", httpListenPort);
            ERROR_OUTPUT << LOG_BADGE("RPCInitializer")
                         << LOG_KV("check jsonrpc_listen_port", httpListenPort) << std::endl;
            BOOST_THROW_EXCEPTION(ListenPortIsUsed());
        }
        INITIALIZER_LOG(INFO) << LOG_BADGE("RPCInitializer")
                              << LOG_DESC("JsonrpcHttpServer started.");
    }
    catch (std::exception& e)
    {
        // TODO: catch in Initializer::init, delete this catch
        INITIALIZER_LOG(ERROR) << LOG_BADGE("RPCInitializer")
                               << LOG_DESC("init RPC/channelserver failed")
                               << LOG_KV("check channel_listen_port", listenPort)
                               << LOG_KV("check jsonrpc_listen_port", httpListenPort)
                               << LOG_KV("EINFO", boost::diagnostic_information(e));

        ERROR_OUTPUT << LOG_BADGE("RPCInitializer") << LOG_DESC("init RPC/channelserver failed")
                     << LOG_KV("check channel_listen_port", listenPort)
                     << LOG_KV("check jsonrpc_listen_port", httpListenPort)
                     << LOG_KV("EINFO", boost::diagnostic_information(e)) << std::endl;
        exit(1);
    }
}
