/* Session-local physical toy. All scene work uses the shared main-thread service. */
#include "../../runtime/native/gameplay.h"
#include "../../runtime/native/input.h"
#include "../../runtime/native/interaction_client.h"
#include "adapters/unity.h"
#include "../sound-framework/sdk/client.h"
#include "../pickup-hands/sdk/client.h"
#include "../pickup-hands/sdk/motion.h"
/* Both legacy SDKs call their connector mh_connect; scope the hints alias. */
#define mh_connect qp_hints_connect
#include "../hints-framework/sdk/client.h"
#undef mh_connect
#include "soft_core.h"
#include "mesh_runtime.h"
#include "visual_candidate.h"
#include "../sound-framework/sdk/preparation_client.h"
#include "collision.h"
#include "carry_core.h"
#include "audio/bank.h"
#include "assets/generated/plush_data.h"
int _fltused=0;
void *memcpy(void *a,const void *b,U64 n){su_copy(a,b,n);return a;}
void *memset(void *a,int v,U64 n){unsigned char *p=a;while(n--)*p++=(unsigned char)v;return a;}
static const MoonSounds *audio;
static const MoonSoundPreparation *audio_preparation;
static unsigned audio_warm_step;
static QPMeshPose published_mesh;
static const MoonHands *hands;
static const MoonHints *hints;
static SURoot carry_walker;
static unsigned sound_handles[sizeof(qp_sound_bank)/sizeof(*qp_sound_bank)];
static PTR log_file,world_field,camera_field,stations,station_id,restore,blocked,typing,firstperson,focus,local_id,host_id,joined;
static PTR rb_get,rb_set,rb_position,rb_set_position,rb_spin,rb_kinematic,rb_gravity,rb_ccd,rb_mass,rb_drag;
static PTR collider_material,mat_ctor,mat_bounce,mat_friction,mat_static,mat_combine;
static PTR cap_radius,cap_height,cap_direction,hit_collider,component_transform,texture_ctor,image_load,material_texture,set_color;
static PTR rb_rotation,gameobject_layer;
static SURoot world,root,tr,body,collider,physics_material,mesh,vertices,normals,material,texture;
static QPSoft soft;
static SPVec3 last_velocity,last_position,moon_center;
static SPVec3 emitter_position;static int emitter_position_valid;
static float squeeze_timer;static int held,approaching,frames,failed,started,host_world,test_tools,test_staged;
static DWORD unity_thread;static int mesh_checked,impact_mesh_pending;
static short (*key_state)(int);static PTR (*foreground)(void);static DWORD (*window_pid)(PTR,DWORD *);
static void event(const char *name){if(!log_file)return;char b[256],*p=cat(b,"{\"event\":\"");p=cat(p,name);p=cat(p,"\",\"mod\":\"squishy-plushie\"}\n");DWORD n=0;WriteFile(log_file,b,(DWORD)(p-b),&n,0);}
#include "bindings.h"
#include "profile.h"
static char *number(char *p,float v){if(!qp_valid(v))return cat(p,"null");if(v<0){*p++='-';v=-v;}U64 n=(U64)(v*100000.f+.5f);p=num(p,n/100000);*p++='.';for(U64 d=10000;d;d/=10)*p++=(char)('0'+n/d%10);*p=0;return p;}
static void sample(void){if(!test_tools||!body)return;SPVec3 v=su_vector(rb_get,su_get(body)),p=su_vector(rb_position,su_get(body));char b[500],*s=cat(b,"{\"event\":\"plush-sample\",\"held\":");s=num(s,held);s=cat(s,",\"compression\":");s=number(s,soft.compression);s=cat(s,",\"impacts\":");s=num(s,soft.impacts);s=cat(s,",\"position\":[");s=number(s,p.x);*s++=',';s=number(s,p.y);*s++=',';s=number(s,p.z);s=cat(s,"],\"speed\":");s=number(s,su_sqrt(su_dot(v,v)));s=cat(s,"}\n");DWORD n=0;WriteFile(log_file,b,(DWORD)(s-b),&n,0);FlushFileBuffers(log_file);}
static QPVector qv(SPVec3 v){return (QPVector){v.x,v.y,v.z};}
static SPVec3 rotate(SUQuat q,SPVec3 v){SPVec3 u=su_v(q.x,q.y,q.z),t=su_mul(su_cross(u,v),2);return su_add(v,su_add(su_mul(t,q.w),su_cross(u,t)));}
static SUQuat rotation(PTR transform){SUQuat q={0,0,0,1};int ok=1;PTR b=call(su.tr_get_rotation,transform,0,&ok);if(ok&&b)su_copy(&q,unbox(b),sizeof(q));return q;}
static SPVec3 local_axis(SPVec3 axis){SUQuat q=rotation(su_get(tr));q.x=-q.x;q.y=-q.y;q.z=-q.z;return rotate(q,axis);}
static int key_e,key_f9,button_left,button_right;
static unsigned qp_interactions[3];
static void input_edges(PTR walker){
    /* Sample before the existing context gate so typing/focus changes consume
     * held edges. Pickup/throw/squeeze eligibility remains in the toy. */
    key_e=moon_interaction_take(qp_interactions[0],walker);
    key_f9=moon_input_action("squishy-plushie.test-view","F9",1).pressed;
    button_left=moon_interaction_take(qp_interactions[1],walker);
    button_right=moon_interaction_take(qp_interactions[2],walker);
}
static int pressed(int key){return key==101?key_e:key==290?key_f9:0;}
static int mouse(int button){return button==0?button_left:button_right;}
static int ray(SPVec3 p,SPVec3 direction,float distance,unsigned char *hit){int ok=1,mask=~(1<<2),trigger=1;void *a[]={&p,&direction,hit,&distance,&mask,&trigger};PTR r=call(su.physics_ray,0,a,&ok);return ok&&r&&*(unsigned char *)unbox(r);}
static int clear_axis(SPVec3 p,SPVec3 up,float radius){int ok=1,mask=~(1<<2),trigger=1;float half=QP_COLLIDER_HEIGHT*.5f-QP_COLLIDER_RADIUS;SPVec3 a=su_add(p,su_mul(up,half)),b=su_sub(p,su_mul(up,half));void *args[]={&a,&b,&radius,&mask,&trigger};PTR r=call(su.physics_capsule,0,args,&ok);return ok&&r&&!*(unsigned char *)unbox(r);}
static int clear(SPVec3 p){return clear_axis(p,su_normal(su_sub(p,moon_center)),QP_CLEARANCE_RADIUS);}
static int pickup_target(PTR camera){
    int ok=1;PTR t=call(component_transform,camera,0,&ok);if(!ok||!t)return 0;
    SPVec3 eye=su_vector(su.tr_get_position,t),dir=su_vector(su.tr_forward,t);unsigned char hit[96];
    if(!ray(eye,dir,QP_PICKUP_REACH,hit))return 0;
    PTR col=call(hit_collider,hit,0,&ok);return ok&&col==su_get(collider);
}
#include "interaction.h"
static int hint_state;
static void pickup_hint(PTR walker,PTR camera,int allow){
    int state=allow&&!held&&moon_interaction_sample(qp_interactions[0]).selected&&!hands->occupied(walker)&&pickup_target(camera);
    if(state){char key[64],text[96];if(!moon_input_label("squishy-plushie.pickup-release","E",key,sizeof(key)))cat(key,"E");cat(cat(text,key),": Pick up plushie");state=hints->publish("squishy-plushie",walker,text);}
    else hints->clear("squishy-plushie");
    if(test_tools&&state!=hint_state){event(state?"pickup-hint-published":"pickup-hint-cleared");hint_state=state;}
}
static int corridor(SPVec3 a,SPVec3 b,SPVec3 end_up){
    SPVec3 delta=su_sub(b,a);float distance=su_sqrt(su_dot(delta,delta));
    /* Short bounded occupancy samples supplement the center ray for the full
     * held toy. Reject large camera teleports rather than crossing a wall. */
    if(distance>(held&&!approaching?1.5f:2.8f))return 0;unsigned char hit[96];if(distance>.0001f&&ray(a,su_mul(delta,1/distance),distance,hit))return 0;
    /* Rotation alone still sweeps the capsule ends; use the actual candidate
     * axis rather than radial world-up for the final occupancy check. */
    SPVec3 start_up=su_vector(su.tr_up,su_get(tr));int count=qp_carry_samples(distance,qv(start_up),qv(end_up));
    for(int i=1;i<=count;i++){float t=(float)i/count;QPVector n=qp_carry_axis(qv(start_up),qv(end_up),t);SPVec3 axis=su_v(n.x,n.y,n.z);float start=QP_COLLIDER_RADIUS-.001f;if(!clear_axis(su_add(a,su_mul(delta,t)),axis,start+(QP_CLEARANCE_RADIUS-start)*t))return 0;}return 1;
}
static void sound_event(const char *name){
    U64 begin=qp_clock();if(su_eq(name,"impact-hard")||su_eq(name,"impact-soft"))qp_profile_current.impact=1;
    event(name);if(!audio||!body)return;char key[96];cat(cat(key,"squishy-plushie."),name);
    for(unsigned i=0;i<sizeof(sound_handles)/sizeof(*sound_handles);i++)if(sound_handles[i]&&su_eq(key,qp_sound_bank[i].key)){
        SPVec3 p=su_vector(rb_position,su_get(body));float at[]={p.x,p.y,p.z};audio->play(sound_handles[i],0x51504c555348ULL,at,1,1);break;
    }
    qp_profile_current.sound+=qp_clock()-begin;
}
static void end_world(void){
    qp_profile_dump();
    if(hints)hints->clear("squishy-plushie");hint_state=0;
    if(hands&&carry_walker)hands->release("squishy-plushie",su_get(carry_walker));su_drop(&carry_walker);
    if(audio)audio->stop(0x51504c555348ULL);
    su_destroy(su_get(root));su_destroy(su_get(mesh));su_destroy(su_get(material));su_destroy(su_get(texture));su_destroy(su_get(physics_material));
#if QP_IMMUTABLE_VISUAL_CANDIDATE
    qp_visual_reset();
#endif
    SURoot *roots[]={&world,&root,&tr,&body,&collider,&physics_material,&mesh,&vertices,&normals,&material,&texture};for(unsigned i=0;i<sizeof(roots)/sizeof(*roots);i++)su_drop(roots[i]);
    su_zero(&soft,sizeof(soft));held=approaching=frames=host_world=test_staged=mesh_checked=impact_mesh_pending=0;squeeze_timer=0;audio_warm_step=0;published_mesh.valid=0;emitter_position_valid=0;event("world-reset");
}
static int anchor(SPVec3 *out,SPVec3 *forward,SPVec3 *up){
    int ok=1;PTR list=call(stations,su_get(world),0,&ok);if(!ok||!list)return 0;int count=integer(get_method(object_class(list),"get_Count",0),list,&ok);if(!ok||count<1||count>16)return 0;
    for(int i=0;i<count;i++){void *item[]={&i};PTR station=call(get_method(object_class(list),"get_Item",1),list,item,&ok);void *args[]={station};PTR id=call(station_id,0,args,&ok);char name[64];if(!ok||!su_string(id,name,sizeof(name))||!su_eq(name,"station/north"))continue;
        PTR transform=call(component_transform,station,0,&ok);SPVec3 origin=su_vector(su.tr_get_position,transform),right=su_vector(su.tr_right,transform);*up=su_vector(su.tr_up,transform);*forward=su_vector(su.tr_forward,transform);
        const float offsets[][2]={{-4,-2},{-3,-2},{-4,-1},{-3,-1}};
        for(unsigned k=0;k<4;k++){SPVec3 q=su_add(origin,su_add(su_mul(right,offsets[k][0]),su_add(su_mul(*up,3),su_mul(*forward,offsets[k][1]))));unsigned char hit[96];if(!ray(q,su_mul(*up,-1),5,hit))continue;
            SPVec3 p,n;su_copy(&p,hit+su.hit_point,12);su_copy(&n,hit+su.hit_normal,12);float floor=su_dot(su_sub(p,origin),*up);if(su_dot(n,*up)<.9f||floor<-.5f||floor>1.f)continue;
            *out=su_add(p,su_mul(*up,.38f));if(clear(*out))return 1;
        }
    }return 0;
}
static int make(SPVec3 p,SPVec3 forward,SPVec3 up){
    int ok=1;PTR go=su_gameobject("Squishy plush astronaut",0);root=su_hold(go);tr=su_hold(su_transform(go));su_set_vector(su.tr_position,su_get(tr),p);su_set_quat(su.tr_rotation,su_get(tr),su_rotation(forward,up));
    PTR m=su_new("UnityEngine","Mesh");mesh=su_hold(m);call(su.mesh_ctor,m,0,&ok);
#if !QP_IMMUTABLE_VISUAL_CANDIDATE
    PTR dynamic=su_method("UnityEngine","Mesh","MarkDynamic","");if(dynamic){call(dynamic,m,0,&ok);event("mesh-dynamic-prepared");}
#endif
    vertices=su_hold(su_array(su.vector_class,QP_VERTICES,qp_positions,12));normals=su_hold(su_array(su.vector_class,QP_VERTICES,qp_normals,12));
    if(!vertices||!normals)return 0;
    void *va[]={su_get(vertices)},*na[]={su_get(normals)};call(su.mesh_vertices,m,va,&ok);call(su.mesh_normals,m,na,&ok);su_int(su.mesh_subcount,m,1);
    PTR uv=su_array(su.uv_class,QP_VERTICES,qp_uv,8),indices=su_array(su.int_class,QP_INDICES,qp_indices,4);void *ua[]={uv};call(su.mesh_uv,m,ua,&ok);int index=0;unsigned char bounds=1;void *ia[]={indices,&index,&bounds};call(su.mesh_indices,m,ia,&ok);
    PTR visual=go;
#if QP_IMMUTABLE_VISUAL_CANDIDATE
    visual=qp_visual_make(su_get(tr));if(!visual||!qp_visual_publish(&soft))return 0;
    event("immutable-visual-candidate-unqualified");
#endif
    PTR filter=su_add_component(visual,"MeshFilter"),renderer=su_add_component(visual,"MeshRenderer");void *ma[]={m};call(su.meshfilter_set,filter,ma,&ok);
    void *shader_args[]={su.string("Universal Render Pipeline/Lit")};PTR shader=call(su.shader_find,0,shader_args,&ok);if(!shader){void *alt[]={su.string("Universal Render Pipeline/Simple Lit")};shader=call(su.shader_find,0,alt,&ok);}if(!shader)return 0;
    PTR mat=su_new("UnityEngine","Material");material=su_hold(mat);void *mc[]={shader};call(su.material_ctor,mat,mc,&ok);float rough=.1f,metal=0;void *r[]={su.string("_Smoothness"),&rough},*me[]={su.string("_Metallic"),&metal};call(su.material_float,mat,r,&ok);call(su.material_float,mat,me,&ok);
    SUColor white={1,1,1,1};void *c[]={su.string("_BaseColor"),&white};call(set_color,mat,c,&ok);
    PTR tex=su_new("UnityEngine","Texture2D");texture=su_hold(tex);int size=2,format=4;unsigned char mip=1,linear=0;void *tc[]={&size,&size,&format,&mip,&linear};call(texture_ctor,tex,tc,&ok);PTR bytes=su_array(su.byte_class,sizeof(qp_albedo)-1,qp_albedo,1);void *load[]={tex,bytes};PTR loaded=call(image_load,0,load,&ok);if(!loaded||!*(unsigned char *)unbox(loaded))ok=0;
    void *mt[]={su.string("_BaseMap"),tex};call(material_texture,mat,mt,&ok);PTR materials=su_array(su.material_class,1,&mat,8);void *mats[]={materials};call(su.renderer_materials,renderer,mats,&ok);
    PTR col=su_add_component(go,"CapsuleCollider");collider=su_hold(col);su_float(cap_radius,col,QP_COLLIDER_RADIUS);su_float(cap_height,col,QP_COLLIDER_HEIGHT);
    int capsule_y=1;void *direction_arg[]={&capsule_y};call(cap_direction,col,direction_arg,&ok);
    if(test_tools){
        float radius=su_number(su_method("UnityEngine","CapsuleCollider","get_radius",""),col),height=su_number(su_method("UnityEngine","CapsuleCollider","get_height",""),col);
        SPVec3 center=su_vector(su_method("UnityEngine","CapsuleCollider","get_center",""),col);
        /* This player strips get_direction. Y is explicitly set above through
         * its inspected setter; available getters verify radius/height/center. */
        int exact=ok&&qp_abs(radius-QP_COLLIDER_RADIUS)<.000001f&&qp_abs(height-QP_COLLIDER_HEIGHT)<.000001f&&su_dot(center,center)<1e-12f;
        event(exact?"collider-readback-verified":"collider-readback-failed");if(!exact)return 0;
    }
    PTR pm=su_new("UnityEngine","PhysicsMaterial");if(!pm)pm=su_new("UnityEngine","PhysicMaterial");physics_material=su_hold(pm);void *pmc[]={su.string("Soft plush bounce")};call(mat_ctor,pm,pmc,&ok);su_float(mat_bounce,pm,.63f);su_float(mat_friction,pm,.45f);su_float(mat_static,pm,.5f);su_int(mat_combine,pm,3);void *cm[]={pm};call(collider_material,col,cm,&ok);
    PTR rb=su_add_component(go,"Rigidbody");body=su_hold(rb);su_float(rb_mass,rb,.35f);su_float(rb_drag,rb,.12f);su_bool(rb_gravity,rb,0);su_int(rb_ccd,rb,2);su_set_vector(rb_set,rb,su_v(0,0,0));last_velocity=su_v(0,0,0);last_position=p;soft.axis=(QPVector){0,1,0};
    if(ok)qp_mesh_commit(&published_mesh,&soft);
    event(ok?"plush-spawned":"plush-create-failed");return ok&&root&&body&&mesh&&collider;
}
static int hold_point(PTR camera,SPVec3 *point,SPVec3 *forward,SPVec3 *up){PTR t=call(component_transform,camera,0,&(int){1});*forward=su_vector(su.tr_forward,t);*up=su_vector(su.tr_up,t);SPVec3 eye=su_vector(su.tr_get_position,t);*point=su_add(eye,su_add(su_mul(*forward,1.10f),su_mul(*up,-.15f)));return clear(*point);}
static void pickup(PTR walker,PTR camera){
    if(!pickup_target(camera)){event("pickup-occluded");return;}PTR col=su_get(collider);
    su_bool(su.collider_enabled,col,0);SPVec3 p,f,u,old=su_vector(rb_position,su_get(body));
    if(!hold_point(camera,&p,&f,&u)||!corridor(old,p,u)){su_bool(su.collider_enabled,col,1);event("pickup-clearance-blocked");return;}
    carry_walker=su_hold(walker);
    if(!carry_walker||!hands->claim("squishy-plushie",walker)){su_drop(&carry_walker);su_bool(su.collider_enabled,col,1);event("pickup-hands-busy");return;}
    su_set_vector(rb_set,su_get(body),su_v(0,0,0));su_set_vector(rb_spin,su_get(body),su_v(0,0,0));su_int(rb_ccd,su_get(body),0);su_bool(rb_kinematic,su_get(body),1);su_int(gameobject_layer,su_get(root),2);last_position=old;held=approaching=1;test_staged=0;sound_event("pickup");
}
static void follow_held(PTR camera,float dt){
    SPVec3 p,f,u;if(!hold_point(camera,&p,&f,&u)||!corridor(last_position,p,u))return;
    SUQuat q=rotation(su_get(tr)),goal=su_rotation(su_mul(f,-1),u);
    MoonHoldPose current={{last_position.x,last_position.y,last_position.z},{q.x,q.y,q.z,q.w}},
        target={{p.x,p.y,p.z},{goal.x,goal.y,goal.z,goal.w}},next;
    if(!moon_hold_step(current,target,(MoonHoldTuning){18,20},dt,&next))return;
    p=su_v(next.position.x,next.position.y,next.position.z);q=(SUQuat){next.rotation.x,next.rotation.y,next.rotation.z,next.rotation.w};
    /* Validate both the requested path (including its teleport bound) and the
     * filtered pose. A blocked frame keeps the last committed position/rotation. */
    if(!corridor(last_position,p,rotate(q,su_v(0,1,0))))return;
    su_set_vector(rb_set_position,su_get(body),p);su_set_quat(rb_rotation,su_get(body),q);
    /* Kinematic Rigidbody setters defer Transform rotation until physics sync.
     * Publish the accepted visible pose now so render frames do not step at
     * the physics cadence. The held collider is disabled throughout. */
    su_set_vector(su.tr_position,su_get(tr),p);su_set_quat(su.tr_rotation,su_get(tr),q);last_position=p;
    SPVec3 remaining=su_sub(p,su_v(target.position.x,target.position.y,target.position.z));
    if(su_dot(remaining,remaining)<.25f)approaching=0;
    if(test_tools&&frames%3==0){
        MoonHoldPose poses[]={current,target,{{p.x,p.y,p.z},{q.x,q.y,q.z,q.w}}};
        /* Read back the actual engine pose, independently of the candidate. */
        SPVec3 actual=su_vector(rb_position,su_get(body));SUQuat actual_q=rotation(su_get(tr));
        poses[2]=(MoonHoldPose){{actual.x,actual.y,actual.z},{actual_q.x,actual_q.y,actual_q.z,actual_q.w}};
        char b[1100],*s=cat(b,"{\"event\":\"plush-follow\",\"dt\":");s=number(s,dt);s=cat(s,",\"poses\":[");
        for(int i=0;i<3;i++){
            if(i)*s++=',';*s++='[';const float values[]={poses[i].position.x,poses[i].position.y,poses[i].position.z,poses[i].rotation.x,poses[i].rotation.y,poses[i].rotation.z,poses[i].rotation.w};
            for(int k=0;k<7;k++){if(k)*s++=',';s=number(s,values[k]);}*s++=']';
        }
        s=cat(s,"]}\n");DWORD n=0;WriteFile(log_file,b,(DWORD)(s-b),&n,0);FlushFileBuffers(log_file);
    }
}
static int release(PTR camera,int throwing){
    SPVec3 p,f,u;int ok=1;PTR t=call(component_transform,camera,0,&ok);f=su_vector(su.tr_forward,t);u=su_vector(su.tr_up,t);p=su_vector(rb_position,su_get(body));if(!ok||!clear(p))return 0;
    su_int(gameobject_layer,su_get(root),0);su_bool(su.collider_enabled,su_get(collider),1);su_bool(rb_kinematic,su_get(body),0);su_int(rb_ccd,su_get(body),2);
    SPVec3 velocity=throwing?su_add(su_mul(f,7.5f),su_mul(u,.65f)):su_v(0,0,0);su_set_vector(rb_set,su_get(body),velocity);su_set_vector(rb_spin,su_get(body),throwing?su_mul(su_cross(u,f),4):su_v(0,0,0));last_velocity=velocity;last_position=p;held=approaching=0;
    hands->release("squishy-plushie",su_get(carry_walker));su_drop(&carry_walker);sound_event(throwing?"throw":"drop");return 1;
}
static void deform(float dt){
    U64 begin=qp_clock();
    if(test_tools&&(soft.compression!=0||soft.speed!=0||soft.cooldown!=0))qp_profile_current.spring_ticks++;
    qp_tick(&soft,dt);
    int changed=qp_mesh_changed(&published_mesh,&soft);
#if QP_IMMUTABLE_VISUAL_CANDIDATE
    /* No mutable mesh/normal arrays, uploads or bounds scans on this branch.
     * Legacy native-mesh receipts cannot qualify a transformed renderer. */
    if(changed){
        if(qp_visual_publish(&soft)){qp_mesh_commit(&published_mesh,&soft);qp_profile_current.visual_updates++;}
        else {failed=1;event("visual-affine-failed");}
    }
    impact_mesh_pending=0;qp_profile_current.deform+=qp_clock()-begin;return;
#else
    int readback=test_tools&&(!mesh_checked||impact_mesh_pending)&&qp_abs(soft.compression)>.08f;
    if(!changed&&!readback){impact_mesh_pending=0;qp_profile_current.deform+=qp_clock()-begin;return;}
    QPVector *p=su.array_address(su_get(vertices),12,0),*n=su.array_address(su_get(normals),12,0);int ok=1;
    if(!p||!n){failed=1;event("deform-array-failed");return;}
    if(changed){
        QPDeformation d=qp_prepare_deformation(&soft);
        for(unsigned i=0;i<QP_VERTICES;i++){
            QPVector position={qp_positions[i][0],qp_positions[i][1],qp_positions[i][2]},normal={qp_normals[i][0],qp_normals[i][1],qp_normals[i][2]};
            p[i]=qp_apply_deformation(&d,position,0);n[i]=qp_apply_deformation(&d,normal,1);
        }
        void *a[]={su_get(vertices)},*b[]={su_get(normals)};call(su.mesh_vertices,su_get(mesh),a,&ok);call(su.mesh_normals,su_get(mesh),b,&ok);call(su.mesh_bounds,su_get(mesh),0,&ok);
        if(!ok){failed=1;event("deform-failed");qp_profile_current.deform+=qp_clock()-begin;return;}
        qp_mesh_commit(&published_mesh,&soft);qp_profile_current.uploads++;
    }
    if(test_tools&&(!mesh_checked||impact_mesh_pending)&&qp_abs(soft.compression)>.08f){
        qp_profile_current.readback=1;
        PTR actual=call(su_method("UnityEngine","Mesh","get_vertices",""),su_get(mesh),0,&ok);
        if(ok&&actual&&su.array_length(actual)==QP_VERTICES){
            const QPVector *got=su.array_address(actual,12,0);float error=0,changed=0;
            for(unsigned i=0;i<QP_VERTICES;i++){QPVector delta=qp_add(got[i],qp_mul(p[i],-1));float e=qp_dot(delta,delta);if(e>error)error=e;QPVector shift=qp_add(got[i],qp_mul(*(const QPVector *)qp_positions[i],-1));e=qp_dot(shift,shift);if(e>changed)changed=e;}
            if(error<1e-10f&&changed>.0001f){mesh_checked=1;event(impact_mesh_pending?"native-mesh-impact-verified":"native-mesh-squish-verified");}else{failed=1;event("native-mesh-squish-failed");}
        }
    }
    impact_mesh_pending=0;qp_profile_current.deform+=qp_clock()-begin;
#endif
}
static void test_view(PTR walker,PTR camera){
    if(!test_tools)return;
    SPVec3 target=su_vector(rb_position,su_get(body)),up=su_normal(su_sub(target,moon_center));int ok=1;
    if(held&&pressed(290)){
        /* Private replay turns through the game's own placement method. Wine's
         * synthetic relative mouse events do not reliably reach this player. */
        PTR camera_tr=call(component_transform,camera,0,&ok),walker_tr=call(component_transform,walker,0,&ok);
        SPVec3 pos=su_vector(su.tr_get_position,walker_tr),aim=su_vector(su.tr_forward,camera_tr);
        up=su_normal(su_sub(pos,moon_center));float v=-su_dot(aim,up);
        SPVec3 flat=su_sub(aim,su_mul(up,su_dot(aim,up)));float h=su_sqrt(su_dot(flat,flat));
        void *a[]={&v,&h};PTR angle=call(su_method("UnityEngine","Mathf","Atan2","System.Single;System.Single"),0,a,&ok);
        if(ok&&angle){
            float pitch=*(float *)unbox(angle)*57.2957795f;
            SPVec3 forward=su_normal(su_add(su_mul(flat,.8660254f),su_mul(su_cross(up,flat),.5f)));
            void *place[]={&pos,&forward,&pitch};call(su_method("MoonCleaners","MoonWalker","Place","UnityEngine.Vector3;UnityEngine.Vector3;System.Single"),walker,place,&ok);
        }else ok=0;
        event(ok?"test-held-turn":"test-held-turn-failed");return;
    }
    if(pressed(290)){
        qp_profile_dump();qp_profile_current.readback=1;
        event("test-view-requested");
        SPVec3 facing=su_vector(su.tr_forward,su_get(tr)),q=su_add(target,su_add(su_mul(facing,1.6f),su_mul(up,3)));unsigned char hit[96];
        su_bool(su.collider_enabled,su_get(collider),0);
        if(ray(q,su_mul(up,-1),6,hit)){
            SPVec3 floor;su_copy(&floor,hit+su.hit_point,12);SPVec3 pos=su_add(floor,su_mul(up,.95f));unsigned char refresh=1;void *a[]={&pos,&refresh};call(su_method("MoonCleaners","MoonWalker","Teleport","UnityEngine.Vector3;System.Boolean"),walker,a,&ok);test_staged=ok?1:0;event(ok?"test-view-staged":"test-view-failed");
        }
        su_bool(su.collider_enabled,su_get(collider),!held);
    }
    if(test_staged&&!held){
        PTR camera_tr=call(component_transform,camera,0,&ok),walker_tr=call(component_transform,walker,0,&ok);SPVec3 eye=su_vector(su.tr_get_position,camera_tr),delta=su_sub(target,eye);float v=-su_dot(delta,up);SPVec3 flat=su_sub(delta,su_mul(up,su_dot(delta,up)));float h=su_sqrt(su_dot(flat,flat));void *a[]={&v,&h};PTR angle=call(su_method("UnityEngine","Mathf","Atan2","System.Single;System.Single"),0,a,&ok);
        if(ok&&angle){float pitch=*(float *)unbox(angle)*57.2957795f;SPVec3 pos=su_vector(su.tr_get_position,walker_tr),forward=su_normal(flat);void *place[]={&pos,&forward,&pitch};call(su_method("MoonCleaners","MoonWalker","Place","UnityEngine.Vector3;UnityEngine.Vector3;System.Single"),walker,place,&ok);}
    }
}
static void tick_frame(PTR walker){
    if(!unity_thread){unity_thread=GetCurrentThreadId();event("main-thread");}if(unity_thread!=GetCurrentThreadId())return;
    PTR w=0;field_value(walker,world_field,&w);if(world&&(!su_alive(su_get(world))||w!=su_get(world))){end_world();failed=0;}
    if(!w||failed)return;int ok=1;if(boolean(restore,w,&ok)||!ok)return;
    if(!world){world=su_hold(w);PTR transform=call(component_transform,w,0,&ok);moon_center=su_vector(su.tr_get_position,transform);PTR a=call(local_id,0,0,&ok),b=call(host_id,0,0,&ok);int in_session=boolean(joined,0,&ok);host_world=ok&&(!in_session||(a&&b&&*(U64 *)unbox(a)!=0&&*(U64 *)unbox(a)==*(U64 *)unbox(b)));event(host_world?"world-ready":"guest-suspended");}
    if(!host_world)return;float dt=su_number(su.time_delta,0);if(!qp_valid(dt)||dt<=0||dt>.1f)return;frames++;
    /* Spread silent clip/pool construction across the existing spawn delay.
     * No collision is allowed to become the first PCM upload or voice creation. */
    if(audio_warm_step<MA_VOICES){
        unsigned count=sizeof(sound_handles)/sizeof(*sound_handles),cue=audio_warm_step<count?sound_handles[audio_warm_step]:0;
        if(!audio_preparation->prepare(cue,audio_warm_step+1)){failed=1;event("audio-preparation-failed");return;}
        if(++audio_warm_step==MA_VOICES)event("audio-prepared");
    }
    if(!root){if(frames<120||frames%30)return;SPVec3 p,f,u;if(!anchor(&p,&f,&u))return;if(!make(p,f,u)){end_world();failed=1;}return;}
    PTR camera=0;field_value(walker,camera_field,&camera);if(!camera||!su_alive(su_get(body)))return;
    input_edges(walker);DWORD pid=0;window_pid(foreground(),&pid);int allow=pid==GetCurrentProcessId()&&boolean(focus,0,&ok)&&!boolean(blocked,walker,&ok)&&!boolean(typing,0,&ok)&&boolean(firstperson,walker,&ok)&&ok;
    if(allow&&test_tools)test_view(walker,camera);
    if(held){follow_held(camera,dt);
        if(allow&&pressed(101))release(camera,0);else if(allow&&mouse(0))release(camera,1);else if(allow&&mouse(1)&&squeeze_timer<=0){qp_hit(&soft,(QPVector){1,0,0},3.5f);squeeze_timer=.4f;sound_event("squeeze");}
    }else {
        SPVec3 p=su_vector(rb_position,su_get(body)),v=su_vector(rb_get,su_get(body)),dv=su_sub(v,last_velocity);float impact=su_sqrt(su_dot(dv,dv));
        if(impact>.30f&&qp_impact(&soft,qv(local_axis(su_normal(dv))),impact)){
            impact_mesh_pending=impact>2.4f;sound_event(impact>2.4f?"impact-hard":"impact-soft");
        }
        SPVec3 up=su_normal(su_sub(p,moon_center));v=su_add(v,su_mul(up,-1.62f*dt));float speed=su_sqrt(su_dot(v,v));if(speed>18)v=su_mul(v,18/speed);su_set_vector(rb_set,su_get(body),v);last_velocity=v;last_position=p;
        if(allow&&pressed(101))pickup(walker,camera);
    }
    pickup_hint(walker,camera,allow);
    if(squeeze_timer>0)squeeze_timer-=dt;deform(dt);if(test_tools&&frames%3==0)sample();
    if(audio){
        /* Physics/follow already sampled or committed this exact position.
         * A newly played voice receives its current position in sound_event. */
        SPVec3 p=last_position;
        if(!emitter_position_valid||p.x!=emitter_position.x||p.y!=emitter_position.y||p.z!=emitter_position.z){
            float position[]={p.x,p.y,p.z};audio->move(0x51504c555348ULL,position);
            qp_profile_current.emitter_moves++;
            emitter_position=p;emitter_position_valid=1;
        }
    }
}
static void tick(PTR walker){
    if(!test_tools){tick_frame(walker);return;}
    U64 begin=qp_clock();su_zero(&qp_profile_current,sizeof(qp_profile_current));
    tick_frame(walker);
    if(begin&&root&&qp_profile_count<8192){qp_profile_current.total=qp_clock()-begin;qp_profile_current.held=held;qp_profile_current.settled=soft.compression==0&&soft.speed==0;qp_profile_current.compression_ppm=(unsigned)(qp_abs(soft.compression)*1000000);qp_profile_rows[qp_profile_count++]=qp_profile_current;}
}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    if(started)return 1;char folder[1000],path[1100];DWORD size=GetEnvironmentVariableA("MOONLAB_EVIDENCE",folder,sizeof(folder));if(!size||size>=sizeof(folder))return 0;cat(cat(path,folder),"\\squishy-plushie.jsonl");log_file=CreateFileA(path,0x40000000,1,0,1,0x80,0);if(log_file==(PTR)(U64)-1)return 0;
    if(!bind_runtime(domain,game)||!su_bind(game))return 0;
    binding_inventory();
