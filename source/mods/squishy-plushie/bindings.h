/* Small current-runtime API inventory; names/signatures only, never guessed RVAs. */
static void binding_inventory(void){
    const char *types[]={"PhysicsMaterial","PhysicMaterial","CapsuleCollider","Collider","Rigidbody"};
    for(unsigned i=0;i<sizeof(types)/sizeof(*types);i++){
        PTR klass=find_class("UnityEngine",types[i]),it=0,m;if(!klass)continue;
        while((m=su.methods(klass,&it))){char b[1400],*p=cat(b,"{\"binding\":\"");p=cat(p,types[i]);p=cat(p,"::");p=cat(p,su.name(m));p=cat(p,"\",\"parameters\":\"");
            DWORD count=su.arity(m);for(DWORD j=0;j<count;j++){if(j)*p++=';';char *type=su.type_name(su.param(m,j));p=cat(p,type);su.free(type);}p=cat(p,"\"}\n");DWORD n=0;WriteFile(log_file,b,(DWORD)(p-b),&n,0);
        }
    }FlushFileBuffers(log_file);
}
