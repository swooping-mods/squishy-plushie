/* Explicit test-only engine observation. A matrix readback proves the submitted
 * geometry contract; actual shader lighting/shadows still require captures. */
#include "visual_readback_math.h"
static int qp_read_value(const char *type,const char *name,PTR object,void *out,U64 size){
    PTR method=su_method("UnityEngine",type,name,"");int ok=1;
    if(!method){char message[192];cat(cat(cat(cat(message,"visual-readback-missing-"),type),"-"),name);event(message);return 0;}
    PTR boxed=call(method,object,0,&ok);
    if(!ok||!boxed)return 0;
    su_copy(out,unbox(boxed),size);return 1;
}
static int qp_visual_readback(void){
    QPMatrix actual,inverse,physical,physical_inverse;QPVector bounds[2],scale,center;
    float radius=0,height=0;int cast=0;unsigned char receives=0;
    int ok=qp_read_value("Renderer","get_localToWorldMatrix",su_get(visual_renderer),&actual,sizeof(actual))&&
        qp_read_value("Transform","get_localToWorldMatrix",su_get(tr),&physical,sizeof(physical))&&
        qp_read_value("Renderer","get_bounds",su_get(visual_renderer),bounds,sizeof(bounds))&&
        qp_read_value("Renderer","get_shadowCastingMode",su_get(visual_renderer),&cast,sizeof(cast))&&
        qp_read_value("Renderer","get_receiveShadows",su_get(visual_renderer),&receives,sizeof(receives))&&
        qp_read_value("Transform","get_localScale",su_get(tr),&scale,sizeof(scale))&&
        qp_read_value("CapsuleCollider","get_radius",su_get(collider),&radius,sizeof(radius))&&
        qp_read_value("CapsuleCollider","get_height",su_get(collider),&height,sizeof(height))&&
        qp_read_value("CapsuleCollider","get_center",su_get(collider),&center,sizeof(center));
    /* The pinned player strips Renderer.worldToLocalMatrix. Invert its actual
     * full renderer matrix, preserving shear; lossyScale/rotation is insufficient. */
    if(!ok||!qp_matrix_inverse(&actual,&inverse)||!qp_matrix_inverse(&physical,&physical_inverse))return 0;
    if(scale.x!=1||scale.y!=1||scale.z!=1||radius!=QP_COLLIDER_RADIUS||
       height!=QP_COLLIDER_HEIGHT||qp_dot(center,center)!=0||cast!=1||!receives)return 0;
    /* Root both arrays before any further managed call. Mesh getters allocate
     * copies; retain neither a raw array pointer nor a boxed value across GC. */
    PTR get_positions=su_method("UnityEngine","Mesh","get_vertices","");
    PTR get_normals=su_method("UnityEngine","Mesh","get_normals","");
    if(!get_positions||!get_normals)return 0;
    SURoot positions=su_hold(call(get_positions,su_get(mesh),0,&ok));
    SURoot normal_values=su_hold(call(get_normals,su_get(mesh),0,&ok));
    if(!positions||!normal_values||!ok||su.array_length(su_get(positions))!=QP_VERTICES||
       su.array_length(su_get(normal_values))!=QP_VERTICES){su_drop(&normal_values);su_drop(&positions);return 0;}
    const QPVector *p=su.array_address(su_get(positions),12,0),*n=su.array_address(su_get(normal_values),12,0);
    float position_error=0,normal_error=0,movement=0;int immutable=1;
    if(!p||!n)ok=0;
    for(unsigned i=0;ok&&i<QP_VERTICES;i++){
        QPVector rest={qp_positions[i][0],qp_positions[i][1],qp_positions[i][2]};
        QPVector normal={qp_normals[i][0],qp_normals[i][1],qp_normals[i][2]};
        immutable&=p[i].x==rest.x&&p[i].y==rest.y&&p[i].z==rest.z&&
            n[i].x==normal.x&&n[i].y==normal.y&&n[i].z==normal.z;
        QPVector expected=qp_matrix_point(&physical,qp_deform(&soft,rest,0));
        QPVector observed=qp_matrix_point(&actual,p[i]);
        QPVector delta=qp_add(observed,qp_mul(expected,-1));
        float error=qp_dot(delta,delta);if(!qp_valid(error)){ok=0;break;}if(error>position_error)position_error=error;
        delta=qp_add(qp_matrix_normal(&inverse,n[i]),qp_mul(qp_matrix_normal(&physical_inverse,qp_deform(&soft,normal,1)),-1));
        error=qp_dot(delta,delta);if(!qp_valid(error)){ok=0;break;}if(error>normal_error)normal_error=error;
        delta=qp_add(observed,qp_mul(qp_matrix_point(&physical,rest),-1));error=qp_dot(delta,delta);
        if(!qp_valid(error)){ok=0;break;}if(error>movement)movement=error;
        if(!qp_bounds_contains(bounds[0],bounds[1],observed,.005f))ok=0;
    }
    su_drop(&normal_values);su_drop(&positions);
#if QP_IMMUTABLE_VISUAL_CANDIDATE
    ok=ok&&immutable;
#else
    (void)immutable;
#endif
    /* Five millimetres allows float cancellation at lunar world coordinates;
     * local-space host parity remains much stricter (two micrometres). */
    ok=ok&&position_error<.000025f&&normal_error<4e-10f&&
        (qp_abs(soft.compression)<.02f||movement>1e-8f);
    char b[600],*s=cat(b,"{\"event\":\"plush-visual-readback\",\"ok\":");s=num(s,ok);
    s=cat(s,",\"immutable\":");s=num(s,immutable);s=cat(s,",\"compression\":");s=number(s,soft.compression);
    s=cat(s,",\"position_error\":");s=number(s,qp_normal_length(position_error));
    s=cat(s,",\"normal_error\":");s=number(s,qp_normal_length(normal_error));
    s=cat(s,",\"held\":");s=num(s,held);s=cat(s,"}\n");DWORD written=0;WriteFile(log_file,b,(DWORD)(s-b),&written,0);
    return ok;
}
