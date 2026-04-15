# C1 SQL Alignment Matrix

Scope: `src/db_access.cpp` vs `sql/tlbbdb.sql` stored procedures.

## Already aligned (implemented)

- `markGeneralSetDirty` (`GuildDbRepo` / `CityDbRepo` / `PlayerShopDbRepo`)
  - Now calls `CALL save_general_set(psKey, pnVal)` first.
  - Falls back to direct `INSERT ... ON DUPLICATE KEY UPDATE` when procedure is not deployed.

## Deferred (requires pool object field migration first)

These procedures take many per-object fields (`poolid`, blob/text payloads, timestamps, item/pet data, etc.).  
Current project stage mainly has scheduler + coarse DB touch SQL, not full in-memory SMU object serialization.

- Guild: `save_guildinfo_new`, `save_guild_user`
- PlayerShop: `save_shopinfo_new`, `save_shopinfo_stall`, `save_shopinfo_stall_buy`, `save_shopinfo_stall_itm`, `save_shopinfo_stall_pet`
- City: `save_cityinfo_new` (and related `save_cityinfo_bld`, `save_cityinfo_ext`)
- Auction: `save_auction_unit` (and `save_auction_itm`, `save_auction_pet`)
- GlobalData: `save_global`
- Mail: `save_mailinfo`
- GuildLeague: `save_league`, `save_league_usr`, `save_league_apply`
- CommissionShop: `save_cshop`, `save_cshopitem`
- PetProcreate: `save_petiteminfo`

## Keep as coarse SQL in current stage (intentional)

- Human:
  - `saveAll`: bulk `UPDATE t_char ...`
  - `saveAllExtra`: bulk `UPDATE t_charextra ...`
- ItemSerial:
  - `saveAll`: monotonic bump SQL in `t_itemkey`
- TopList:
  - `saveAll` / `saveAllByType`: coarse touch update

Rationale: these are currently used to keep scheduler/log behavior and DB activity shape stable before full pool-data path (phase E).

## Next step (C2)

- Verify `t_general_set` writes in runtime:
  - `GUILD_NEW`, `PSHOP_NEW`, `CITY_NEW`
- Confirm both procedure path and fallback path behavior on your DB deployment.