#define M(F,NS,T,N,S) F=su_method(NS,T,N,S);if(!F){char missing[192];cat(cat(cat(missing,"missing-"),T),"-" N);event(missing);return 0;}
    M(stations,"MoonCleaners","MoonWorld","get_Stations","");M(station_id,"MoonCleaners","MoonWorld","StationId","MoonCleaners.MoonCollector");M(restore,"MoonCleaners","MoonWorld","get_IsRestoringRendering","");M(blocked,"MoonCleaners","MoonWalker","get_UIBlocked","");M(firstperson,"MoonCleaners","MoonWalker","get_IsFirstPerson","");M(typing,"MoonCleaners","MoonInput","get_Typing","");M(focus,"UnityEngine","Application","get_isFocused","");M(joined,"MoonCleaners","MoonCrew","get_Joined","");M(local_id,"Steamworks","SteamUser","GetSteamID","");M(host_id,"MoonCleaners","MoonCrew","get_HostId","");
    M(component_transform,"UnityEngine","Component","get_transform","");M(rb_get,"UnityEngine","Rigidbody","get_linearVelocity","");M(rb_set,"UnityEngine","Rigidbody","set_linearVelocity","UnityEngine.Vector3");M(rb_position,"UnityEngine","Rigidbody","get_position","");M(rb_set_position,"UnityEngine","Rigidbody","set_position","UnityEngine.Vector3");M(rb_spin,"UnityEngine","Rigidbody","set_angularVelocity","UnityEngine.Vector3");M(rb_rotation,"UnityEngine","Rigidbody","set_rotation","UnityEngine.Quaternion");M(rb_kinematic,"UnityEngine","Rigidbody","set_isKinematic","System.Boolean");M(rb_gravity,"UnityEngine","Rigidbody","set_useGravity","System.Boolean");M(rb_ccd,"UnityEngine","Rigidbody","set_collisionDetectionMode","UnityEngine.CollisionDetectionMode");M(rb_mass,"UnityEngine","Rigidbody","set_mass","System.Single");M(rb_drag,"UnityEngine","Rigidbody","set_linearDamping","System.Single");M(gameobject_layer,"UnityEngine","GameObject","set_layer","System.Int32");
    M(cap_radius,"UnityEngine","CapsuleCollider","set_radius","System.Single");M(cap_height,"UnityEngine","CapsuleCollider","set_height","System.Single");M(cap_direction,"UnityEngine","CapsuleCollider","set_direction","System.Int32");
    const char *pm=find_class("UnityEngine","PhysicsMaterial")?"PhysicsMaterial":"PhysicMaterial";const char *pt=find_class("UnityEngine","PhysicsMaterial")?"UnityEngine.PhysicsMaterial":"UnityEngine.PhysicMaterial";const char *combine=find_class("UnityEngine","PhysicsMaterial")?"UnityEngine.PhysicsMaterialCombine":"UnityEngine.PhysicMaterialCombine";
    M(mat_ctor,"UnityEngine",pm,".ctor","System.String");M(mat_bounce,"UnityEngine",pm,"set_bounciness","System.Single");M(mat_friction,"UnityEngine",pm,"set_dynamicFriction","System.Single");M(mat_static,"UnityEngine",pm,"set_staticFriction","System.Single");M(mat_combine,"UnityEngine",pm,"set_bounceCombine",combine);M(collider_material,"UnityEngine","Collider","set_sharedMaterial",pt);
    M(texture_ctor,"UnityEngine","Texture2D",".ctor","System.Int32;System.Int32;UnityEngine.TextureFormat;System.Boolean;System.Boolean");M(image_load,"UnityEngine","ImageConversion","LoadImage","UnityEngine.Texture2D;System.Byte[]");M(material_texture,"UnityEngine","Material","SetTexture","System.String;UnityEngine.Texture");M(set_color,"UnityEngine","Material","SetColor","System.String;UnityEngine.Color");
