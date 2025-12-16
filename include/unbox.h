#ifndef UNBOX_H
#define UNBOX_H

#include "types.h"

// Constants
#define CASE_KEY_PRICE 2.50f  // Key price in USD (standard CS2 key price)

// Load all available cases
int get_available_cases(Case *out_cases, int *count);

// Unbox a case (deducts balance, creates skin instance, adds to inventory)
int unbox_case(int user_id, int case_id, Skin *out_skin);

// Calculate drop rates
void calculate_drop_rates(Case *case_data);

// RNG for unbox (legacy function, kept for compatibility)
int roll_unbox(Case *case_data);

// Animation simulation: Generate preview skins for animation display
// Returns array of preview skins that can be shown during unbox animation
int generate_unbox_preview(int case_id, Skin *out_preview_skins, int preview_count);

// Broadcast rare drop: Log and notify about rare unboxed items
// This can be used by server to broadcast to all connected clients
void broadcast_rare_unbox(int user_id, const Skin *unboxed_skin);

#endif // UNBOX_H
