#include <linux/linkage.h>
#include <linux/init.h>

    __CPUINIT

ENTRY(mt_secondary_startup)
    b secondary_startup
END(mt_secondary_startup)

