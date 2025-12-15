#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include <stddef.h>

// Convert rarity enum to string
const char *rarity_to_string(SkinRarity rarity);

// Convert wear float to string label (FN, MW, FT, WW, BS)
const char *wear_to_string(WearCondition wear_float);

// Format skin name with StatTrak and wear condition
// Output: "StatTrak™ AK-47 | Redline (FT, Pattern #661)" or "AK-47 | Redline (FT, Pattern #661)"
void format_skin_name(const Skin *skin, char *out_buffer, size_t buffer_size);

// Format skin display string with all attributes
// Output: "StatTrak™ AK-47 | FT | Float: 0.1234567890 | Pattern: #661 | $150.00"
void format_skin_display(const Skin *skin, char *out_buffer, size_t buffer_size);

// Generate unique ID
int generate_id(int *counter);

#endif // UTILS_H

