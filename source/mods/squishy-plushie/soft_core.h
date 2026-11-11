#ifndef SQUISHY_PLUSHIE_SOFT_CORE_H
#define SQUISHY_PLUSHIE_SOFT_CORE_H
/* Volume-preserving directional squish. Physics stays with Unity's collider;
 * these values deform owned vertex/normal arrays rather than object scale. */
typedef struct {float x,y,z;} QPVector;
typedef struct {float compression,speed,cooldown;QPVector axis;unsigned impacts;} QPSoft;
static inline float qp_abs(float x){return x<0?-x:x;}
static inline float qp_sqrt(float x){if(x<=0)return 0;float n=x>1?x:1;for(int i=0;i<16;i++)n=.5f*(n+x/n);return n;}
static inline float qp_bound(float v,float lo,float hi){return v<lo?lo:v>hi?hi:v;}
static inline float qp_dot(QPVector a,QPVector b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static inline QPVector qp_mul(QPVector a,float s){return (QPVector){a.x*s,a.y*s,a.z*s};}
static inline QPVector qp_add(QPVector a,QPVector b){return (QPVector){a.x+b.x,a.y+b.y,a.z+b.z};}
static inline int qp_valid(float x){return x==x&&x>-1e12f&&x<1e12f;}
static inline int qp_hit(QPSoft *s,QPVector axis,float impulse){
    float length=qp_sqrt(qp_dot(axis,axis));
    if(!s||!qp_valid(impulse)||!qp_valid(length)||length<.001f||impulse<.25f||s->cooldown>0)return 0;
    s->axis=qp_mul(axis,1/length);s->speed+=qp_bound(impulse*.65f,.4f,3.6f);s->cooldown=.12f;s->impacts++;return 1;
}
static inline int qp_impact(QPSoft *s,QPVector axis,float impulse){
    if(!qp_hit(s,axis,impulse))return 0;
    /* Collision compression begins on the observed contact frame. Waiting for
     * spring velocity alone hid the squash during fast thrown contacts. */
    float contact=qp_bound(impulse*.035f,.025f,.28f);
    if(s->compression<contact)s->compression=contact;
    return 1;
}
static inline void qp_tick(QPSoft *s,float dt){
    if(!s||!qp_valid(dt)||dt<=0||dt>.25f)return;
    while(dt>0){float h=dt>1.f/240?1.f/240:dt;dt-=h;
        s->speed+=(-155*s->compression-12*s->speed)*h;s->compression+=s->speed*h;
        if(s->compression>.30f){s->compression=.30f;if(s->speed>0)s->speed=0;}
        if(s->compression<-.12f){s->compression=-.12f;if(s->speed<0)s->speed=0;}
        s->cooldown-=h;if(s->cooldown<0)s->cooldown=0;
    }
    if(qp_abs(s->compression)<.00005f&&qp_abs(s->speed)<.0005f)s->compression=s->speed=0;
}
static inline QPVector qp_deform(const QPSoft *s,QPVector p,int normal){
    float along=1-s->compression,across=1/qp_sqrt(along),d=qp_dot(p,s->axis);
    if(normal){along=1/along;across=1/across;}
    QPVector out=qp_add(qp_mul(p,across),qp_mul(s->axis,d*(along-across)));
    /* Ground-side compression retains its approximate contact rather than
     * floating the entire visual above an unchanged physical collider. */
    if(!normal)out=qp_add(out,qp_mul(s->axis,-.26f*s->compression));
    else {float n=qp_sqrt(qp_dot(out,out));if(n>.00001f)out=qp_mul(out,1/n);}
    return out;
}
#endif
