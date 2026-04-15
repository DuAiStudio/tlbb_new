# ShareMemory64

面向 **Linux 生产**（如 **CentOS 9**）的 64 位 ShareMemory 服务重建工程，行为与日志以 **IDA 中的 Linux ShareMemory** 及旧版 `tlbb2007` 逻辑为参照；数据落库使用 **MySQL 8.0** 与 **unixODBC（ODBC 8.x）**。

## 环境与依赖

| 项目 | 说明 |
|------|------|
| 编译器 | C++20（GCC/Clang） |
| 系统 | Linux x86_64（生产推荐 CentOS 9 同类） |
| 数据库 | MySQL 8.0，库表与存储过程以仓库内 `sql/` 或业务侧 SQL 为准 |
| ODBC | unixODBC + MySQL ODBC 驱动 |
| 共享内存 | **Linux**：System V `shmget` / `shmat`（见 `ShareMemLinux`，语义对齐 `tlbb2007/Common/ShareMemAPI.cpp` 的 `__LINUX__` 分支） |

## 配置（`Config/` 目录）

| 文件 | 作用 |
|------|------|
| `ShareMemInfo.ini` | DSN、库账号、`sharemem.keycount` / `KeyN` / `TypeN` / `IntervalN` 等 |
| `ServerInfo.ini` | `system.currentserverid`、`EnableShareMem`、各服 SM Key 覆盖、`itemserialkey` 等（用于 **ItemSerial 的 ServerID** 映射等） |
| `WorldInfo.ini` | **WorldID**、**区服/服务器 ID**（用于启动日志 `Config::WorldID` / `Config::ZoneID` 一行） |

程序从 **`Config/ShareMemInfo.ini`** 启动（可用命令行第一个参数覆盖路径）。若 `ServerInfo.ini` 中当前服 **`EnableShareMem=0`**，初始化会拒绝启动。

## 日志目录

| 平台 | 默认日志根目录 |
|------|----------------|
| Windows（本地调试） | `D:/Log`（见 `src/main.cpp`） |
| Linux（生产） | `./Log`（需在**进程工作目录**下存在或可创建） |

主要文件：`ShareMemory_YYYY-MM-DD.<pid>.log`、`Config_YYYY-MM-DD.<pid>.log`、`Debug_YYYY-MM-DD.<pid>.log`。

**Debug 日志**：与旧版一致，启动时在 **`Debug_*.log`** 中记录 **`(###Hurricane###) Server Dev:… ClientVer:…`** 与 **`ShareMemory Starting…(YYMMDDHHMM)(0)`**（与 `ShareMemory_*.log` 中对应行相同，便于单独留版本/启动戳样本）。`logDebug` 不写终端。

**终端同步**：默认 **`logShare` 与 `logConfig`** 在写文件的同时 **各复制一行到标准输出**（与旧版前台一致：`Load …ini`、`Attach ShareMem` 等均可见）。启动时 **`main` 还会向 stdout 打印 ASCII 横幅**（不写 ShareMemory 日志文件）。若不需要前台重复，将 `Logger::init(baseDir, false)` 第二参数设为 `false`。

**Hurricane 行**：首段运行日志为 `(###Hurricane###) Server Dev:1271 ClientVer:533`（数值可通过 CMake 变量 **`SHAREMEMORY_SERVER_DEV`** / **`SHAREMEMORY_CLIENT_VER`** 覆盖，见 `CMakeLists.txt`）。

## 构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

生成可执行文件 **`sharememory64`**（名称以 `CMakeLists.txt` 为准）。

## 运行

在包含 **`Config/`** 的工作目录下执行（与线上一致，例如 `.../Server/`）：

```bash
./build/sharememory64
# 或指定配置路径：
./build/sharememory64 /path/to/ShareMemInfo.ini
```

运行时命令文件（与旧版同名）：`saveall.cmd`、`exit.cmd`、`crash.cmd`、`startsave.cmd`、`stopsave.cmd`、`cleanbattleguild.cmd`（放在当前工作目录，由主循环轮询处理）。
扩展命令文件：`probe_pool.cmd`、`probe_once.cmd`、`probe_result.cmd`、`smu_summary.cmd`、`smu_summary_reset.cmd`、`smu_summary_total.cmd`、`smu_summary_peek.cmd`。

