#include "GShader.h"
#include "GMatrix.h"
//#include "GTriShader.h"
#include <vector>


class TriangleGradient : public GShader {
public:
   TriangleGradient(GPoint p0, GPoint p1, GPoint p2, GColor c0, GColor c1, GColor c2){
      fP0 = p0;
      fP1 = p1;
      fP2 = p2;
      fC0 = c0;
      fC1 = c1;
      fC2 = c2;
   }

   bool isOpaque() override{
     return false;
   }

   // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
   bool setContext(const GMatrix& ctm) override{

     GPoint U = fP1-fP0;
     GPoint V = fP2-fP0;
     fLocalMatrix.set6(U.x(), V.x(), fP0.x(), U.y(), V.y(), fP0.y());

     dc0 = GColor::MakeARGB(fC1.fA-fC0.fA,fC1.fR-fC0.fR,fC1.fG-fC0.fG,fC1.fB-fC0.fB);
     dc1 = GColor::MakeARGB(fC2.fA-fC0.fA,fC2.fR-fC0.fR,fC2.fG-fC0.fG,fC2.fB-fC0.fB);

     fLocalMatrix.invert(&gInverse);
     fInverse.setIdentity();

     if (!ctm.invert(&fInverse)) {
       return false;
     }

     ctm.invert(&fInverse);

     P = gInverse;
     P.preConcat(fInverse);
     return true;

   }

  /**
   *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
   *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
   *  can hold at least [count] entries.
   */
   void shadeRow(int x, int y, int count, GPixel row[]) override{
     GPoint local = fInverse.mapXY(x + 0.5, y + 0.5);
     //C[0] = P’x * DC1 + P’y * DC2 + C0
     float fx = gInverse.mapXY(local.fX, local.fY).fX;
     float fy = gInverse.mapXY(local.fX, local.fY).fY;
     GColor c;
     c.fA = fx*dc0.fA + fy*dc1.fA + fC0.fA;
     c.fR = fx*dc0.fR + fy*dc1.fR + fC0.fR;
     c.fG = fx*dc0.fG + fy*dc1.fG + fC0.fG;
     c.fB = fx*dc0.fB + fy*dc1.fB + fC0.fB;
     c.fA = std::max(0.0f, std::min(1.0f, c.fA));
     c.fR = std::max(0.0f, std::min(1.0f, c.fR));
     c.fG = std::max(0.0f, std::min(1.0f, c.fG));
     c.fB = std::max(0.0f, std::min(1.0f, c.fB));

     row[0] = color2pixel(c);

     GColor dc;
     dc.fA = P[0]*dc0.fA + P[3]*dc1.fA ;
     dc.fR = P[0]*dc0.fR + P[3]*dc1.fR ;
     dc.fG = P[0]*dc0.fG + P[3]*dc1.fG ;
     dc.fB = P[0]*dc0.fB + P[3]*dc1.fB ;


     for (int i = 1; i < count; ++i){
          GColor nc;
          //C[i] = C[i - 1] + DC
          nc.fA = c.fA + dc.fA ;
          nc.fR = c.fR + dc.fR ;
          nc.fG = c.fG + dc.fG ;
          nc.fB = c.fB + dc.fB ;

          nc.fA = std::max(0.0f, std::min(1.0f, nc.fA));
          nc.fR = std::max(0.0f, std::min(1.0f, nc.fR));
          nc.fG = std::max(0.0f, std::min(1.0f, nc.fG));
          nc.fB = std::max(0.0f, std::min(1.0f, nc.fB));

        	row[i] = color2pixel(nc);

          c = nc;
      }
    }

private:
  GPoint fP0;
  GPoint fP1;
  GPoint fP2;

  GColor fC0;
  GColor fC1;
  GColor fC2;

  GColor dc0, dc1;

  GMatrix fLocalMatrix;
  GMatrix fInverse;
  GMatrix gInverse;
  GMatrix P;

  int float2int255(float f){
    //GASSERT(0 <= f && f <= 1);
    return GFloorToInt(f * 255 + 0.5);
  }

  GPixel color2pixel(const GColor& color){
    return GPixel_PackARGB(float2int255(color.fA), float2int255(color.fR*color.fA),float2int255(color.fG*color.fA), float2int255(color.fB*color.fA));
  }

};
