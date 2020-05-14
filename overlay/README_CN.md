# P2P 网络

## 功能介绍
该模块实现了 P2P 网络通信及业务消息解析功能，具体如下：
- 节点连接管理。包括配置已知节点、发现新节点、主动连接其他节点、监听其他节点连接。
- 节点通信。跟其他节点提供消息交互。如发起交易，区块同步，区块共识消息等。
- 消息解析处理。在收到消息后，解析并传递给内部其他模块。

## 模块结构

类名称 | 声明文件 | 功能
|:--- | --- | ---
| `PeerManager` | [peer_manager.h](./peer_manager.h) | 节点模块的管理者。为该模块提供定时器和线程执行环境；同时给其他模块提供发送单播、广播消息的接口。
|`PeerNetwork`|  [peer_network.h](./peer_network.h) | 节点消息处理实现者，继承自 `Network` 类。该类有两个功能：一是管理节点连接：如连接其他节点，清空失效连接；二是处理接收到的消息，如获取节点，发起交易，同步区块，区块共识，账本升级等消息。
|`Network`|  [network.h](../common/network.h)  | 节点网络通信的实现者。使用`asio::io_service` 异步 IO 模块监听网络事件，管理所有的网络连接，如新建连接，关闭连接，保持连接心跳，分发解析接收到的消息。
|`Peer`|  [peer.h](./peer.h) | 一个 TCP 连接的封装者，继承自 `Connection` 类，参考 [network.h](../common/network.h)。提供发送数据的基本接口，提供当前 TCP 的状态，使用 `websocketpp::server`和 `websocketpp::client` 做为管理对象。
|`Broadcast`| [broadcast.h](./broadcast.h)  | 广播消息的管理者。提供发送广播消息，记录广播消息，清空广播消息的功能。被 `PeerNetwork` 调用。


## 协议定义
P2P 消息使用 Google protocol buffer 定义，参考文件 [overlay.proto](../proto/overlay.proto)，类型如下：
```
OVERLAY_MSGTYPE_PEERS  #获取节点信息
OVERLAY_MSGTYPE_TRANSACTION  #发起交易
OVERLAY_MSGTYPE_LEDGERS   #获取区块
OVERLAY_MSGTYPE_PBFT    #区块共识
OVERLAY_MSGTYPE_LEDGER_UPGRADE_NOTIFY   #账本升级
```

这些消息按照类型可以分为单播和广播。如下：
- 单播。`OVERLAY_MSGTYPE_PEERS` 和 `OVERLAY_MSGTYPE_LEDGERS`
- 广播。`OVERLAY_MSGTYPE_TRANSACTION`、`OVERLAY_MSGTYPE_PBF`、`OVERLAY_MSGTYPE_LEDGER_UPGRADE_NOTIFY`