## 版本号与 Git 标签（A2）

- **语义化版本**：单文件 **`VERSION`**（一行，如 `0.2.0`）。`CMakeLists.txt` 会读取该文件作为 `project(... VERSION ...)`，编译宏 **`SHAREMEMORY64_VERSION`** 与之一致，便于对照二进制与提交。
- **主分支**：日常开发以 **`main`** 为可集成分支；每次可发布变更应 **小步提交**（消息写清动机），便于 `git bisect`。
- **里程碑标签**：在重要节点打 **附注标签**，便于服务器与文档回溯，例如：
  - `v0.1.0` — 首版可运行（仅 ODBC 等）
  - `v0.2.0` — 接入 Linux 共享内存等

本地打标签并推送（需已配置 `remote`）：

```bash
git status
git add -A && git commit -m "Describe change in full sentences."
git tag -a v0.2.0 -m "v0.2.0: Linux SysV SHM, README/versioning"
git push origin main
git push origin v0.2.0
```

检出某版本复现构建：

```bash
git fetch --tags
git checkout v0.2.0
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
```

以后 bump 版本：编辑 **`VERSION`**，提交后再打新 tag（如 `v0.3.0`）。

## 回归检查（A3）

改完代码后，用一次**成功冷启动**生成的 `ShareMemory_*.log` 做最小验收。

1. **关键字清单**：`scripts/sharememory_startup_keywords.txt`（每行一段**子串**，注释行以 `#` 开头）。
2. **自动检查**（任选一个平台）：
   - Linux / Git Bash：`bash scripts/check_sharememory_log.sh ./Log/ShareMemory_YYYY-MM-DD.xxxxx.log`
   - Windows：`powershell -ExecutionPolicy Bypass -File scripts\check_sharememory_log.ps1 D:\Log\ShareMemory_YYYY-MM-DD.xxxxx.log`
3. **人工黄金样本**：可将当时日志**前约 50 行**保存为私有备份；仓库内仅提供形状说明：`scripts/golden_sharememory_startup_reference.log`（勿当作精确逐字比对）。

若某次启动**未包含**清单中的关键字（例如卡在 `Map ShareMem Error`），脚本会报 `MISSING:` 并非零退出。调整 `sharememory_startup_keywords.txt` 时保持「少而稳」——只列必须通过的几段，避免把易变统计写死。

## F阶段（内建命令闭环，无外部脚本）

- **F1 下发触发**：在运行目录投放 `probe_pool.cmd`（可选配 `probe_pool.types`）。
- **F2 查看结果**：读取最新日志中的：
  - `[E2-STRIDE][RESULT] verdict=...`
  - `[POOL-PROBE][RESULT] verdict=...`
- **F3 一次闭环**：投放 `probe_once.cmd`，服务会立即执行一轮探针并输出：
  - `[F-PROBE-ONCE][RESULT] verdict=... total=... warn=... fail=... invalid_tokens=...`
- **F4 查询最近结果**：投放 `probe_result.cmd`，服务会回显：
  - `[F-PROBE-LAST][RESULT] verdict=... total=... warn=... fail=... invalid_tokens=...`
- **F5 结果文件落盘**（便于外部自动采集）：
  - 每次 `POOL-PROBE` 完成后写 `probe_last.result`
  - 每次 `probe_once.cmd` 完成后写 `probe_once.result`

## SMHead 布局（B1）

- **源码依据**：`tlbb2007/Common/DB_Struct.h` 中 `SMHead` 为 `SM_KEY` + `unsigned long m_Size` + `UINT m_HeadVer`；`BaseType.h`（`__LINUX__`）里 **`ULONG` / `unsigned long` 在 LP64 上为 64 位**。
- **本工程**：`include/share_mem_linux.hpp` 中 **`SMHead` 为 24 字节**（`uint64_t` + `uint64_t` + `uint32_t` + 尾部对齐），与 **Linux x86_64 下老源码语义**一致。
- **IDA 核对**：在 IDA 中打开 **Linux ShareMemory ELF**，对 **`SMHead`** 或首段共享区导出结构体，确认 **sizeof = 24** 且字段顺序一致；若升级程序使用 **ILP32** 或其它打包方式，以 IDA 为准改 `SMHead` 并复测与 World 共段。

