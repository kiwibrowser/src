/**
 * Powf may leave an unresolved symbol pointing to a libstdc++.so powf.
 * However, not all libstdc++.so include this function, so optionally
 * replace the powf function with calls to expf and logf.
 */

#ifdef VMW_RESOLVE_POWF

extern float expf(float x);
extern float logf(float x);
extern float powf(float x, float y);

float powf(float x, float y) {
    return expf(logf(x)*y);
}

#endif
