#include <iostream>
#include <algorithm>
#include "GCanvas.h"
#include "GBitmap.h"
#include "GPixel.h"
#include "GMath.h"
#include "GColor.h"
#include "GPaint.h"
#include "GRect.h"
#include "GBlendModes.h"
#include "GPoint.h"
#include "GPaint.h"
#include "GEdge.h"
#include "GPath.h"
#include "GShader.h"
//#include "GTriShader.h"
#include "shai_trigradient.cpp"
#include "shai_trishader.cpp"
#include "shai_triboth.cpp"

#include <stack>

class EmptyCanvas : public GCanvas {
private:
    std::stack<GMatrix> de_stack;

    GPixel getPixel(int x, int y){
      GASSERT(x>=0 && x<=fDevice.width());
      GASSERT(y>=0 && y<= fDevice.height());
      GPixel *temp_ptr = fDevice.getAddr(x, y);
      return *temp_ptr;
    }

    GPixel blend(const GPixel& source, const GPixel& dest, const GBlendMode mode){
      switch(mode){
        case (GBlendMode::kClear): return 0;
        case (GBlendMode::kSrc): return source;      //!<     S
        case (GBlendMode::kDst): return dest;      //!<     D
        case (GBlendMode::kSrcOver): return SRC_OVER(source, dest);
        case (GBlendMode::kDstOver): return DST_OVER(source, dest);  //!<     D + (1 - Da)*S
        case (GBlendMode::kSrcIn): return K_SRC_IN(source, dest);    //!<     Da * S
        case (GBlendMode::kDstIn): return K_DST_IN(source, dest);    //!<     Sa * D
        case (GBlendMode::kSrcOut): return K_SRC_OUT(source, dest);   //!<     (1 - Da)*S
        case (GBlendMode::kDstOut): return K_DST_OUT(source, dest);   //!<     (1 - Sa)*D
        case (GBlendMode::kSrcATop): return K_SRC_ATOP(source, dest);  //!<     Da*S + (1 - Sa)*D
        case (GBlendMode::kDstATop): return K_DST_ATOP(source, dest);  //!<     Sa*D + (1 - Da)*S
        case (GBlendMode::kXor):  return K_XOR(source, dest);    //!<     (1 - Sa)*D + (1 - Da)*S)
      }
      //printf("HERE\n");
      return 0;
    }

    void blit(int x_left, int x_right, int y, const GPaint& paint){
      if(x_right > fDevice.width()){
        x_right = fDevice.width();
      }

      //if a shader is present it overwrites
      if (paint.getShader()==nullptr){

        GPixel source = color2pixel(paint.getColor().pinToUnit());

        for (int i=x_left; i<x_right; i++){
            GPixel dest = getPixel(i, y);
            GPixel result = blend(source, dest, paint.getBlendMode());
            GPixel *temp_addr = fDevice.getAddr(i, y);
            *temp_addr = result;
          }
      }else{
        //get new pixels
        if (!paint.getShader()->setContext(de_stack.top())) {
              return;
        }
        if (x_right<x_left){
          return;
        }
        GPixel new_pixels[x_right-x_left];
        paint.getShader()->shadeRow(x_left, y, x_right-x_left,new_pixels);

        //actually paint given pixels
        for (int i=x_left; i<x_right; i++){
          GPixel *temp_addr = fDevice.getAddr(i, y);
          if(paint.getShader()->isOpaque()){
            *temp_addr = new_pixels[i-x_left];
          }else{
            GPixel dest = getPixel(i, y);
            GPixel result = blend(new_pixels[i-x_left], dest, paint.getBlendMode());
            *temp_addr = result;

          }
        }

      }
    }