## 段长与池容量（B2）

- **原版公式**（`tlbb2007/Server/SMU/SMUManager.h`）：`Attach(key, sizeof(T) * nMaxCount + sizeof(SMHead))`，即 **整段字节数 = `poolSlots * sizeof(元素类型) + sizeof(SMHead)`**。
- **本工程**：`include/share_mem_sizes.hpp` 中 **`attachSizeBytes(type, poolSlots)`** 使用在**默认 `poolSlots`** 下从参考日志标定的 **整段总字节**（已含 `SMHead`），再按 `poolSlots` **线性缩放**。若你的 World/IDA 与默认池容量不一致，在 **`ShareMemInfo.ini`** 里为第 `N` 项设置 **`sharemem.poolsizeN=…`**（覆盖 `defaultPoolSize()`），使 **`attachSizeBytes` 的 `poolSlots` 与配置一致**。
- **核对**：附着已有段时，若内核段长与本次计算的 `totalBytes` 不一致，日志会出现 **`ShareMem IPC_STAT shm_segsz=… requested_totalBytes=…`**；若仅 **`SMHead.m_Size`** 与计算值不一致但 key 正确，会打 **`m_Size=… calc=… kernel_seg=… (continuing; key ok)`**（与 `share_mem_linux.cpp` 行为一致）。

## 启动顺序与 SysV 生命周期（B3）

- **与 World 的顺序**：共享段由 **`shmget`/`shmat`** 建立或接入；**推荐先启动 ShareMemory**（冷启动时由本进程 `IPC_CREAT` 建段），再启动 **World / Login** 等会 **附着同 Key** 的进程。若段已由上次运行保留在系统内，顺序可放宽为「先 ShareMemory 或先 World」——只要各方 **Key 与段长（B2）一致**。若 World 先跑且段尚不存在，World 侧需能 **创建或等待**（依你方 IDA/脚本；本服务侧为「先试打开，再创建」）。
- **退出与 `IPC_RMID`**：进程退出时仅 **`shmdt` 脱离映射**，**不**调用 **`shmctl(IPC_RMID)`** 删除段，避免 World 仍在使用时段被销毁。运维清理：先停 **所有** 使用该段的进程，再 **`ipcrm -m`** 或重启机器策略依环境而定。
- **析构顺序**：`ShareMemoryService` 中 **`shmRegions_` 声明在 `managers_` 之前**，以便析构时先析构 SMU 管理器、再 `shmdt`（与线上一致：不在此程序内「拆段」）。

## World/Zone 校验策略（D3）

- 启动时会比较 `Config::WorldID/ZoneID` 与 DB（`t_general_set` 中 `WORLD_ID` / `ZONE_ID`）。
- 默认 **严格模式**：`system.strict_world_zone_check=1`（默认），不一致时记录 `CheckWorldIDZoneID()...Fails` 并终止初始化。
- 可选宽松模式：设 `system.strict_world_zone_check=0`（`ShareMemInfo.ini` 或 `ServerInfo.ini`），不一致时仅记录 `CheckWorldIDZoneID()...Warn (strict disabled)` 并继续。

## ServerID 校验策略（World/GW 联动）

- 启动时会校验 `currentServerId`；若 `<=0`，默认视为错误（常见症状：World/GW 日志出现 `ServerID=0`、`not in lock by sharemem`）。
- 默认 **严格模式**：`system.strict_server_id_check=1`，`currentServerId<=0` 时记录 `CheckCurrentServerID()...Fails` 并终止启动。
- 可选宽松模式：`system.strict_server_id_check=0`，仅告警继续（不建议线上使用）。
- 当 `system.currentserverid` 未配置或为 0 时，会尝试回退使用 `WorldInfo.ini` 的 server/zone id。

## 命令文件路径（D4）

