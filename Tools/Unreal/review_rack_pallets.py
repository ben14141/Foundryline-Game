"""Eye-level rendered review of pallets on the starter shop's storage racks (no saves). Screenshots go to
Saved/Screenshots/Reviews with the prefix given by FOUNDRYLINE_SHOT_PREFIX (default Storage_)."""
import unreal,time,json,os,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools
base=Path(unreal.Paths.project_saved_dir());shots=base/'Screenshots/Reviews';out=base/'Logs/RackPalletsVisualQA.json'
prefix=os.environ.get('FOUNDRYLINE_SHOT_PREFIX','Storage_')
start=time.monotonic();q={'shots':[],'errors':[]};cam=None;handle=None;stage='start';last=0;view=0
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def player():return next(a for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if a.is_local_controller())
def look(p,loc,target,fov=60):
 global cam
 if not cam:cam=T.create_review_camera(p)
 cam.set_actor_location_and_rotation(loc,unreal.MathLibrary.find_look_at_rotation(loc,target),False,False);cam.camera_component.set_field_of_view(fov);p.set_view_target_with_blend(cam,0)
# Every rack is found from its shelf meshes in the running world; each is shot from its fork side at eye level (165 cm)
# and from just above a shelf edge, where a gap under a pallet shows best.
V=[]
def racks():
 out={}
 for w in worlds():
  for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Actor):
   for c in a.get_components_by_class(unreal.StaticMeshComponent):
    m=c.static_mesh
    if m and m.get_name() in ('SM_Rack_Shelf400','SM_PalletRack_1Level','SM_PalletRack_2Levels','SM_PalletRack_3Levels'):
     l=c.get_world_location();k=(round(l.x/50),round(l.y/50));r=c.get_world_rotation();out.setdefault(k,{'loc':l,'yaw':r.yaw,'levels':[]})['levels'].append(l.z)
  break
 return list(out.values())
def plan():
 for i,r in enumerate(racks()):
  import math
  y=math.radians(r['yaw']);fx,fy=math.cos(y),math.sin(y);sx,sy=-fy,fx;L=r['loc'];base=min(r['levels'])
  V.append(('Rack%d_EyeLevel'%i,(L.x+fx*380,L.y+fy*380+60,base+125),(L.x,L.y,base+110),62))
  for j,z in enumerate(sorted(r['levels'])):V.append(('Rack%d_Level%d_ShelfEdge'%(i,j),(L.x+fx*140+sx*150,L.y+fy*140+sy*150,z+26),(L.x,L.y+sy*40,z+22),45))
 q['racks']=[{'loc':[r['loc'].x,r['loc'].y,r['loc'].z],'yaw':r['yaw'],'levels':sorted(r['levels'])} for r in racks()]
def tick(dt):
 global stage,last,view
 try:
  t=time.monotonic()-start
  if t>200:raise RuntimeError('timeout at '+stage)
  if stage=='start' and t>3:stage='pie';last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage=='pie' and t-last>14:
   p=player();T.queue_property_view(p);q['stock']=json.loads(T.stock_rack_review(p));stage='plan';last=t
  elif stage=='plan' and t-last>3:plan();stage='view';last=t
  elif stage=='view':
   if view>=len(V):stage='done';last=t
   elif t-last>4 and not q.get('pending'):n,c,g,f=V[view];look(player(),unreal.Vector(*c),unreal.Vector(*g),f);q['pending']=n;last=t
   elif q.get('pending') and t-last>2.5:n=q.pop('pending');view+=1;last=t;path=str(shots/(prefix+n+'.png'));q['shots'].append(path);unreal.AutomationLibrary.take_high_res_screenshot(1920,1080,path)
  elif stage=='done' and t-last>2:
   out.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR');return
  out.write_text(json.dumps(q,indent=1))
 except Exception:
  q['errors'].append(traceback.format_exc());out.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
handle=unreal.register_slate_post_tick_callback(tick)
