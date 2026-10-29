"""PIE-only real walking-capsule audit plus normal CharacterMovement travel.
No model/inventory/save mutation. Setup teleports are separate from measured input.
"""
import unreal,time,json,traceback,runpy,builtins
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
log=root/'Saved/Logs/PlayerCollisionNativeQA.json'
T=unreal.FoundrylineNetworkTestTools
q={'completed':False,'checks':{},'sweeps':[],'walks':[],'errors':[]}
start=time.monotonic();stage=0;last=0;cases=[];index=0;original=None;door=None;door_was_open=False;door_start=None;door_end=None
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
def pc():return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def v(a):return unreal.Vector(*a)
def xyz(a):return [a.x,a.y,a.z]
def probe(a,b):return json.loads(T.probe_player_collision(pc(),v(a),v(b)))
def record(n,b):q['checks'][n]=bool(b)
def write():log.write_text(json.dumps(q,indent=2))
def finish():
 if original:
  p=pc().get_controlled_pawn();p.set_actor_location(v(original['position']),False,True);pc().set_control_rotation(original['rotation']);p.get_movement_component().stop_movement_immediately()
 q['completed']=True;write();unreal.unregister_slate_post_tick_callback(handle)
 if getattr(builtins,'foundryline_collision_chain_tree_review',False):runpy.run_path(str(root/'Tools/Unreal/review_tree_collision_visual.py'))
def inventory():
 rows=[]
 for a in unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.Actor):
  for c in a.get_components_by_class(unreal.PrimitiveComponent):
   tags=[str(t) for t in c.component_tags]
   if not any('Collision' in t or 'PawnBody' in t for t in tags):continue
   if c.get_collision_enabled()==unreal.CollisionEnabled.NO_COLLISION:continue
   center,extent,_=unreal.SystemLibrary.get_component_bounds(c)
   if center.z-extent.z>=201 or center.z+extent.z<=22:continue
   rows.append((c,center,extent,tags))
 return rows