- `saveall.cmd`：记录 `Get SaveAll Commands`，下一轮心跳触发全量保存。
- `exit.cmd`：记录 `Get Exit Commands`，先保存后记录 `Loop...Exit requested after save` 并退出主循环。
- `crash.cmd`：记录 `Get Crash Commands`，执行一轮 crash-save（日志 `Crash SaveAll cycle...OK`）。
- `cleanbattleguild.cmd`：执行 City 清理流程并在保存后退出。
- `smu_summary.cmd`：立即输出一轮 `[D-SUMMARY]` 调度摘要（不等待周期）。
- `smu_summary_reset.cmd`：重置当前调度摘要窗口计数（`calls/errors` 清零，并重置窗口计时）。
- `smu_summary_total.cmd`：立即输出一轮摘要并追加 `lifetime calls/errors`（自进程启动累计）。
- `smu_summary_peek.cmd`：立即输出当前窗口摘要但不清零窗口计数（只读查看）。
- `smu_summary.types`（可选）：与摘要命令同目录放置类型过滤（如 `Human,Auction` 或 `all`），仅对本次摘要命令生效。
- `probe_once.cmd`：立即执行一轮 `POOL-PROBE` 并输出 `[F-PROBE-ONCE][RESULT] ...` 闭环结论。
- `probe_result.cmd`：输出最近一次探针缓存结果 `[F-PROBE-LAST][RESULT] ...`。

## E1 只读池探针（实验）

- 新增命令文件：`probe_pool.cmd`
- 放到运行目录后，主循环会执行一次只读探针日志，不改共享段、不写数据库。
- 日志前缀：`[POOL-PROBE]`，内容包括：
  - 段头字段：`head.key` / `head.size`
  - 当前配置槽位数：`slots`
  - IDA 步长：`stride`
  - IDA 理论总长与差值：`ida_total`、`delta(head.size-ida_total)`
  - 段体起始与前 3 个槽位推导地址：`s0/s1/s2`
  - 统一结论：`status=OK/WARN/FAIL`（并带 `e2=OK/WARN/NA`、`key_match`、`attach_match`）
  - 总览行：`[POOL-PROBE] summary total=... filtered=... ok=... warn=... fail=... ida_aligned=... ida_delta_nonzero=... ida_delta_pos=... ida_delta_neg=... ida_delta_abs_max=...`
  - 机读结论行：`[POOL-PROBE][RESULT] verdict=OK/WARN/FAIL ...`
- 类型过滤（E5，可选）：
  - `system.probe_pool_types=GlobalData,ItemSerial` 只探指定类型
  - 留空或设为 `all` 表示全量探针
- 运行时临时过滤（E6，无需重启）：
  - 与 `probe_pool.cmd` 同目录放置 `probe_pool.types`
  - 内容示例：`GlobalData,ItemSerial`（也支持按行写、支持注释行 `#`/`;`）
  - 可直接参考模板：`scripts/sample_probe_pool.types`
  - 触发后本轮探针优先使用该文件，随后自动删除 `probe_pool.types`
  - 若存在未知类型 token，会记录 `Get ProbePool Types invalid tokens: ...`
  - summary 会附带 `invalid_tokens=...`

## E2 试点：真实 stride 只读校验（GlobalData / ItemSerial）

- 启动时对 `GlobalDataSMU` 与 `ItemSerialKeySMU` 额外执行只读校验：
  - 期望值：`expected = sizeof(SMHead) + stride * slots`
  - 实际值：共享段头 `SMHead.m_Size`
- 日志前缀：`[E2-STRIDE]`
- 启动后会输出汇总：`[E2-STRIDE] summary pilot_total=... ok=... warn=... fail=... na=... strict=...`
- 机读结论行：`[E2-STRIDE][RESULT] verdict=OK/WARN/FAIL ...`
- 严格开关（默认关闭）：
  - `system.strict_stride_probe_check=0`：不一致仅告警（默认）
  - `system.strict_stride_probe_check=1`：不一致即初始化失败

## E阶段完成口径（进入F前）

- E1 完成：`POOL-PROBE` 具备段级明细、差值统计、总览与机读结果行。
- E2 完成：试点校验（GlobalData/ItemSerial）具备明细、汇总与机读结果行。
- 建议进入 F 阶段时，以两条机读结果行作为自动化入口：
  - `[E2-STRIDE][RESULT] verdict=...`
  - `[POOL-PROBE][RESULT] verdict=...`

## C3 ItemSerial 保存策略（按服单行）