    int clip(const GPoint& p0, const GPoint& p1, Edge edges[], int num_edges){
      //determining which point has higher y value
      bool flipped = false;

      GPoint top;
      GPoint bot;
      if (p0.y()<p1.y()){
        top = p0;
        bot = p1;
      }else if (p0.y()==p1.y()){
        if (p0.x()<p1.x()){
          top = p0;
          bot = p1;
        }else{
          top = p1;
          bot = p0;
          flipped=true;
        }
      }else{
        top = p1;
        bot = p0;
        flipped = true;
      }

      //Conditions when no edge is made
      //edge out of bounds of device vertically completely
      if ((top.y()<0 && bot.y()<0) || (top.y()>fDevice.height() && bot.y()>fDevice.height())){
        return num_edges;
      }
      //line is horizontal
      if (p0.y()==p1.y()){
        return num_edges;
      }

      //At least one edge
      Edge e0 = makeEdge(top, bot);
      int tmp_index = num_edges;
      if (flipped){
        e0.wind = -1;
      }
      edges[num_edges]=e0;
      num_edges++;


      //clip top
      if(e0.top_y<=0){
        float clipped_x = top.x()+(bot.x()-top.x())*(-top.y())/(bot.y()-top.y());
        top.set(clipped_x, 0);
        e0.top_y = 0;
        e0.cur_x = clipped_x;
      }

      //clip bottom
      if(bot.y()>fDevice.height()){
        float clipped_x = bot.x()-(bot.x()-top.x())*(bot.y()-fDevice.height())/(bot.y()-top.y());
        //no need as edge doesn't remeber bottom x values
        bot.set(clipped_x, fDevice.height());
        e0.bot_y = fDevice.height();
      }

      // horizontally entirely outside of device
      //left
      if (top.x()<0 && bot.x()<0){
        e0 = makeEdge(GPoint::Make(0,top.y()), GPoint::Make(0, bot.y()));
        if (flipped==true){
          e0.wind = -1;
        }else{
          e0.wind = 1;
        }
        edges[tmp_index]=e0;
        return num_edges;
      }else if (top.x()>fDevice.width() && bot.x()>fDevice.width()){ //to the right
        e0 = makeEdge(GPoint::Make(fDevice.width(), top.y()), GPoint::Make(fDevice.width(), bot.y()));
        if (flipped==true){
          e0.wind = -1;
        }else{
          e0.wind = 1;
        }
        edges[tmp_index]=e0;
        return num_edges;
      }

      //top point horizontally outside on the left
      if (e0.cur_x<0){
        //printf("%f\n", e0.cur_x);
        float clipped_y = e0.top_y+(-top.x())*(bot.y()-top.y())/(bot.x()-top.x());
        edges[num_edges] = makeEdge(GPoint::Make(0,e0.top_y), GPoint::Make(0, clipped_y));
        edges[num_edges].wind = e0.wind;
        e0.top_y = clipped_y;
        e0.cur_x = 0;
        num_edges++;
      }else if (e0.cur_x>fDevice.width()){ //outside on the right
        float clipped_y = e0.bot_y-(bot.x()-fDevice.width())*(bot.y()-top.y())/(bot.x()-top.x());
        edges[num_edges] = makeEdge(GPoint::Make(fDevice.width(),e0.top_y), GPoint::Make(fDevice.width(), clipped_y));
        edges[num_edges].wind = e0.wind;
        e0.top_y = clipped_y;
        e0.cur_x = fDevice.width();
        num_edges++;
      }

      //bottom point to the fLeft
      if (bot.x()<0){
        float clipped_y = bot.y()-(bot.x())*(top.y()-bot.y())/(top.x()-bot.x());
        edges[num_edges] = makeEdge(GPoint::Make(0,clipped_y), GPoint::Make(0, e0.bot_y));
        edges[num_edges].wind = e0.wind;
        e0.bot_y = clipped_y;
        num_edges++;
      }else if (bot.x()>fDevice.width()){ //outside on the right
        float clipped_y = bot.y()-(bot.x()-fDevice.width())*(bot.y()-top.y())/(bot.x()-top.x());
        edges[num_edges] = makeEdge(GPoint::Make(fDevice.width(),clipped_y), GPoint::Make(fDevice.width(), e0.bot_y));
        edges[num_edges].wind = e0.wind;
        e0.bot_y = clipped_y;
        num_edges++;
      }

      edges[tmp_index]=e0;
      return num_edges;
    }

    GPoint evaluateQuadratic(GPoint points[], float t){
        GPoint res;
        GPoint A = points[0] + (-2) * points[1] + points[2];
        GPoint B = 2 * (points[1] + (-1) * points[0]);
        GPoint C = points[0];
        res = (A * t + B) * t + C;
        return res;
    }
    GPoint evaluateCubic(GPoint points[], float t){
        GPoint res;
        GPoint A = -1 * points[0] + 3 * points[1] + (-3) * points[2] + points[3];
        GPoint B = 3 * points[0] + (-6) * points[1] + 3 * points[2];
        GPoint C = 3 * (points[1] + (-1) * points[0]);
        GPoint D = points[0];
        res = ( (A * t + B) * t + C ) * t + D;
        return res;
    }

