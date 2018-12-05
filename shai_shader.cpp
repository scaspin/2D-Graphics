
#include "GShader.h"
#include "GBitmap.h"
#include "GMatrix.h"

class BitmapShader : public GShader {
public:
   BitmapShader(const GBitmap& bitmap, const GMatrix& localM, TileMode mode) : fBitmap(bitmap), fLocalMatrix(localM), tileMode(mode){
   }

  // Return true iff all of the GPixels that may be returned by this shader will be opaque.
   bool isOpaque() override{
     return this->fBitmap.isOpaque();
   }

   // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
   bool setContext(const GMatrix& ctm) override{

     //no inverse exists
     if (!ctm.invert(&fInverse)) {
          return false;
     }

     // after fInverse is computed from the ctm
     GMatrix localInverse;
     fLocalMatrix.invert(&localInverse);
	    fInverse.postConcat(localInverse);
     //fInverse.postScale(1/fBitmap.width(), 1/fBitmap.height());

     //apply this transformation
     GMatrix tmp;
     tmp.setConcat(ctm, fLocalMatrix);
     return tmp.invert(&fInverse);


   }

  /**
   *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
   *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
   *  can hold at least [count] entries.
   */
   void shadeRow(int x, int y, int count, GPixel row[]) override{
     //GPoint local = fInverse.mapXY(x + 0.5f, y + 0.5f);
     GPoint local = fInverse.mapXY(x+0.5f, y+0.5f);

     for (int i = 0; i < count; ++i) {

         float sourceX = local.x()/fBitmap.width();
         float sourceY = local.y()/fBitmap.height();

         if (tileMode==kClamp){
           sourceX = std::min(std::max(sourceX, 0.0f), 0.9999f);
           sourceY = std::min(std::max(sourceY,0.0f), 0.9999f);
         }else if(tileMode==kRepeat){
             sourceX = sourceX - GFloorToInt(sourceX);
             sourceY = sourceY - GFloorToInt(sourceY);
         }else if(tileMode==kMirror){
             //t = t * 0.5
             sourceX *= 0.5;
             sourceY *= 0.5;
             // t = t - floor(t)
             sourceX -= floor(sourceX);
             sourceY -= floor(sourceY);
             //if (t > 0.5)
             //t = 1 - t
             if (sourceX>0.5){
               sourceX = 1 - sourceX;
             }
             if (sourceY>0.5){
               sourceY = 1 - sourceY;
             }
             sourceX *= 2;
             sourceY *= 2;
         }

         // sourceX = std::min(std::max(sourceX, 0), 1);
         // sourceY = std::min(std::max(sourceY,0), 1);

         int isourceX = GFloorToInt(sourceX * fBitmap.width());
         int isourceY = GFloorToInt(sourceY * fBitmap.height());

         isourceX = std::max(0, std::min(fBitmap.width()-1, isourceX));
         isourceY = std::max(0, std::min(fBitmap.height()-1, isourceY));

         row[i] = *fBitmap.getAddr(isourceX, isourceY);

         local.fX += fInverse[0];
         local.fY += fInverse[3];

     }

   }

private:
  TileMode tileMode;
  GBitmap fBitmap;
  GMatrix fLocalMatrix;
//  GMatrix fLocalInverse;
  GMatrix fInverse;

};

std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& bitmap, const GMatrix& localM, BitmapShader::TileMode mode) {
    if (!bitmap.pixels()) {
        return nullptr;
    }

    return std::unique_ptr<GShader>(new BitmapShader(bitmap, localM, mode));
};
