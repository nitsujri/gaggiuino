#!/usr/bin/env python3
"""Test the backup decoder without hardware or modifying original backups."""
import ctypes,struct,tempfile,unittest,zlib
from pathlib import Path
from inspect_settings import inspect,schema,ROOT
class DecoderTest(unittest.TestCase):
 def test_real_v12_page_and_full_capture_agree(self):
  page=inspect(ROOT/'tmp/settings-backup/eeprom-page.bin');full=inspect(ROOT/'tmp/settings-backup/controller-flash.bin')
  self.assertEqual(page,full);self.assertEqual(page['schema_version'],12)
  self.assertEqual(page['values']['scalesF1'],3920);self.assertEqual(page['values']['scalesF2'],4210)
 def test_v13_crc_and_layout(self):
  config=schema(13)();config.profiles[0].name=b'Turbo';config.profiles[0].shotRatioX100=250
  config.tankFullDistanceMm=30;config.trayZeroRaw[0]=-123456
  values=bytes(config);raw=struct.pack('<H2xI',13,123)+values+struct.pack('<II',123^13,zlib.crc32(values))
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/'settings.bin';p.write_bytes(raw);result=inspect(p)
   self.assertTrue(result['values_crc_valid']);self.assertEqual(result['values']['profiles'][0]['shotRatioX100'],250)
   self.assertEqual(result['values']['trayZeroRaw'][0],-123456)
   damaged=bytearray(raw);damaged[100]^=1;p.write_bytes(damaged)
   with self.assertRaisesRegex(ValueError,'CRC32'):inspect(p)
   p.write_bytes(raw[:50])
   with self.assertRaises((ValueError,struct.error)):inspect(p)
   p.write_bytes(struct.pack('<H',99))
   with self.assertRaisesRegex(ValueError,'Unsupported'):inspect(p)
if __name__=='__main__':unittest.main()