    //number of segments needed to evaluate curve under tolerance
    int numSegQuad(GPoint points[]){
        float tolerance = 0.25;

        GPoint vect = 0.25 * (points[0] + (-2) * points[1] + points[2]);
        float vectMag = sqrt(vect.x()*vect.x() + vect.y()*vect.y());
        return GCeilToInt(sqrt(vectMag / tolerance));
    }

    int numSegCub(GPoint points[]){
        float tolerance = 0.25;

        GPoint vect1 = points[0] + (-2) * points[1] + points[2];
        GPoint vect2 = points[1] + (-2) * points[2] + points[3];

        float vectMag1 = sqrt(vect1.x()*vect1.x() + vect1.y()*vect1.y());
        float vectMag2 = sqrt(vect2.x()*vect2.x() + vect2.y()*vect2.y());

        return GCeilToInt(sqrt(0.75 * std::max(vectMag1, vectMag2) / tolerance));
    }

    GMatrix multMatrices(GMatrix& M, GMatrix& m){
        GMatrix mult;
        float a = M[0]*m[0]+M[1]*m[3];
        float b = M[0]*m[1]+M[1]*m[4];
        float c = M[0]*m[2]+M[1]*m[5]+M[2];
        float d = M[3]*m[0]+M[4]*m[3];
        float e = M[3]*m[1]+M[4]*m[4];
        float f = M[3]*m[2]+M[4]*m[5]+M[5];
        return GMatrix(a,b,c,d,e,f);
    }

    GPoint makeNewPoint(GPoint A,GPoint B,GPoint C,GPoint D,float u,float v){
      float newX = (1.0f-u)*(1-v)*A.x() + (1-v)*u*B.x() + (1-u)*v*D.x() + u*v*C.x();
      float newY = (1.0f-u)*(1-v)*A.y() + (1-v)*u*B.y() + (1-u)*v*D.y() + u*v*C.y();

      return GPoint::Make(newX, newY);
    }

    GColor makeNewColor(GColor A, GColor B, GColor C, GColor D, float u, float v){
      float newA = (1-u)*(1-v)*A.fA + (1-v)*u*B.fA + (1-u)*v*D.fA + u*v*C.fA;
      float newR = (1-u)*(1-v)*A.fR + (1-v)*u*B.fR + (1-u)*v*D.fR + u*v*C.fR;
      float newG = (1-u)*(1-v)*A.fG + (1-v)*u*B.fG + (1-u)*v*D.fG + u*v*C.fG;
      float newB = (1-u)*(1-v)*A.fB + (1-v)*u*B.fB + (1-u)*v*D.fB + u*v*C.fB;

      return GColor::MakeARGB(newA, newR, newG, newB);
    }

public:
    const GBitmap fDevice;
    EmptyCanvas(const GBitmap& device) : fDevice(device) {
      GMatrix I;
      I.setIdentity();
      de_stack.push(I);
    }

    /**
     *  Fill the entire canvas with the specified color, using SRC porter-duff mode.
     */
    void drawPaint(const GPaint& paint) override {
      for (int i=0; i<fDevice.height(); i++){
        blit(0, fDevice.width(), i, paint);
      }
    }
    /**
     *  Fill the rectangle with the color, using SRC_OVER porter-duff mode.
     *
     *  The affected pixels are those whose centers are "contained" inside the rectangle:
     *      e.g. contained == center > min_edge && center <= max_edge
     *
     *  Any area in the rectangle that is outside of the bounds of the canvas is ignored.
     */
    void drawRect(const GRect& rect, const GPaint& paint) override {
        // GIRect rect_rounded = rect.round();

        // if(rect_rounded.left()<0){rect_rounded.fLeft=0;}
        // if(rect_rounded.right()>fDevice.width()){rect_rounded.fRight=fDevice.width();}
        // if(rect_rounded.top()<0){rect_rounded.fTop=0;}
        // if(rect_rounded.bottom()>fDevice.height()){rect_rounded.fBottom=fDevice.height();}

        //blend each pixel in rectangle with past pixel in bitmap
        // for (int i=rect_rounded.top(); i<rect_rounded.bottom(); i++){
        //   blit(rect_rounded.left(), rect_rounded.right(), i, paint);
        // }

        GPoint points[4] = {
            GPoint::Make(rect.left(), rect.top()),
            GPoint::Make(rect.right(), rect.top()),
            GPoint::Make(rect.right(), rect.bottom()),
            GPoint::Make(rect.left(), rect.bottom())
        };

        drawConvexPolygon(points, 4, paint);
    }

