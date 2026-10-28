#ifndef NABO_KITCHEN_STORE_H
#define NABO_KITCHEN_STORE_H
#include "kitchen_journal.h"
/* NVS uses a dedicated namespace and two alternating versioned blobs. */
kj_io nabo_kitchen_store_io(void);
#endif
