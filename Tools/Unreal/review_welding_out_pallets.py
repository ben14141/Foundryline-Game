"""Single-player rendered review of the welding station's OUT pallets: the station is placed through the real controller
commands, then its two OUT pallets are given 1, 5 and 10 finished racks (SetWeldingOutStacks review fixture) and shot from
outside the OUT end, at eye height from the forklift side, and with the station panel showing OUT full.
Screenshots go to Saved/Screenshots/Reviews; the report goes to Saved/Logs/WeldingOutPalletsVisualQA.json. No save slots."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools
base=Path(unreal.Paths.project_saved_dir());shots=base/'Screenshots/Reviews';out=base/'Logs/WeldingOutPalletsVisualQA.json'
start=time.monotonic();q={'checks':{},'notes':{},'shots':[],'errors':[]};cam=None;handle=None
CENTER=unreal.Vector(-1768.02,1823.12,20)
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def player():return next(a for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if a.is_local_controller())
def look(p,loc,target,fov=60):
 global cam
 if not cam:cam=T.create_review_camera(p)
 cam.set_actor_location_and_rotation(loc,unreal.MathLibrary.find_look_at_rotation(loc,target),False,False);cam.camera_component.set_field_of_view(fov);p.set_view_target_with_blend(cam,0)
def shot(name):
 path=str(shots/(name+'.png'));unreal.AutomationLibrary.take_high_res_screenshot(1920,1080,path);q['shots'].append(path)
def report(p):
 r=json.loads(T.welding_station_report(p));return r['stations'][0] if r['stations'] else None
def at(s,dx,dy,z):return unreal.Vector(s['x']+dx,s['y']+dy,z)
# OUT is the +X end of an unturned station: pallets on the 2 ft fork line (+236 cm), 2 ft either side of the centre line.
SHOTS=[
 ((1,0),'WeldingOut_1Rack',(560,-360,270),(205,-20,50),55),
 ((5,0),'WeldingOut_5Racks',(560,-360,270),(205,-20,80),55),
 ((5,5),'WeldingOut_10Racks',(560,-360,270),(205,0,80),55),
 ((5,5),'WeldingOut_10Racks_ForkSide',(560,-40,150),(220,0,80),62),
 ((5,5),'WeldingOut_10Racks_FromBench',(-40,-390,250),(205,0,70),60),
 ((5,5),'WeldingOut_10Racks_Panel',(620,-650,430),(0,0,40),70),
]
spot=CENTER;stage='start';last=0;index=0;sid=None
def tick(dt):
 global stage,last,spot,index,sid
 try:
  t=time.monotonic()-start
  if t>300:raise RuntimeError('OUT pallet review timeout at '+stage)
  if stage=='start' and t>3:stage='pie';last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage=='pie' and t-last>14:
   p=player();T.queue_property_view(p)
   T.queue_factory_lane(p,0,unreal.Vector(-2217.6,1275,20),unreal.Vector(-2217.6,2170,20),304.8)
   T.queue_factory_lane(p,1,unreal.Vector(-1455,1214.04,20),unreal.Vector(-810,1214.04,20),121.92)
   T.queue_factory_lane(p,2,unreal.Vector(-1370,1450,20),unreal.Vector(-1370,1450,20),200);stage='place';last=t
  elif stage=='place' and t-last>4:
   p=player();best=None
   for dx in [0,1,-1,2,-2]:
    for dy in [0,1,-1,2,-2]:
     c=unreal.Vector(CENTER.x+dx*15.24,CENTER.y+dy*15.24,20)
     if json.loads(T.preview_welding_station(p,c,0,False))['valid']:best=c;break
    if best:break
   spot=best or CENTER;q['notes']['commit']=json.loads(T.preview_welding_station(p,spot,0,False,True));T.queue_factory_action(p,'ToolStation');stage='placed';last=t
  elif stage=='placed' and t-last>3:
   s=report(player());q['checks']['station_placed']=bool(s)
   if not s:raise RuntimeError('Station was not placed')
   sid=s['id'];q['notes']['station']=s;stage='set';last=t
  elif stage=='set':
   if index>=len(SHOTS):stage='done';last=t
   else:
    p=player();stacks,name,c,g,f=SHOTS[index];s=report(p)
    q['checks']['stacks_valid_%d_%d'%stacks]=T.set_welding_out_stacks(p,sid,stacks[0],stacks[1]);look(p,at(s,*c),at(s,*g),f);stage='shoot';last=t
  # Advance first: the screenshot call can tick Slate again before it returns.
  elif stage=='shoot' and t-last>2.8:
   p=player();s=report(p);name=SHOTS[index][1];q['notes'][name]={'out':s['out'],'supply':s['supply']};index+=1;stage='after';last=t;shot(name)
  elif stage=='after' and t-last>1.5:stage='set';last=t
  elif stage=='done' and t-last>1:
   q['checks']['ten_racks_reported']=q['notes']['WeldingOut_10Racks']['out']==10;q['checks']['status_counts_ten']='10 finished on OUT' in q['notes']['WeldingOut_10Racks_Panel']['supply'] or 'OUT full (10 on 2 pallets)' in q['notes']['WeldingOut_10Racks_Panel']['supply']
   q['checks']['all_shot']=len(q['shots'])==len(SHOTS);q['completed']=True;out.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR');return
  q['stage']=stage;out.write_text(json.dumps(q,indent=1))
 except Exception:
  q['errors'].append(traceback.format_exc());out.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
handle=unreal.register_slate_post_tick_callback(tick)
