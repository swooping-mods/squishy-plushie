/* Explicit private collision qualification, separate from submission timings.
 * One rigid static plane is moved between cases, far above the starter base.
 * Unity advances the real rigidbody/capsule and generates every admitted hit. */
static SURoot qp_throw_plane;
static SPVec3 qp_throw_origin,qp_throw_up,qp_throw_right,qp_throw_forward,qp_throw_normal;
static int qp_throw_case,qp_throw_started,qp_throw_checks;
static float qp_throw_age,qp_throw_contact_age,qp_throw_peak,qp_throw_trough;
static void qp_throw_reset(void){
    su_destroy(su_get(qp_throw_plane));su_drop(&qp_throw_plane);
    qp_throw_case=qp_throw_started=qp_throw_checks=0;qp_throw_age=0;qp_throw_contact_age=-1;
}
static void qp_throw_fail(const char *why){event(why);failed=1;}
static char *qp_throw_bits(char *s,const float *values,unsigned count){
    *s++='[';for(unsigned i=0;i<count;i++){unsigned bits;su_copy(&bits,values+i,4);if(i)*s++=',';s=num(s,bits);}*s++=']';*s=0;return s;
}
static void qp_throw_write(char *b,char *s){s=cat(s,"}\n");DWORD n=0;WriteFile(log_file,b,(DWORD)(s-b),&n,0);}
static int qp_throw_launch(void){
    static const float speeds[]={2,5,10},cosines[]={1,.819152044f,.422618262f},sines[]={0,.573576436f,.906307787f};
    int surface=qp_throw_case/9,angle=qp_throw_case/3%3,level=qp_throw_case%3;
    if(!qp_throw_plane){
        qp_throw_up=su_normal(su_sub(last_position,moon_center));
        qp_throw_forward=su_normal(su_cross(su_vector(su.tr_right,su_get(tr)),qp_throw_up));
        qp_throw_right=su_normal(su_cross(qp_throw_up,qp_throw_forward));
        qp_throw_origin=su_add(last_position,su_mul(qp_throw_up,80));
        int ok=1,cube=3;void *a[]={&cube};PTR go=call(su.primitive,0,a,&ok);qp_throw_plane=su_hold(go);
        if(!ok||!qp_throw_plane)return 0;
    }
    qp_throw_normal=surface?su_mul(qp_throw_forward,-1):qp_throw_up;
    SPVec3 tangent=su_mul(qp_throw_right,angle==1?-1:1),binormal=su_normal(su_cross(tangent,qp_throw_normal));
    PTR plane_tr=su_transform(su_get(qp_throw_plane));
    su_set_vector(su.tr_position,plane_tr,su_sub(qp_throw_origin,su_mul(qp_throw_normal,.1f)));
    su_set_quat(su.tr_rotation,plane_tr,su_rotation(binormal,qp_throw_normal));
    su_set_vector(su.tr_scale,plane_tr,su_v(32,.2f,32));
    /* Different body orientations make the same world contact strike different
     * signed local axes, including a fully inverted capsule in the wall set. */
    float tilt=surface?-cosines[angle]:cosines[angle];
    SPVec3 body_up=su_add(su_mul(qp_throw_normal,tilt),su_mul(tangent,sines[angle]));
    SUQuat q=su_rotation(binormal,body_up);
    float support=QP_COLLIDER_RADIUS+(QP_COLLIDER_HEIGHT*.5f-QP_COLLIDER_RADIUS)*qp_abs(tilt);
    SPVec3 p=su_add(qp_throw_origin,su_mul(qp_throw_normal,support+.8f));
    SPVec3 v=su_mul(su_add(su_mul(qp_throw_normal,-cosines[angle]),su_mul(tangent,sines[angle])),speeds[level]);
    su_set_vector(rb_set,su_get(body),su_v(0,0,0));su_set_vector(rb_spin,su_get(body),su_v(0,0,0));
    su_set_vector(rb_set_position,su_get(body),p);su_set_quat(rb_rotation,su_get(body),q);
    su_set_vector(su.tr_position,su_get(tr),p);su_set_quat(su.tr_rotation,su_get(tr),q);
    su_set_vector(rb_set,su_get(body),v);last_velocity=v;last_position=p;
    su_zero(&soft,sizeof(soft));soft.axis=(QPVector){0,1,0};published_mesh.valid=0;mesh_checked=1;impact_mesh_pending=0;deform(0);
    qp_throw_age=qp_throw_peak=qp_throw_trough=0;qp_throw_contact_age=-1;qp_throw_checks=0;
    char b[500],*s=cat(b,"{\"event\":\"plush-throw-start\",\"case\":");s=num(s,qp_throw_case);
    s=cat(s,",\"wall\":");s=num(s,surface);s=cat(s,",\"angle_degrees\":");s=num(s,angle==0?0:angle==1?35:65);
    s=cat(s,",\"launch_speed\":");s=number(s,speeds[level]);qp_throw_write(b,s);
    return 1;
}
static void qp_throw_lab(float dt){
    if(qp_throw_case>=18)return;
    if(!qp_throw_started){if(!qp_throw_launch()){qp_throw_fail("plush-throw-setup-failed");return;}qp_throw_started=1;return;}
    SPVec3 before=last_velocity,dv;float impulse=free_physics(dt,&dv);qp_throw_age+=dt;
    if(impulse&&qp_throw_contact_age<0){
        SPVec3 p=su_vector(rb_position,su_get(body)),up=su_vector(su.tr_up,su_get(tr));
        float support=QP_COLLIDER_RADIUS+(QP_COLLIDER_HEIGHT*.5f-QP_COLLIDER_RADIUS)*qp_abs(su_dot(up,qp_throw_normal));
        float gap=su_dot(su_sub(p,qp_throw_origin),qp_throw_normal)-support;
        float incoming=-su_dot(before,qp_throw_normal),outward=su_dot(dv,qp_throw_normal);
        SPVec3 world_axis=rotate(rotation(su_get(tr)),su_v(soft.axis.x,soft.axis.y,soft.axis.z));
        float alignment=su_dot(world_axis,su_normal(dv));
        /* Contact has to be near the owned surface, oppose incoming motion and
         * retain the signed velocity-change direction in the rendered body. */
        if(gap<-.08f||gap>.45f||incoming<.35f||outward<.35f||alignment<.9999f){qp_throw_fail("plush-throw-contact-failed");return;}
        qp_throw_contact_age=0;
        char b[700],*s=cat(b,"{\"event\":\"plush-throw-contact\",\"case\":");s=num(s,qp_throw_case);
        s=cat(s,",\"incoming_speed\":");s=number(s,su_sqrt(su_dot(before,before)));s=cat(s,",\"normal_speed\":");s=number(s,incoming);
        s=cat(s,",\"impulse\":");s=number(s,impulse);s=cat(s,",\"gap\":");s=number(s,gap);
        s=cat(s,",\"axis_alignment\":");s=number(s,alignment);s=cat(s,",\"local_axis\":");s=qp_throw_bits(s,&soft.axis.x,3);qp_throw_write(b,s);
    }
    /* Bit-preserving samples let a host replay the real collision trace through
     * the frozen original CPU solver, without bundling that oracle in the mod. */
    float values[]={dt,impulse,soft.axis.x,soft.axis.y,soft.axis.z};
    deform(dt);
    char b[800],*s=cat(b,"{\"event\":\"plush-throw-step\",\"case\":");s=num(s,qp_throw_case);
    s=cat(s,",\"input_bits\":");s=qp_throw_bits(s,values,5);s=cat(s,",\"state_bits\":");s=qp_throw_bits(s,&soft.compression,6);
    s=cat(s,",\"impacts\":");s=num(s,soft.impacts);qp_throw_write(b,s);
    if(soft.compression>qp_throw_peak)qp_throw_peak=soft.compression;if(soft.compression<qp_throw_trough)qp_throw_trough=soft.compression;
    int phase=soft.compression>.025f?1:soft.compression<-.001f?2:0;
    if(qp_throw_contact_age>=0&&phase&&!(qp_throw_checks&phase)){
        if(!qp_visual_readback()){qp_throw_fail("plush-throw-renderer-failed");return;}
        qp_throw_checks|=phase;
        s=cat(b,"{\"event\":\"plush-throw-renderer\",\"case\":");s=num(s,qp_throw_case);s=cat(s,",\"phase\":");s=num(s,phase);qp_throw_write(b,s);
    }
    if(qp_throw_contact_age>=0)qp_throw_contact_age+=dt;
    if(qp_throw_contact_age>1.8f&&qp_throw_checks==3){
        s=cat(b,"{\"event\":\"plush-throw-passed\",\"case\":");s=num(s,qp_throw_case);
        s=cat(s,",\"peak\":");s=number(s,qp_throw_peak);s=cat(s,",\"trough\":");s=number(s,qp_throw_trough);qp_throw_write(b,s);
        qp_throw_case++;qp_throw_started=0;
        if(qp_throw_case==18){event("plush-throws-complete");FlushFileBuffers(log_file);}
    }else if(qp_throw_age>6)qp_throw_fail("plush-throw-timeout-failed");
}
