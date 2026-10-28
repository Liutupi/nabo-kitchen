"""Generate the 24 kHz Opus alarm used when XiaoZhi owns the speaker."""
import argparse
import math
from pathlib import Path
import struct
import subprocess
import tempfile
import wave

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--ffmpeg',default='ffmpeg');parser.add_argument('--volume',type=int,default=100);args=parser.parse_args()
    if not 0 <= args.volume <= 100: parser.error('volume must be between 0 and 100')
    rate=24000;samples=[]
    for i in range(rate*105//100):
        within=i%(rate*35//100);envelope=min(1,within/(rate*.01),max(0,(rate*.20-within)/(rate*.01))) if within<rate*.20 else 0
        value=(math.sin(2*math.pi*2800*i/rate)*0.11+math.sin(2*math.pi*3300*i/rate)*0.09)*envelope*args.volume/100
        samples.append(struct.pack('<h',round(value*32767)))
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        source=Path(tmp)/'alarm.wav'
        with wave.open(str(source),'wb') as wav:
            wav.setnchannels(1);wav.setsampwidth(2);wav.setframerate(rate);wav.writeframes(b''.join(samples))
        subprocess.run([args.ffmpeg,'-v','error','-y','-i',str(source),'-c:a','libopus','-b:a','24k','-application','audio',str(args.output)],check=True)
    print(f'Wrote {args.output.stat().st_size} bytes to {args.output}')
if __name__=='__main__':main()
