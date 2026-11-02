import pathlib, struct, zlib, os

def mkimage(script):
 data=script.encode('ascii'); payload=struct.pack('>II',len(data),0)+data
 fields=[0x27051956,0,int(os.environ.get('SOURCE_DATE_EPOCH', '0')),len(payload),0,0,zlib.crc32(payload),5,2,6,0,b'C10H USB one-shot'.ljust(32,b'\0')]
 header=struct.pack('>7I4B32s',*fields);fields[1]=zlib.crc32(header)
 return struct.pack('>7I4B32s',*fields)+payload

if __name__ == '__main__':
 import argparse
 p=argparse.ArgumentParser(description='Build a U-Boot legacy script; writes a regular file only')
 p.add_argument('input');p.add_argument('output');args=p.parse_args()
 src=pathlib.Path(args.input);dst=pathlib.Path(args.output)
 if src.resolve()==dst.resolve():p.error('Use a separate output file')
 script=src.read_text(encoding='ascii').replace('\r\n','\n')
 if not script.endswith('\n'):script+='\n'
 dst.write_bytes(mkimage(script));print('Wrote',dst)
