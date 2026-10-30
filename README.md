# NativeUfs-alioth

UFS driver fork for the Xiaomi Redmi K40 (`alioth`, SM8250-AC). It follows
upstream [ShengUfs](https://github.com/UniXNU/ShengUfs) for the controller and
the root-volume path, and carries this board's transport configuration.

| Repository | Contents |
| --- | --- |
| [ShengUfs](https://github.com/UniXNU/ShengUfs) | upstream: SM8550 transport and sheng persistent storage |
| [NativeUfs-alioth](https://github.com/FFT-infra/NativeUfs-alioth) | this fork: SM8250 identity, dual-stream takeover, the persistent log window read back |

## What the fork changes

- Accepts the SM8250 controller identity and takes the transport once, rather
  than retrying a probe that already succeeded (`ab54059`).
- Spins on command completion and polls boot progress per command (`df39b05`).
- Reads the persistent diagnostic log window back (`8ef95fc`), which is what
  `scripts/read-sheng-log.py` decodes through TWRP in the other boot slot.

## Streams

Upstream defaults to sheng's stream `0x60`. This board uses `0xe0` and `0x4e0`
(`native-ufs-stream` and `native-ufs-stream2`), found as exact mappings on
exclusive bypass contexts, so the driver validates and reuses them. Dual-stream
takeover stays a fallback for unsafe mappings.

## Build

Selected from the device repository's `feeds.toml` as the `ShengUfs` source; the
device repository also lists it in `board.toml` archives. Build through the
UniXNU CLI:

```sh
./unixnu build kernel -D ../UniXNU-alioth
```

Read [UFS porting](https://unixnu.com/zh/porting/ufs), the
[driver page](https://unixnu.com/zh/drivers/community-native-ufs-alioth) and
[`AGENTS.md`](AGENTS.md). Build and interface details:
[documentation](https://unixnu.com/drivers/sheng-ufs). Component sources and
dependencies: [catalog](https://unixnu.com/reference/components).

Device page: <https://unixnu.com/zh/devices/alioth>.
Porting record: [`../UniXNU-alioth/porting.toml`](../UniXNU-alioth/porting.toml).
