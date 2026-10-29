/* Verify old 30-recipe KST1 state survives the real expanded KRC index. */
#include "kitchen_ui.h"
#include "kitchen_save.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t u32(const unsigned char* p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static unsigned load(const char* path, kc_item* items, unsigned capacity) {
    FILE* f=fopen(path,"rb");assert(f);unsigned char h[48];assert(fread(h,1,48,f)==48);
    assert(memcmp(h,"KRC1",4)==0);unsigned count=u32(h+12);assert(count<=capacity);
    assert(fseek(f,(long)u32(h+16),SEEK_SET)==0);
    for(unsigned i=0;i<count;++i){unsigned char row[16];assert(fread(row,1,16,f)==16);items[i].hash=u32(row);}
    fclose(f);return count;
}
static unsigned find(const kc_catalog* c,uint32_t hash) {
    for(unsigned i=0;i<c->count;++i)if(c->items[i].hash==hash)return i;
    assert(!"missing stable recipe hash");return 0;
}
int main(void) {
    static kc_item old_items[KR_MAX_RECIPES],new_items[KR_MAX_RECIPES];
    kc_catalog old={old_items,KR_MAX_RECIPES,load("tests/fixtures/kitchen_recipes_v0.krc",old_items,KR_MAX_RECIPES)};
    kc_catalog now={new_items,KR_MAX_RECIPES,load("assets/kitchen/kitchen_recipes.krc",new_items,KR_MAX_RECIPES)};
    assert(old.count==30 && now.count>=36 && now.count<=KR_MAX_RECIPES);
    for(unsigned i=0;i<old.count;++i)(void)find(&now,old.items[i].hash);
    ku_ui source={0},restored={0},scratch={0}; source.catalog=&old;restored.catalog=&now;
    source.conditions=(ku_conditions){2,30,0};source.saved_conditions=source.conditions;
    source.month=10;source.difficulty=2;source.reroll_limit=3;source.sound=1;source.volume=50;
    source.day=20754;source.daily_seed=123;source.favorites[0]=(UINT64_C(1)<<0)|(UINT64_C(1)<<29);
    source.has_saved=1;source.saved_count=2;source.saved_result[0]=0;source.saved_result[1]=29;
    source.store.history_count=1;source.store.history[0]=(ks_record){29,20754};
    source.store.cooked_count=1;source.store.cooked[0]=(ks_cooked){0,3,20754};
    source.sessions[29]=(ks_session){3,1,0};source.timer_steps[29]=2;
    kt_init(&source.timers);uint32_t id;assert(kt_add(&source.timers,90,1000,&id));
    source.timer_ids[0]=id;source.timer_recipe[0]=29;source.timer_step[0]=1;
    uint8_t buffer[KU_SAVE_MAX];size_t length=ku_save_encode(&source,buffer,sizeof(buffer),42,1000,-1);assert(length);
    FILE* fixture=fopen("tests/fixtures/kitchen_state_v0.kst","rb");assert(fixture);
    uint8_t old_bytes[KU_SAVE_MAX];size_t old_length=fread(old_bytes,1,sizeof old_bytes,fixture);fclose(fixture);
    assert(length==old_length+4 && old_bytes[4]==1 && buffer[4]==2);
    length=old_length;
    memcpy(buffer,old_bytes,length);
    restored.date_unconfirmed=1;uint32_t sequence=0;int lost=0;
    assert(ku_save_decode(&restored,&scratch,buffer,length,&sequence,2000,-1,&lost));
    unsigned first=find(&now,old.items[0].hash), last=find(&now,old.items[29].hash);
    assert(sequence==42 && lost && restored.date_unconfirmed==0);
    assert(restored.saved_result[0]==first && restored.saved_result[1]==last);
    assert(ku_recipe_bit(restored.favorites,first) && ku_recipe_bit(restored.favorites,last));
    assert(restored.store.history[0].recipe==last && restored.store.cooked[0].recipe==first);
    assert(restored.sessions[last].visited==3 && restored.sessions[last].index==1 && restored.timer_steps[last]==2);
    assert(restored.timers.count==1 && restored.timers.items[0].status==KT_PAUSED && restored.timer_recipe[0]==last);
    puts("PASS: real 30-to-expanded catalog preserves stable IDs, progress, favorites and paused timer");
}
