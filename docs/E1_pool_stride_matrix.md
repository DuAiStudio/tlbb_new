# E1 Pool/Stride Alignment Matrix

This document is the E1 baseline for pool-driven migration.  
Goal: compare current segment-size policy with IDA-observed slot stride, without changing runtime behavior.

## Definitions

- `slots`: pool count from config (`defaultPoolSize` or `sharemem.poolsizeN`)
- `attach`: current segment bytes from `attachSizeBytes(type, slots)` (already used in runtime)
- `stride`: IDA-observed slot step size from SMUPool init loops
- `head`: `sizeof(SMHead)` on LP64, fixed at `24`
- `ida_total`: `head + stride * slots` (theoretical total by IDA stride model)

## Default Matrix (current code baseline)

| type | default slots | attach at default | IDA stride | ida_total = 24 + stride\*slots | attach - ida_total |
| --- | ---: | ---: | ---: | ---: | ---: |
| Human | 200000 | 478722520 | 191489 | 38297800024 | -37819077504 |
| Guild | 20000 | 75622420 | 73850 | 1477000024 | -1401377604 |
| Mail | 50000 | 77209620 | 377 | 18850024 | 58359596 |
| PlayerShop | 50000 | 267534356 | 522528 | 26126400024 | -25858865668 |
| GlobalData | 512 | 21524 | 21 | 10776 | 10748 |
| CommissionShop | 50000 | 69830 | 6981 | 349050024 | -348980194 |
| ItemSerial | 1 | 41 | 21 | 45 | -4 |
| PetProcreate | 10000 | 1172500 | 4580 | 45800024 | -44627524 |
| City | 1024 | 9780035 | 38353 | 39273496 | -29493461 |
| GuildLeague | 20000 | 188948 | 369 | 7380024 | -7191076 |
| Auction | 50000 | 93075020 | 2482 | 124100024 | -31025004 |
| TopList | 10000 | 13717 | 13697 | 136970024 | -136956307 |

## How to use with `probe_pool.cmd`

1. Start service normally.
2. Create `probe_pool.cmd` in run directory.
3. Observe `[POOL-PROBE]` lines:
   - `head.key/head.size` for segment header check
   - `slots/stride` for index step sanity
   - `body/s0/s1/s2` for address progression check
4. Compare `head.size` from runtime with expected `attach` and this matrix.
5. Prefer the runtime `POOL-PROBE` fields for batch checks:
   - Per-segment: `ida_total`, `delta`
   - Summary: `ida_aligned`, `ida_delta_nonzero`, `ida_delta_pos`, `ida_delta_neg`, `ida_delta_abs_max`
   - Result line: `[POOL-PROBE][RESULT] verdict=OK/WARN/FAIL ...`

## Interpretation Notes

- Large negative `attach - ida_total` means current `attach` is much smaller than pure `stride*slots` model.
- This is expected at E1 stage because current `attachSizeBytes` still follows reference runtime totals/log-compatible scaling, not full in-memory object reconstruction.
- E2/E3 can move selected SMUs toward real object-layout-driven sizing only after field-level pool model is validated.
