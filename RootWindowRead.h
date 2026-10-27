// SPDX-License-Identifier: BSD-2-Clause-Patent
#pragma once
// Shared production window reader. Ops owns synchronization, block I/O,
// integrity verification and copying to the caller. No callback runs under
// this guard: the IOKit wrapper completes the request after this returns.
// The boot progress layer, when the kernel links it. Weak so the host test, which has no such
// layer, compiles and links this header unchanged; taking the address is what keeps that
// testable, because a bare call of a static declaration has internal linkage and no body.
#if defined (__APPLE__) && defined (KERNEL)
extern "C" void boot_progress_poll (void) __attribute__ ((weak_import));
#define UFS_PROGRESS_POLL() do { if (&boot_progress_poll) boot_progress_poll (); } while (0)
#else
#define UFS_PROGRESS_POLL() do { } while (0)
#endif
static bool ufsWindowGeometry (uint64_t absolute, uint64_t bytes)
{
  constexpr uint64_t limit = (uint64_t (UINT32_MAX) + 1) * 4096;
  return bytes && !(bytes & 511) && !(absolute & 4095) && absolute < limit && bytes <= limit - absolute;
}
struct UfsWindowResult { IOReturn status; uint64_t done; };
template <class Ops> struct UfsWindowGuard {
  Ops &ops;
  explicit UfsWindowGuard (Ops &value) : ops(value) { ops.lock (); }
  ~UfsWindowGuard () { ops.unlock (); }
  UfsWindowGuard (const UfsWindowGuard &) = delete;
  UfsWindowGuard &operator= (const UfsWindowGuard &) = delete;
};
template <class Ops>
static UfsWindowResult ufsReadWindow (Ops &ops, uint64_t absolute, uint64_t bytes,
                                     uint64_t block, uint64_t count, uint8_t *bounce)
{
  if (!bounce || !ufsWindowGeometry (absolute, bytes) || !count || block >= bytes / 512 || count > bytes / 512 - block)
    return { kIOReturnBadArgument, 0 };
  UfsWindowGuard<Ops> guard (ops);
  uint64_t start = absolute + block * 512, remaining = count * 512;
  UfsWindowResult result = { kIOReturnSuccess, 0 };
  while (remaining) {
    uint64_t lba = (start + result.done) / 4096, skip = (start + result.done) % 4096;
    uint64_t blocks = (skip + remaining + 4095) / 4096;
    if (blocks > kDataMax / 4096) blocks = kDataMax / 4096;
    result.status = ops.read (lba, static_cast<uint32_t> (blocks), bounce);
    if (result.status != kIOReturnSuccess) break;
    // The window lock protects this same snapshot through verify AND copy.
    // The transport lock alone protects only the DMA-to-bounce copy.
    for (uint64_t index = 0; index < blocks; ++index) {
      uint64_t windowBlock = lba + index - absolute / 4096;
      uint64_t offset = windowBlock * 4096;
      if (offset >= bytes) { result.status = kIOReturnBadArgument; break; }
      uint64_t length = bytes - offset < 4096 ? bytes - offset : 4096;
      result.status = ops.verify (windowBlock, bounce + index * 4096, static_cast<size_t> (length));
      if (result.status != kIOReturnSuccess) break;
    }
    if (result.status != kIOReturnSuccess) break;
    /* One progress poll per transport command, not one per 4 KiB block. It was inside
     * ops.verify(), so a full-window command took 32 lock acquisitions of the progress
     * mutex; the counter it reports only moves once per block and the panel reads it once a
     * second, so the extra 31 were pure cost. The macro is empty in the host test, which has
     * no progress layer. */
    UFS_PROGRESS_POLL ();
    uint64_t usable = blocks * 4096 - skip;
    if (usable > remaining) usable = remaining;
    uint64_t copied = ops.copy (result.done, bounce + skip, usable);
    if (copied > usable) { result.status = kIOReturnIOError; break; }
    result.done += copied;
    if (copied != usable) { result.status = kIOReturnIOError; break; }
    remaining -= usable;
  }
  return result;
}
