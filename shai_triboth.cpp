#include "GShader.h"
#include "GMatrix.h"
//#include "GTriShader.h"
#include <vector>


class TriangleBoth : public GShader {
private:
  GShader *shade0;
  GShader *shade1;
public:
  TriangleBoth(GShader* shader0, GShader* shader1): shade0(shader0), shade1(shader1) {}

   bool isOpaque() override {
     return shade0->isOpaque()&&shade1->isOpaque();
   }

   bool setContext(const GMatrix& ctm) override {
       shade0->setContext(ctm);
       shade1->setContext(ctm);
       return true;
   }

   void shadeRow(int x, int y, int count, GPixel row[]) override {
       GPixel row0[count+1];
       GPixel row1[count+1];
       shade0->shadeRow(x,y,count,row0);
       shade1->shadeRow(x,y,count,row1);
       for (int i = 0; i < count; i++) {
    			float new_a = GRoundToInt((GPixel_GetA(row0[i]) * GPixel_GetA(row1[i])) / 255.0f);
    			float new_r = GRoundToInt((GPixel_GetR(row0[i]) * GPixel_GetR(row1[i])) / 255.0f);
    			float new_g = GRoundToInt((GPixel_GetG(row0[i]) * GPixel_GetG(row1[i])) / 255.0f);
    			float new_b = GRoundToInt((GPixel_GetB(row0[i]) * GPixel_GetB(row1[i])) / 255.0f);
			    row[i] = GPixel_PackARGB(new_a, new_r, new_g, new_b);
		   }
   }
};
