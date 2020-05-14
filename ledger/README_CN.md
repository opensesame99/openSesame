[English](README.md) | 中文

## 模块简介
账本模块主要负责账本的执行和区块生成。主要包括：
- 根据配置生成创世账户和创世区块；
- 执行共识通过后的提案内的交易；
- 根据执行完成的交易打包生成新的区块；
- 定时从区块链网络同步区块。

## 模块组成
类名称 | 声明文件 | 功能
|:--- | --- | ---
|`Trie`                  | [trie.h](./trie.h)                                   | 字典树基类。字典树是 `opensesame` 的底层数据查询和存取结构，除了字典特性外， `opensesame` 还为 `Trie` 加入了默克尔根的特性。Trie定义了字典树的框架功能，并实现了部分接口。
|`KVTrie`                | [kv_trie.h](./kv_trie.h)                             | `Trie` 的派生类，具体实现了默克尔前缀树的功能。
|`Environment`           | [environment.h](./environment.h)                     | 交易的执行容器，为交易提供了事务特性。执行交易时变动的数据都会写入 `Environment` 的缓存，交易内的所有操作全部执行结束后，统一提交更新。
|`FeeCalculate`          | [fee_calculate.h](./fee_calculate.h)                 | 费用计算类，定义了各类交易操作的费用标准，对外提供费用计算接口。
|`AccountFrm`            | [account.h](./account.h)                             | 账户类。用户在 `opensesame` 链上的行为主体，记录了包括账户属性、账户状态和内容资产在内的所有用户数据，用户的所有操作都要以 `AccountFrm` 为基础来实现。
|`TransactionFrm`        | [transaction_frm.h](./transaction_frm.h)             | 交易执行类，负责交易的执行处理，交易内的具体操作交由 `OperationFrm` 执行。
|`OperationFrm`          | [operation_frm.h](./operation_frm.h)                 | 操作执行类，根据操作类型，具体执行交易中的操作。
|`ContractManager`       | [contract_manager.h](./contract_manager.h)           |智能合约管理类。为智能合约提供代码执行环境和管理工作。包括加载代码解释器、提供内置变量和接口、合约代码和参数检查、代码执行等。主要被 `OperationFrm` 的创建账户和转账操作触发。
|`LedgerManager`         | [ledger_manager.h](./ledger_manager.h)               | 账本管理类。统筹区块的执行管理，调度`ledger` 下各个子模块执行完共识提案中的交易后，生成新区块，写入数据库，定时从网络内同步最新区块。
|`LedgerContext`         | [ledgercontext_manager.h](./ledgercontext_manager.h) | 账本的执行上下文，承载账本的内容数据和属性状态数据等。
|`LedgerContextManager`  | [ledgercontext_manager.h](./ledgercontext_manager.h) | `LedgerContext` 的管理类，方便多线程执行调度。
|`LedgerFrm`             | [ledger_frm.h](./ledger_frm.h)                       | 账本执行类，负责账本的具体处理工作。主要是将账本中的交易逐一交由 `TransactionFrm` 执行。

## 框架流程
- 程序启动时，`LedgerManager` 初始化，并根据配置文件创建创世账户和创世区块。
- 区块链网络开始运行后，`LedgerManager` 接收到经由 `glue` 模块传递过来的共识提案，对提案做合法性检查。
- 通过合法检查后，将共识提案交给 `LedgerContextManager`。
- `LedgerContextManager` 为共识提案的处理生成执行上下文 `LedgerContext` 对象，`LedgerContext` 将提案交由 `LedgerFrm` 具体处理。
- `LedgerFrm` 创建 `Environment` 对象，为执行提案内的交易提供事务容器，然后将提案的交易逐一取出，交由 `TransactionFrm` 处理。
- `TransactionFrm` 再将交易内的操作逐一取出，交由 `OperationFrm` 执行。
- `OperationFrm` 根据类型分别执行交易内的不同操作，并将操作变更的数据写入 `Environment` 的缓存。其中， `OperationFrm` 执行的创建账户操作如果是创建合约账户，或者执行转账操作（包括转移资产和转移BU币），会触发 `ContractManager` 加载并执行合约代码，合约执行对数据的变更也会写入 `Environment` 中。
- 在交易执行过程中，会调用 `FeeCalculate` 计算实际费用。
- 每笔交易内的所有操作完成后，会将 `Environment` 中的变更缓存统一提交更新。
- 等提案内的所有交易执行完成后，`LedgerManager` 对提案打包生成新的区块，并将新区块和更新的数据写入数据库。
- 此外，`LedgerManager` 会通过定时器定时从区块链网络同步最新区块。

