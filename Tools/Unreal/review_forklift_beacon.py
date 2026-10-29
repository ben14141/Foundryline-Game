"""Forklift upgrade-kit review: a steel purchase books a truck; the forklift unloads it and lifts it into the raw-steel rack.
Every peer reports each forklift's beacon light (present, at the bulb, amber, on) and carriage lift. With one player the
run is rendered and screenshots go to Saved/Screenshots/Reviews (Forklift_*); with more it checks the client's view.
Report: Saved/Logs/ForkliftBeaconQA.json. No save slots."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools
base=Path(unreal.Paths.project_saved_dir());shots=base/'Screenshots/Reviews';out=base/'Logs/ForkliftBeaconQA.json'
start=time.monotonic();stage='start';last=0;q={'checks':{},'notes':{},'shots':[],'errors':[],'max_lift':{}};cam=None;handle=None;taken=set()
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def players():return [p for w in worlds() for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller()]
def host():return next(p for p in players() if p.has_authority())
def report(p):return json.loads(T.forklift_beacon_report(p))
def body(p):
 for a in unreal.GameplayStatics.get_all_actors_of_class(p.get_world(),unreal.FoundrylineFactoryPlant):
  for c in a.get_components_by_class(unreal.StaticMeshComponent):
   m=c.get_editor_property('static_mesh')
   if m and m.get_name()=='SM_Forklift_Body' and c.get_attach_parent().is_visible():return c
def look(p,loc,target,fov=55):
 global cam
 if not cam:cam=T.create_review_camera(p)
 cam.set_actor_location_and_rotation(loc,unreal.MathLibrary.find_look_at_rotation(loc,target),False,False);cam.camera_component.set_field_of_view(fov);p.set_view_target_with_blend(cam,0)
def shot(name):
 if name in taken or len(players())>1:return
 taken.add(name);path=str(shots/('Forklift_'+name+'.png'));unreal.AutomationLibrary.take_high_res_screenshot(1920,1080,path);q['shots'].append(path)
def finish():
 q['completed']=True;out.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
def tick(dt):
 global stage,last
 try:
  t=time.monotonic()-start
  if t>420:raise RuntimeError('Forklift review timeout at '+stage)
  if stage=='start' and t>3:stage='pie';last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage=='pie' and t-last>14:
   ps=players();q['notes']['players']=len(ps);r=[report(p) for p in ps];q['notes']['idle']=r
   q['checks']['every_peer_has_a_beacon_light_at_the_bulb']=all(f['light'] and f['at_bulb'] and f['blue'] for x in r for f in x['forklifts'] if f['shown']) and all(any(f['shown'] for f in x['forklifts']) for x in r)
   q['checks']['shown_beacons_are_on']=all(f['light_on'] for x in r for f in x['forklifts'] if f['shown'])
   q['checks']['steel_staged']=T.start_forklift_lift_review(host());stage='run';last=t
  elif stage=='run':
   for i,p in enumerate(players()):
    for f in report(p)['forklifts']:q['max_lift'][str(i)]=max(q['max_lift'].get(str(i),0),f['lift'])
   if q['max_lift'].get('0',0)<5:T.advance_factory_test(host(),3)
   b=body(players()[0])
   if b and len(players())==1:
    loc=b.get_world_location();fwd=b.get_forward_vector();look(players()[0],loc-fwd*520+unreal.Vector(0,260,300),loc+unreal.Vector(0,0,120),55)
    if t-last>6:shot('Driving')
    lift=q['max_lift'].get('0',0)
    if lift>40:shot('LiftingPallet')
   if t-last>8 and q['max_lift'] and all(v>40 for v in q['max_lift'].values()) or t-last>200:stage='beacon';last=t
  elif stage=='beacon':
   b=body(players()[0])
   if b and len(players())==1:
    loc=b.get_world_location()+unreal.Vector(0,0,263);fwd=b.get_forward_vector()
    if t-last<4:look(players()[0],loc-fwd*380+unreal.Vector(0,0,-100),loc,45)
    elif t-last<8:look(players()[0],loc-fwd*1500+unreal.Vector(400,400,-90),loc,40)
   if t-last>3:shot('Beacon_EyeLevel')
   if t-last>7:shot('Beacon_AcrossShop');stage='done';last=t
  elif stage=='done' and t-last>2:
   r=[report(p) for p in players()];q['notes']['final']=r
   q['checks']['forklift_lifted_pallet_on_every_peer']=len(q['max_lift'])==len(players()) and all(v>40 for v in q['max_lift'].values())
   q['checks']['client_sees_same_beacon']=len(players())==1 or all(any(f['shown'] and f['light_on'] and f['at_bulb'] for f in x['forklifts']) for x in r)
   finish();return
  q['stage']=stage;out.write_text(json.dumps(q,indent=1))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
