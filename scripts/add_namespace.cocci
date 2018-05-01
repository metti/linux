@has_ns_import@
declarer name MODULE_IMPORT_NS;
identifier virtual.ns;
@@
MODULE_IMPORT_NS(ns);

@has_module_license@
declarer name MODULE_LICENSE;
expression license;
@@
MODULE_LICENSE(license);

@do_import depends on has_module_license && !has_ns_import@
declarer name MODULE_LICENSE;
expression license;
identifier virtual.ns;
@@
MODULE_LICENSE(license);
+ MODULE_IMPORT_NS(ns);
