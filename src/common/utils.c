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

// Convert wear to string
const char *wear_to_string(WearCondition wear)
{
    switch (wear)
    {
    case WEAR_FN:
        return "FN";
    case WEAR_MW:
        return "MW";
    case WEAR_FT:
        return "FT";
    case WEAR_WW:
        return "WW";
    case WEAR_BS:
        return "BS";
    default:
        return "Unknown";
    }
}

// Generate unique ID
int generate_id(int *counter)
{
    return (*counter)++;
}
