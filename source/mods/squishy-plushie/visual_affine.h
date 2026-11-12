#ifndef QP_VISUAL_AFFINE_H
#define QP_VISUAL_AFFINE_H
#include "soft_core.h"
/* Plushie-local candidate, not an extracted framework. A visual-only hierarchy
 * represents R * diagonal(scale) * inverse(R), with an outer contact offset.
 * Physics/carry transforms and the mesh's original arrays remain independent. */
typedef struct {QPVector right,axis,forward,scale,offset;} QPAffinePose;
static inline QPVector qp_cross(QPVector a,QPVector b){
    return (QPVector){a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
static inline int qp_affine_pose(const QPSoft *s,QPAffinePose *out){
    if(!s||!out||!qp_valid(s->compression)||s->compression<-.12f||s->compression>.30f)return 0;
    /* At rest the baseline ignores even an unset event axis. Publish identity
     * to restore all three nodes before suppressing further idle writes. */
    if(s->compression==0){
        *out=(QPAffinePose){{1,0,0},{0,1,0},{0,0,1},{1,1,1},{0,0,0}};return 1;
    }
    QPVector axis=s->axis;
    if(!qp_valid(axis.x)||!qp_valid(axis.y)||!qp_valid(axis.z))return 0;
    float length=qp_dot(axis,axis);
    /* qp_hit already normalizes. Reject malformed state rather than silently
     * changing the signed event axis or its accepted compression behavior. */
    if(qp_abs(length-1)>.00001f)return 0;
    QPVector reference=qp_abs(axis.y)<.9f?(QPVector){0,1,0}:(QPVector){1,0,0};
    QPVector right=qp_cross(axis,reference);right=qp_mul(right,1/qp_sqrt(qp_dot(right,right)));
    QPVector forward=qp_cross(right,axis);
    float along=1-s->compression,across=1/qp_sqrt(along);
    *out=(QPAffinePose){right,axis,forward,{across,along,across},qp_mul(axis,-.26f*s->compression)};
    return 1;
}
/* Independent hierarchy evaluation seam for the frozen-baseline host checks.
 * Rendering must additionally qualify Unity's inverse-transpose normals,
 * transformed bounds, shadows and material behavior before candidate activation. */
static inline QPVector qp_affine_apply(const QPAffinePose *p,QPVector v,int normal){
    QPVector local={qp_dot(v,p->right),qp_dot(v,p->axis),qp_dot(v,p->forward)};
    local.x*=normal?1/p->scale.x:p->scale.x;
    local.y*=normal?1/p->scale.y:p->scale.y;
    local.z*=normal?1/p->scale.z:p->scale.z;
    QPVector out=qp_add(qp_add(qp_mul(p->right,local.x),qp_mul(p->axis,local.y)),qp_mul(p->forward,local.z));
    if(!normal)return qp_add(out,p->offset);
    float length=qp_sqrt(qp_dot(out,out));return length>.00001f?qp_mul(out,1/length):out;
}
#endif
