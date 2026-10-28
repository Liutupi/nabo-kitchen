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
int view_boot_at(unsigned elapsed){
    ku_startup(&state,1000);
    return kv_prepare(&scene,&state,&assets,1000+(int64_t)elapsed,80,"20:30",frame);
}
int view_boot_page(unsigned elapsed){
    ku_startup(&state,1000);
    ku_tick(&state,1000+(int64_t)elapsed);
    return state.page;
}
int view_boot_skip(void){
    ku_startup(&state,1000);
    ku_key(&state,KI_OK,KI_SHORT,1100);
    return state.page;
}
/* Additional visual-state probes; no user data or device interaction. */
int view_combo(unsigned selected) {
    if (selected >= 4 || catalog.count < 4) return 0;
    state.page = 4; state.result_count = 4; state.result_focus = (uint8_t)selected;
    for (unsigned i = 0; i < 4; ++i) state.result[i] = (uint16_t)i;
    state.recipe = state.result[selected];
    return kv_prepare(&scene, &state, &assets, 1000, 80, "20:30", frame);
}
int view_paper_scroll(unsigned offset) {
    if (state.page != 5 || offset > UINT16_MAX) return 0;
    state.paper_scroll = (uint16_t)offset;
    return kv_prepare(&scene, &state, &assets, 1000, 80, "20:30", frame);
}
unsigned view_paper_max(void) { return state.paper_scroll_max; }
int view_step_timer(unsigned kind) {
    if (state.page != 6 || kind > 2) return 0;
    kt_init(&state.timers);memset(state.timer_ids, 0, sizeof(state.timer_ids));
    uint32_t id;if (!kt_add(&state.timers, 90, 1000, &id)) return 0;
    state.timer_ids[0] = id;state.timer_recipe[0] = state.recipe;
    state.timer_step[0] = state.sessions[state.recipe].index;
    if (kind == 1) kt_toggle(&state.timers, id, 21000);
    if (kind == 2 && !kt_tick(&state.timers, 91000)) return 0;
    return kv_prepare(&scene, &state, &assets, kind == 2 ? 91000 : 21000, 80, "20:30", frame);
}
int view_complete_progress(void) {
    if (state.page != 8) return 0;
    state.store.cooked[0] = (ks_cooked){state.recipe, 1, state.day};
    state.store.cooked_count = 1;
    state.store.history[0] = (ks_record){state.recipe, state.day};
    state.store.history_count = 1;
    return kv_prepare(&scene, &state, &assets, 1000, 80, "20:30", frame);
}

int view_scene_time(unsigned elapsed) {
    return kv_prepare(&scene, &state, &assets, 1000 + (int64_t)elapsed, 80, "20:30", frame);
}
