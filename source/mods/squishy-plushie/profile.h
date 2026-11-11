/* Test-only bounded attribution. Buffer clocks in memory: disk flushes must
 * not become part of the collision/deformation path being measured. */
typedef struct {U64 total,deform,sound;unsigned held,impact,uploads,readback,compression_ppm,settled;} QPProfileRow;
static QPProfileRow qp_profile_rows[8192],qp_profile_current;
static unsigned qp_profile_count;
static U64 qp_profile_frequency;
static int (*qp_counter)(U64 *);
static U64 qp_clock(void){U64 now=0;if(test_tools&&qp_counter)qp_counter(&now);return now;}
static void qp_profile_bind(void){
    PTR kernel=LoadLibraryA("kernel32.dll");
    qp_counter=(int(*)(U64 *))GetProcAddress(kernel,"QueryPerformanceCounter");
    int (*frequency)(U64 *)=(int(*)(U64 *))GetProcAddress(kernel,"QueryPerformanceFrequency");
    if(!qp_counter||!frequency||!frequency(&qp_profile_frequency)||!qp_profile_frequency)qp_counter=0;
}
static void qp_profile_dump(void){
    if(!qp_counter||!qp_profile_count)return;
    for(unsigned i=0;i<qp_profile_count;i++){
        QPProfileRow *r=&qp_profile_rows[i];char b[512],*p=cat(b,"{\"event\":\"plush-profile\",\"total_us\":");
        p=num(p,r->total*1000000/qp_profile_frequency);p=cat(p,",\"deform_us\":");p=num(p,r->deform*1000000/qp_profile_frequency);
        p=cat(p,",\"sound_us\":");p=num(p,r->sound*1000000/qp_profile_frequency);p=cat(p,",\"held\":");p=num(p,r->held);
        p=cat(p,",\"impact\":");p=num(p,r->impact);p=cat(p,",\"uploads\":");p=num(p,r->uploads);
        p=cat(p,",\"readback\":");p=num(p,r->readback);p=cat(p,",\"compression_ppm\":");p=num(p,r->compression_ppm);
        p=cat(p,",\"settled\":");p=num(p,r->settled);p=cat(p,"}\n");DWORD n=0;WriteFile(log_file,b,(DWORD)(p-b),&n,0);
    }
    FlushFileBuffers(log_file);qp_profile_count=0;
}
