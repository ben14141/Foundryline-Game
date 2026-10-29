"""Four-player native-input door/collision/movement QA. No property/save writes.
Controlled setup teleports place the test player; measured travel uses normal
AddMovementInput through Unreal client prediction and server movement validation.
"""
import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file(): raise RuntimeError('Foundryline only')
report=root/'Saved/Logs/PlayerInteractionAcceptance.json'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
qa={'started':time.monotonic(),'stage':0,'actions':[],'samples':[],'errors':[],'completed':False,'render_mode':'NullRHI'}
def write(): report.write_text(json.dumps({k:v for k,v in qa.items() if k!='started'},indent=2))
def worlds(): return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def local_pc(w): return next(pc for pc in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if pc.is_local_controller())
def pair():
 ws=worlds();server=next(w for w in ws if unreal.GameplayStatics.get_game_mode(w))
 clients=sorted([w for w in ws if not unreal.GameplayStatics.get_game_mode(w)],key=lambda w:w.get_path_name())
 return server,clients[0]
def server_pc(server,client):
 return next(pc for pc in unreal.GameplayStatics.get_all_actors_of_class(server,unreal.PlayerController) if not pc.is_local_controller() and pc.get_controlled_pawn() and pc.get_controlled_pawn().get_class().get_name()=='FoundrylineWalkCharacter')

def door(w):
 return next(c for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Actor) for c in a.get_components_by_class(unreal.StaticMeshComponent) if c.get_class().get_name()=='FoundrylinePersonnelDoorComponent' and c.static_mesh.get_name()=='SM_Door_A1')
def place_approach(server,client):
 d=door(server);anchor=d.get_owner().get_actor_location();hinge=d.get_editor_property('relative_location')
 pos=unreal.Vector(anchor.x-240,anchor.y+hinge.y-55,anchor.z+112)
 for pc in [server_pc(server,client),local_pc(client)]:
  pc.get_controlled_pawn().set_actor_location(pos,False,True);pc.set_control_rotation(unreal.Rotator(0,0,0))
 qa['doorway_x']=anchor.x;qa['approach_x']=pos.x
 return True
def aim_open(server,client):
 d=door(server);center,extent,_=unreal.SystemLibrary.get_component_bounds(d)
 pos=unreal.Vector(center.x,center.y-140,d.get_owner().get_actor_location().z+112)
 eye=unreal.Vector(pos.x,pos.y,pos.z+80)
 facing=unreal.MathLibrary.find_look_at_rotation(eye,center)
 for pc in [server_pc(server,client),local_pc(client)]:
  pc.get_controlled_pawn().set_actor_location(pos,False,True);pc.set_control_rotation(facing)
 return True
def snapshot(label):
 server,client=pair();rows=[]
 for w in worlds():
  pc=local_pc(w);p=pc.get_controlled_pawn();pos=p.get_actor_location();d=door(w)
  rows.append({'world':w.get_path_name(),'host':bool(unreal.GameplayStatics.get_game_mode(w)),'local_controller':pc.get_name(),'pawn':p.get_class().get_name(),'location':[pos.x,pos.y,pos.z],'door_open':d.is_open(),'yaw':d.get_editor_property('relative_rotation').yaw,'diagnostics':unreal.FoundrylineNetworkTestTools.inspect_player(pc)})
 sp=server_pc(server,client).get_controlled_pawn().get_actor_location()
 qa['samples'].append({'label':label,'seconds':round(time.monotonic()-qa['started'],1),'worlds':rows,'target_server_location':[sp.x,sp.y,sp.z]});write()
def finish():
 qa['completed']=True
 if not qa['errors']:
  samples={s['label']:s for s in qa['samples']}
  def client_row(s): return next(w for w in s['worlds'] if not w['host'] and w['pawn']=='FoundrylineWalkCharacter')
  blocked=samples['closed_door_blocks'];opened=samples['open_door_crossed'];b=client_row(blocked);o=client_row(opened)
  qa['checks']={'four_players':all(len(s['worlds'])==4 for s in qa['samples']),'door_close_all_peers':all(not w['door_open'] for w in blocked['worlds']),'closed_door_blocks_client':b['location'][0]<qa['doorway_x']-20 and b['location'][0]>qa['approach_x']+50,'door_reopens_all_peers':all(w['door_open'] for w in opened['worlds']),'open_door_crossed_client':o['location'][0]>qa['doorway_x']+150,'server_received_travel':opened['target_server_location'][0]>qa['doorway_x']+150,'client_server_converged':sum((a-b)**2 for a,b in zip(o['location'],opened['target_server_location']))<25**2,'stays_on_floor':90<o['location'][2]<140,'door_yaw_matches':all(abs(w['yaw']-opened['worlds'][0]['yaw'])<0.1 for w in opened['worlds'])}
 unreal.unregister_slate_post_tick_callback(handle);write();unreal.log('FOUNDRYLINE_INTERACTION_QA_DONE')
def tick(dt):
 try:
  t=time.monotonic()-qa['started']
  if qa['stage']==0 and t>3: levels.editor_request_begin_play();qa['stage']=1;return
  if qa['stage']==0:return
  schedule=[12,16,18,22,24,28,30,32,36,38,40,44]
  i=len(qa['actions'])
  if i<len(schedule) and t>schedule[i]:
   server,client=pair();pc=local_pc(client);ok=True
   if i==0:unreal.SystemLibrary.set_timer_for_next_tick(pc,'ToggleWalk');label='Client first person'
   elif i==1:ok=aim_open(server,client);label='Set up open-leaf interaction'
   elif i==2:unreal.SystemLibrary.set_timer_for_next_tick(pc,'UseDoor');label='Client E closes door'
   elif i==3:ok=place_approach(server,client);label='Set up outside closed doorway'
   elif i==4:ok=unreal.FoundrylineNetworkTestTools.drive_player(pc,unreal.Vector(1,0,0),2);label='Native client walks toward closed door'
   elif i==5:snapshot('closed_door_blocks');label='Sample closed-door collision'
   elif i==6:unreal.SystemLibrary.set_timer_for_next_tick(pc,'UseDoor');label='Client E opens door'
   elif i==7:snapshot('reopened');label='Sample replicated open state'
   elif i==8:ok=place_approach(server,client);label='Reset approach for open-door traversal'
   elif i==9:ok=unreal.FoundrylineNetworkTestTools.drive_player(pc,unreal.Vector(1,0,0),2);label='Native client walks through open doorway'
   elif i==10:snapshot('moving');label='Sample travel'
   elif i==11:snapshot('open_door_crossed');label='Sample settled client and server'
   qa['actions'].append({'label':label,'accepted':ok,'seconds':round(t,1)});write()
  if t>48:finish()
 except Exception:
  qa['errors'].append(traceback.format_exc());unreal.log_error(qa['errors'][-1]);finish()
handle=unreal.register_slate_post_tick_callback(tick)
write()
