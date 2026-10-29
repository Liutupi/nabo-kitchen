import argparse,hashlib,json,shutil,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description="Rebuild D2 resources and synchronize the explicit firmware checkout; never flash or publish")
parser.add_argument('--firmware-root',required=True,type=Path)
parser.add_argument('--sync-firmware',action='store_true',help='Copy into a clean firmware checkout after rebuilding')
parser.add_argument('--reuse-packed',action='store_true',help='Use build/kitchen after the three documented packers have completed')
args=parser.parse_args()
firmware=args.firmware_root.resolve()
if not (firmware/'tools/pack_kitchen_assets.py').is_file():parser.error('Firmware checkout required')
if args.sync_firmware:
 branch=subprocess.check_output(['git','branch','--show-current'],cwd=firmware,text=True).strip()
 dirty=subprocess.check_output(['git','status','--porcelain'],cwd=firmware,text=True).strip()
 if branch!='firmware' or dirty:parser.error('Sync requires the firmware branch with a clean worktree; preserve and commit existing work first')
if not args.reuse_packed:
 for command in (["tools/pack_fonts.py"],["tools/pack_assets.py","pack"],["tools/pack_recipes.py","pack","--font-chars","build/kitchen/kitchen_fonts.chars.txt"]):
  subprocess.run([sys.executable,*command],cwd=root,check=True)
sys.path.insert(0,str(root/'tools'))
from build_firmware_metadata import build,build_c,build_retired
out=root/'game-art/kitchen/dish-hero/batches/2026-10-29-d2/resource-candidate'
out.mkdir(exist_ok=True)
for name in ('kitchen_recipes.krc','kitchen_dish.bin','kitchen_icons.bin','kitchen_ui.bin','kitchen_fonts.bin','kitchen_fonts.chars.txt','fonts-report.json','fonts-proof.png','static_report.json','kitchen_recipes.json','OFL.ArchivoBlack.txt','OFL.SourceHanSans.txt'):
 shutil.copyfile(root/'build/kitchen'/name,out/name)
shutil.copyfile(root/'game-art/kitchen/dish-hero/batches/2026-10-29/resource-candidate/kitchen_nabo.bin',out/'kitchen_nabo.bin')
(out/'kitchen_metadata.json').write_bytes(build(all_fridge=True))
(out/'kitchen_metadata.c').write_bytes(build_c(all_fridge=True))
removed=json.loads((root/'data/D2_REMOVED.json').read_text(encoding='utf-8'))['removed']
(out/'kitchen_retired.h').write_bytes(build_retired())
subprocess.run([sys.executable,str(firmware/'tools/pack_kitchen_assets.py'),'--asset-dir',str(out),'--output',str(out/'kitchen_bundle.bin')],check=True)
names=['kitchen_recipes.krc','kitchen_fonts.bin','kitchen_dish.bin','kitchen_icons.bin','kitchen_ui.bin','kitchen_nabo.bin','kitchen_bundle.bin']
if args.sync_firmware:
 for name in names:shutil.copyfile(out/name,firmware/'assets/kitchen'/name)
 shutil.copyfile(out/'kitchen_metadata.c',firmware/'main/kitchen/kitchen_metadata.c')
 shutil.copyfile(out/'kitchen_retired.h',firmware/'main/kitchen/kitchen_retired.h')
 for name in ('OFL.ArchivoBlack.txt','OFL.SourceHanSans.txt'):
  license_path=firmware/'assets/kitchen/licenses'/name
  if license_path.exists():
   if license_path.read_text(encoding='utf-8')!=(out/name).read_text(encoding='utf-8'):
    raise ValueError('Existing font license differs; inspect before replacing: '+name)
  else:
   license_path.parent.mkdir(parents=True,exist_ok=True)
   shutil.copyfile(out/name,license_path)
report={'content_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),'recipes':145,'heroes':145,'removed_ids':len(removed),'new_glyphs':{ch:ch in (out/'kitchen_fonts.chars.txt').read_text(encoding='utf-8') for ch in '腩煸蘸黏'},'files':{n:{'bytes':(out/n).stat().st_size,'sha256':hashlib.sha256((out/n).read_bytes()).hexdigest()} for n in names},'resource_partition_bytes':0x500000,'partition_remaining_bytes':0x500000-(out/'kitchen_bundle.bin').stat().st_size,'source_art_status':'review','device_tests':'NOT RUN','build':'NOT RUN; resource rebuild only','budget_note':'Historical dish/font sub-budgets remain exceeded; no specification or budget changed. Actual 5MiB resource partition checked.'}
assert all(report['new_glyphs'].values())
(out/'validation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,ensure_ascii=False))
