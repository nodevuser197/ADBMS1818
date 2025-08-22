# ADBMS1818 / LTC6813 Library

Driver library for the Analog Devices **ADBMS1818** (18-cell battery monitor),
LTC6813 is basically the same chip so it works there too. It's plain C. Made it
for an STM32 project originally but i kept the core free of any HAL so you can
move it to ESP32 / Arduino / RP2040 with only a small adapter file (explained
further down).

Does the normal battery-monitor things — cell voltages, aux/GPIO, status regs,
balancing, config, selftests, open-wire — and on top of that there's a generic
COMM-register bridge so you can talk to *any* external I2C or SPI chip wired to
the ADBMS gpio pins. No device specific code baked in, that was kind of the
whole point.

## Software hierarchy (top -> bottom)

Code is layered. Each layer only really knows about the one right under it, so
swapping a layer doesn't drag the rest with it.

```
┌────────────────────────────────────────────────────────────────┐
│  APPLICATION            prog/src/application.c                   │
│  Your battery logic: read loops, balancing, limits, external    │
│  device helpers. This is the layer you actually touch.          │
└───────────────┬────────────────────────────────────────────────┘
                │
┌───────────────▼────────────────────────────────────────────────┐
│  DRIVER / COMMANDS      lib/src/genericType.c                   │
│                         lib/inc/commandList.h (+ .c)            │
│  Builds the ADBMS command frames (ADCV, RDCVx, WRCFG, WRCOMM…), │
│  wraps them with PEC, hands them down to the wrapper.           │
│  commandList just holds the raw 2-byte command codes.           │
└───────────────┬───────────────────────────┬────────────────────┘
                │                           │
┌───────────────▼─────────────┐ ┌───────────▼────────────────────┐
│  PARSE / PACK               │ │  COMM BRIDGE                    │
│  lib/src/parseCreate.c      │ │  lib/src/commHelper.c           │
│  Packs config/comm structs  │ │  Fills the COMM register for    │
│  into wire bytes, parses    │ │  I2C / SPI master transfers to  │
│  the read-back + PEC check  │ │  external chips. No HAL here.   │
│  into the cell_asic_ model  │ │                                 │
│                             │ │  non-blocking engine ->         │
│                             │ │  lib/src/commNb.c               │
│                             │ │  timing math ->                 │
│                             │ │  lib/src/adbmsTiming.c          │
└───────────────┬─────────────┘ └───────────┬────────────────────┘
                │                           │
┌───────────────▼───────────────────────────▼────────────────────┐
│  DATA MODEL             lib/inc/dataSetList.h                    │
│  cell_asic_ struct + all the enums (MD, CH, ICOMM, FCOMM …).    │
│  Every layer talks in terms of this one model.                  │
└───────────────┬────────────────────────────────────────────────┘
                │
┌───────────────▼────────────────────────────────────────────────┐
│  HAL WRAPPER (platform)  prog/src/wrapper.c   (blocking SPI)    │
│                          prog/src/wrapperNb.c (IT/DMA/blocking) │
│  Only files that include the STM32 HAL. Port THESE for another  │
│  MCU and leave everything above alone.                          │
└───────────────┬────────────────────────────────────────────────┘
                │
        ┌───────▼────────┐
        │  MCU / HAL      │  STM32 HAL, ESP-IDF, Arduino core …
        └────────────────┘
```

Rule of thumb: **only the `wrapper*.c` files (and the CI stubs) are allowed to
know about the HAL.** Everything in `lib/` is portable C. If you ever spot a
`#include "stm32..."` in there its a bug, go fix it.

#### Directory map

| Path | what's in it |
|------|--------------|
| `lib/inc`, `lib/src` | portable driver core, no HAL |
| `prog/inc`, `prog/src` | application + platform wrappers |
| `ci/stubs` | fake STM32 headers so GCC/CI can build on a PC |
| `ref/` | html reference + pinout images |
| `Makefile` | compile-check build (uses the stubs) |