    void drawConvexPolygon(const GPoint points[], int num, const GPaint& paint) override{

      if(num<=2){// || sizeof(*points)<2){
        //printf("less than two point fed into convexPoly");
        return;
      }

      GPoint new_points[num];
      de_stack.top().mapPoints(new_points, points, num);

      Edge clipped_edges[num*3];
      int count=0;
      int num_edges;

      //turn all points into edges
      for (int i=0; i<num-1; i++){
        num_edges=clip(new_points[i], new_points[i+1], clipped_edges, count);
        count=num_edges;
      }
      //final edge
      num_edges=clip(new_points[num-1], new_points[0], clipped_edges, count);
      count=num_edges;

      //sort already clipped edges
      std::sort(&clipped_edges[0], &clipped_edges[count], edgeCompare);
      Edge left = clipped_edges[0];
      Edge right = clipped_edges[1];
      int current_y = clipped_edges[0].top_y;
      int current_edge_index = 2;

      //printf("left:%d\tright:%d\ty:%d\n", GRoundToInt(left.cur_x), GRoundToInt(right.cur_x), current_y);

      while(current_y<clipped_edges[count-1].bot_y && current_y<=fDevice.height() && current_y>=0){
        if(current_edge_index>count){
          return;
        }
        //check to see if finished edge, if so replace
        else if (left.bot_y<=current_y){
          left=clipped_edges[current_edge_index];
          current_edge_index++;
        }else if(right.bot_y<=current_y){
          right=clipped_edges[current_edge_index];
          current_edge_index++;
        }

        if (paint.getShader()){
          paint.getShader()->setContext(de_stack.top());
        }

        //actually draw
        blit(GRoundToInt(left.cur_x), GRoundToInt(right.cur_x), current_y, paint);
        left.cur_x=left.cur_x+left.m;
        right.cur_x=right.cur_x+right.m;
        current_y++;
      }
    }

