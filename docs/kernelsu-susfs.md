# KernelSU and SUSFS in this kernel

`drivers/kernelsu` holds the SukiSU-Ultra kernel source, branch `builtin`, commit
`1cfa57340590971cba7521ed536da8ebbedc5f0a`, vendored unchanged. That branch is the one that
carries `CONFIG_KSU_SUSFS`, the `CMD_SUSFS_*` supercall dispatch (`supercall/dispatch.c`)
and the hook entry points the kernel has to call; `main` and KernelSU-Next keep SUSFS
out of the kernel.

The SUSFS4KSU patch `0001-susfs-android13-5.15.patch` (kept in
`tickernelz/android_kernel_meizu_mblu21` under `patches/`) is applied on top. Besides
`fs/susfs.c` and the proc, selinux and mm hooks, it inserts the KernelSU hook call sites,
so no hand-written hook patch is needed:

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
ksud feature list                                 # su_compat, kernel_umount, selinux_hide enabled
ls /sys/fs/susfs                                  # susfs is built in, no guard LKM needed
ksud susfs status
```

With the driver built in, `init_boot` is the plain source build again: nothing loads
`kernelsu.ko` from the ramdisk. A module that still ships `susfs_guard_lkm.ko` only
duplicates it and has to be disabled.
