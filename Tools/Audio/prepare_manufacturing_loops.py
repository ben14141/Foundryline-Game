"""Prepare private, calibrated mono loops from the licensed Manufacturing Ambiences pack.
Original vendor assets are unchanged. Derived WAVs must not be published in the public repo.
"""
import json,sys,warnings
from pathlib import Path
import numpy as np
from scipy.io import wavfile
from scipy.signal import butter,sosfiltfilt
warnings.filterwarnings('ignore',category=wavfile.WavFileWarning)
ROOT=Path(__file__).resolve().parents[2]
INPUT=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'Saved/AudioReview/Manufacturing'
OUT=ROOT/'SourceAssets/Audio/ManufacturingDerived';OUT.mkdir(parents=True,exist_ok=True)
rows=json.loads((INPUT/'manifest.json').read_text());sr=48000

def read(number,ambience=False):
 match=('Ambience_'+str(number)+'_') if ambience else ('Working_'+str(number)+'_')
 r=next(r for r in rows if match in Path(r['file']).name)
 rate,x=wavfile.read(r['file']);assert rate==sr
 scale=2147483648 if x.dtype==np.int32 else 32768 if x.dtype==np.int16 else 1
 x=x.astype(np.float64)/scale
 if x.ndim>1:x=x.mean(axis=1)
 return x,r

def filter_audio(x,low,high):
 return sosfiltfilt(butter(3,[low,high],btype='bandpass',fs=sr,output='sos'),x)

def stable(x,seconds=9):
 n=int(seconds*sr);best=None
 for start in range(sr,max(sr+1,len(x)-n),sr):
  a=x[start:start+n]
  if len(a)!=n:continue
  e=np.sqrt(np.mean(a[:len(a)//2400*2400].reshape(-1,2400)**2,axis=1)+1e-15)
  score=np.std(e)/(np.mean(e)+1e-10)
  if best is None or score<best[0]:best=(score,a)
 return (best[1] if best else np.resize(x,n)).copy()

def loop(x):
 n=int(.12*sr);w=np.linspace(0,1,n);out=x[n:].copy();out[-n:]=x[-n:]*(1-w)+x[:n]*w;return out

profiles=[('WeldingArc',14,250,6500,.55),('PlasmaCut',13,700,7500,.45),('PlasmaGantry',24,60,1600,.18),('BrakeHydraulics',1,90,2200,.45),('SheetRoller',10,60,1800,.40),('ForkliftDrive',24,55,1400,.28),('ForkliftHydraulics',3,180,2500,.24),('CollectorCart',7,500,3400,.09),('RoomAmbience',9,80,2200,.035)]
report=[]
for name,number,low,high,gain in profiles:
 x,source=read(number,name=='RoomAmbience');x=stable(filter_audio(x,low,high))
 if name=='WeldingArc':
  # Shape the recorded broadband texture into a restrained intermittent arc layer.
  t=np.arange(len(x))/sr;rng=np.random.default_rng(207)
  ripple=.82+.18*np.sin(2*np.pi*120*t)
  clicks=rng.random(len(x))<.004;grains=np.convolve(clicks.astype(float),np.exp(-np.arange(240)/45),mode='same')
  x=x*ripple*(.7+np.minimum(grains,2)*.45)
 # Control rare transient peaks before normalization so steady operation stays audible.
 raw_rms=max(float(np.sqrt(np.mean(x*x))),1e-10)
 x=filter_audio(2.5*raw_rms*np.tanh(x/(2.5*raw_rms)),low,high)
 x=loop(x);x-=x.mean();rms=float(np.sqrt(np.mean(x*x)))
 x*=min(10**(-20/20)/max(rms,1e-10),.35/max(float(abs(x).max()),1e-10))
 pcm=np.round(np.clip(x,-.999,.999)*32767).astype(np.int16);wavfile.write(OUT/(name+'.wav'),sr,pcm)
 report.append({'role':name,'source_asset':source['asset'],'source_number':number,'duration':len(x)/sr,'rms_dbfs':float(20*np.log10(np.sqrt(np.mean(x*x))+1e-12)),'peak_dbfs':float(20*np.log10(abs(x).max()+1e-12)),'loop':True,'base_gain':gain,'eq_hz':[low,high],'note':'Processed industrial texture for welding; not a labeled welding field recording' if name=='WeldingArc' else 'Filtered factory recording'})
# Brief contact accent at the press stroke, not a repeating banging loop.
x,source=read(23);x=filter_audio(x,150,4500);window=2400;env=np.mean(x[:len(x)//window*window].reshape(-1,window)**2,axis=1);peak=int(np.argmax(env))*window
start=max(0,peak-int(.04*sr));x=np.resize(x[start:start+int(.65*sr)],int(.65*sr));x[:480]*=np.linspace(0,1,480);x[-2400:]*=np.linspace(1,0,2400);x*=min(.09/max(float(abs(x).max()),1e-8),10**(-29/20)/max(float(np.sqrt(np.mean(x*x))),1e-8))
wavfile.write(OUT/'BrakeContact.wav',sr,np.round(x*32767).astype(np.int16));report.append({'role':'BrakeContact','source_asset':source['asset'],'loop':False,'duration':.65,'peak_dbfs':float(20*np.log10(abs(x).max()+1e-12)),'base_gain':.24})
(OUT/'manifest.json').write_text(json.dumps({'pack':'Manufacturing Ambiences – Factory Sounds Collection','vendor_assets_modified':False,'profiles':report},indent=2))
print(json.dumps(report,indent=2))
