#ifndef GUARD_ACE_MACROS_H
#define GUARD_ACE_MACROS_H

/**
 * ACE Macros
 * General purpose macros used throughout all code
 * Insert all somewhat universal macros here even if used only by single module
 * so future expansion will keep macro dependencies at bay
 */

/**
 * Block count - determine how many blocks of given size are in given length
 * blockCountFloor - counts only full blocks
 * blockCountCeil - counts also non-filled block
 */
#define blockCountFloor(length, blockSize) (length/blockSize)
#define blockCountCeil(length, blockSize) ((length + blockSize-1)/blockSize)
 
/// Math
// #define abs(x) (x<0 ? -x : x) // already implemented (where?)
#define min(x,y) ((x)<(y)? (x): (y))
#define max(x,y) ((x)>(y)? (x): (y))
#define clamp(x, min, max) ((x) < (min)? (min) : ((x) > (max) ? (max) : (x)))


#endif // GUARD_ACE_MACROS_H
