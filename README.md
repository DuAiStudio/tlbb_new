# ShareMemory64 (CentOS 9)

这是一个基于旧版 `ShareMemory` 与 IDA 升级版符号抽取后的 64 位 C++ 重建骨架，目标运行环境：

- CentOS 9
- MySQL 8.0
- unixODBC / ODBC 8.0

## 已实现

- 兼容老版主流程：`init -> loop -> exit`
- 命令文件触发：`saveall.cmd` / `startsave.cmd` / `stopsave.cmd` / `exit.cmd`
- `exit.cmd` 语义对齐旧流程：先触发一次 `saveall` 周期，再执行退出
- 命令冲突处理固定优先级：`exit` > `saveall` > `startsave/stopsave`；同轮 `startsave+stopsave` 时 `stopsave` 生效
- SMU 普通保存节奏由 `ShareMemInfo.ini` 的 `IntervalN` 统一调度，内部不再叠加二次周期判断
- 增加调度可观测日志：`[SCHED-INIT]`（启动计划）与 `[SMU-RUNTIME]`（每 120 秒输出：下一次触发倒计时 + 保存增量统计）
- 调度日志增加 `key_idx/type_code` 标识，可与 `ShareMemInfo.ini` 中 `KeyN/TypeN` 逐项对照
- 日志文件已接入同类结构输出：`ShareMemory_YYYY-MM-DD.<pid>.log`、`Config_YYYY-MM-DD.<pid>.log`、`Debug_YYYY-MM-DD.<pid>.log`
- `ShareMemory` 日志文案继续对齐：新增旧模板风格类型名（如 `8HumanSMU`）及 `SaveAll...OK!/SMUCount/TotalSMUSize` 组日志
- `SaveAll` 组日志已扩展到更多模块（Human/GlobalData/CommissionShop/ItemSerial/GuildLeague/Auction/TopList）
- `SaveAll` 日志增加上下文值输出（如 `ItemSerial Serial`、`Auction/TopList Count`、`ServerID/Key`）
- `Guild/Mail/PlayerShop` 的 `SaveAll` 日志已增加上下文统计（成员数/有效邮件数/摊位数）
- SMU 管理容器（Human/Guild/Mail/ItemSerial/City/GlobalData/Auction/TopList/CommissionShop/PlayerShop/PetProcreate/GuildLeague）
- ODBC 连接重试基础能力
- 配置化 SMU key 和池大小
- `HumanSMU` / `GuildSMU` 已接入真实 SQL 心跳写回：
  - `t_char.savetime=UNIX_TIMESTAMP()`
  - `t_guild_new.dataversion=dataversion+1`
- `MailSMU` / `ItemSerialSMU` 已接入真实 SQL 心跳写回：
  - `t_mail` 周期触发可写回检查（普通/强制两档）
  - `t_itemkey.serial` 周期递增刷写（普通小批量/强制全量）
- `CitySMU` / `GlobalDataSMU` 已接入真实 SQL 心跳写回：
  - `t_city_new` 普通保存与全量保存入口
  - `t_global` 普通保存与全量保存入口
- `AuctionSMU` / `TopListSMU` / `CommissionShopSMU` 已接入真实 SQL 心跳写回：
  - `t_auction` + `t_auction_item` + `t_auction_pet` 普通保存与全量保存入口
  - `AuctionSMU` 默认阶段包含过期拍卖状态推进（`curstatus` 更新）
  - `TopListSMU` 默认阶段包含榜单脏数据修正（`keyvalue<0` 归零）
  - `t_toplist` 普通保存与全量保存入口
  - `CommissionShopSMU` 默认阶段包含寄售过期处理（`isvalid` 更新）
  - `t_cshop` + `t_cshopitem` 普通保存与全量保存入口
- SMU 执行模型升级为三段式：`DoDefault -> DoNormalSave -> DoSaveAll`
- `startsave.cmd / stopsave.cmd` 已接入运行时开关：开启后 `HumanSMU`/`MailSMU`/`CommissionShopSMU`/`AuctionSMU`/`TopListSMU` 提高保存频率并追加下线场景优先写回策略
- 已引入 DB 访问层（`DB*` 风格）：`Human/Guild/Auction/Mail/TopList/CommissionShop/City/GlobalData/ItemSerial` 的 SQL 访问从 SMU 管理器中抽离到独立 Repo
- `HumanDbRepo` 已升级为字段级保存策略：区分在线常规保存、下线优先保存，并联动 `t_charextra.dbversion` 写回
- `GuildDbRepo` 已升级为主从联动保存：`t_guild_new` 保存时联动 `t_guild_user.gptime` 写回
- `AuctionDbRepo` 已升级为主从细分保存：`t_auction` 与 `t_auction_item/t_auction_pet` 分阶段联动写回（按活跃拍卖条件）
- `TopListDbRepo` 已升级为按榜类型分批保存（热点类型优先批次）
- `MailDbRepo` 已增加优先队列批次保存（按 `pindex` 分段）
- 核心 SMU 增加运行时保存统计摘要日志（每约 120 秒输出 normal/all/logout 成功失败计数）
- 服务主循环增加全局单行报表（`[SMU-GLOBAL]`），可在一行内查看各 SMU 统计快照
- 全局报表升级为“周期增量”模式（`dn/da/dl`），并对失败增量达到阈值的 SMU 打 `!` 告警标记

## 构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## 运行

```bash
# 在 Server 目录启动（与 IDA 程序一致）
./sharememory64
```

默认配置路径：`./Config/ShareMemInfo.ini`，并自动读取同目录下 `ServerInfo.ini` 做当前 `ServerID` 的 Key 覆盖。
若 `ServerInfo.ini` 的当前 `ServerX.EnableShareMem=0`，服务将按配置拒绝启动。
启动阶段会输出 `[INIT-ORDER]`，用于核对模块执行顺序与 `ShareMemInfo.ini` 保持一致。

## SQL

把 `D:\MyCode\Tlbb\Sql` 下 SQL 同步到工程：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\sync_sql.ps1
```

然后在 CentOS 上把 `sql/*.sql` 导入 MySQL 8.0。

## 下一步（按 IDA 逐步补齐）

1. 补 `SMULogicManager<T>::DoDefault/DoNormalSave/DoSaveAll` 的真实逻辑
2. 对接旧版各 `DB*` 数据访问类与 SQL 字段映射
3. 替换当前 `GenericSmuLogicManager` 为按类型的真实管理器
