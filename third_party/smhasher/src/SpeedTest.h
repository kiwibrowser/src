#pragma once

#include "Types.h"

void BulkSpeedTest ( pfHash hash, uint32_t seed );
void TinySpeedTest ( pfHash hash, int hashsize, int keysize, uint32_t seed, bool verbose, double & outCycles );

//-----------------------------------------------------------------------------
