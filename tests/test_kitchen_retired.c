/* D2 drops only known retired hashes; corruption still rejects atomically. */
#include "kitchen_save.h"
#include "kitchen_retired.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    kc_item old_items[2]={{0}}, new_item={0};
    old_items[0].hash=123; old_items[1].hash=0x9759ba75u; /* baicai-jitui-tang */
    assert(kitchen_recipe_retired(old_items[1].hash));
    new_item.hash=123;
    kc_catalog old={old_items,2,2}, now={&new_item,1,1};
    ku_ui source={0},dest={0},scratch={0};
    source.catalog=&old;dest.catalog=&now;
    source.conditions=(ku_conditions){2,30,0};source.saved_conditions=source.conditions;
    source.month=10;source.difficulty=2;source.reroll_limit=3;source.sound=1;source.volume=70;
    source.has_saved=1;source.saved_count=2;source.saved_result[0]=0;source.saved_result[1]=1;
    source.favorites[0]=source.wanted[0]=3;
    source.store.history_count=2;source.store.history[0]=(ks_record){1,20754};source.store.history[1]=(ks_record){0,20753};
    source.store.cooked_count=2;source.store.cooked[0]=(ks_cooked){1,4,20754};source.store.cooked[1]=(ks_cooked){0,3,20753};
    source.sessions[0]=(ks_session){1,0,0};source.sessions[1]=(ks_session){3,1,0};
    source.timer_steps[0]=1;source.timer_steps[1]=2;
    kt_init(&source.timers);
    for(unsigned i=0;i<2;++i) {
        uint32_t id;assert(kt_add(&source.timers,90000,1000,&id));
        source.timer_ids[i]=id;source.timer_recipe[i]=(uint16_t)(1-i);source.timer_step[i]=0;
    }
    uint8_t bytes[KU_SAVE_MAX];size_t n=ku_save_encode(&source,bytes,sizeof bytes,55,1000,-1);assert(n);
    uint32_t seq=0;int lost=0;
    assert(ku_save_decode(&dest,&scratch,bytes,n,&seq,2000,-1,&lost));
    assert(seq==55 && dest.volume==70 && dest.sound && dest.dirty);
    assert(!dest.has_saved && !dest.saved_count);
    assert(dest.store.history_count==1 && dest.store.history[0].recipe==0);
    assert(dest.store.cooked_count==1 && dest.store.cooked[0].count==3);
    assert(dest.favorites[0]==1 && dest.wanted[0]==1 && dest.sessions[0].visited==1);
    assert(dest.timers.count==1 && dest.timer_ids[0]==2 && dest.timer_recipe[0]==0);
    assert(lost && dest.timers.items[0].status==KT_PAUSED);
    n=ku_save_encode(&dest,bytes,sizeof bytes,56,2000,-1);assert(n);
    assert(ku_save_decode(&dest,&scratch,bytes,n,&seq,3000,-1,&lost));
    assert(seq==56 && !dest.dirty && dest.timers.count==1);
    old_items[1].hash=0xdeadbeefu;
    n=ku_save_encode(&source,bytes,sizeof bytes,57,1000,-1);assert(n);
    ku_ui before=dest;
    assert(!ku_save_decode(&dest,&scratch,bytes,n,&seq,4000,-1,&lost));
    assert(!memcmp(&before,&dest,sizeof dest));
    puts("PASS: retired recipes removed across menu, history, cooked, sessions, favorites and timers; unknown IDs reject atomically");
}
