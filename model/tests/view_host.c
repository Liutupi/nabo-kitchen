#include "kitchen_view.h"
#include "kitchen_metadata.h"
#include <string.h>
static krc_pack recipes;static kc_item items[KR_MAX_RECIPES];static kc_catalog catalog={items,KR_MAX_RECIPES,0};
static kfn_pack fonts;static ksp_pack dishes,icons,ui;static kan_pack animation;static kan_action idle;
static kv_assets assets;static ku_ui state;static kv_scene scene;static uint8_t frame[192*192];
int view_open(const uint8_t *r,size_t rn,const uint8_t *f,size_t fn,const uint8_t *d,size_t dn,
              const uint8_t *i,size_t in,const uint8_t *u,size_t un,const uint8_t *a,size_t an){
    if(!krc_open(&recipes,r,rn)||!kc_load(&catalog,&recipes)||!kfn_open(&fonts,f,fn)||
       !ksp_open(&dishes,d,dn)||!ksp_open(&icons,i,in)||!ksp_open(&ui,u,un)||!kan_open(&animation,a,an)||
       !kan_find(&animation,"idle",&idle)||animation.width!=192||animation.height!=192)return 0;
    kan_player player;if(!kan_start(&player,&animation,&idle,frame,sizeof(frame))||kan_next(&player)!=1)return 0;
    return kv_assets_init(&assets,&catalog,&recipes,&fonts,&dishes,&icons,&ui,&animation);
}
int view_page(unsigned page,unsigned recipe){
    if(page>13||recipe>=catalog.count)return 0;
    if(!ku_init(&state,&catalog,&recipes,20754,10,42,km_ingredient_order,km_ingredient_order_count,km_fridge_order,km_fridge_order_count))return 0;
    state.page=(uint8_t)page;state.recipe=(uint16_t)recipe;
    if(page==4){state.result[0]=(uint16_t)recipe;state.result_count=1;}
    if(page==7){uint32_t id;if(!kt_add(&state.timers,90,1000,&id))return 0;}
    if(page==10)state.fridge=UINT64_MAX;
    if(page==11&&!ku_add_shopping(&state,recipe))return 0;
    if(page==12){state.store.cooked[0]=(ks_cooked){(uint16_t)recipe,1,20754};state.store.cooked_count=1;}
    return kv_prepare(&scene,&state,&assets,1000,80,"20:30",frame);
}
int view_draw(uint16_t *pixels,int y,int height){kg_canvas c;kg_init(&c,pixels,240,y,height,0x18c6);return kv_draw(&c,&scene);}
int view_step(unsigned step){state.page=6;state.sessions[state.recipe].index=(uint8_t)step;
    return kv_prepare(&scene,&state,&assets,1000,80,"20:30",frame);}
