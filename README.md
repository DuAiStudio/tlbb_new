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

主要文件：`ShareMemory_YYYY-MM-DD.<pid>.log`、`Config_YYYY-MM-DD.<pid>.log`。

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

## SMHead 布局（B1）

- **源码依据**：`tlbb2007/Common/DB_Struct.h` 中 `SMHead` 为 `SM_KEY` + `unsigned long m_Size` + `UINT m_HeadVer`；`BaseType.h`（`__LINUX__`）里 **`ULONG` / `unsigned long` 在 LP64 上为 64 位**。
- **本工程**：`include/share_mem_linux.hpp` 中 **`SMHead` 为 24 字节**（`uint64_t` + `uint64_t` + `uint32_t` + 尾部对齐），与 **Linux x86_64 下老源码语义**一致。
- **IDA 核对**：在 IDA 中打开 **Linux ShareMemory ELF**，对 **`SMHead`** 或首段共享区导出结构体，确认 **sizeof = 24** 且字段顺序一致；若升级程序使用 **ILP32** 或其它打包方式，以 IDA 为准改 `SMHead` 并复测与 World 共段。

## 段长与池容量（B2）

- **原版公式**（`tlbb2007/Server/SMU/SMUManager.h`）：`Attach(key, sizeof(T) * nMaxCount + sizeof(SMHead))`，即 **整段字节数 = `poolSlots * sizeof(元素类型) + sizeof(SMHead)`**。
- **本工程**：`include/share_mem_sizes.hpp` 中 **`attachSizeBytes(type, poolSlots)`** 使用在**默认 `poolSlots`** 下从参考日志标定的 **整段总字节**（已含 `SMHead`），再按 `poolSlots` **线性缩放**。若你的 World/IDA 与默认池容量不一致，在 **`ShareMemInfo.ini`** 里为第 `N` 项设置 **`sharemem.poolsizeN=…`**（覆盖 `defaultPoolSize()`），使 **`attachSizeBytes` 的 `poolSlots` 与配置一致**。
- **核对**：附着已有段时，若内核段长与本次计算的 `totalBytes` 不一致，日志会出现 **`ShareMem IPC_STAT shm_segsz=… requested_totalBytes=…`**；若仅 **`SMHead.m_Size`** 与计算值不一致但 key 正确，会打 **`m_Size=… calc=… kernel_seg=… (continuing; key ok)`**（与 `share_mem_linux.cpp` 行为一致）。

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

## 后续工作（概要）

更细的分阶段任务（文档整理、IDA 结构对齐、SQL/日志对齐、池数据驱动等）见开发过程中的**阶段 A～F** 路线图；实现时建议按阶段小步提交，并与 **`D:\Log`（或 `./Log`）样例日志** 对照回归。
