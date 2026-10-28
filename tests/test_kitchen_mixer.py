"""Test the production boot speech / alarm mixer without hardware."""
import os
import subprocess
import tempfile
from pathlib import Path

board = Path(__file__).resolve().parents[1]/'main/kitchen'
source = r'''
#include "nabo_kitchen_mixer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    bool pending = true;
    /* Leaving boot early does not cancel the scheduled greeting; only the
     * clock matters, and repeated worker iterations cannot replay it. */
    assert(!nk_boot_voice_due(&pending, 1399, 1400, false) && pending);
    assert(nk_boot_voice_due(&pending, 1400, 1400, false) && !pending);
    assert(!nk_boot_voice_due(&pending, 1401, 1400, false));
    pending = true;
    assert(!nk_boot_voice_due(&pending, 1200, 1400, true) && !pending);
    assert(!nk_boot_voice_due(&pending, 1400, 1400, false));
    pending = true;
    assert(!nk_boot_voice_due(&pending, 2000, 1400, true) && !pending);
    assert(!nk_boot_voice_due(NULL, 2000, 1400, false));
    static const uint8_t voice[] = {0x10,0x27,0xf0,0xd8,0xff,0x7f,0x00,0x80};
    nk_mixer m; nk_mixer_init(&m,voice,sizeof(voice)); int16_t out[400] = {0};
    nk_mixer_fill(&m,out,2,false,true,false,50);
    assert(out[0]==5000 && out[1]==-5000 && m.voice_active);
    memset(out,0,sizeof(out));nk_mixer_fill(&m,out,3,false,false,false,100);
    assert(out[0]==32767 && out[1]==-32768 && out[2]==0 && !m.voice_active);
    memset(out,0,sizeof(out));nk_mixer_fill(&m,out,2,false,true,false,0);
    assert(out[0]==0 && out[1]==0);
    memset(out,0,sizeof(out));nk_mixer_fill(&m,out,2,true,false,false,100);
    assert(out[0]==0 && !m.voice_active);
    for (unsigned i=0;i<400;++i) out[i]=32700;
    nk_mixer_fill(&m,out,400,false,false,true,100);
    int changed=0;for (unsigned i=0;i<400;++i) if(out[i]!=32700)changed=1;
    assert(changed && m.tone_left==16400);
    memset(out,0,sizeof(out));nk_mixer_fill(&m,out,400,true,false,false,100);
    for(unsigned i=0;i<400;++i)assert(out[i]==0);
    assert(m.tone_left==0);
    puts("PASS: speech chunking, volume, reset, alarm mixing and clipping");
}
'''
with tempfile.TemporaryDirectory() as directory:
    tmp = Path(directory)
    (tmp/'test.c').write_text(source)
    subprocess.run([os.environ.get('CC','cc'),'-std=c11','-Wall','-Wextra','-Werror','-I',str(board),
                    str(board/'nabo_kitchen_mixer.c'),str(tmp/'test.c'),'-o',str(tmp/'test')],check=True)
    subprocess.run([str(tmp/'test')],check=True)
