import pathlib, struct

def disable_emmc(b):
 h=list(struct.unpack('>10I', b[:40])); strings=b[h[3]:h[3]+h[8]]
 pos=h[2]; stack=[]; out=bytearray(); changed=0
 while True:
  start=pos; token=struct.unpack_from('>I',b,pos)[0];pos+=4
  if token==1:
   end=b.index(0,pos);stack.append(b[pos:end].decode());pos=(end+4)&~3
  elif token==2:stack.pop()
  elif token==3:
   n,off=struct.unpack_from('>II',b,pos);pos+=8
   name=strings[off:strings.index(0,off)].decode(); val=b[pos:pos+n];pos=(pos+n+3)&~3
   if '/'.join(stack)=='/soc/mmc@ffe07000' and name=='status':
    assert val==b'okay\0';val=b'disabled\0'; changed+=1
    out.extend(struct.pack('>III',3,len(val),off)+val+b'\0'*((-len(val))%4));continue
  elif token not in (4,9):raise ValueError(token)
  out.extend(b[start:pos])
  if token==9:break
 assert changed==1
 result=bytearray(b[:h[2]])+out+strings
 h[1]=len(result);h[3]=h[2]+len(out);h[9]=len(out)
 result[:40]=struct.pack('>10I',*h)
 return bytes(result)

if __name__ == '__main__':
 import argparse
 p=argparse.ArgumentParser(description='Disable only the eMMC node in the E900V22C baseline DTB')
 p.add_argument('input');p.add_argument('output');args=p.parse_args()
 src=pathlib.Path(args.input);dst=pathlib.Path(args.output)
 if src.resolve()==dst.resolve():p.error('Use a separate output file')
 data=disable_emmc(src.read_bytes());dst.write_bytes(data)
 print('Wrote',dst,len(data),'bytes')
