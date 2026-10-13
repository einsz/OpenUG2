/* All approved Xbox/XDVDFS vectors, plus independent synthetic pack/path tests. */
static uint8_t *x6_pack(int count, size_t *length, size_t *record_at, size_t *desc_at) {
    size_t key_at=156, rh=key_at+(size_t)count*8, dh=rh+8+(size_t)count*124;
    size_t pixel_group=dh+8+(size_t)count*60, pixel_header=pixel_group+8;
    size_t base=(pixel_header+8+127)&~(size_t)127;
    size_t n=base+(size_t)count*1024;
    uint8_t *b=calloc(1,n); if(!b)return NULL;
    a4_chunk(b,0xB3300000u,(uint32_t)n-8);
    a4_chunk(b+8,0xB3310000u,(uint32_t)pixel_group-16);
    a4_chunk(b+16,0x33310001u,124);a4_u32(b+24,5);
    a4_chunk(b+148,0x33310002u,(uint32_t)count*8);
    a4_chunk(b+rh,0x33310004u,(uint32_t)count*124);
    a4_chunk(b+dh,0x33310005u,(uint32_t)count*60);
    a4_chunk(b+pixel_group,0xB3320000u,(uint32_t)n-(uint32_t)pixel_group-8);
    a4_chunk(b+pixel_header,0x33320002u,(uint32_t)n-(uint32_t)pixel_header-8);
    for(int i=0;i<count;i++){
        uint32_t key=i<23?ug_controller_family_icon_key(UG_CONTROLLER_XBOX,i):0x10000000u+(uint32_t)i;
        uint8_t *r=b+rh+8+(size_t)i*124;
        a4_u32(b+key_at+8*i,key);a4_u32(r+36,key);a4_u32(r+48,(uint32_t)i*1024);
        a4_u32(r+56,key==0x68fdf2fdu?512:1024);a4_u32(r+64,key==0x68fdf2fdu?512:1024);
        r[68]=r[70]=32;r[74]=key==0x68fdf2fdu?0x22:0x24;r[78]=1;
        for(int block=0;block<64;block++){
            uint8_t *p=b+base+(size_t)i*1024+(size_t)block*(r[74]==0x22?8:16);
            if(r[74]==0x24){memset(p,255,8);p[9]=0xf8;}else{p[1]=0xf8;}
        }
    }
    *length=n;*record_at=rh+8;*desc_at=dh;return b;
}
static void test_xbox_vectors(void) {
    char *txt=read_file("xbox_controller_vectors.json",NULL);CHECK(txt!=NULL);if(!txt)return;
    jp=txt;J *root=jparse();const J *cases=jget(root,"cases");CHECK(cases&&cases->n==7);
    uint8_t out[4096];
    for(int i=0;i<cases->n;i++){
        const J *v=cases->kids[i];const char *name=jstrv(v,"name");
        if(jget(v,"encoded_hex")){
            size_t n;uint8_t *b=unhex(jstrv(v,"encoded_hex"),&n);
            CHECK(ug_xbox_icon_decode((int)jnum(v,"format"),(int)jnum(v,"width"),(int)jnum(v,"height"),b,n,out));
            const J *rows=jget(v,"expected_rgba_rows"),*pixel=jget(v,"expected_rgba_pixel");
            if(rows)for(int y=0;y<rows->n;y++)for(int x=0;x<rows->kids[y]->n;x++)for(int c=0;c<4;c++)CHECK(out[4*(y*4+x)+c]==rows->kids[y]->kids[x]->kids[c]->num);
            if(pixel)for(int p=0;p<(int)jnum(v,"expected_pixel_count");p++)for(int c=0;c<4;c++)CHECK(out[4*p+c]==pixel->kids[c]->num);
            free(b);
        }else if(jget(v,"available_bytes")){
            size_t n=(size_t)jnum(v,"available_bytes");uint8_t *b=calloc(1,n);
            memset(out,0x7b,sizeof out);CHECK(!ug_xbox_icon_decode((int)jnum(v,"format"),32,32,b,n,out));CHECK(out[0]==0x7b);free(b);
        }else if(!strcmp(name,"descriptor_shape")||!strcmp(name,"descriptor_invalid_extent")){
            size_t n,rec,desc;uint8_t *b=x6_pack((int)jnum(v,"record_count"),&n,&rec,&desc);
            CHECK(b!=NULL);if(!b)continue;
            CHECK((desc-rec)==6076);CHECK(jnum(cases->kids[4],"expected_record_stride")==124);CHECK(jnum(cases->kids[4],"expected_descriptor_stride")==60);
            if(jget(v,"descriptor_bytes"))a4_u32(b+desc+4,(uint32_t)jnum(v,"descriptor_bytes"));
            UgController p;UgTexLib lib;ug_controller_init(&p);ug_texlib_init(&lib);
            CHECK(ug_controller_load_xbox_pack(&p,&lib,b,n)==!strcmp(name,"descriptor_shape"));
            if(!strcmp(name,"descriptor_shape")){CHECK(lib.n==23);CHECK(ug_controller_family_available(&p,UG_CONTROLLER_XBOX));}
            else CHECK(lib.n==0);
            ug_texlib_free(&lib);free(b);
        }
        /* outside_member is exercised through an ISO whose member is 1000
         * bytes: pack header at 900 declares 200 bytes; no pack read succeeds. */
    }
    CHECK(!ug_xbox_icon_decode(0x26,32,32,out,sizeof out,out));
    jfree(root);free(txt);
}
static unsigned x6_entry(uint8_t *b,const char *name,uint32_t sector,uint32_t n,int dir){
    unsigned bytes=14+(unsigned)strlen(name);memset(b,0,bytes);a4_u32(b+4,sector);a4_u32(b+8,n);
    b[12]=dir?0x10:0x20;b[13]=(uint8_t)strlen(name);memcpy(b+14,name,strlen(name));return bytes;
}
static void test_xdvdfs_vectors(void){
    char *txt=read_file("xdvdfs_vectors.json",NULL);CHECK(txt!=NULL);if(!txt)return;
    jp=txt;J *root=jparse();size_t n;uint8_t *entry=unhex(jstrv(root,"root_entry_hex"),&n);
    size_t len=42*2048;uint8_t *iso=calloc(1,len);CHECK(iso!=NULL);if(!iso){free(entry);jfree(root);free(txt);return;}
    uint8_t *vd=iso+32*2048;a4_u32(vd+20,(uint32_t)jnum(root,"root_sector"));a4_u32(vd+24,(uint32_t)jnum(root,"root_byte_size"));
    memcpy(iso+33*2048,entry,n);uint64_t off=0;uint32_t size=0;
    CHECK(ug_xdvdfs_find(iso,len,jstrv(root,"expected_path"),&off,&size));
    CHECK(off==(uint64_t)jnum(root,"expected_file_offset"));CHECK(size==(uint32_t)jnum(root,"expected_file_size"));
    const J *bad=jget(root,"malformed_cases");
    for(int i=0;i<bad->n;i++){
        const J *v=bad->kids[i];memcpy(iso+33*2048,entry,n);
        iso[33*2048]=(uint8_t)jnum(v,"left_pointer_dwords");
        if(jget(v,"declared_name_bytes"))iso[33*2048+13]=(uint8_t)jnum(v,"declared_name_bytes");
        CHECK(!ug_xdvdfs_find(iso,len,"/TEST",&off,&size));
    }
    memcpy(iso+33*2048,entry,n);iso[33*2048+2]=5;
    a4_u32(vd+24,40);x6_entry(iso+33*2048+20,"OTHER",40,16,0);
    iso[33*2048+20]=5; /* well-formed child entry's left returns to itself */
    CHECK(!ug_xdvdfs_find(iso,len,"/TEST",&off,&size));
    memcpy(iso+33*2048,entry,n);a4_u32(vd+24,20);
    CHECK(!ug_xdvdfs_find(iso,32*2048+24,"/TEST",&off,&size));
    a4_u32(iso+33*2048+4,100);CHECK(!ug_xdvdfs_find(iso,len,"/TEST",&off,&size));
    free(iso);free(entry);jfree(root);free(txt);
}
static void test_xbox_pack_and_iso(void){
    size_t n,rec,desc;uint8_t *b=x6_pack(1,&n,&rec,&desc);CHECK(b!=NULL);if(!b)return;
    UgController p;UgTexLib lib;ug_controller_init(&p);ug_texlib_init(&lib);
    CHECK(ug_controller_family_icon_count(UG_CONTROLLER_PS2)==19);CHECK(ug_controller_family_icon_count(UG_CONTROLLER_XBOX)==23);CHECK(!ug_controller_family_icon_count(UG_CONTROLLER_PC));
    CHECK(!strcmp(ug_controller_family_icon_name(UG_CONTROLLER_XBOX,10),"A"));CHECK(ug_controller_family_icon_key(UG_CONTROLLER_XBOX,22)==0xf6baf515u);
    CHECK(ug_controller_load_xbox_pack(&p,&lib,b,n));CHECK(lib.n==1);CHECK(ug_controller_select(&p,UG_CONTROLLER_XBOX)==UG_VIS_OK);
    CHECK(ug_controller_family_icon_texture(&p,UG_CONTROLLER_XBOX,0)==1);CHECK(!ug_controller_family_icon_texture(&p,UG_CONTROLLER_XBOX,1));CHECK(!ug_controller_family_icon_texture(&p,UG_CONTROLLER_PS2,0));
    CHECK(ug_controller_token(&p,"ACCEPT",NULL)==UG_VIS_UNBOUND);CHECK(ug_controller_bind(&p,"ACCEPT",ug_controller_family_icon_key(UG_CONTROLLER_XBOX,0))==UG_VIS_OK);
    int tex=0;CHECK(ug_controller_token(&p,"ACCEPT",&tex)==UG_VIS_OK&&tex==1);
    CHECK(ug_controller_select(&p,UG_CONTROLLER_PS2)==UG_VIS_UNAVAILABLE);CHECK(ug_controller_token(&p,"ACCEPT",&tex)!=UG_VIS_OK&&tex==0);ug_texlib_free(&lib);
    /* Invalid known record is unavailable, never a replacement. */
    ug_controller_init(&p);ug_texlib_init(&lib);b[rec+74]=0x26;CHECK(!ug_controller_load_xbox_pack(&p,&lib,b,n));CHECK(!lib.n);b[rec+74]=0x24;
    a4_u32(b+rec+48,(uint32_t)n);CHECK(!ug_controller_load_xbox_pack(&p,&lib,b,n));a4_u32(b+rec+48,0);
    b[rec+68]=16;CHECK(!ug_controller_load_xbox_pack(&p,&lib,b,n));b[rec+68]=32;
    size_t ilen=42*2048+n;uint8_t *iso=calloc(1,ilen);CHECK(iso!=NULL);if(!iso){free(b);return;}
    a4_u32(iso+32*2048+20,33);a4_u32(iso+32*2048+24,24);x6_entry(iso+33*2048,"NFSUNDER",34,28,1);
    x6_entry(iso+34*2048,"ZZDATA5.BIN",40,(uint32_t)n,0);memcpy(iso+40*2048,b,n);
    char path[]="/tmp/ugui_xbox_test_XXXXXX";int fd=mkstemp(path);CHECK(fd>=0);
    if(fd>=0){FILE *f=fdopen(fd,"wb");CHECK(fwrite(iso,1,ilen,f)==ilen);fclose(f);
        CHECK(ug_controller_load_xbox(&p,&lib,path));CHECK(lib.n==1);CHECK(strstr(p.xbox_source,path)!=NULL);ug_texlib_free(&lib);
        /* Approved outside_member vector; follows the exact 1000/900/200 bounds. */
        a4_u32(iso+34*2048+8,1000);memset(iso+40*2048,0,n);a4_chunk(iso+40*2048+900,0xb3300000u,192);
        f=fopen(path,"wb");CHECK(fwrite(iso,1,ilen,f)==ilen);fclose(f);ug_controller_init(&p);ug_texlib_init(&lib);
        CHECK(!ug_controller_load_xbox(&p,&lib,path));CHECK(!lib.n&&!p.xbox_available);remove(path);
    }
    ug_texlib_free(&lib);free(iso);free(b);
}
static void run_amend6_xbox_tests(void){RUN(test_xbox_vectors);RUN(test_xdvdfs_vectors);RUN(test_xbox_pack_and_iso);}
