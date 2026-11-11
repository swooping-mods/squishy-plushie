#ifndef SQUISHY_PLUSHIE_CARRY_CORE_H
#define SQUISHY_PLUSHIE_CARRY_CORE_H
#include "collision.h"
/* A capsule axis has no sign. Blend the nearest equivalent axis so a toy lying
 * on its side can lift from the floor without an instant upright-volume jump. */
static inline QPVector qp_carry_axis(QPVector start,QPVector end,float t){
    if(qp_dot(start,end)<0)start=qp_mul(start,-1);
    QPVector a=qp_add(qp_mul(start,1-t),qp_mul(end,t));float n=qp_sqrt(qp_dot(a,a));return n>.001f?qp_mul(a,1/n):end;
}
static inline int qp_carry_samples(float distance,QPVector start,QPVector end){
    float alignment=qp_bound(qp_abs(qp_dot(start,end)),0,1);
    float half=QP_COLLIDER_HEIGHT*.5f-QP_COLLIDER_RADIUS;
    int travel=(int)(distance/.04f)+1,turn=(int)(half*qp_sqrt(2-2*alignment)/.01f)+1;
    return travel>turn?travel:turn;
}
#endif
