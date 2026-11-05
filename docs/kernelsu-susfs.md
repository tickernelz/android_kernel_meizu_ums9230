# KernelSU and SUSFS in this kernel

`drivers/kernelsu` holds the SukiSU-Ultra kernel source, branch `builtin`, commit
`1cfa57340590971cba7521ed536da8ebbedc5f0a`, vendored with three local deltas (below). That branch is the one that
carries `CONFIG_KSU_SUSFS`, the `CMD_SUSFS_*` supercall dispatch (`supercall/dispatch.c`)
and the hook entry points the kernel has to call; `main` and KernelSU-Next keep SUSFS
out of the kernel.

The SUSFS4KSU patch `0001-susfs-android13-5.15.patch` (kept in
`tickernelz/android_kernel_meizu_mblu21` under `patches/`, 30 files +4574/-4) is applied on top. Besides
`fs/susfs.c` and the proc, selinux and mm hooks, it inserts the KernelSU hook call sites,
so no hand-written hook bodies are needed; the hook *call sites* it inserts are fixed up in one
place, `include/linux/ksu_su_compat.h`, which is also where `ksu_su_compat_enabled` is declared.
The sites are:

| File | Call |
|---|---|
| `fs/exec.c` | `ksu_handle_execveat`, `ksu_handle_execveat_sucompat`, `ksu_handle_post_execveat_sucompat`, preinit |
| `fs/open.c` | `ksu_handle_faccessat` |
| `fs/stat.c` | `ksu_handle_stat`, `ksu_handle_vfs_fstat` |
| `fs/read_write.c` | `ksu_handle_sys_read` |
| `drivers/input/input.c` | `ksu_handle_input_handle_event` |
| `kernel/sys.c`, `fs/susfs.c` | `ksu_handle_setresuid` |
| `kernel/reboot.c` | `ksu_handle_sys_reboot` |

`arch/arm64/configs/vendor/mblu21.config` turns both on (`CONFIG_KSU=y`,
`CONFIG_KSU_SUSFS=y`); the SUSFS sub-options default to `y` in `drivers/kernelsu/Kconfig`.
The kernel reports uapi version 6, which is what the SukiSU 4.3.0 `ksud` on the phone
expects.

## Verify on the device

```
zcat /proc/config.gz | grep -E '^CONFIG_KSU'      # CONFIG_KSU=y, CONFIG_KSU_SUSFS=y
ksud feature list                                 # kernel_umount starts disabled; enable it with ksud feature set
ls /sys/fs/susfs                                  # susfs is built in, no guard LKM needed
ksud susfs status
```

With the driver built in, `init_boot` is the plain source build again: nothing loads
`kernelsu.ko` from the ramdisk. A module that still ships `susfs_guard_lkm.ko` only
duplicates it and has to be disabled.

## Local deltas against the vendored branch

`1cfa5734` is copied as-is except these, each needed on this tree:

| File | Delta |
|---|---|
| `drivers/kernelsu/Makefile` | `KSU_VERSION`, `VERSION_TAG` and `KSU_VERSION_FULL` come from fixed values for the vendored commit instead of `git` and `api.github.com`; the build namespace has no network, so the computed value used to change per host (41148 now, from `KSU_UPSTREAM_MAIN_COMMITS` = the upstream `main` commit count when this was vendored) |
| `fs/exec.c`, `fs/open.c`, `fs/stat.c` | `ksu_su_compat_enabled` is declared once in `include/linux/ksu_su_compat.h` as `bool` and read with `READ_ONCE()`; declaring it locally as `struct static_key_true` made `__jump_table` resolve one byte off into `ksu_module_mounted` |
| `fs/read_write.c` | the `ksu_handle_sys_read` call passes `&buf` and `&count`, matching the three-argument definition |
| `drivers/kernelsu/feature/sucompat.c` | includes `linux/ksu_su_compat.h`, so the compiler matches the definition with the declaration the `fs/*` hook sites use |
| `drivers/kernelsu/selinux/sepolicy.c` | `susfs_add_sus_avd_allowed_by_type` takes `u32 c_type`, not `u16` |

`CONFIG_KSU` is `default y` inside `drivers/kernelsu/Kconfig`, so any build of this tree that
does not mention it still gets the driver. Whether that default stays is an open decision
(ZHA-4925); until then this work stays on the `zha4900-ksu` branch.
