#ifndef _LINUX_EXPORT_H
#define _LINUX_EXPORT_H

/*
 * Export symbols from the kernel to modules.  Forked from module.h
 * to reduce the amount of pointless cruft we feed to gcc when only
 * exporting a simple symbol or two.
 *
 * Try not to add #includes here.  It slows compilation and makes kernel
 * hackers place grumpy comments in header files.
 */

#ifndef __ASSEMBLY__

#ifdef MODULE
extern struct module __this_module;
#define THIS_MODULE (&__this_module)
#else
#define THIS_MODULE ((struct module *)0)
#endif

struct namespace_import {
	const char *namespace;
};

#define NS_SEPARATOR "."

#define MODULE_IMPORT_NS(ns)						\
	static const struct namespace_import __knsimport_##ns		\
	asm("__knsimport_" #ns)						\
	__attribute__((section("__knsimport"), used))			\
	= { #ns }

#ifdef CONFIG_MODULES

#if defined(__KERNEL__) && !defined(__GENKSYMS__)
#ifdef CONFIG_MODVERSIONS
/* Mark the CRC weak since genksyms apparently decides not to
 * generate a checksums for some symbols */
#if defined(CONFIG_MODULE_REL_CRCS)
#define __CRC_SYMBOL(sym, sec)						\
	asm("	.section \"___kcrctab" sec "+" #sym "\", \"a\"	\n"	\
	    "	.weak	__crc_" #sym "				\n"	\
	    "	.long	__crc_" #sym " - .			\n"	\
	    "	.previous					\n");
#else
#define __CRC_SYMBOL(sym, sec)						\
	asm("	.section \"___kcrctab" sec "+" #sym "\", \"a\"	\n"	\
	    "	.weak	__crc_" #sym "				\n"	\
	    "	.long	__crc_" #sym "				\n"	\
	    "	.previous					\n");
#endif
#else
#define __CRC_SYMBOL(sym, sec)
#endif

#ifdef CONFIG_HAVE_ARCH_PREL32_RELOCATIONS
#include <linux/compiler.h>
/*
 * Emit the ksymtab entry as a pair of relative references: this reduces
 * the size by half on 64-bit architectures, and eliminates the need for
 * absolute relocations that require runtime processing on relocatable
 * kernels.
 */
#define __KSYMTAB_ENTRY(sym, sec, ns, nspost, nspost2)					\
	__ADDRESSABLE(sym)						\
	asm("	.section \"___ksymtab" sec "+" #sym "\", \"a\"	\n"	\
	    "	.balign	8					\n"	\
	    "__ksymtab_" #sym nspost2 ":				\n"	\
	    "	.long	" #sym "- .				\n"	\
	    "	.long	__kstrtab_" #sym #nspost "- .			\n"	\
	    "	.long	__kstrtab_" #sym #nspost "- .			\n"	\
	    "	.previous					\n")

struct kernel_symbol {
	int value_offset;
	int name_offset;
    int namespace_offset;
};
#else
#define __KSYMTAB_ENTRY(sym, sec, ns, nspost, nspost2)			\
	static const struct kernel_symbol __ksymtab_##sym##nspost	\
    asm("__ksymtab_" #sym nspost2)
	__attribute__((section("___ksymtab" sec "+" #sym), used))	\
	__attribute__((aligned(sizeof(void *))))                        \
	= { (unsigned long)&sym, __kstrtab_##sym##nspost, ns }

struct kernel_symbol {
	unsigned long value;
	const char *name;
	const char *namespace;
};
#endif

/* For every exported symbol, place a struct in the __ksymtab section */
#define ___EXPORT_SYMBOL(sym, sec, ns, nspost, nspost2)			\
	extern typeof(sym) sym;						\
	__CRC_SYMBOL(sym, sec)						\
	static const char __kstrtab_##sym##nspost[]				\
	__attribute__((section("__ksymtab_strings"), used, aligned(1)))	\
	= #sym;								\
	__KSYMTAB_ENTRY(sym, sec, ns, nspost, nspost2)

#if defined(__DISABLE_EXPORTS)

/*
 * Allow symbol exports to be disabled completely so that C code may
 * be reused in other execution contexts such as the UEFI stub or the
 * decompressor.
 */
#define __EXPORT_SYMBOL(sym, sec, ns, nspost, nspost2)

#elif defined(CONFIG_TRIM_UNUSED_KSYMS)

#include <generated/autoksyms.h>

/*
 * For fine grained build dependencies, we want to tell the build system
 * about each possible exported symbol even if they're not actually exported.
 * We use a symbol pattern __ksym_marker_<symbol> that the build system filters
 * from the $(NM) output (see scripts/gen_ksymdeps.sh). These symbols are
 * discarded in the final link stage.
 */
#define __ksym_marker(sym)	\
	static int __ksym_marker_##sym[0] __section(".discard.ksym") __used

#define __EXPORT_SYMBOL(sym, sec, ns, nspost, nspost2)			\
	__ksym_marker(sym);					\
	__cond_export_sym(sym, sec, ns, nspost, nspost2,		\
			  __is_defined(__KSYM_##sym))
#define __cond_export_sym(sym, sec, ns, nspost, nspost2, conf)		\
	___cond_export_sym(sym, sec, ns, nspost, nspost2, conf)
#define ___cond_export_sym(sym, sec, ns, nspost, nspost2, enabled)	\
	__cond_export_sym_##enabled(sym, sec, ns, nspost, nspost2)
#define __cond_export_sym_1(sym, sec, ns, nspost, nspost2)		\
	___EXPORT_SYMBOL(sym, sec, ns, nspost, nspost2)
#define __cond_export_sym_0(sym, sec, ns, nspost, nspost2) /* nothing */

#else
#define __EXPORT_SYMBOL ___EXPORT_SYMBOL
#endif

#define EXPORT_SYMBOL(sym)					\
	__EXPORT_SYMBOL(sym, "", NULL, ,)

#define EXPORT_SYMBOL_GPL(sym)					\
	__EXPORT_SYMBOL(sym, "_gpl", NULL, ,)

#define EXPORT_SYMBOL_GPL_FUTURE(sym)				\
	__EXPORT_SYMBOL(sym, "_gpl_future", NULL, ,)

#define EXPORT_SYMBOL_NS(sym, ns)                               \
	__EXPORT_SYMBOL(sym, "", #ns, __##ns, NS_SEPARATOR #ns)

#define EXPORT_SYMBOL_NS_GPL(sym, ns)                               \
	__EXPORT_SYMBOL(sym, "_gpl", #ns, __##ns, NS_SEPARATOR #ns)

#ifdef CONFIG_UNUSED_SYMBOLS
#define EXPORT_UNUSED_SYMBOL(sym) __EXPORT_SYMBOL(sym, "_unused", , ,)
#define EXPORT_UNUSED_SYMBOL_GPL(sym) __EXPORT_SYMBOL(sym, "_unused_gpl", , ,)
#else
#define EXPORT_UNUSED_SYMBOL(sym)
#define EXPORT_UNUSED_SYMBOL_GPL(sym)
#endif

#endif	/* __KERNEL__ && !__GENKSYMS__ */

#if defined(__GENKSYMS__)
/*
 * When we're running genksyms, ignore the namespace and make the _NS
 * variants look like the normal ones. There are two reasons for this:
 * 1) In the normal definition of EXPORT_SYMBOL_NS, the 'ns' macro
 *    argument is itself not expanded because it's always tokenized or
 *    concatenated; but when running genksyms, a blank definition of the
 *    macro does allow the argument to be expanded; if a namespace
 *    happens to collide with a #define, this can cause issues.
 * 2) There's no need to modify genksyms to deal with the _NS variants
 */
#define EXPORT_SYMBOL_NS(sym, ns)                              \
	EXPORT_SYMBOL(sym)
#define EXPORT_SYMBOL_NS_GPL(sym, ns)                          \
	EXPORT_SYMBOL_GPL(sym)
#endif

#else /* !CONFIG_MODULES... */

#define EXPORT_SYMBOL(sym)
#define EXPORT_SYMBOL_NS(sym, ns)
#define EXPORT_SYMBOL_NS_GPL(sym, ns)
#define EXPORT_SYMBOL_GPL(sym)
#define EXPORT_SYMBOL_GPL_FUTURE(sym)
#define EXPORT_UNUSED_SYMBOL(sym)
#define EXPORT_UNUSED_SYMBOL_GPL(sym)

#define MODULE_IMPORT_NS(ns)
#endif /* CONFIG_MODULES */
#endif /* !__ASSEMBLY__ */

#endif /* _LINUX_EXPORT_H */
