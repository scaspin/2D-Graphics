#include "GShader.h"
#include "GMatrix.h"
#include <vector>


class LinearGradient : public GShader {
public:
   LinearGradient(GPoint p0, GPoint p1, const GColor colors[], int count, TileMode mode){
      if (p0.x()<p1.x()){
        fP0 = p0;
        fP1 = p1;

      }else if (p0.x()==p1.x() && p0.y()<p1.y()){
        fP0 = p0;
        fP1 = p1;
      }else{
        fP0 = p1;
        fP1 = p0;
      }

      fColorCount = count;
      for (int i=0; i<fColorCount; i++){
        colorVector.push_back(colors[i]);
      }

      tileMode = mode;

   }

   bool isOpaque() override{
     return false;
   }

   // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
   bool setContext(const GMatrix& ctm) override{

     float dx = fP1.x() - fP0.x();
     float dy = fP1.y() - fP0.y();
     fLocalMatrix.set6(dx, -dy, fP0.x() ,dy, dx, fP0.y());

     fLocalMatrix.invert(&gInverse);
     fInverse.setIdentity();

     if (!ctm.invert(&fInverse)) {
       return false;
     }

     ctm.invert(&fInverse);
     return true;


   }

  /**
   *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
   *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
   *  can hold at least [count] entries.
   */
   void shadeRow(int x, int y, int count, GPixel row[]) override{
     GPoint local = fInverse.mapXY(x + 0.5, y + 0.5);
     for (int i = 0; i < count; ++i)
      {
        	GColor c;
        	if (fColorCount == 1)
        	  {
        	    c = colorVector.front();
        	  }
        	else
        	{
              //get position in 1:1 shader at origin
        	    float t = gInverse.mapXY(local.fX, local.fY).fX;

              if (tileMode==kClamp){
                //clamp clamp clamp
                t = std::max(0.0f, std::min(1.0f, t));
              }else if (tileMode==kRepeat){
                t= t-GFloorToInt(t);
              }else if(tileMode==kMirror){
                t *=0.5;
                t -=floor(t);
                if(t>0.5){
                  t = 1-t;
                }
                t*=2;
              }

              int num_intervals = colorVector.size()-1;
              int index = t*num_intervals;
              float u = (t - (1.0/num_intervals)*index)*num_intervals;

              GColor c1, c2;
              // if (index>=2){
              // printf("index=%d numcolors=%d\n", index, fColorCount);
              // }

          	  c1 = colorVector[index+1];
              c2 = colorVector[index];

        	    c.fA = c1.fA*u + c2.fA*(1-u);
        	    c.fR = c1.fR*u + c2.fR*(1-u);
        	    c.fG = c1.fG*u + c2.fG*(1-u);
        	    c.fB = c1.fB*u + c2.fB*(1-u);
        	}

        	row[i] = color2pixel(c);
          local.fX += fInverse[0];
          local.fY += fInverse[3];

      }


   }

private:
  GPoint fP0;
  GPoint fP1;

  std::vector<GColor> colorVector;
  int fColorCount;

  GMatrix fLocalMatrix;
  GMatrix fInverse;
  GMatrix gInverse;

  TileMode tileMode;

  int float2int255(float f){
    //GASSERT(0 <= f && f <= 1);
    return GFloorToInt(f * 255 + 0.5);
  }

  GPixel color2pixel(const GColor& color){
    return GPixel_PackARGB(float2int255(color.fA), float2int255(color.fR*color.fA),float2int255(color.fG*color.fA), float2int255(color.fB*color.fA));
  }

};


std::unique_ptr<GShader> GCreateLinearGradient(GPoint p0, GPoint p1, const GColor colors[], int count, GShader::TileMode mode)
{
  return std::unique_ptr<GShader>(new LinearGradient(p0, p1, colors, count, mode));
}
