#ifndef QP_INTERACTIONS_H
#define QP_INTERACTIONS_H
/* Clear per-item policy: pickup/drop, throw and squeeze consume their key.
 * Other props/providers can choose zero flags for nonexclusive delivery. */
#define QP_INTERACTION_FLAGS MOON_INTERACTION_EXCLUSIVE
static int qp_interaction_query(void *context,void *actor,MoonInteractionTarget *target){
    PTR walker=actor;if(failed||!host_world||!root||!body||!su_alive(su_get(root)))return 0;
    PTR current=0;field_value(walker,world_field,&current);int ok=1;DWORD pid=0;window_pid(foreground(),&pid);
    if(pid!=GetCurrentProcessId()||current!=su_get(world)||!su_alive(current)||boolean(restore,current,&ok)||!boolean(focus,0,&ok)||boolean(blocked,walker,&ok)||boolean(typing,0,&ok)||!boolean(firstperson,walker,&ok)||!ok)return 0;
    unsigned action=(unsigned)(U64)context;PTR camera=0;field_value(walker,camera_field,&camera);if(!su_alive(camera))return 0;
    if(held){if(su_get(carry_walker)!=walker)return 0;}
    else if(action||hands->occupied(walker)||!pickup_target(camera))return 0;
    PTR transform=call(component_transform,camera,0,&ok);if(!ok||!su_alive(transform))return 0;SPVec3 eye=su_vector(su.tr_get_position,transform),point=su_vector(rb_position,su_get(body)),delta=su_sub(point,eye);
    *target=(MoonInteractionTarget){root,held?0:su_sqrt(su_dot(delta,delta)),held?MOON_INTERACTION_CARRY_PRIORITY:0,action};return ok;
}
static int qp_interactions_bind(void){
    const char *actions[]={"squishy-plushie.pickup-release","squishy-plushie.throw","squishy-plushie.squeeze"},*bindings[]={"E","LeftMouse","RightMouse"};
    for(unsigned i=0;i<3;i++)if(!(qp_interactions[i]=moon_interaction_register(actions[i],bindings[i],QP_INTERACTION_FLAGS,qp_interaction_query,(void *)(U64)i)))return 0;
    return 1;
}
#endif
