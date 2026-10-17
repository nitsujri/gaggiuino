#!/usr/bin/env python3
"""Archive or verify the Indigo first-load candidate. Never accesses hardware."""
import argparse,hashlib,json,re,shutil,subprocess,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
DEST=ROOT/'tmp/indigo-release'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def sources():
 paths=[]
 for directory in ['src','lib','scales-calibration','scripts','test','test-lib','tools/indigo-preview','tools/indigo-release','tools/touchscreen-audit']:
  paths.extend(p for p in (ROOT/directory).rglob('*') if p.is_file() and '__pycache__' not in p.parts and p.suffix not in ('.pyc','.DS_Store'))
 paths.extend(ROOT/p for p in ['platformio.ini','extra_defines.ini','AGENTS.md'] if (ROOT/p).is_file())
 return sorted(set(paths))
def verify():
 report=json.loads((DEST/'manifest.json').read_text());failed=[]
 for name,expected in report['sourceHashes'].items():
  p=ROOT/name
  if not p.is_file() or sha(p)!=expected:failed.append('source '+name)
 for name,record in report['artifacts'].items():
  p=ROOT/name
  if not p.is_file() or sha(p)!=record['sha256']:failed.append('artifact '+name)
 if failed:raise SystemExit('Verification failed:\n'+'\n'.join(failed))
 print(f"Verified {len(report['sourceHashes'])} source/config files and {len(report['artifacts'])} artifacts. Hardware acceptance remains pending.")
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--verify',action='store_true');ap.add_argument('--uploaded',action='store_true');args=ap.parse_args()
 if args.verify:verify();return
 DEST.mkdir(parents=True,exist_ok=True)
 log=(ROOT/'tmp/indigo-build.log').read_text();cal=(ROOT/'tmp/indigo-calibration-build.log').read_text();tests=(ROOT/'tmp/indigo-tests.log').read_text()
 assert '[SUCCESS]' in log and '[SUCCESS]' in cal
 count=int(re.search(r'(\d+) test cases: \1 succeeded',tests)[1]);assert count>=49
 subprocess.run(['python3','tools/indigo-release/test_inspect_settings.py'],cwd=ROOT,check=True)
 subprocess.run(['python3','tools/touchscreen-audit/audit.py'],cwd=ROOT,check=True,stdout=subprocess.DEVNULL)
 for source,dest in [('.pio/build/all-pcb-stlink/firmware.bin','firmware.bin'),('.pio/build/all-pcb-stlink/firmware.elf','firmware.elf'),('tmp/indigo-build.log','main-build.log'),('tmp/indigo-calibration-build.log','calibration-build.log'),('tmp/indigo-tests.log','native-tests.log'),('extra_defines.ini','extra_defines.ini'),('docs/touchscreen-redesign/LOADING.txt','LOADING.txt')]:
  shutil.copy2(ROOT/source,DEST/dest)
 assert (DEST/'scales-calibration.bin').is_file(), 'Copy calibration image before changing PIO environments'
 src=sources();source_hashes={str(p.relative_to(ROOT)):sha(p) for p in src}
 with zipfile.ZipFile(DEST/'source-snapshot.zip','w',compression=zipfile.ZIP_DEFLATED) as z:
  for p in src:z.write(p,str(p.relative_to(ROOT)))
  for p in (ROOT/'docs/touchscreen-redesign').glob('*'):
   if p.is_file() and p.name!='release-status.json':z.write(p,str(p.relative_to(ROOT)))
 backups=['tmp/settings-backup/controller-flash.bin','tmp/settings-backup/eeprom-page.bin','tmp/settings-backup/running-config.bin','tmp/settings-backup/settings.json']
 artifacts={str(p.relative_to(ROOT)):{'bytes':p.stat().st_size,'sha256':sha(p)} for p in list(DEST.iterdir())+[ROOT/p for p in backups] if p.is_file() and p.name not in ('manifest.json','SHA256SUMS','package.log','usb-firstboot.log')}
 report={'schema':'gaggiuino-indigo-release/2','softwareReadyForFirstLoad':True,'hardwareVerified':False,'uploaded':args.uploaded,'productionActuatorsEnabled':True,'newTftRequired':False,'displayRequirement':'unchanged stock frozen Nextion/TJC HMI; physical compatibility pending','controllerEnvironment':'all-pcb-stlink','baseCommit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'sourceState':'working-tree snapshot; source hashes included','settingsSchema':13,'previousSettingsSchema':12,'settingsBytes':1056,'metadataBytes':1072,'initialRecipes':['Turbo','SOUP'],'defaultProfile':'Turbo','tofFullInitialMm':30,'trayBaselineStatus':'UI capture+Save required for automatic boiler fill/pressure release; Home Flush manual bypass','trayMinimumG':150,'trayWeightReferenceG':200,'trayWeightReferenceStatus':'provisional aftermarket reference; not verified OEM weight',
 'verified':{'mainBuild':'PASS','calibrationBuild':'PASS','nativeTests':count,'sanitizers':['address','undefined'],'backupDecoderTests':2,'ramBytes':int(re.search(r'RAM:.*used (\d+) bytes',log)[1]),'flashBytes':int(re.search(r'Flash:.*used (\d+) bytes',log)[1]),'hardwareAcceptance':'NOT RUN'},'sourceHashes':source_hashes,'artifacts':artifacts}
 (DEST/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
 status={k:v for k,v in report.items() if k not in ('sourceHashes','artifacts')};status['manifest']='tmp/indigo-release/manifest.json';status['firmware']=artifacts['tmp/indigo-release/firmware.bin']
 (ROOT/'docs/touchscreen-redesign/release-status.json').write_text(json.dumps(status,indent=2)+'\n')
 (DEST/'SHA256SUMS').write_text(''.join(f"{r['sha256']}  {name}\n" for name,r in sorted(artifacts.items())))
 verify();print(json.dumps(status,indent=2))
if __name__=='__main__':main()
