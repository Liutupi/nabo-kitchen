/* Synthetic fixture writer: compile with the 128-recipe KST1 implementation,
 * kitchen_metadata_50.c and the archived 128-recipe KRC. No user data. */
#include "kitchen_ui.h"
#include "kitchen_save.h"
#include "kitchen_metadata.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t u32(const unsigned char *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static unsigned ingredient(const char *id) {
    for (unsigned i=0; i<km_ingredient_count; ++i) if (!strcmp(km_ingredients[i].id,id)) return i;
    assert(!"missing old ingredient ID");return 0;
}
int main(void) {
    assert(KR_MAX_RECIPES == 128 && km_ingredient_count == 50);
    static kc_item items[128];
    FILE* f=fopen("tests/fixtures/kitchen_recipes_128_50ingredients.krc","rb");assert(f);
    unsigned char header[48];assert(fread(header,1,sizeof header,f)==sizeof header);
    assert(!memcmp(header,"KRC1",4) && u32(header+12)==128);
    assert(!fseek(f,(long)u32(header+16),SEEK_SET));
    for (unsigned i=0;i<128;++i){unsigned char row[16];assert(fread(row,1,16,f)==16);items[i].hash=u32(row);}
    fclose(f);
    kc_catalog cat={items,128,128};ku_ui state={0};state.catalog=&cat;
    state.conditions=(ku_conditions){2,30,0};state.saved_conditions=state.conditions;
    state.month=10;state.difficulty=2;state.reroll_limit=3;state.sound=1;state.volume=50;
    state.fridge=(UINT64_C(1)<<ingredient("shrimp"))|(UINT64_C(1)<<ingredient("tomato"))|(UINT64_C(1)<<ingredient("fish_whole"));
    state.excluded=(UINT64_C(1)<<ingredient("clam"))|(UINT64_C(1)<<ingredient("egg"));
    state.shopping_count=3;
    state.shopping[0]=(ku_shopping){11,12500,(uint8_t)ingredient("shrimp"),0,0};
    state.shopping[1]=(ku_shopping){12,300,(uint8_t)ingredient("egg"),1,1};
    state.shopping[2]=(ku_shopping){13,45000,(uint8_t)ingredient("fish_whole"),0,0};
    state.has_saved=1;state.saved_count=2;state.saved_result[0]=2;state.saved_result[1]=127;
    ku_recipe_flip(state.favorites,2);ku_recipe_flip(state.favorites,127);
    kt_init(&state.timers);
    uint8_t buf[KU_SAVE_MAX];size_t len=ku_save_encode(&state,buf,sizeof buf,12850,0,-1);assert(len);
    f=fopen("tests/fixtures/kitchen_state_128_50ingredients.kst","wb");assert(f);
    assert(fwrite(buf,1,len,f)==len);assert(!fclose(f));
    printf("128-recipe, 50-ingredient KST1 fixture: %zu bytes\n",len);
}
