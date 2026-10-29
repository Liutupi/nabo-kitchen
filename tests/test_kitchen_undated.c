/* Offline provisional dates must not manufacture calendar-dependent results. */
#include "kitchen_ui.h"
#include "kitchen_save.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

/* The UI dispatches through these readers; use a single untimed step so the
 * test exercises production completion/recommendation without ESP-IDF/cJSON. */
static int timed_step;
int kc_read_step(const krc_pack* pack, unsigned recipe, unsigned step, kc_step* out,
                 unsigned* count) {
    (void)pack;
    if (recipe >= 2 || step != 0 || !out || !count) return 0;
    memset(out, 0, sizeof(*out));
    if (timed_step) out->timer_seconds = 60;
    *count = 1;
    return 1;
}
int kc_read_ingredients(const krc_pack* pack, unsigned recipe, kc_ingredient* out,
                        unsigned capacity, unsigned* count) {
    (void)pack; (void)recipe; (void)out; (void)capacity;
    if (!count) return 0;
    *count = 0;
    return 1;
}

static kc_item items[2];
static kc_catalog catalog = {items, 2, 2};
static krc_pack pack;

static void setup(void) {
    memset(items, 0, sizeof(items));
    for (unsigned i = 0; i < 2; ++i) {
        items[i].recipe = (kr_recipe){UINT64_C(1) << i, UINT64_C(1) << i, 10, 100, KR_VEG, 0, 0};
        items[i].hash = i + 1;
        items[i].difficulty = 1;
        items[i].servings = 1;
    }
    items[0].season_mask = 1u; /* January */
}

static ku_ui fresh(uint32_t seed, unsigned month, int unknown, int recent) {
    ku_ui u; assert(ku_init(&u,&catalog,&pack,20754,month,seed,NULL,0,NULL,0));
    u.date_unconfirmed=(uint8_t)unknown;
    if(recent){u.store.history_count=1;u.store.history[0]=(ks_record){0,19000};}
    return u;
}
static unsigned select_one(ku_ui* u) {
    ku_key(u,KI_OK,KI_LONG,0);assert((u->page==3 || u->page==4) && u->saved_count==1);return u->saved_result[0];
}
static void timer_home_roundtrip(void) {
    ku_ui u=fresh(42,1,0,0);
    timed_step=1;u.page=6;u.recipe=0;
    ku_key(&u,KI_OK,KI_SHORT,1000);
    assert(u.page==6 && u.timers.count==1 && u.timer_steps[0]==1);
    uint32_t id=u.timers.items[0].id;
    ku_key(&u,KI_UP,KI_LONG,1001);assert(u.outside);
    ku_key(&u,KI_OK,KI_SHORT,1002);assert(!u.outside && u.page==1);
    ku_key(&u,KI_UP,KI_SHORT,1003);assert(u.focus==5);
    ku_key(&u,KI_OK,KI_SHORT,1004);assert(u.page==7 && u.timers.items[0].id==id);
    ku_key(&u,KI_OK,KI_LONG,2000);assert(u.timers.items[0].status==KT_PAUSED);
    uint8_t bytes[KU_SAVE_MAX];size_t n=ku_save_encode(&u,bytes,sizeof bytes,20,2000,-1);assert(n);
    ku_ui restored=fresh(42,1,0,0),scratch={0};
    assert(ku_save_decode(&restored,&scratch,bytes,n,NULL,0,-1,NULL));
    assert(restored.page==1 && restored.timers.count==1 && restored.timers.items[0].status==KT_PAUSED);
    ku_key(&restored,KI_UP,KI_SHORT,1);assert(restored.focus==5);
    ku_key(&restored,KI_OK,KI_SHORT,2);assert(restored.page==7);
    ku_key(&restored,KI_OK,KI_LONG,3);assert(restored.timers.items[0].status==KT_RUNNING);
    ku_key(&restored,KI_DOWN,KI_LONG,4);assert(!restored.timers.count && restored.timer_steps[0]==1);
    assert(!restored.timer_ids[0] && !restored.store.history_count);
    ku_key(&restored,KI_UP,KI_DOUBLE,5);assert(restored.page==1 && restored.focus==5);
    ku_key(&restored,KI_OK,KI_SHORT,6);assert(restored.page==7 && !restored.timers.count);
    ku_key(&restored,KI_UP,KI_DOUBLE,7);assert(restored.page==1);
    /* Three independent timers: canceling the selected final row keeps peers
     * and clamps the selection so the last remaining row stays operable. */
    for(unsigned i=0;i<3;++i){
        assert(kt_add(&restored.timers,60,10,&id));
        restored.timer_ids[i]=id;restored.timer_recipe[i]=0;restored.timer_step[i]=0;
    }
    ku_key(&restored,KI_OK,KI_SHORT,11);assert(restored.page==7);
    ku_key(&restored,KI_DOWN,KI_SHORT,12);ku_key(&restored,KI_DOWN,KI_SHORT,13);
    assert(restored.focus==2);uint32_t first=restored.timers.items[0].id;
    ku_key(&restored,KI_DOWN,KI_LONG,14);
    assert(restored.timers.count==2 && restored.focus==1 && restored.timers.items[0].id==first);
    timed_step=0;
}
int main(void) {
    setup();timer_home_roundtrip();unsigned recent_changes=0;
    for(unsigned seed=0;seed<256;++seed){
        ku_ui a=fresh(seed,1,0,0),b=fresh(seed,12,1,0),c=fresh(seed,12,1,1);
        unsigned chosen=select_one(&a);assert(chosen==select_one(&b));recent_changes+=chosen!=select_one(&c);
    }
    assert(recent_changes);
    ku_ui u=fresh(12,10,1,0);select_one(&u);unsigned saved=u.saved_result[0];
    ku_set_day(&u,25000,12,99);u.page=1;select_one(&u);assert(u.saved_result[0]==saved && u.draw_count==1);
    for(unsigned i=0;i<300;++i){u.page=4;ku_key(&u,KI_DOWN,KI_SHORT,i*2000);assert(u.page==3);}
    assert(u.draw_count==301 && !u.store.history_count);
    ku_ui restored={0},scratch={0};restored.catalog=&catalog;uint8_t bytes[KU_SAVE_MAX];
    size_t n=ku_save_encode(&u,bytes,sizeof bytes,9,0,-1);assert(n);uint16_t version;
    assert(ku_save_header(bytes,n,&version,NULL,NULL)&&version==2);
    assert(ku_save_decode(&restored,&scratch,bytes,n,NULL,0,-1,NULL));assert(restored.draw_count==301 && !restored.date_unconfirmed);
    for(unsigned j=0;j<3;++j){u.page=6;u.recipe=(uint16_t)(j%2);ks_reset_session(&u.sessions[u.recipe]);ku_key(&u,KI_OK,KI_SHORT,0);assert(u.page==8 && !u.has_saved);}
    ks_stats stats=ks_get_stats(&u.store,10000);assert(stats.total==3 && stats.unique==2 && !stats.streak && !stats.today);
    assert(u.store.history[0].day==0 && ks_recent_age(&u.store,0)==0 && ks_recent_age(&u.store,1)==1);
    ks_store levels={0};levels.cooked_count=1;levels.cooked[0].count=60;assert(ks_get_stats(&levels,0).level==3);
    levels.cooked_count=100;levels.cooked[0].count=199;assert(ks_get_stats(&levels,0).level==3);
    levels.cooked[0].count=200;assert(ks_get_stats(&levels,0).level==4);
    puts("PASS: date-independent weighted draws, 301 persisted draws, completed-cooking recency and count-only levels");
}
