#ifndef GEdge_DEFINED
#define GEdge_DEFINED

#include "GPaint.h"
#include "GPixel.h"

struct Edge{
  int top_y;
  int bot_y;
  float cur_x;
  float m;
  int wind;
  bool active;
};

Edge makeEdge(GPoint p0, GPoint p1){
  Edge e;
  GPoint top;
  //GASSERT(p0.y()!=p1.y() || p0!=p1);
  if (p0.y()<p1.y()){
    e.top_y = GRoundToInt(p0.y());
    top = p0;
    e.bot_y = GRoundToInt(p1.y());
    e.cur_x = p0.x();
    e.wind = 1;
  }else{
    e.top_y = GRoundToInt(p1.y());
    top = p1;
    e.bot_y = GRoundToInt(p0.y());
    e.cur_x = p1.x();
    e.wind = -1;
  }

  e.active =true;
  e.m = (p1.x()-p0.x())/(p1.y()-p0.y());
  e.cur_x = e.cur_x + (e.m)*(GRoundToInt(top.y())-top.y()+0.5f);
  return e;
}

//returns true if edge e0 is "greater" than e1
bool edgeCompare(Edge e0, Edge e1){
  //sort by y
  if (e0.top_y<e1.top_y){
    return true;
  }else if(e0.top_y==e1.top_y){
    //sort by x if y's are the same
    if (e0.cur_x<e1.cur_x){
      return true;
    }
    //sort by m if y,x are same
    else if(e0.cur_x==e1.cur_x){
      if (e0.m<e1.m){
        return true;
      }
    }
  }
  return false;
}

/*
  Rewrote this so doesn't need it anymore but just in case
*/
bool edgeXCompare(Edge e0, Edge e1){
  if(e0.active){
    if (e0.top_y<e1.top_y){
      return true;
    }
      if (e0.cur_x<e1.cur_x){
        return true;
      }
    }
  return false;
}



#endif
