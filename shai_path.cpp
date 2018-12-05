#include "GPath.h"
#include "GMatrix.h"

GPath& GPath::addRect(const GRect& r, Direction dir){

  moveTo(r.left(), r.top());
  if (dir==kCW_Direction){
    lineTo(r.right(), r.top());
    lineTo(r.right(), r.bottom());
    lineTo(r.left(), r.bottom());
  }else{
    lineTo(r.left(), r.bottom());
    lineTo(r.right(), r.bottom());
    lineTo(r.right(), r.top());
  }

  return *this;
};
GPath& GPath::addPolygon(const GPoint pts[], int count){
  if (count<3){
    return *this;
  }

  moveTo(pts[0]);
  for (int i=1; i<count; i++){
    lineTo(pts[i]);
  }

  return *this;
};
GRect GPath::bounds() const{

  if (fPts.size()==0){
    return GRect::MakeWH(0,0);
  }

  float minX = fPts[0].x(), maxX = fPts[0].x();
  float minY = fPts[0].y(), maxY = fPts[0].y();

  for (int i=1; i<fPts.size(); i++){
    minX = std::min(minX, fPts[i].x());
    maxX = std::max(maxX, fPts[i].x());
    minY = std::min(minY, fPts[i].y());
    maxY = std::max(maxY, fPts[i].y());
  }

  //printf("%f, %f, %f, %f\n", minX, minY, maxX, maxY);
  return GRect::MakeLTRB(minX, minY, maxX, maxY);
};
void GPath::transform(const GMatrix& m){
  // GPoint points[fPts.size()];
  // std::copy(fPts.begin(), fPts.end(), points);
  //
  // m.mapPoints(points, points, fPts.size());
  m.mapPoints(fPts.data(), fPts.data(), fPts.size());
  return;
};
GPath& GPath::addCircle(GPoint center, float radius, Direction dir){
  float tanStuff = tan(3.1415926/ 8.0);
  float rootToot = sqrt(2)/2;

  GPoint points[16];
  points[0] = GPoint::Make(1,0);
  points[1] = GPoint::Make(1, tanStuff);
  points[2] = GPoint::Make(rootToot, tanStuff);
  points[3] = GPoint::Make(tanStuff, 1);

  //translate for rest of coordinates
  for(int i = 4; i<16; i++){
    points[i] = GPoint::Make(-points[i-4].y(), points[i-4].x());
  }

  GMatrix trans;
  trans.setScale(radius, radius);
  trans.postTranslate(center.x(), center.y());

  trans.mapPoints(points, points, 16);

  moveTo(points[0]);
  if (dir==kCCW_Direction){
    for (int i=14; i>=0; i-=2){
      quadTo(points[i], points[i+1]);
    }
  }else{
    for (int i=2; i<16; i+=2){
      quadTo(points[i], points[i-1]);
    }
    quadTo(points[0], points[15]);
  }

  return *this;
}

void GPath::ChopQuadAt(const GPoint src[3], GPoint dst[5], float t){
  dst[0] = src[0];
  dst[4] = src[2];

  dst[1] = (1 - t) * src[0] + t * src[1];
  dst[3] = (1 - t) * src[1] + t * src[2];
  dst[2] = (1 - t) * dst[1] + t * dst[3];
}
void GPath::ChopCubicAt(const GPoint src[4], GPoint dst[7], float t){
  dst[0] = src[0];
  dst[1] = (1 - t) * src[0] + t * src[1];
  dst[5] = (1 - t) * src[2] + t * src[3];
  dst[6] = src[3];

  GPoint inter = (1 - t) * src[1] + t * src[2];
  dst[2] = (1 - t) * dst[1] + t * inter;
  dst[4] = (1 - t) * inter + t * dst[5];
  dst[3] = (1 - t) * dst[2] + t * dst[4];
};