    void drawPath(const GPath& path, const GPaint& paint) override{
      if (path.countPoints()<=2){
        return;  //don't draw
      }

      GPath transformed_path = path;
      transformed_path.transform(de_stack.top());

      GPath::Edger edges_edger(transformed_path);

      int num_points = transformed_path.countPoints();
      Edge clipped_edges[num_points*500];

      int count = 0;
      int num_edges = 0;

      //get all edges needed
      for (int i=0; i<num_points; i++){
        GPoint edge_points[4];
        GPath::Verb status = edges_edger.next(edge_points);

        //just in case
        if (status==GPath::kDone){
          break;
        }else if(status==GPath::kLine){
          num_edges=clip(edge_points[0], edge_points[1], clipped_edges, count);
          count=num_edges;
        }else if(status==GPath::kQuad){
          int numSegments = numSegQuad(edge_points);
          float delta = 1.0 / numSegments;
          GPoint p1, p2;
          for (float d = 0.0; d < 1.0; d += delta){
              p1 = p2;
              p2 = evaluateQuadratic(edge_points, d);
              if (d != 0.0){
                num_edges=clip(p1, p2, clipped_edges, count);
                count = num_edges;
              }
          }
        }else if(status==GPath::kCubic){
          int numSegments = numSegCub(edge_points);
          float delta = 1.0 / numSegments;
          GPoint p1, p2;
          for (float d = 0.0; d < 1.0; d += delta){
              p1 = p2;
              p2 = evaluateCubic(edge_points, d);
              if (d != 0.0){
                num_edges=clip(p1, p2, clipped_edges, count);
                count = num_edges;
              }
          }
        }
      }
      //printf("num_edges: %d\n", num_edges);

      if(count==0){
        return;
      }
      //printf("Number of edges: %d", count);
      //SCAN CNVERT NOW W WINDVILL
      std::sort(&clipped_edges[0], &clipped_edges[count], edgeCompare);

      //winding value starts out at 0;
      int windfill = 0;
      std::vector<float> starts;
      std::vector<float> ends;

      for(int i=clipped_edges[0].top_y; i<=clipped_edges[count-1].bot_y; i++){
          //printf("here\n");
          windfill = 0;

          starts.clear();
          ends.clear();

         for (int j=0; j<count; j++){
            //what makes an edge active (between bounds and not discarded yet)
            //"active reduces time taken for later draws"

            Edge e = clipped_edges[j];

            if (e.active && e.top_y<=i && e.bot_y>i){
              if (windfill==0){
                if (starts.size()>0 && std::find(starts.begin(), starts.end(), e.cur_x)!=starts.end()){
                  //printf("here\n");
                }else{
                  starts.push_back(e.cur_x);
                  //windfill=windfill+e.wind;
                }
                //printf("start: %f\t", e.cur_x);
              }

              windfill=windfill+e.wind;

              //printf("winding: %d\n", e.wind);

              if (windfill==0){
                if (ends.size()>0 && std::find(ends.begin(), ends.end(), e.cur_x)!=ends.end()){
                    // printf("problem 2\n");
                }else{
                  ends.push_back(e.cur_x);
                  //blit(starts.pop_back(), GRoundToInt(ends.pop_back()), i, paint);
                }
                //printf("end: %f\n", e.cur_x);
              }
              //post process edge
              if (i>e.bot_y+1){
                e.active = false;
              }else{
                clipped_edges[j].cur_x=e.cur_x+e.m;
                //printf("slope: %f\n", e.m);
                //e.cur_x=e.cur_x++;
              }
            }
            //std::sort(&clipped_edges[0], &clipped_edges[count], edgeCompare);
          }

          //std::sort(starts[0], starts[starts.size()-1],floatLesser);
          //std::sort(ends[0], ends[ends.size()-1],floatLesser);

          sort(starts.begin(), starts.end());
          sort(ends.begin(), ends.end());

          for (int j=0; j<std::min(ends.size(), starts.size()); j++){
            int start = GRoundToInt(starts[j]);
            int end = GRoundToInt(ends[j]);
            if (start>end){
              int temp = start;
              start = end;
              end=temp;
            }
            //printf("start: %d\t end: %d\n", start, end);
          blit(start, end, i, paint);
          }
      }

    }

    void save() override{
        GMatrix saved = de_stack.top();
        GMatrix copy(
            saved[0], saved[1], saved[2],
            saved[3], saved[4], saved[5]);

        de_stack.push(copy);
    }
    void restore() override{
        de_stack.pop();
    }
    void concat(const GMatrix& transformation) override{
        de_stack.top().preConcat(transformation);
    }

