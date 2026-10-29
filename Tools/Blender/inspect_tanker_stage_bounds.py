from pathlib import Path
exec((Path(__file__).parent/'split_tanker_production_stages.py').read_text().split('entries=[]')[0])
for stage in range(4):
 points=[me.vertices[i].co for p in faces[stage] for i in p.vertices]
 print('STAGE_BOUNDS',stage,[(min(p[k] for p in points),max(p[k] for p in points)) for k in range(3)])
unique={(tuple(lo),tuple(hi)) for stage,lo,hi in group.values() if stage==1}
for lo,hi in sorted(unique,key=lambda b:b[1][0]-b[0][0],reverse=True)[:8]:print('SHELL_COMPONENT',lo,hi)
