#!/usr/bin/env python3
"""Read-only original-HMI inventory and actual Indigo field traceability."""
import argparse,csv,hashlib,json,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def digest(p):
 p=ROOT/p
 return hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None
def git(*args):return subprocess.check_output(['git',*args],cwd=ROOT,text=True).strip()
def records(path,legacy=False):
 s=(ROOT/path).read_text();prefix,nested=s.split('struct profile_t {',1);profile,tail=nested.split('} profiles[MAX_PROFILES];',1)
 rx=r'^\s*(char|bool|uint\d+_t|int32_t|int|float)\s+(\w+)(?:\[[^\]]+\])?;'
 return ([{'type':t,'name':n} for t,n in re.findall(rx,profile,re.M)],
         [{'type':t,'name':n} for t,n in re.findall(rx,prefix.split('struct eepromValues_t'+('_v12' if legacy else '')+' {',1)[1]+tail.split('};',1)[0],re.M)])
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',default='docs/touchscreen-redesign/inventory.json');args=ap.parse_args()
 hmi=json.loads((ROOT/'tools/touchscreen-audit/original-interface.json').read_text())
 oldp,oldg=records('src/eeprom_data/legacy/eeprom_data_v12.h',True);newp,newg=records('src/eeprom_data/eeprom_data.h')
 source=(ROOT/'lib/Common/indigo_fields.cpp').read_text();p,s=source.split('const Field systemFields[] =',1)
 rx=r'\{"([^"]+)",\s*"([^"]+)".*?Kind::(\w+),\s*(\d+),'
 descriptors={'profile':re.findall(rx,p,re.S),'system':re.findall(rx,s,re.S)}
 mappedp={r[0] for r in descriptors['profile']}|{'name','shotPreset'}
 mappedg={r[0] for r in descriptors['system']}|{'activeProfile','trayGuardCalibrated','trayZeroRaw'}
 missing={'profile':sorted({f['name'] for f in newp}-mappedp),'system':sorted({f['name'] for f in newg}-mappedg)}
 entries=[]
 for page in hmi:
  events=[];components=[];ids=set()
  for c in page['components']:
   a=c['attributes'];components.append({'name':a.get('objname'),'id':a.get('id'),'type':a.get('type'),'scope':a.get('vscope'),'rect':[a.get(k) for k in ('x','y','w','h')],'attributes':a})
   for event,code in c.get('events',{}).items():
    code='\n'.join(l for l in code.splitlines() if l.strip() and not l.strip().startswith('//'))
    if not code:continue
    packets=re.findall(r'printh ((?:[A-Fa-f0-9]{2} ?)+)',code)
    for packet in packets:
     b=bytes.fromhex(packet)
     if len(b)==4 and b[:3]==b'\x23\x02\x50':ids.add(b[3])
    events.append({'component':a.get('objname'),'event':event,'packets':packets,'code':code})
  entries.append({'page':page['name'],'reportedLogicalPageIDs':sorted(ids),'components':components,'events':events})
 flags=re.findall(r'^\s*(-D[^\s;]+)',(ROOT/'extra_defines.ini').read_text(),re.M)
 assets=['lcd-hmi/nextion-lcd.HMI','nextion-basic-lcd.tft','nextion-discovery-lcd.tft','tjc-basic-lcd.tft']
 unchanged={f:git('hash-object',f)==git('rev-parse','release/stm32-blackpill:'+f) for f in assets}
 assert all(unchanged.values()),'Stock display assets changed'
 paths=[str(p.relative_to(ROOT)) for d in ['src','lib/Common','lib/HX711','scales-calibration','scripts'] for p in (ROOT/d).rglob('*') if p.is_file()]
 nextion=(ROOT/'src/lcd/nextion.cpp').read_text()
 report={'schema':'touchscreen-audit/2','baselineCommit':git('rev-parse','release/stm32-blackpill'),'controllerCommit':git('rev-parse','HEAD'),'controllerDirty':bool(git('status','--porcelain')),
 'sourceOfTruth':'Frozen repository HMI and archived schema12; native Indigo C++ implementation',
 'counts':{'pages':len(entries),'components':sum(len(e['components']) for e in entries),'nonemptyEvents':sum(len(e['events']) for e in entries),'originalProfileFieldsIncludingName':len(oldp),'originalGlobalFieldsIncludingSelection':len(oldg),'currentProfileFieldsIncludingName':len(newp),'currentGlobalFieldsIncludingSelection':len(newg)},
 'originalProfileFields':oldp,'originalGlobalFields':oldg,'profileFields':newp,'globalFields':newg,'nativeDescriptors':descriptors,'missingNativeFields':missing,
 'specialMappings':{'profile.name':'Rename keyboard','profile.shotPreset':'legacy preserved; shotRatioX100 canonical editor','system.activeProfile':'explicit persistent profile selection','system.trayGuardCalibrated':'Capture NO-TRAY action + Save','system.trayZeroRaw':'Capture NO-TRAY both channels + Save'},
 'pages':entries,'stockDisplayAssetsUnchanged':unchanged,'readAddresses':sorted(set(re.findall(r'myNex\.read(?:Number|Str)\("([^"]+)"',nextion))),'writeAddresses':sorted(set(re.findall(r'myNex\.write(?:Num|Str)\("([^"]+)"',nextion))),
 'localBuildFlags':flags,'localReadiness':{'indigoEnabled':'-DINDIGO_UI' in flags,'tofEnabled':'-DTOF_VL53L0X' in flags,'newTftRequired':False,'nativeControllerImplemented':True,'settingsMigrationImplemented':True,'trayBaseline':'capture via UI required before automatic boiler fill/pressure release; Home Flush manual bypass','hardwareVerified':False},
 'hashes':{p:digest(p) for p in assets+['tools/touchscreen-audit/original-interface.json','tmp/settings-backup/controller-flash.bin','tmp/settings-backup/eeprom-page.bin']},'controllerSourceHashes':{p:digest(p) for p in sorted(paths)}}
 out=ROOT/args.output;out.write_text(json.dumps(report,indent=2)+'\n')
 # Add new schema fields once, retaining original provenance rows.
 matrix=ROOT/'docs/touchscreen-redesign/field-matrix.csv';rows=list(csv.DictReader(matrix.open()));known={(r['scope'],r['field']) for r in rows}
 for scope,items in [('profile',newp),('system',newg)]:
  for f in items:
   if(scope,f['name']) not in known:
    rows.append(dict(scope=scope,field=f['name'],c_type=f['type'],original_schema_line='NEW schema13',new_location='Native descriptor or special action; inventory.json',prototype='reference only',native_redesign='IMPLEMENTED',persistence='schema13 scoped Save + CRC'))
 with matrix.open('w') as f:
  w=csv.DictWriter(f,fieldnames=rows[0].keys());w.writeheader();w.writerows(rows)
 print(json.dumps({'counts':report['counts'],'missingNativeFields':missing,'stockDisplayAssetsUnchanged':unchanged,'output':str(out)},indent=2))
 if any(missing.values()):raise SystemExit(1)
if __name__=='__main__':main()