def tick(dt):
 global stage,last,cases,index,original,door,door_was_open,door_start,door_end
 try:
  elapsed=time.monotonic()-start
  if elapsed>150:raise RuntimeError('Collision native timeout stage '+str(stage))
  if stage==0:
   if not unreal.EditorLevelLibrary.get_pie_worlds(False):return
   if not pc().get_controlled_pawn() or pc().get_controlled_pawn().get_class().get_name()!='FoundrylineWalkCharacter':return
   p=pc().get_controlled_pawn();original={'position':xyz(p.get_actor_location()),'rotation':pc().get_control_rotation()};stage=1;last=elapsed
  elif stage==1 and elapsed-last>2:
   rows=inventory();record('physical_bounds_present',bool(rows))
   for c,center,extent,tags in rows:
    path=c.get_path_name();hit=None;attempts=[]
    for axis,sign in [(0,1),(0,-1),(1,1),(1,-1)]:
     a=[center.x,center.y,112];a[axis]+=sign*([extent.x,extent.y][axis]+95);b=[center.x,center.y,112]
     result=probe(a,b);attempts.append(result)
     if result['blocked'] and not result['penetrating'] and result['component']==path:hit={'component':path,'tags':tags,'start':a,'end':b,'result':result};break
    q['sweeps'].append(hit if hit else {'component':path,'tags':tags,'unreachable_from_clear_side':True,'attempts':attempts})
    if hit:
     label=path+' '.join(tags);parent=c.get_attach_parent()
     if isinstance(parent,unreal.StaticMeshComponent) and parent.static_mesh:label+=' '+parent.static_mesh.get_name()
     category=next((x for x in ['Tree','Office','Chair','Fork','Pallet','OperatorConsole','PowerUnit','Extraction','Plasma','Equipment','Furniture','Rack','Truck'] if x.lower() in label.lower()),'Other')
     if category=='Tree' and 'tree_surrounding_clearance' not in q:
      clear=probe([center.x+extent.x+55,center.y-120,112],[center.x+extent.x+55,center.y+120,112]);q['tree_surrounding_clearance']=clear;record('vegetation_beside_trunk_remains_passable',not clear['blocked'])
     if category not in [x['category'] for x in cases]:hit['category']=category;cases.append(hit)
   record('real_player_capsule_30x90',all(s.get('result',{}).get('capsule_radius',30)==30 and s.get('result',{}).get('capsule_half_height',90)==90 for s in q['sweeps']))
   record('direct_physical_hits_present',len(cases)>=3)
   desk=probe([-2750,540,112],[-2940,540,112]);q['office_desk']=desk;record('actual_office_workstation_blocks',desk['blocked'] and 'FoundrylineOperationsTerminal' in desk['actor'] and not desk['penetrating'])
   if q['checks']['actual_office_workstation_blocks']:cases.append({'category':'OfficeDesk','start':[-2750,540,112],'end':[-2940,540,112],'component':desk['component']})
   clear_a=[-500,1750,112];clear_b=[-500,2050,112];clear=probe(clear_a,clear_b);q['clear_aisle']=clear;record('shop_aisle_no_invisible_wall',clear['valid'] and not clear['blocked'])
   for name,a,b,blocked in [('shop_wall',[-1000,-150,112],[-1000,150,112],True),('office_exterior',[-3350,1300,112],[-3050,1300,112],True),('office_partition',[-2700,650,112],[-2700,950,112],True),('office_corridor',[-2500,500,112],[-2500,1300,112],False)]:
    result=probe(a,b);q[name]=result;record(name+'_physical_clearance',result['valid'] and result['blocked']==blocked and not result['penetrating'])
   stage=2;last=elapsed;write()
  elif stage==2:
   if index>=len(cases):stage=4;return
   item=cases[index];p=pc().get_controlled_pawn();p.get_movement_component().stop_movement_immediately();p.set_actor_location(v(item['start']),False,True);pc().set_control_rotation(unreal.MathLibrary.find_look_at_rotation(v(item['start']),v(item['end'])));T.hold_camera_key(pc(),'W',1.6);stage=3;last=elapsed
  elif stage==3:
   item=cases[index];p=pc().get_controlled_pawn();delta=[b-a for a,b in zip(item['start'],item['end'])];length=sum(x*x for x in delta)**.5;direction=v([x/length for x in delta])
   if elapsed-last>1.9:
    at=p.get_actor_location();remaining=sum((x-y)**2 for x,y in zip(xyz(at),item['end']))**.5;crossed=sum((x-y)*d for x,y,d in zip(xyz(at),item['end'],xyz(direction)))>0
    q['walks'].append({'category':item['category'],'component':item['component'],'start':item['start'],'target':item['end'],'reached':xyz(at),'crossed':crossed,'remaining':remaining});record('normal_walking_blocks_'+item['category'],not crossed and remaining>=20);p.get_movement_component().stop_movement_immediately();index+=1;stage=2;write()
  elif stage==4:
   doors=[c for a in unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.Actor) for c in a.get_components_by_class(unreal.StaticMeshComponent) if c.get_class().get_name()=='FoundrylinePersonnelDoorComponent']
   record('personnel_door_present',bool(doors))
   if not doors:finish();return
   door=doors[0];door_was_open=door.is_open();center,extent,_=unreal.SystemLibrary.get_component_bounds(door)
   # Close via the normal owning-controller interaction; do not mutate the leaf.
   p=pc().get_controlled_pawn();normal=[1,0,0] if extent.x<extent.y else [0,1,0];eye=[center.x-normal[0]*140,center.y-normal[1]*140,192];p.set_actor_location(v([eye[0],eye[1],112]),False,True);pc().set_control_rotation(unreal.MathLibrary.find_look_at_rotation(v(eye),center))
   stage=41;last=elapsed
  elif stage==41 and elapsed-last>1:
   q['door_before_close']={'selected':door.get_path_name(),'open':door.is_open(),'view':T.inspect_player(pc())}
   if door.is_open():unreal.SystemLibrary.set_timer_for_next_tick(pc(),'UseDoor')
   stage=5;last=elapsed
  elif stage==5 and elapsed-last>2:
   record('door_closed_by_normal_interaction',not door.is_open());center,extent,_=unreal.SystemLibrary.get_component_bounds(door);normal=[1,0,0] if extent.x<extent.y else [0,1,0];door_start=[center.x-normal[0]*150,center.y-normal[1]*150,112];door_end=[center.x+normal[0]*80,center.y+normal[1]*80,112];closed=probe(door_start,door_end);q['closed_door']=closed;record('closed_door_real_capsule_blocks',closed['blocked'] and closed['component']==door.get_path_name());p=pc().get_controlled_pawn();p.set_actor_location(v(door_start),False,True);pc().set_control_rotation(unreal.MathLibrary.find_look_at_rotation(v([door_start[0],door_start[1],192]),center));stage=51;last=elapsed
  elif stage==51 and elapsed-last>1:unreal.SystemLibrary.set_timer_for_next_tick(pc(),'UseDoor');stage=6;last=elapsed
  elif stage==6 and elapsed-last>2:
   q['door_after_open']={'open':door.is_open(),'view':T.inspect_player(pc())}
   opened=probe(door_start,door_end);q['open_door']=opened;record('open_door_releases_actual_opening',door.is_open() and not opened['blocked']);p=pc().get_controlled_pawn();p.set_actor_location(v(door_start),False,True);delta=[b-a for a,b in zip(door_start,door_end)];length=sum(x*x for x in delta)**.5;T.drive_player(pc(),v([x/length for x in delta]),2.5);stage=7;last=elapsed
  elif stage==7 and elapsed-last>3:
   at=xyz(pc().get_controlled_pawn().get_actor_location());delta=[b-a for a,b in zip(door_start,door_end)];record('normal_walking_passes_open_door',sum((x-y)*d for x,y,d in zip(at,door_end,delta))>0);finish()
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
