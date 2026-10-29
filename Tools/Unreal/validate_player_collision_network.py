"""Four-peer actual character collision/prediction acceptance; no player saves."""
import unreal,time,json,traceback
from pathlib import Path
T=unreal.FoundrylineNetworkTestTools
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
out=root/'Saved/Logs/PlayerCollisionNetworkQA.json'
q={'checks':{},'errors':[],'sweeps':[],'walks':[],'completed':False}
start=time.monotonic();last=0;stage=0;cases=[];index=0;target=None
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def players():return [p for w in worlds() for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller()]
def v(a):return unreal.Vector(*a)
def xyz(a):return [a.x,a.y,a.z]
def write():out.write_text(json.dumps(q,indent=2))
def check(n,b):q['checks'][n]=bool(b);write()
def finish():
 q['completed']=True;write();unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play();unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def targets(p):
 rows=[]
 for actor in unreal.GameplayStatics.get_all_actors_of_class(p.get_world(),unreal.Actor):
  for c in actor.get_components_by_class(unreal.PrimitiveComponent):
   tags=[str(t) for t in c.component_tags]
   if not any('Collision' in t or 'PawnBody' in t for t in tags) or c.get_collision_enabled()==unreal.CollisionEnabled.NO_COLLISION:continue
   center,extent,_=unreal.SystemLibrary.get_component_bounds(c)
   if center.z-extent.z>=201 or center.z+extent.z<=22:continue
   rows.append((c,center,extent,tags))
 return rows
def tick(dt):
 global stage,last,cases,index,target
 try:
  t=time.monotonic()-start
  if t>180:raise RuntimeError('Network collision timeout stage '+str(stage))
  if stage==0 and t>3:levels.editor_request_begin_play();stage=1;last=t
  elif stage==1 and t-last>16:
   ps=players();check('four_live_players',len(ps)==4);check('all_first_person',all(p.get_controlled_pawn().get_class().get_name()=='FoundrylineWalkCharacter' for p in ps));host=next(p for p in ps if unreal.GameplayStatics.get_game_mode(p.get_world()));target=next(p for p in ps if p!=host)
   # Place observers outside the test property so their real capsules do not
   # falsely masquerade as machinery collision in another player's sweep.
   for i,p in enumerate(ps):T.place_player_collision_review(p,unreal.Vector(6000+i*200,0,112),0)
   counts=[len(targets(p)) for p in ps];q['active_bound_counts']=counts;check('physical_bounds_on_server_and_clients',min(counts)>5 and len(set(counts))==1)
   for c,center,extent,tags in targets(host):
    name=c.get_path_name()+' '.join(tags);parent=c.get_attach_parent()
    if isinstance(parent,unreal.StaticMeshComponent) and parent.static_mesh:name+=' '+parent.static_mesh.get_name()
    category=next((n for n in ['Tree','Office','Chair','Fork','OperatorConsole','PowerUnit','Extraction','Plasma','Equipment','Rack','Furniture'] if n.lower() in name.lower()),'Other')
    if category in [x['category'] for x in cases]:continue
    for axis,sign in [(0,1),(0,-1),(1,1),(1,-1)]:
     a=[center.x,center.y,112];a[axis]+=sign*([extent.x,extent.y][axis]+95);b=[center.x,center.y,112];hit=json.loads(T.probe_player_collision(host,v(a),v(b)))
     if hit['blocked'] and not hit['penetrating'] and hit['component']==c.get_path_name():cases.append({'category':category,'start':a,'end':b});break
   check('at_least_three_actual_object_categories',len(cases)>=3)
   cases.append({'category':'OfficeDesk','start':[-2750,540,112],'end':[-2940,540,112]})
   for p in ps:
    for item in cases:
     hit=json.loads(T.probe_player_collision(p,v(item['start']),v(item['end'])));q['sweeps'].append({'world':p.get_world().get_path_name(),'category':item['category'],'hit':hit});check(p.get_world().get_path_name()+'_'+item['category']+'_blocks',hit['blocked'] and not hit['penetrating'])
    clear=json.loads(T.probe_player_collision(p,unreal.Vector(-500,1750,112),unreal.Vector(-500,2050,112)));check(p.get_world().get_path_name()+'_clear_aisle',not clear['blocked'])
   stage=2;last=t
  elif stage==2:
   if index>=len(cases):finish();return
   item=cases[index];delta=[b-a for a,b in zip(item['start'],item['end'])];length=sum(x*x for x in delta)**.5;direction=v([x/length for x in delta]);yaw=unreal.MathLibrary.find_look_at_rotation(v(item['start']),v(item['end'])).yaw;check('place_'+item['category'],T.place_player_collision_review(target,v(item['start']),yaw));stage=3;last=t
  elif stage==3 and t-last>.75:
   item=cases[index];T.hold_camera_key(target,'W',1.8);stage=4;last=t
  elif stage==4 and t-last>3:
   item=cases[index];at=xyz(target.get_controlled_pawn().get_actor_location());authority=json.loads(T.authoritative_player_collision_position(target));server=authority['position'];delta=[b-a for a,b in zip(item['start'],item['end'])];crossed=sum((x-y)*d for x,y,d in zip(at,item['end'],delta))>0;server_crossed=sum((x-y)*d for x,y,d in zip(server,item['end'],delta))>0;travel=sum((x-y)**2 for x,y in zip(at,item['start']))**.5;distance=sum((a-b)**2 for a,b in zip(at,server))**.5;q['walks'].append({'category':item['category'],'client':at,'server':server,'travel':travel,'agreement_cm':distance});check('actual_client_movement_blocks_'+item['category'],not crossed and travel>10);check('authoritative_movement_blocks_'+item['category'],not server_crossed and authority['found']);check('client_server_converge_'+item['category'],distance<25);index+=1;stage=2;write()
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
