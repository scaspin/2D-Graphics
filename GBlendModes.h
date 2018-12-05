#ifndef Blend_DEFINED
#define Blend_DEFINED

#include "GPaint.h"
#include "GPixel.h"


int float2int255(float f){
  //GASSERT(0 <= f && f <= 1);
  return GFloorToInt(f * 255 + 0.5);
}

GPixel color2pixel(const GColor& color){
  return GPixel_PackARGB(float2int255(color.fA), float2int255(color.fR*color.fA),float2int255(color.fG*color.fA), float2int255(color.fB*color.fA));
}

/*
    next four methods written by Mike Reed for COMP475
*/
// turn 0xAABBCCDD into 0x00AA00CC00BB00DD
uint64_t expand(uint32_t x) {
    uint64_t hi = x & 0xFF00FF00;  // the A and G components
    uint64_t lo = x & 0x00FF00FF;  // the R and B components
    return (hi << 24) | lo;
}
// turn 0xXX into 0x00XX00XX00XX00XX
uint64_t replicate(uint64_t x) {
    return (x << 48) | (x << 32) | (x << 16) | x;
}
// turn 0x..AA..CC..BB..DrD into 0xAABBCCDD
uint32_t compact(uint64_t x) {
    return ((x >> 24) & 0xFF00FF00) | (x & 0xFF00FF);
}
uint32_t quad_mul_div255(uint32_t x, uint8_t invA) {
    uint64_t prod = expand(x) * invA;
    prod += replicate(128);
    prod += (prod >> 8) & replicate(0xFF);
    prod >>= 8;
    return compact(prod);
}

int CLEAR(const GPixel& source, const GPixel& dest){
  return 0;
}

int SRC(const GPixel& source, const GPixel& dest){
  return source;
}

int DST(const GPixel& source, const GPixel& dest){
  return dest;
}

//!<     S + (1 - Sa)*D
//Porter Duff mode for layering colors
int SRC_OVER(const GPixel& source, const GPixel& dest){
    return (source + quad_mul_div255(dest, 255 - GPixel_GetA(source)));
}

//!<     D + (1 - Da)*S
//Porter Duff mode for layering colors
int DST_OVER(const GPixel& source, const GPixel& dest){
    return (dest + quad_mul_div255(source, 255 - GPixel_GetA(dest)));
}

//!<     Da * S
int K_SRC_IN(const GPixel& source, const GPixel& dest){
    return quad_mul_div255(source, GPixel_GetA(dest));
}

//!<     Sa * D
int K_DST_IN(const GPixel& source, const GPixel& dest){
  //return dest;
  //return GPixel_GetA(source)* dest;
  return (quad_mul_div255(dest, GPixel_GetA(source)));

}

//!<     (1 - Da)*S
int K_SRC_OUT(const GPixel& source, const GPixel& dest){
  return quad_mul_div255(source, 255-GPixel_GetA(dest));
}

//!<     (1 - Sa)*D
int K_DST_OUT(const GPixel& source, const GPixel& dest){
  return quad_mul_div255(dest, 255-GPixel_GetA(source));
}

//!<     Da*S + (1 - Sa)*D
int K_SRC_ATOP(const GPixel& source, const GPixel& dest){
  return quad_mul_div255(source,GPixel_GetA(dest))+quad_mul_div255(dest,255-GPixel_GetA(source));
}

//!<     Sa*D + (1 - Da)*S
int K_DST_ATOP(const GPixel& source, const GPixel& dest){
  return quad_mul_div255(dest, GPixel_GetA(source))+quad_mul_div255(source,255-GPixel_GetA(dest));
}

//!<     (1 - Sa)*D + (1 - Da)*S
int K_XOR(const GPixel& source, const GPixel& dest){
  return quad_mul_div255(dest,255-GPixel_GetA(source))+quad_mul_div255(source,255-GPixel_GetA(dest));

}

#endif
