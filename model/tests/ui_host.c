/* Test-only wrapper; never linked into firmware. */
#include "kitchen_ui.h"
#include "kitchen_save.h"
#include "kitchen_journal.h"
#include <stdio.h>
static krc_pack pack;
static kc_item items[KR_MAX_RECIPES];
static kc_catalog catalog={items,KR_MAX_RECIPES,0};
static ku_ui ui;
static ku_ui scratch;
static kj_journal journal;
static uint8_t journal_buffer[KU_SAVE_MAX];
int host_init(const unsigned char *data,size_t length,int32_t day,unsigned month,uint32_t seed,
              const uint8_t *order,unsigned ingredients,const uint8_t *fridge_order,unsigned fridge_count){
    return krc_open(&pack,data,length)&&kc_load(&catalog,&pack)&&ku_init(&ui,&catalog,&pack,day,month,seed,order,ingredients,fridge_order,fridge_count);
}
void host_key(int key,int kind,int64_t now){ku_key(&ui,(ki_key)key,(ki_kind)kind,now);}
void host_tick(int64_t now){ku_tick(&ui,now);}
void host_set_fridge(uint64_t have,uint64_t excluded){ui.fridge=have;ui.excluded=excluded;}
unsigned host_fridge(uint16_t *out,unsigned capacity){return ku_fridge_matches(&ui,out,capacity);}
int host_add_shopping(unsigned recipe,unsigned people){ui.conditions.people=(uint8_t)people;return ku_add_shopping(&ui,recipe);}
void host_bought(unsigned index){if(index<ui.shopping_count)ui.shopping[index].bought=1;}
unsigned host_shopping(ku_shopping *out,unsigned capacity){unsigned count=ui.shopping_count;
    for(unsigned i=0;i<count&&i<capacity;++i)out[i]=ui.shopping[i];return count;}
void host_prepare_save(void){
    ui.conditions=(ku_conditions){3,60,2};ui.saved_conditions=ui.conditions;ui.has_saved=1;ui.saved_count=2;
    ui.saved_result[0]=0;ui.saved_result[1]=5;ui.saved_notes=1;ui.rerolls_used=2;
    ui.favorites=(UINT64_C(1)<<0)|(UINT64_C(1)<<5);ui.excluded=UINT64_C(1)<<2;ui.fridge=UINT64_C(1)<<7;
    ks_complete_step(&ui.store,&ui.sessions[0],0,1,ui.day);ui.timer_steps[0]=1;
    ku_add_shopping(&ui,0);uint32_t id;
    if(kt_add(&ui.timers,90,1000,&id)){ui.timer_ids[0]=id;ui.timer_recipe[0]=0;ui.timer_step[0]=0;}
    if(kt_add(&ui.timers,40,1000,&id)){ui.timer_ids[1]=id;ui.timer_recipe[1]=5;ui.timer_step[1]=1;kt_toggle(&ui.timers,id,2500);}
}
size_t host_save(uint8_t *buf,size_t cap,uint32_t seq,int64_t mono,int64_t wall){return ku_save_encode(&ui,buf,cap,seq,mono,wall);}
int host_load(const uint8_t *buf,size_t len,uint32_t *seq,int64_t mono,int64_t wall,int *lost){return ku_save_decode(&ui,&scratch,buf,len,seq,mono,wall,lost);}
int host_journal_load(const kj_io *io,int64_t mono,int64_t wall,int *lost){return kj_load(&journal,io,&ui,&scratch,journal_buffer,mono,wall,lost);}
int host_journal_store(const kj_io *io,int64_t mono,int64_t wall){return kj_store(&journal,io,&ui,journal_buffer,mono,wall);}
void host_reverse_catalog(void){for(unsigned i=0;i<catalog.count/2;++i){kc_item temp=items[i];items[i]=items[catalog.count-i-1];items[catalog.count-i-1]=temp;}}
const char *host_save_info(int64_t now){static char out[512];unsigned favorite=0;
    for(unsigned i=0;i<catalog.count;++i)if(ui.favorites&(UINT64_C(1)<<i))++favorite;
    snprintf(out,sizeof(out),"{\"people\":%u,\"minutes\":%u,\"combo\":%u,\"saved0\":\"%s\",\"saved1\":\"%s\","
      "\"favorites\":%u,\"history\":%u,\"shopping\":%u,\"timers\":%u,\"status0\":%d,\"left0\":%lld,\"status1\":%d,\"left1\":%lld}",
      ui.conditions.people,ui.conditions.minutes,ui.conditions.combo,ui.saved_count?items[ui.saved_result[0]].id:"",ui.saved_count>1?items[ui.saved_result[1]].id:"",
      favorite,ui.store.history_count,ui.shopping_count,ui.timers.count,ui.timers.count?ui.timers.items[0].status:-1,
      (long long)(ui.timers.count?kt_remaining(&ui.timers.items[0],now):0),ui.timers.count>1?ui.timers.items[1].status:-1,
      (long long)(ui.timers.count>1?kt_remaining(&ui.timers.items[1],now):0));return out;
}
const char *host_snapshot(void){
    static char out[1024];char result[128]={0};size_t at=0;
    for(unsigned i=0;i<ui.result_count;++i){int n=snprintf(result+at,sizeof(result)-at,"%s%u",i?",":"",ui.result[i]);if(n<0||(size_t)n>=sizeof(result)-at)break;at+=(size_t)n;}
    ks_session *s=&ui.sessions[ui.recipe];
    snprintf(out,sizeof(out),"{\"page\":%u,\"focus\":%u,\"recipe\":%u,\"result\":[%s],\"notes\":%u,"
             "\"result_focus\":%u,\"rerolls\":%u,\"index\":%u,\"visited\":%u,\"completed\":%u,"
             "\"timers\":%u,\"outside\":%u,\"message\":%u,\"fridge\":\"%llu\",\"favorites\":\"%llu\","
             "\"excluded\":\"%llu\",\"shopping_count\":%u,\"settings_mode\":%u,\"book_filter\":%u}",
             ui.page,ui.focus,ui.recipe,result,ui.notes,ui.result_focus,ui.rerolls_used,s->index,s->visited,s->completed,
             ui.timers.count,ui.outside,ui.message,(unsigned long long)ui.fridge,(unsigned long long)ui.favorites,
             (unsigned long long)ui.excluded,ui.shopping_count,ui.settings_mode,ui.book_filter);
    return out;
}