#undef M
    world_field=get_field(find_class("MoonCleaners","MoonWalker"),"world");camera_field=get_field(find_class("MoonCleaners","MoonWalker"),"viewCamera");hit_collider=su.hit_collider;
    PTR user=LoadLibraryA("user32.dll");key_state=(short(*)(int))GetProcAddress(user,"GetAsyncKeyState");foreground=(PTR(*)(void))GetProcAddress(user,"GetForegroundWindow");window_pid=(DWORD(*)(PTR,DWORD *))GetProcAddress(user,"GetWindowThreadProcessId");char test[8];test_tools=GetEnvironmentVariableA("MOONLAB_TEST_TOOLS",test,sizeof(test))==1&&test[0]=='1';
    if(test_tools)qp_profile_bind();
    audio=ma_connect(domain,game);audio_preparation=ma_connect_preparation(domain,game);hands=mh_connect(domain,game);hints=qp_hints_connect(domain,game);if(!world_field||!camera_field||!key_state||!foreground||!window_pid||!audio||!audio_preparation||!hands||!hints||!audio->on_update)return 0;
    if(!qp_interactions_bind()){event("interaction-framework-unavailable");return 0;}if(!audio->on_update(tick))return 0;
    for(unsigned i=0;i<sizeof(sound_handles)/sizeof(*sound_handles);i++)if(qp_sound_bank[i].pcm){sound_handles[i]=audio->register_cue(qp_sound_bank+i);if(!sound_handles[i])return 0;}
    (void)key_state;(void)prepare_hook;(void)install_hooks;started=1;event("ready");return 1;
}
