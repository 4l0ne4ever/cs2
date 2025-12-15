// utils.c - Utility Functions

#include "../include/types.h"
#include <stdio.h>
#include <string.h>

// Convert rarity to string
const char *rarity_to_string(SkinRarity rarity)
{
    switch (rarity)
    {
    case RARITY_CONSUMER:
        return "Consumer";
    case RARITY_INDUSTRIAL:
        return "Industrial";
    case RARITY_MIL_SPEC:
        return "Mil-Spec";
    case RARITY_RESTRICTED:
        return "Restricted";
    case RARITY_CLASSIFIED:
        return "Classified";
    case RARITY_COVERT:
        return "Covert";
    case RARITY_CONTRABAND:
        return "Contraband";
    default:
        return "Unknown";
    }
}

// Convert wear float to string label (FN, MW, FT, WW, BS)
const char *wear_to_string(WearCondition wear_float)
{
    if (wear_float < 0.07f)
        return "FN"; // Factory New
    else if (wear_float < 0.15f)
        return "MW"; // Minimal Wear
    else if (wear_float < 0.37f)
        return "FT"; // Field-Tested
    else if (wear_float < 0.45f)
        return "WW"; // Well-Worn
    else
        return "BS"; // Battle-Scarred
}

// Format skin name with StatTrak and wear condition
void format_skin_name(const Skin *skin, char *out_buffer, size_t buffer_size)
{
    if (!skin || !out_buffer || buffer_size == 0)
        return;

    const char *stattrak_prefix = skin->is_stattrak ? "StatTrak™ " : "";
    const char *wear_label = wear_to_string(skin->wear);
    
    snprintf(out_buffer, buffer_size, "%s%s (%s, Pattern #%d)",
             stattrak_prefix, skin->name, wear_label, skin->pattern_seed);
}

// Format skin display string with all attributes
void format_skin_display(const Skin *skin, char *out_buffer, size_t buffer_size)
{
    if (!skin || !out_buffer || buffer_size == 0)
        return;

    const char *stattrak_prefix = skin->is_stattrak ? "StatTrak™ " : "";
    const char *wear_label = wear_to_string(skin->wear);
    const char *rarity_name = rarity_to_string(skin->rarity);
    
    snprintf(out_buffer, buffer_size, "%s%s | %s | Float: %.10f | Pattern: #%d | $%.2f",
             stattrak_prefix, skin->name, wear_label, skin->wear, skin->pattern_seed, skin->current_price);
}

// Generate unique ID
int generate_id(int *counter)
{
    return (*counter)++;
}
