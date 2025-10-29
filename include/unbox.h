#ifndef UNBOX_H
#define UNBOX_H

#include "types.h"

// Load all available cases
int get_available_cases(Case *out_cases, int *count);

// Unbox a case
int unbox_case(int user_id, int case_id, Skin *out_skin);

// Calculate drop rates
void calculate_drop_rates(Case *case_data);

// RNG for unbox
int roll_unbox(Case *case_data);

#endif // UNBOX_H
