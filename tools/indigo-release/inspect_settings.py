#!/usr/bin/env python3
"""Read-only decoder for frozen STM32 schema12 and Indigo schema13 backups.

No ST-Link access, hardware writes or flash commands. Accepts a full512KB flash
backup or a raw settings-page image. ARM32 layout is explicit, even on64-bit hosts.
"""
import argparse,ctypes,json,re,struct,zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
T={'uint16_t':ctypes.c_uint16,'uint8_t':ctypes.c_uint8,'int32_t':ctypes.c_int32,'int':ctypes.c_int32,'float':ctypes.c_float,'bool':ctypes.c_bool,'char':ctypes.c_char}
def fields(source):
 out=[]
 source=re.sub(r'//[^\n]*|/\*.*?\*/','',source,flags=re.S)
 for typ,name,arr in re.findall(r'\b(uint16_t|uint8_t|int32_t|int|float|bool|char)\s+(\w+)\s*(?:\[([^]]+)\])?\s*;',source):
  t=T[typ]
  if arr:t=t*{'PROFILE_NAME_LENGTH':25,'MAX_PROFILES':5}.get(arr,int(arr) if arr.isdigit() else 0)
  out.append((name,t))
 return out

def schema(version):
 path=ROOT/('src/eeprom_data/legacy/eeprom_data_v12.h' if version==12 else 'src/eeprom_data/eeprom_data.h')
 text=path.read_text();body=text.split('struct eepromValues_t'+('_v12' if version==12 else '')+' {',1)[1]
 prefix,nested=body.split('struct profile_t {',1);profile,tail=nested.split('} profiles[',1);tail=tail.split(';',1)[1].split('};',1)[0]
 class Profile(ctypes.LittleEndianStructure):_fields_=fields(profile)
 class Config(ctypes.LittleEndianStructure):_fields_=fields(prefix)+[('profiles',Profile*5)]+fields(tail)
 assert ctypes.sizeof(Profile)==200
 assert ctypes.sizeof(Config)==(1040 if version==12 else 1056)
 return Config

def plain(v):
 if isinstance(v,ctypes.Structure):return {k:plain(getattr(v,k)) for k,_ in v._fields_}
 if isinstance(v,bytes):return v.split(b'\0',1)[0].decode('latin1')
 if isinstance(v,ctypes.Array):return [plain(x) for x in v]
 return v

def inspect(path):
 raw=Path(path).read_bytes()
 if len(raw)==524288:raw=raw[0x7c000:]
 version=struct.unpack_from('<H',raw)[0]
 if version not in (12,13):raise ValueError(f'Unsupported settings version {version}')
 Config=schema(version);size=ctypes.sizeof(Config);timestamp=struct.unpack_from('<I',raw,4)[0];xor=struct.unpack_from('<I',raw,8+size)[0]
 if xor!=timestamp^version:raise ValueError('Version/timestamp metadata check failed')
 values=raw[8:8+size]
 if version==13 and struct.unpack_from('<I',raw,12+size)[0]!=zlib.crc32(values):raise ValueError('Settings values CRC32 check failed')
 return {'schema_version':version,'values_bytes':size,'timestamp':timestamp,'metadata_valid':True,'values_crc_valid':version==13,'values':plain(Config.from_buffer_copy(values))}
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('image');args=ap.parse_args();print(json.dumps(inspect(args.image),indent=2))
