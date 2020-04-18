#ifndef ANDROID_NDK_VERSION_H
#define ANDROID_NDK_VERSION_H

/**
 * Major vision of this NDK.
 *
 * For example: 16 for r16.
 */
#define __NDK_MAJOR__ 16

/**
 * Minor vision of this NDK.
 *
 * For example: 1 for r16b.
 */
#define __NDK_MINOR__ 0

/**
 * Beta vision of this NDK.
 *
 * For example: 1 for r16 beta 1, 0 for r16.
 */
#define __NDK_BETA__ 0

/**
 * Build number for this NDK.
 *
 * For a local development build of the NDK, this is -1.
 */
#define __NDK_BUILD__ 4442984

/**
 * Set to 1 if this is a canary build, 0 if not.
 */
#define __NDK_CANARY__ 0

#endif  /* ANDROID_NDK_VERSION_H */
