/* KST1 made from the real 128-recipe KRC and prior 50-ingredient metadata;
 * link this with synthetic or real 58-ingredient metadata to exercise reorder. */
#include "kitchen_ui.h"
#include "kitchen_save.h"
#include "kitchen_metadata.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t u32(const unsigned char* p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static unsigned catalog_load(const char* path, kc_item* items, unsigned capacity) {
    FILE* f=fopen(path,"rb");assert(f);unsigned char header[48];assert(fread(header,1,48,f)==48);
    assert(!memcmp(header,"KRC1",4));unsigned count=u32(header+12);assert(count<=capacity);
    assert(!fseek(f,(long)u32(header+16),SEEK_SET));
    for(unsigned i=0;i<count;++i){unsigned char row[16];assert(fread(row,1,16,f)==16);items[i].hash=u32(row);}
    fclose(f);return count;
}
static unsigned id(const char* value) {
    for(unsigned i=0;i<km_ingredient_count;++i)if(!strcmp(km_ingredients[i].id,value))return i;
    assert(!"missing ingredient ID");return 0;
}
static unsigned hash_index(const kc_catalog* c,uint32_t hash) {
    for(unsigned i=0;i<c->count;++i)if(c->items[i].hash==hash)return i;
    assert(!"missing recipe ID");return 0;
}
int main(int argc, char** argv) {
    int expect_missing = argc == 2 && !strcmp(argv[1], "--expect-missing");
    assert(km_ingredient_count == 58);
    static kc_item old_items[KR_MAX_RECIPES],new_items[KR_MAX_RECIPES];
    kc_catalog old={old_items,KR_MAX_RECIPES,catalog_load("tests/fixtures/kitchen_recipes_128_50ingredients.krc",old_items,KR_MAX_RECIPES)};
    const char* input=getenv("KITCHEN_TEST_KRC");
    kc_catalog now={new_items,KR_MAX_RECIPES,catalog_load(input ? input : "assets/kitchen/kitchen_recipes.krc",new_items,KR_MAX_RECIPES)};
    assert(old.count == 128 && now.count >= 128);
    for(unsigned i=0;i<128;++i)(void)hash_index(&now,old.items[i].hash);
    uint8_t bytes[KU_SAVE_MAX];FILE* f=fopen("tests/fixtures/kitchen_state_128_50ingredients.kst","rb");assert(f);
    size_t n=fread(bytes,1,sizeof bytes,f);fclose(f);assert(n);
    ku_ui dest={0},scratch={0};dest.catalog=&now;dest.fridge=5;dest.excluded=7;uint32_t seq=0;int lost=0;
    int decoded=ku_save_decode(&dest,&scratch,bytes,n,&seq,1000,-1,&lost);
    if (expect_missing) {assert(!decoded && dest.fridge==5 && dest.excluded==7);puts("PASS: missing stable ingredient rejects migration without partial mutation");return 0;}
    assert(decoded);assert(seq==12850 && !lost);
    /* Confirm we exercised index changes, not just extra trailing ingredients. */
    assert(id("shrimp") != 43 || id("tomato") != 48 || id("fish_whole") != 19 || id("clam") != 12 || id("egg") != 15);
    uint64_t expected_fridge=(UINT64_C(1)<<id("shrimp"))|(UINT64_C(1)<<id("tomato"))|(UINT64_C(1)<<id("fish_whole"));
    uint64_t expected_excluded=(UINT64_C(1)<<id("clam"))|(UINT64_C(1)<<id("egg"));
    assert(dest.fridge==expected_fridge && dest.excluded==expected_excluded);
    assert(dest.shopping_count==3);
    assert(dest.shopping[0].id==id("shrimp") && dest.shopping[0].uid==11 && dest.shopping[0].qty100==12500 && !dest.shopping[0].bought);
    assert(dest.shopping[1].id==id("egg") && dest.shopping[1].uid==12 && dest.shopping[1].qty100==300 && dest.shopping[1].bought);
    assert(dest.shopping[2].id==id("fish_whole") && dest.shopping[2].uid==13 && dest.shopping[2].qty100==45000 && !dest.shopping[2].bought);
    unsigned a=hash_index(&now,old.items[2].hash), b=hash_index(&now,old.items[127].hash);
    assert(dest.saved_count==2 && dest.saved_result[0]==a && dest.saved_result[1]==b);
    assert(ku_recipe_bit(dest.favorites,a) && ku_recipe_bit(dest.favorites,b));
    puts("PASS: real 128/50 KST1 migrates reordered 58 ingredients, shopping, favorites and saved recipes");
}