- `ItemSerial` 保存改为按 `sid=ServerID` 定位单行自增，不再执行全表 `serial+1`。
- 初始化与保存阶段会确保当前 `sid` 行存在（缺失时自动插入一行）。
- 运行日志仍输出：`ItemSerial Save...OK! Serial=... ,ServerID=... ,Rows=...`，便于与原版对齐排查。

## C4 Human DoSaveAll 收敛（事务 + 分段行数）

- `Human` 的保存路径调整为事务化两段：
  - 主表：`t_char` 的 `savetime`
  - 扩展表：`t_charextra` 的 `dbversion`
- 两段任一失败会回滚，避免“主表成功 / extra 失败”造成不一致。
- 保存日志输出分段行数：`Rows=...(char=..., extra=...)`，便于对齐与排障。
- `saveLogout` 模式也复用同样事务语义，并输出 `Human SaveLogout...OK! Rows=...` 分段行数。
- 普通周期保存改为轻量事务批次（`LIMIT 2000`）并输出 `Human NormalSave...OK! Rows=...`。
- `Human NormalSave` 日志做了节流聚合：行数变化时即时输出；稳定期按固定轮次输出，减少高频刷屏。
- 三条路径统一失败计数：连续失败会升级告警，恢复成功后输出恢复日志。
- 新增可调开关：`system.human_normal_save_log_every`（默认 `6`）与 `system.human_save_failure_warn_threshold`（默认 `3`），支持在 `ShareMemInfo.ini`/`ServerInfo.ini` 覆盖。
- 失败升级支持分级与抑制：`system.human_save_failure_error_threshold`（默认 `6`）控制 `WARN->ERROR` 切换，`system.human_save_failure_escalate_log_every`（默认 `3`）控制持续失败时的升级日志输出间隔。
- 三条路径（`NormalSave/SaveLogout/SaveAll`）新增分路径失败窗口汇总日志，持续故障时可直接定位异常路径。
- 可选仅异常窗口输出：`system.human_stage_window_log_anomaly_only`（默认 `1`）时，窗口汇总仅在窗口内存在失败时输出，避免正常期日志噪声。

## C5 Auction 保存收敛（事务 + 三段行数）

- `Auction` 保存改为单事务三段：`t_auction` + `t_auction_item` + `t_auction_pet`，任一失败即回滚。
- 日志细化为：`Rows=...(auction=..., item=..., pet=...)`，更便于定位是哪一段写回异常。

## C6 CommissionShop 保存收敛（事务 + 分段行数）

- `CommisionShop` 保存改为单事务两段：`t_cshopitem` + `t_cshop`，任一失败即回滚。
- 日志细化为：`Rows=...(item=..., shop=...)`，可直接判断是寄售项还是店铺主表写回异常。

## C7 PlayerShop 保存收敛（事务 + 分段行数）

- `PlayerShop` 保存改为单事务两段：`t_pshop_new` + `t_pshop_stall`，任一失败即回滚。
- 日志细化为：`Rows=...(shop=..., stall=...)`，可直接判断是哪一张表写回异常。

## C8 Guild 保存收敛（事务 + 分段行数）

- `Guild` 保存改为单事务两段：`t_guild_new` + `t_guild_user`，任一失败即回滚。
- 日志继续输出分段行数：`Rows=...(guild=..., user=...)`，并保留 `GUILD_NEW` 脏标记推进。

## C9 TopList 保存收敛（事务 + 分段行数）

- `TopList` 双更新路径（`aid` 维度 + `type` 维度）改为单事务执行，任一失败即回滚。
- 现有 `TopList[ID:0/1/2]` 日志保持不变，行数统计仍基于两段总和。

## C10 City/GlobalData 事务入口统一

- `City` 与 `GlobalData` 新增统一 `saveAllTransactional` 入口（当前仍是单段 SQL，但事务入口对齐到统一模板）。
- 对应 SMU 逻辑已切换到事务接口调用，后续若扩展多段写回可直接在 Repo 内演进而不改调度层。

## D5 调度稳定性摘要（按 SMU）

