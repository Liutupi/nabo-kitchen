#pragma once
#include <stdbool.h>
/* Starts isolated device tasks after NVS, display, audio and battery setup. */
bool offline_kitchen_start(bool storage_ready, bool audio_ready);