    void drawMesh(const GPoint verts[], const GColor colors[], const GPoint texs[], int count, const int indices[], const GPaint& paint) override{
      //The triangles are specified by successive triples of indices.
      int n = 0;
      GPoint p0, p1, p2;
      GColor color0, color1, color2;
      GPoint texs0, texs1, texs2;

      // GPaint grad;
      // grad.setColor({ 1, 1, 0, 0 });
      GPaint newpaint = paint;

      for (int i = 0; i < count; ++i) {
        p0 = verts[indices[n]];
        p1 = verts[indices[n+1]];
        p2 = verts[indices[n+2]];
        GPoint points[] = {p0, p1, p2};

        if (colors!=NULL && texs!=NULL){
          color0 = colors[indices[n]];
          color1 = colors[indices[n+1]];
          color2 = colors[indices[n+2]];
          texs0 = texs[indices[n]];
          texs1 = texs[indices[n+1]];
          texs2 = texs[indices[n+2]];

          GMatrix T, P, invT, fin;
          T.set6(texs1.x()-texs0.x(),texs2.x()-texs0.x(),texs0.x(),texs1.y()-texs0.y(),texs2.y()-texs0.y(),texs0.y());
          P.set6(p1.x()-p0.x(),p2.x()-p0.x(),p0.x(),p1.y()-p0.y(),p2.y()-p0.y(),p0.y());
          T.invert(&invT);
          fin = multMatrices(P, invT);
          TriangleShader *s0 =  new TriangleShader(paint.getShader(), fin);
          TriangleGradient *s1 =  new TriangleGradient(p0, p1, p2, color0, color1, color2);

          TriangleBoth *sBOTH = new TriangleBoth(s0, s1);
          newpaint = GPaint(sBOTH);
          drawConvexPolygon(points, 3, newpaint);

        }else if (colors!=NULL){
          color0 = colors[indices[n]];
          color1 = colors[indices[n+1]];
          color2 = colors[indices[n+2]];

          TriangleGradient *sh =  new TriangleGradient(p0, p1, p2, color0, color1, color2);
          newpaint = GPaint(sh);
          drawConvexPolygon(points, 3,newpaint);

        }else if (texs!=NULL){
          texs0 = texs[indices[n+0]];
          texs1 = texs[indices[n+1]];
          texs2 = texs[indices[n+2]];

          /*
                          | T1x-T0x  T2x-T0x  T0x |
          | Uy Vy T0y |   | T1y-T0y  T2x-T0y  T0y |
          |  0  0  1  |   |    0        0      1  |
          */
          GMatrix T, P, invT, fin;
          T.set6(texs1.x()-texs0.x(),texs2.x()-texs0.x(),texs0.x(),texs1.y()-texs0.y(),texs2.y()-texs0.y(),texs0.y());

          /*
          P = | Ux Vx P0x | = | P1x-P0x  P2x-P0x  P0x |
              | Uy Vy P0y |   | P1y-P0y  P2y-P0y  P0y |
              |  0  0  1  |   |    0        0      1  |
          */
          P.set6(p1.x()-p0.x(),p2.x()-p0.x(),p0.x(),p1.y()-p0.y(),p2.y()-p0.y(),p0.y());

          T.invert(&invT);
          fin = multMatrices(P, invT);

          TriangleShader *sh =  new TriangleShader(paint.getShader(), fin);
          newpaint = GPaint(sh);
          drawConvexPolygon(points, 3, newpaint);
        }
        n=n+3;
      }
    }

    void drawQuad(const GPoint verts[4], const GColor colors[4], const GPoint texs[4],int level, const GPaint& paint) override{
      GPoint A = verts[0];
      GPoint B = verts[1];
      GPoint C = verts[2];
      GPoint D = verts[3];

      GPoint new_verts[4];
      int indecies[6] = {0,1,3,1,2,3};

      for (int u=0; u<=level; u++){
        for (int v=0; v<=level; v++){
          float u1 = (float)u/(float)(level+1);
          float u2 = (float)(u+1)/(float)(level+1);

          float v1 = (float)v/(float)(level+1);
          float v2 = (float)(v+1)/(float)(level+1);

          new_verts[0]=makeNewPoint(A,B,C,D, u1, v1);
          new_verts[1]=makeNewPoint(A,B,C,D, u2, v1);
          new_verts[2]=makeNewPoint(A,B,C,D, u2, v2);
          new_verts[3]=makeNewPoint(A,B,C,D, u1, v2);

          GColor new_colors[4];
          GColor *cp = nullptr;
          GPoint new_texs[4];
          GPoint *pc = nullptr;

          if(colors){
            new_colors[0] = makeNewColor(colors[0], colors[1],colors[2],colors[3], u1, v1);
            new_colors[1] = makeNewColor(colors[0], colors[1],colors[2],colors[3], u2, v1);
            new_colors[2] = makeNewColor(colors[0], colors[1],colors[2],colors[3], u2, v2);
            new_colors[3] = makeNewColor(colors[0], colors[1],colors[2],colors[3], u1, v2);

            cp = new_colors;
          }

          if(texs){
            new_texs[0] = makeNewPoint(texs[0], texs[1], texs[2], texs[3], u1, v1);
            new_texs[1] = makeNewPoint(texs[0], texs[1], texs[2], texs[3], u2, v1);
            new_texs[2] = makeNewPoint(texs[0], texs[1], texs[2], texs[3], u2, v2);
            new_texs[3] = makeNewPoint(texs[0], texs[1], texs[2], texs[3], u1, v2);

            pc = new_texs;
          }

          drawMesh(new_verts, cp, pc, 2, indecies, paint);
        }

      }

    }

};

/*
    returns unique instance of canvas
 */
std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& device) {
    if (!device.pixels()) {
        return nullptr;
    }
    return std::unique_ptr<GCanvas>(new EmptyCanvas(device));
}
