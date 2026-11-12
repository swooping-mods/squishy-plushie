#ifndef QP_VISUAL_CANDIDATE_H
#define QP_VISUAL_CANDIDATE_H
/* Deliberately excluded from ordinary builds until the deferred accepted-feel
 * gates pass. This is not a runtime setting or consumer-facing service. */
#ifndef QP_IMMUTABLE_VISUAL_CANDIDATE
#define QP_IMMUTABLE_VISUAL_CANDIDATE 0
#endif
#if QP_IMMUTABLE_VISUAL_CANDIDATE
#include "visual_affine.h"
static SURoot qp_visual_basis,qp_visual_scale,qp_visual_inverse;
static PTR qp_visual_child(const char *name,PTR parent,SURoot *transform){
    int ok=1;PTR go=su_gameobject(name,0),tr=su_transform(go);unsigned char keep_world=0;
    if(!go||!tr){su_destroy(go);return 0;}
    void *args[]={parent,&keep_world};call(su.tr_parent,tr,args,&ok);
    if(!ok){su_destroy(go);return 0;}
    *transform=su_hold(tr);return *transform?go:0;
}
static PTR qp_visual_make(PTR parent){
    if(!qp_visual_child("Plush contact basis",parent,&qp_visual_basis))return 0;
    if(!qp_visual_child("Plush visual compression",su_get(qp_visual_basis),&qp_visual_scale))return 0;
    return qp_visual_child("Plush original mesh",su_get(qp_visual_scale),&qp_visual_inverse);
}
static void qp_visual_reset(void){
    su_drop(&qp_visual_inverse);su_drop(&qp_visual_scale);su_drop(&qp_visual_basis);
}
static int qp_visual_publish(const QPSoft *s){
    QPAffinePose p;if(!qp_affine_pose(s,&p))return 0;
    int ok=1;SUQuat q={0,0,0,1};
    if(s->compression!=0){
        SPVec3 forward=su_v(p.forward.x,p.forward.y,p.forward.z),up=su_v(p.axis.x,p.axis.y,p.axis.z);
        void *args[]={&forward,&up};PTR boxed=call(su.look_rotation,0,args,&ok);
        if(!ok||!boxed)return 0;su_copy(&q,unbox(boxed),sizeof(q));
    }
    SUQuat inverse={-q.x,-q.y,-q.z,q.w};
    SPVec3 offset=su_v(p.offset.x,p.offset.y,p.offset.z),scale=su_v(p.scale.x,p.scale.y,p.scale.z);
    void *translation[]={&offset},*basis[]={&q},*stretch[]={&scale},*undo[]={&inverse};
    call(su.tr_local_position,su_get(qp_visual_basis),translation,&ok);
    call(su.tr_local_rotation,su_get(qp_visual_basis),basis,&ok);
    call(su.tr_scale,su_get(qp_visual_scale),stretch,&ok);
    call(su.tr_local_rotation,su_get(qp_visual_inverse),undo,&ok);
    return ok;
}
#endif
#endif
