#ifndef QP_VISUAL_READBACK_MATH_H
#define QP_VISUAL_READBACK_MATH_H
#include "soft_core.h"
/* Unity Matrix4x4 memory is column-major. Do not reconstruct a sheared visual
 * transform from rotation/lossyScale: that loses the directional compression. */
typedef struct {float m[16];} QPMatrix;
static inline int qp_matrix_valid(const QPMatrix *m){
    for(unsigned i=0;i<16;i++)if(!qp_valid(m->m[i]))return 0;
    return qp_abs(m->m[3])<1e-6f&&qp_abs(m->m[7])<1e-6f&&
        qp_abs(m->m[11])<1e-6f&&qp_abs(m->m[15]-1)<1e-6f;
}
static inline QPVector qp_matrix_point(const QPMatrix *m,QPVector p){
    return (QPVector){m->m[0]*p.x+m->m[4]*p.y+m->m[8]*p.z+m->m[12],
        m->m[1]*p.x+m->m[5]*p.y+m->m[9]*p.z+m->m[13],
        m->m[2]*p.x+m->m[6]*p.y+m->m[10]*p.z+m->m[14]};
}
static inline int qp_matrix_inverse(const QPMatrix *m,QPMatrix *out){
    if(!qp_matrix_valid(m))return 0;
    float a=m->m[0],b=m->m[4],c=m->m[8],d=m->m[1],e=m->m[5],f=m->m[9],g=m->m[2],h=m->m[6],i=m->m[10];
    float determinant=a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g);
    if(!qp_valid(determinant)||qp_abs(determinant)<1e-6f)return 0;
    float r=1/determinant;
    QPMatrix inverse={{(e*i-f*h)*r,(f*g-d*i)*r,(d*h-e*g)*r,0,
        (c*h-b*i)*r,(a*i-c*g)*r,(b*g-a*h)*r,0,
        (b*f-c*e)*r,(c*d-a*f)*r,(a*e-b*d)*r,0,0,0,0,1}};
    QPVector offset=qp_matrix_point(&inverse,(QPVector){-m->m[12],-m->m[13],-m->m[14]});
    inverse.m[12]=offset.x;inverse.m[13]=offset.y;inverse.m[14]=offset.z;
    if(!qp_matrix_valid(&inverse))return 0;
    *out=inverse;return 1;
}
static inline QPVector qp_matrix_normal(const QPMatrix *inverse,QPVector n){
    QPVector v={inverse->m[0]*n.x+inverse->m[1]*n.y+inverse->m[2]*n.z,
        inverse->m[4]*n.x+inverse->m[5]*n.y+inverse->m[6]*n.z,
        inverse->m[8]*n.x+inverse->m[9]*n.y+inverse->m[10]*n.z};
    float length=qp_sqrt(qp_dot(v,v));return length>1e-5f?qp_mul(v,1/length):v;
}
static inline int qp_bounds_contains(QPVector center,QPVector extents,QPVector p,float tolerance){
    return qp_valid(center.x)&&qp_valid(center.y)&&qp_valid(center.z)&&
        qp_valid(extents.x)&&qp_valid(extents.y)&&qp_valid(extents.z)&&
        extents.x>0&&extents.y>0&&extents.z>0&&
        qp_valid(p.x)&&qp_valid(p.y)&&qp_valid(p.z)&&
        qp_abs(p.x-center.x)<=extents.x+tolerance&&
        qp_abs(p.y-center.y)<=extents.y+tolerance&&
        qp_abs(p.z-center.z)<=extents.z+tolerance;
}
#endif
