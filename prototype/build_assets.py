"""Reproduce browser-only cutouts; accepted masters/font remain read-only."""
from pathlib import Path
from PIL import Image
import json,hashlib

ROOT=Path(__file__).resolve().parent.parent
OUT=Path(__file__).resolve().parent/'assets'
OUT.mkdir(parents=True,exist_ok=True)
records=[]
for name,relative in [('C','costume/costume_final.png'),('K2','grok-start/K2.png')]:
    source=ROOT/'game-art/kitchen'/relative
    im=Image.open(source).convert('RGBA');data=[]
    for r,g,b,a in im.get_flattened_data():
        data.append((r,g,b,0 if g>80 and g-r>20 and g-b>20 else a))
    im.putdata(data)
    bbox=im.getchannel('A').point(lambda a:255 if a>=64 else 0).getbbox()
    im=im.crop(bbox);im.save(OUT/(name+'.png'))
    records.append({'name':name,'source':str(source.relative_to(ROOT)),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'crop':bbox,'processing':'uniform green-key and crop only, no repaint or mirroring','output_sha256':hashlib.sha256((OUT/(name+'.png')).read_bytes()).hexdigest()})
manifest=OUT/'sources.json'
existing=json.loads(manifest.read_text(encoding='utf-8')) if manifest.exists() else {}
existing['cutouts']=records
manifest.write_text(json.dumps(existing,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'cutouts':len(records),'font_unchanged':True}))
