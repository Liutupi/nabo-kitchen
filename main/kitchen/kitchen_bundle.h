#ifndef KITCHEN_BUNDLE_H
#define KITCHEN_BUNDLE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
  const void *data;
  size_t size;
} kb_blob;
typedef struct {
  kb_blob recipes, fonts, dish, icons, ui, animation;
} kb_assets;
/* Validates the complete bundle, including each payload CRC and bounded index.
 */
bool kb_open(const void *data, size_t size, kb_assets *out);
/* Same checks; invoke progress after each <=4096-byte checksum slice. Caller
 * may block/yield from its worker. No RTOS dependency or validation bypass. */
bool kb_open_cooperative(const void *data, size_t size, kb_assets *out,
                         void (*progress)(void *), void *context);
#endif