## The COMM register — how the bridge works

The ADBMS can act as a **I2C or SPI master** over four of its GPIO pins. You
don't bit-bang those pins yourself, instead you fill a 6 byte COMM register and
the chip replays it onto the bus for you. 6 bytes = **3 slots**, and every slot
looks like:

```
ICOMn (4 bits) | Dn (data byte) | FCOMn (4 bits)
   "before"          payload        "after"
```

* **ICOMn** — what to do *before* the byte: I2C start/stop/blank, or SPI CS low/high.
* **Dn** — the byte (real data on write, dummy `0xFF` on read).
* **FCOMn** — what to do *after*: ACK/NACK, stop, hold or raise CS.

All the legal values are in the `ICOMM` / `FCOMM` enums in `dataSetList.h`, and
`commHelper.c` fills the slots for you (`comm_i2c_write`, `comm_spi_read_cont`
and friends) so you almost never poke raw nibbles by hand.

### one transaction, start to finish

```
fill comm_ struct  ->  WRCOMM  ->  STCOMM  ->  (RDCOMM -> parseComm)
   (RAM only)          (load)      (execute)     (read back, only if reading)
```

1. **commHelper** fills `ic->comm` in RAM. nothing on the wire yet.
2. **WRCOMM** writes those 6 bytes into the chip's COMM register.
3. **STCOMM** tells the chip to replay them on the I2C/SPI bus.
4. **RDCOMM + parseComm** pulls back whatever the slave answered (read only).

### STCOMM is special (this one bites people)

STCOMM holds **CS low the entire time**: the 4-byte command *plus* 72 extra
clocks (9 dummy bytes) all go out inside one single CS-low window. Raise CS right
after the command and the chip just never replays the register — silent fail.
During those 72 clocks MOSI is "dont care", we send `0xFF` only to be explicit
about it. See `adbms1818_Stcomm()`.

## Non-blocking COMM engine (`commNb`)

The plain path in `genericType.c` is blocking, the CPU just sits inside every SPI
transfer waiting. For interrupt / DMA driven systems use the **commNb** engine
instead. It runs the whole `WRCOMM -> STCOMM -> RDCOMM -> parse` chain as an
event driven state machine, so `commNb_Start()` returns right away and the
transaction actually finishes from the SPI ISR.

### the design in one picture

```
            commNb.c  (portable core, NO HAL)
                 │  CommNbBackend = 6 function pointers
                 ▼
        wrapperNb.c (STM32 adapter)  ──►  HAL_SPI_*_IT / _DMA / blocking
```

The core never touches the HAL. It only calls out through a `CommNbBackend` that
you register once. The STM32 adapter (`commNb_stm32_init`) fills that backend in
for you, so on STM32 you end up writing basically **zero** SPI/CS/ISR code.

### state machine

```
Start ─► WRCOMM ─tx─► STCOMM ─tx─► needs_rdcomm?
                                     │yes            │no
                                     ▼               ▼
                               RDCOMM_TX ─tx─►   next frame / done
                               RDCOMM_RX ─rx─► parse ─► next frame / done
```

Every SPI completion (`commNb_OnTxDone` / `commNb_OnRxDone`, called out of the
ISR) pushes it forward one step. There's a re-entrancy guard, which means a
*blocking* backend can call those same hooks inline and the chain just unwinds
to completion in a loop instead — no recursion, no blowing the stack. That's the
trick that lets one single code path cover IT, DMA **and** blocking modes.

### multi-frame

Transfers longer than 3 bytes need more than one frame. Rather than pre-building
some array up front you hand the engine a **frame callback**. After each frame
it asks the callback to fill the next one and say how many are still left.
Because the callback runs every time, a read continuation can depend on bytes
that got parsed out of the previous frame — same as the blocking template loop,
just sliced into steps.

### transfer mode is chosen in code, not a build flag

