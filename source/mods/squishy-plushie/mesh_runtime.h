#ifndef QP_MESH_RUNTIME_H
#define QP_MESH_RUNTIME_H
#include "soft_core.h"
/* Immutable coefficients shared by every vertex in one submitted pose. */
typedef struct {QPVector axis;float along,across,normal_along,normal_across,offset;} QPDeformation;
typedef struct {float compression;QPVector axis;int valid;} QPMeshPose;
static inline QPDeformation qp_prepare_deformation(const QPSoft *s){
    float along=1-s->compression,across=1/qp_sqrt(along);
    return (QPDeformation){s->axis,along,across,1/along,1/across,-.26f*s->compression};
}
static inline float qp_normal_length(float squared){
#if defined(__x86_64__) && (defined(__clang__) || defined(__GNUC__))
    /* The freestanding Win64 build has no CRT sqrtf. SSE is guaranteed on
     * this target; use its full-precision sqrt, not an approximate reciprocal. */
    float length;__asm__("sqrtss %1, %0":"=x"(length):"x"(squared));return length;
#else
    return qp_sqrt(squared);
#endif
}
static inline QPVector qp_apply_deformation(const QPDeformation *d,QPVector p,int normal){
    float along=normal?d->normal_along:d->along,across=normal?d->normal_across:d->across;
    QPVector out=qp_add(qp_mul(p,across),qp_mul(d->axis,qp_dot(p,d->axis)*(along-across)));
    if(!normal)return qp_add(out,qp_mul(d->axis,d->offset));
    float length=qp_normal_length(qp_dot(out,out));return length>.00001f?qp_mul(out,1/length):out;
}
static inline int qp_mesh_changed(const QPMeshPose *published,const QPSoft *s){
    return !published->valid||published->compression!=s->compression||
        (s->compression!=0&&(published->axis.x!=s->axis.x||published->axis.y!=s->axis.y||published->axis.z!=s->axis.z));
}
static inline void qp_mesh_commit(QPMeshPose *published,const QPSoft *s){
    *published=(QPMeshPose){s->compression,s->axis,1};
}
#endif
