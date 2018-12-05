
#include "GMatrix.h"

/**
    Helper method to multiply two matrices in the order they are given
*/
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

/**
 *  Set this matrix to identity.
 */
void GMatrix::setIdentity(){
    set6(1,0,0,0,1,0);
}

/**
 *  Set this matrix to translate by the specified amounts.
 */
void GMatrix::setTranslate(float tx, float ty){
    set6(1,0,tx,0,1,ty);
}

/**
 *  Set this matrix to scale by the specified amounts.
 */
void GMatrix::setScale(float sx, float sy){
    set6(sx,0,0,0,sy,0);
}

/**
 *  Set this matrix to rotate by the specified radians.
 *
 *  Note: since positive-Y goes down, a small angle of rotation will increase Y.
 */
void GMatrix::setRotate(float radians){
    set6(cos(radians), -1*sin(radians), 0, sin(radians), cos(radians),0);
}

/**
 *  Set this matrix to the concatenation of the two specified matrices, such that the resulting
 *  matrix, when applied to points will have the same effect as first applying the primo matrix
 *  to the points, and then applying the secundo matrix to the resulting points.
 *
 *  Pts' = Secundo * Primo * Pts
 */
void GMatrix::setConcat(const GMatrix& secundo, const GMatrix& primo){
    GMatrix temp = multMatrices(secundo, primo);
    *this = temp;
}

/*
 *  If this matrix is invertible, return true and (if not null) set the inverse parameter.
 *  If this matrix is not invertible, return false and ignore the inverse parameter.
 */
bool GMatrix::invert(GMatrix* inverse) const{
  float det = fMat[0]*fMat[4]-fMat[1]*fMat[3];

  //no inverse
  if(det==0){
    return false;
  }

  det = 1.0/det;

  inverse->set6(det*fMat[4], -det*fMat[1],det*(fMat[1]*fMat[5]-fMat[4]*fMat[2]), -det*fMat[3], det*fMat[0], det*(fMat[3]*fMat[2]-fMat[0]*fMat[5]));

  return true;
}

/**
 *  Transform the set of points in src, storing the resulting points in dst, by applying this
 *  matrix. It is the caller's responsibility to allocate dst to be at least as large as src.
 *
 *  Note: It is legal for src and dst to point to the same memory (however, they may not
 *  partially overlap). Thus the following is supported.
 *
 *  GPoint pts[] = { ... };
 *  matrix.mapPoints(pts, pts, count);
 */
void GMatrix::mapPoints(GPoint dst[], const GPoint src[], int count) const{

      for (int i=0; i<count; i++){
        GPoint temp_src = src[i];
        float new_x = fMat[0]*temp_src.x()+fMat[1]*temp_src.y()+fMat[2];
        float new_y = fMat[3]*temp_src.x()+fMat[4]*temp_src.y()+fMat[5];

        dst[i]=GPoint::Make(new_x,new_y);

      }

}
