/* Native DRM linear dumb-buffer probe. Atomic plane-only commit, no modeset. */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <drm_mode.h>
static volatile sig_atomic_t done, move_block;
static void sig(int n){if(n==SIGUSR1)move_block=1;else done=1;}
static void fail(const char *s){perror(s);exit(1);}
static const char *names[]={"FB_ID","CRTC_ID","SRC_X","SRC_Y","SRC_W","SRC_H","CRTC_X","CRTC_Y","CRTC_W","CRTC_H"};
static uint32_t ids[10];static uint64_t old[10];
static uint32_t *pixels;static uint32_t pitch,w,h;
static void rect(unsigned x,unsigned y,unsigned rw,unsigned rh){
 for(unsigned yy=y;yy<y+rh && yy<h;yy++)for(unsigned xx=x;xx<x+rw && xx<w;xx++)pixels[yy*(pitch/4)+xx]=0xffffffff;
}
static void draw(unsigned x){
 for(unsigned y=0;y<h;y++)for(unsigned xx=0;xx<w;xx++)pixels[y*(pitch/4)+xx]=0xff000000;
 rect(0,0,w,8);rect(0,h-8,w,8);rect(0,0,8,h);rect(w-8,0,8,h);
 rect(w/2-3,0,6,h);rect(0,h/2-3,w,6);rect(x,160,100,100);
 __sync_synchronize();
}
static int apply(int fd,uint32_t plane,const uint64_t *v){
 drmModeAtomicReq *r=drmModeAtomicAlloc();if(!r)return -1;
 for(int i=0;i<10;i++)if(drmModeAtomicAddProperty(r,plane,ids[i],v[i])<0){drmModeAtomicFree(r);return -1;}
 int ret=drmModeAtomicCommit(fd,r,0,NULL); /* ALLOW_MODESET intentionally absent */
 drmModeAtomicFree(r);return ret;
}
int main(void){
 int fd=-1;uint32_t crtc=0,connector=0,plane=0,fb=0;char path[64];
 setvbuf(stdout,NULL,_IOLBF,0);
 for(int n=0;n<4 && fd<0;n++){
  snprintf(path,sizeof(path),"/dev/dri/card%d",n);int f=open(path,O_RDWR|O_CLOEXEC);if(f<0)continue;
  drmModeRes *r=drmModeGetResources(f);if(!r){close(f);continue;}
  for(int j=0;j<r->count_connectors;j++){
   drmModeConnector *c=drmModeGetConnector(f,r->connectors[j]);
   if(c && c->connector_type==DRM_MODE_CONNECTOR_DSI && c->connection==DRM_MODE_CONNECTED && c->encoder_id){
    drmModeEncoder *e=drmModeGetEncoder(f,c->encoder_id);
    if(e && e->crtc_id){fd=f;crtc=e->crtc_id;connector=c->connector_id;}
    if(e)drmModeFreeEncoder(e);
   }
   if(c)drmModeFreeConnector(c);
   if(fd>=0)break;
  }
  drmModeFreeResources(r);if(fd<0)close(f);
 }
 if(fd<0){fprintf(stderr,"No active DSI connector\n");return 1;}
 if(drmSetMaster(fd))fail("drmSetMaster");
 if(drmSetClientCap(fd,DRM_CLIENT_CAP_UNIVERSAL_PLANES,1) || drmSetClientCap(fd,DRM_CLIENT_CAP_ATOMIC,1))fail("DRM atomic capability");
 drmModeCrtc *c=drmModeGetCrtc(fd,crtc);if(!c || !c->mode_valid){fprintf(stderr,"No active native mode\n");return 1;}
 w=c->mode.hdisplay;h=c->mode.vdisplay;
 if(w!=800 || h!=1280){fprintf(stderr,"Unexpected mode %ux%u\n",w,h);return 1;}
 drmModePlaneRes *pr=drmModeGetPlaneResources(fd);if(!pr)fail("get planes");
 for(unsigned j=0;j<pr->count_planes;j++){
  drmModePlane *p=drmModeGetPlane(fd,pr->planes[j]);if(!p)continue;
  drmModeObjectProperties *props=drmModeObjectGetProperties(fd,p->plane_id,DRM_MODE_OBJECT_PLANE);int primary=0;
  if(props)for(unsigned k=0;k<props->count_props;k++){
   drmModePropertyRes *v=drmModeGetProperty(fd,props->props[k]);
   if(v && !strcmp(v->name,"type") && props->prop_values[k]==DRM_PLANE_TYPE_PRIMARY)primary=1;
   if(v)drmModeFreeProperty(v);
  }
  if(primary && p->crtc_id==crtc){
   int supported=0;for(unsigned k=0;k<p->count_formats;k++)if(p->formats[k]==DRM_FORMAT_XRGB8888)supported=1;
   if(supported){
    plane=p->plane_id;
    for(unsigned k=0;k<props->count_props;k++){
     drmModePropertyRes *v=drmModeGetProperty(fd,props->props[k]);
     if(v){for(int a=0;a<10;a++)if(!strcmp(v->name,names[a])){ids[a]=v->prop_id;old[a]=props->prop_values[k];}drmModeFreeProperty(v);}
    }
   }
  }
  if(props)drmModeFreeObjectProperties(props);
  drmModeFreePlane(p);
  if(plane)break;
 }
 drmModeFreePlaneResources(pr);
 for(int i=0;i<10;i++)if(!ids[i]){fprintf(stderr,"Missing plane property %s\n",names[i]);return 1;}
 struct drm_mode_create_dumb d={.width=w,.height=h,.bpp=32};
 if(drmIoctl(fd,DRM_IOCTL_MODE_CREATE_DUMB,&d))fail("create dumb");
 pitch=d.pitch;
 uint32_t handles[4]={d.handle,0},pitches[4]={d.pitch,0},offsets[4]={0};
 if(drmModeAddFB2(fd,w,h,DRM_FORMAT_XRGB8888,handles,pitches,offsets,&fb,0))fail("addfb2 linear XRGB8888");
 struct drm_mode_map_dumb m={.handle=d.handle};if(drmIoctl(fd,DRM_IOCTL_MODE_MAP_DUMB,&m))fail("map dumb");
 pixels=mmap(NULL,d.size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,m.offset);if(pixels==MAP_FAILED)fail("mmap");
 draw(80);
 uint64_t val[]={fb,crtc,0,0,(uint64_t)w<<16,(uint64_t)h<<16,0,0,w,h};
 printf("card=%s connector=%u crtc=%u plane=%u mode=%ux%u clock_khz=%u\n",path,connector,crtc,plane,w,h,c->mode.clock);
 printf("linear dumb: handle=%u framebuffer=%u pitch=%u size=%llu format=XRGB8888 modifier=LINEAR\n",d.handle,fb,pitch,(unsigned long long)d.size);
 for(int i=0;i<10;i++)printf("%s: %llu -> %llu\n",names[i],(unsigned long long)old[i],(unsigned long long)val[i]);
 fflush(stdout);fsync(STDOUT_FILENO);
 signal(SIGINT,sig);signal(SIGTERM,sig);signal(SIGUSR1,sig);
 if(apply(fd,plane,val))fail("atomic plane commit without modeset");
 printf("READY: plane-only commit; geometry border/cross/block at x80 y160. SIGUSR1 moves to x520.\n");fsync(STDOUT_FILENO);
 while(!done){pause();if(move_block){draw(520);move_block=0;printf("UPDATED: block x520 y160\n");fsync(STDOUT_FILENO);}}
 if(apply(fd,plane,old))perror("restore original plane");
 munmap(pixels,d.size);drmModeRmFB(fd,fb);
 struct drm_mode_destroy_dumb destroy={.handle=d.handle};drmIoctl(fd,DRM_IOCTL_MODE_DESTROY_DUMB,&destroy);
 drmModeFreeCrtc(c);drmDropMaster(fd);close(fd);return 0;
}