```c
commNb_stm32_init(&hspi1, GPIOD, GPIO_PIN_2, COMM_NB_MODE_IT);   // interrupt
commNb_stm32_init(&hspi1, GPIOD, GPIO_PIN_2, COMM_NB_MODE_DMA);  // dma
commNb_stm32_init(&hspi1, GPIOD, GPIO_PIN_2, COMM_NB_MODE_BLOCKING);
```

and you can even flip it at runtime later with `commNb_stm32_setMode()`.

### Smart auto-wake

This is the bit that makes the engine actually aware of the chip's power state.
The ADBMS drops into a low power state when the bus goes quiet, and waking it
back up costs time — but only if it really went to sleep. So the engine keeps an
eye on how long the bus has been idle (through the backend's `now_ms`) and just
decides for itself:

| idle time since last activity | chip state | what the engine does |
|------------------------------|------------|----------------------|
| `< 4.3 ms` (tIDLE min)       | still awake | nothing. hot path, **zero cost** |
| `4.3 ms … 1.8 s`            | isoSPI idle / standby | cheap wake (~10 µs / IC) |
| `> 1.8 s` (tWDT min)        | asleep | full wake (~400 µs / IC) |

The thresholds are the datasheet *minimums* on purpose — rather wake a hair too
early than try to talk to a chip thats already asleep. And because COMM is only
a gpio-level bridge (it doesnt use the ADC reference at all), a cold wake just
needs `tWAKE`, not the 4.4 ms `tREFUP`. In a BMS that polls constantly the idle
time is pretty much always under 4.3 ms anyway, so on the normal path the wake
logic costs you nothing.

If some *other* code talks to the chain (a blocking read, a config write) then
call `commNb_NoteActivity()` right after, that restarts the idle timer so the
engine wont go and wake a chip thats already awake for no reason.

> heads up: auto-wake only does anything when the backend gives it a `now_ms()`.
> Leave it `NULL` and waking the chain is back on you (the old behaviour).

## Timing helpers (`adbmsTiming`)

Instead of scattering magic `HAL_Delay(3)` numbers everywhere this module hands
you datasheet-grounded values, keyed on mode and device count where it actually
matters:

```c
uint32_t conv = adbms_adcvTime_us(ADC_7_KHZ, 0);          // 2488 us, 18 cells
uint32_t wait = adbms_pollWait_us(conv, from_standby);     // adds tREFUP if needed
uint32_t wake = adbms_wakeTime_us(TOTAL_IC, ADBMS_FROM_SLEEP);
uint32_t gap  = adbms_cmdGap_ns(TOTAL_IC);                 // CSB-high t5/t6
```

One handy gotcha it bakes in: in a daisy-chain **every device converts at the
same time**, so the ADC wait does NOT scale with how many devices you have — you
just wait the raw conversion time. Device count only matters for wake-up and the
inter-command CSB-high gap, nothing else.

## Porting to a different MCU

1. copy `prog/src/wrapper.c` / `wrapperNb.c` into a new adapter for your platform.
2. implement the few primitives — `cs_low/high`, blocking SPI, and for the
   non-blocking engine the `CommNbBackend` ones: `tx_async`, `rx_async`, plus
   the optional `delay_ns` and `now_ms`.
3. point your SPI completion ISR at `commNb_OnTxDone` / `commNb_OnRxDone`. (the
   STM32 adapter already does this with strong HAL callbacks — set
   `COMM_NB_DEFINE_CALLBACKS=0` if you'd rather own those yourself.)

Everything under `lib/` compiles unchanged. The public headers already have the
`extern "C"` guards so an Arduino C++ project can just include them as-is.

## Building (CI compile-check)

You dont need an ARM toolchain to sanity check the code. The `Makefile` builds
the lib against the fake HAL in `ci/stubs/` with plain host GCC:

```sh
make            # -> "Build OK"
make clean
```

This is what the GitHub Actions workflow runs on every push. Wont catch runtime
bugs obviously, but it keeps the syntax, types and the PEC/packing logic honest
across platforms.