- 新增周期摘要日志：`[D-SUMMARY] idx=... type=... calls=... errors=...`，用于观察各 SMU 调度窗口内调用量与异常量。
- 新增配置：`system.smu_dispatch_summary_interval_sec`（默认 `60`，`0` 表示关闭）。
- 该摘要位于调度层（heartbeat 分发）而非 SQL 层；用于稳定性趋势观察，不替代各 SMU 详细保存日志。
- D6 扩展：`system.smu_dispatch_summary_anomaly_only`（默认 `0`）可仅输出 `errors>0` 的 SMU 行；同时固定输出总览行：`[D-SUMMARY] total calls=... errors=... emitted=...`。
- D7 扩展：投放 `smu_summary.cmd` 可立即触发一轮摘要输出，便于线上临时抓取窗口统计。
- D8 扩展：投放 `smu_summary_reset.cmd` 可立即清零当前窗口计数，便于重建观测基线。
- D9 扩展：投放 `smu_summary_total.cmd` 可输出累计统计（不清零累计值），用于跨窗口观察长期趋势。
- D10 扩展：投放 `smu_summary_peek.cmd` 可“只看不清零”当前窗口统计，适合临时排障采样。
- D11 扩展：可通过 `smu_summary.types` 对摘要做临时类型过滤，日志会回显 `active_filter` 与 `invalid_tokens`。

## 退出删段策略（B4）

- **默认安全策略**：`detach-only`（不 `IPC_RMID`），避免 World/Login 仍在用时被删段。
- **可选开启删段**：配置 `sharemem.ipc_rmid_on_exit=1`（`ShareMemInfo.ini`）或 `system.ipc_rmid_on_exit=1`（`ServerInfo.ini`，优先覆盖）后，进程退出会对每个已附着段执行 `shmctl(IPC_RMID)`。
- **启动可见**：日志会打印 `ShareMem Exit Policy: ...`；若开启删段，退出阶段会记录 `ShareMem IPC_RMID ok/failed shmid=...`。

## 当前实现范围（与代码一致）

- 主流程：**读配置 → 建管理器壳 → 初始化（DB、`WorldID/ZoneID` 比较、Linux 共享内存附着/创建、`PostInit` 日志）→ 主循环**（约 1s 一拍，SMU 心跳约 5s）。
- **多 SMU**：Human、Guild、Mail、PlayerShop、GlobalData、CommissionShop、ItemSerial、PetProcreate、City、GuildLeague、Auction、TopList；未识别类型走占位管理器。
- **ODBC**：连接与重连、`DbRepo` 内**简化 SQL** 做周期/强制存盘（与原版存储过程级行为可能仍有差异，需按 IDA/SQL 逐步收紧）。
- **`t_general_set`**：成功存盘后 **`GUILD_NEW` / `PSHOP_NEW` / `CITY_NEW`** 脏标记（`sKey`/`nVal`，与 `tlbbdb.sql` 一致）。
- **共享内存**：Linux 下为真实 `shm` 映射；**不**在此仓库内实现完整「池内对象 + 全量字段写库」，该部分需结合 `tlbb2007` 与 IDA 分阶段迁移。

## SQL 同步（可选）

若使用仓库自带脚本从其它目录同步 SQL：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\sync_sql.ps1
```

在服务器上将 `sql/*.sql` 导入 MySQL 8.0 后再跑服务。

## C2 验证（`t_general_set`）

在触发一次 `saveall.cmd`（或等待正常存盘周期）后，校验三项脏标记：
- `GUILD_NEW`
- `PSHOP_NEW`
- `CITY_NEW`

Linux / Git Bash：
```bash
bash scripts/check_general_set.sh 127.0.0.1 3306 root yourpass tlbbdb
```

Windows：
```powershell
powershell -ExecutionPolicy Bypass -File scripts\check_general_set.ps1 -Host 127.0.0.1 -Port 3306 -User root -Password yourpass -Database tlbbdb
```

两脚本默认期望 `nVal=1`，可传最后一个参数（或 `-Expected`）改期望值。

## 后续工作（概要）

更细的分阶段任务（文档整理、IDA 结构对齐、SQL/日志对齐、池数据驱动等）见开发过程中的**阶段 A～F** 路线图；实现时建议按阶段小步提交，并与 **`D:\Log`（或 `./Log`）样例日志** 对照回归。
