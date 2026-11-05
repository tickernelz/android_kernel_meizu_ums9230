/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_KSU_SU_COMPAT_H
#define __LINUX_KSU_SU_COMPAT_H

#include <linux/compiler.h>
#include <linux/types.h>

extern bool ksu_su_compat_enabled;

static inline bool ksu_su_compat_active(void)
{
	return READ_ONCE(ksu_su_compat_enabled);
}

#endif
