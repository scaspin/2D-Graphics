
#include "GShader.h"
#include "GBitmap.h"
#include "GMatrix.h"

class TriangleShader : public GShader {
private:
  GShader* fRealShader;
  GMatrix  fExtraTransform;

  GMatrix multMatrices(const GMatrix& M, const GMatrix& m){
      GMatrix mult;
      float a = M[0]*m[0]+M[1]*m[3];
      float b = M[0]*m[1]+M[1]*m[4];
      float c = M[0]*m[2]+M[1]*m[5]+M[2];
      float d = M[3]*m[0]+M[4]*m[3];
      float e = M[3]*m[1]+M[4]*m[4];
      float f = M[3]*m[2]+M[4]*m[5]+M[5];
      return GMatrix(a,b,c,d,e,f);
  }

public:
  TriangleShader(GShader* shader, const GMatrix& extraTransform): fRealShader(shader), fExtraTransform(extraTransform) {}

   bool isOpaque() override { return fRealShader->isOpaque(); }

   bool setContext(const GMatrix& ctm) override {
       return fRealShader->setContext(multMatrices(ctm,(const GMatrix)fExtraTransform));
   }

   void shadeRow(int x, int y, int count, GPixel row[]) override {
       fRealShader->shadeRow(x, y, count, row);
   }
};
