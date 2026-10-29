"""Single-player rendered review of two-welder assembly (Phase 4): two welders are hired and given Welder A and Welder B
through the real employee commands; both weld one rack at 170%, then Welder B is released mid-unit and walks away while
Welder A carries on at 100%. Screenshots go to Saved/Screenshots/Reviews; the report goes to
Saved/Logs/WeldingTwoWelderVisualQA.json. No save slots."""
import unreal,time,json,traceback,os
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools
base=Path(unreal.Paths.project_saved_dir());shots=base/'Screenshots/Reviews';out=base/'Logs/WeldingTwoWelderVisualQA.json'
start=time.monotonic();q={'checks':{},'notes':{},'shots':[],'errors':[],'samples':[]};cam=None;handle=None
CENTER=unreal.Vector(-1806.12,1823.12,20)
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
# FOUNDRYLINE_VIEW=client: two rendered players; setup runs on the host, the camera and screenshots on the client.
CLIENT=os.environ.get('FOUNDRYLINE_VIEW')=='client'
def controllers():return [a for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if a.is_local_controller()]
def host():return next(a for a in controllers() if a.has_authority())
def player():return next((a for a in controllers() if not a.has_authority()),host()) if CLIENT else host()
def make_guid(text):
 g=unreal.Guid()
 for n,i in zip('abcd',range(0,32,8)):g.set_editor_property(n,int(text.replace('-','')[i:i+8],16))
 return g
def look(p,loc,target,fov=60):
 global cam
 if not cam:cam=T.create_review_camera(p)
 cam.set_actor_location_and_rotation(loc,unreal.MathLibrary.find_look_at_rotation(loc,target),False,False);cam.camera_component.set_field_of_view(fov);p.set_view_target_with_blend(cam,0)
taken=set()
def shot(name):
 if name in taken:return
 taken.add(name);name=('Client_' if CLIENT else '')+name;path=str(shots/(name+'.png'));q['shots'].append(path)
 if CLIENT:unreal.SystemLibrary.execute_console_command(player().get_world(),'HighResShot 1600x900 filename="'+path+'"',player())
 else:unreal.AutomationLibrary.take_high_res_screenshot(1920,1080,path)
def report(p):
 r=json.loads(T.welding_station_report(p));return r['stations'][0] if r['stations'] else None
def staff(p):return json.loads(T.employee_report(p))
def at(s,dx,dy,z):return unreal.Vector(s['x']+dx,s['y']+dy,z)
OVER=((430,-230,330),(0,0,100),58)
WIDE=((520,-120,420),(-40,120,60),66)
spot=CENTER;stage='start';last=0;welders=[];plant=None;left=None
def tick(dt):
 global stage,last,spot,welders,plant,left
 try:
  t=time.monotonic()-start
  if t>520:raise RuntimeError('Two-welder review timeout at '+stage)
  if stage=='start' and t>3:stage='pie';last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage=='pie' and t-last>14:
   # Lanes and the station go in from the host; hiring, assignment and the view come from the reviewing player.
   T.queue_property_view(player());p=host();T.queue_property_view(p)
   T.queue_factory_lane(p,0,unreal.Vector(-2217.6,1275,20),unreal.Vector(-2217.6,2170,20),304.8)
   T.queue_factory_lane(p,1,unreal.Vector(-1455,1214.04,20),unreal.Vector(-810,1214.04,20),121.92)
   T.queue_factory_lane(p,2,unreal.Vector(-1394,1450,20),unreal.Vector(-1394,1450,20),200);stage='place';last=t
  elif stage=='place' and t-last>4:
   p=host();best=None
   for dx in [0,1,-1,2,-2]:
    for dy in [0,1,-1,2,-2]:
     c=unreal.Vector(CENTER.x+dx*15.24,CENTER.y+dy*15.24,20)
     if json.loads(T.preview_welding_station(p,c,0,False))['valid']:best=c;break
    if best:break
   spot=best or CENTER;T.preview_welding_station(p,spot,0,False,True);T.queue_factory_action(p,'ToolStation');stage='fixture';last=t
  elif stage=='fixture' and t-last>3:
   # A client's placement reaches its own view only after the server applies it and the snapshot replicates.
   p=player();s=report(p);q['checks']['station_placed']=bool(s)
   if not s and t-last<12:return
   if not s:raise RuntimeError('Station was not placed')
   q['checks']['bins_filled']=T.setup_welding_assembly_review(host(),s['id']);T.select_welding_station(p,s['id']);T.queue_factory_action(p,'StationSupply');stage='hire';last=t
  elif stage=='hire' and t-last>2:
   p=player();plant=staff(p)['plant']
   for n in range(2):T.queue_employee_hire(p,1,make_guid('484952450000000400000001'+format(n+1,'08x')),make_guid(plant))
   stage='assign';last=t
  elif stage=='assign' and t-last>2.5:
   p=player();people=sorted(e['id'] for e in staff(p)['employees'] if e['role']==3);q['checks']['two_welders_hired']=len(people)>=2
   if len(people)<2:raise RuntimeError('Expected two hired welders')
   welders=people[-2:];s=report(p)
   for slot,w in enumerate(welders):T.queue_employee_assignment(p,w,1,s['id'],make_guid(plant),slot)
   stage='run';last=t
  elif stage=='run':
   p=player();s=report(p)
   if 'walk_shot' not in q['notes']:
    a=next((e for e in staff(p)['employees'] if e['id']==welders[0]),None);wp=a and a.get('position')
    if a and a.get('activity')=='Walking' and wp and abs(wp[0]-s['x'])<520 and abs(wp[1]-s['y'])<520:
     q['notes']['walk_shot']=wp;q['notes']['hoods_walking']=json.loads(T.welding_hood_report(p));look(p,unreal.Vector(wp[0]+260,wp[1]-260,210),unreal.Vector(wp[0],wp[1],150),45);stage='shoot_walk';last=t
   q['samples'].append({k:s[k] for k in ('stage','work','welders','at_work','rate','guns_in_hand','sparks','supply')}) if len(q['samples'])<3000 else None
   if s['stage']==2 and s['at_work']==2 and s['work']>=25:
    q['notes']['both']=s;q['checks']['both_slots_filled']=s['welders']==welders;q['checks']['rate_170_with_both']=abs(s['rate']-1.7)<.01;look(p,at(s,*OVER[0]),at(s,*OVER[1]),OVER[2]);stage='shoot_both';last=t
   elif s['stage']<2 or s['at_work']<2:T.advance_factory_test(p,2)
  elif stage=='shoot_walk' and t-last>.35:stage='run';last=t;shot('Hood_WelderWalking_VisorUp')
  elif stage=='shoot_both' and t-last>2.6:
   s=report(player());q['checks']['two_guns_and_sparks']=s['guns_in_hand']==2 and s['sparks']==2;h=json.loads(T.welding_hood_report(player()));q['notes']['hoods_welding']=h;q['checks']['both_hoods_visors_down']=h['equipped']==2 and h['closed']==2;stage='close';last=t;shot('Phase4_BothWelders_Working')
  elif stage=='close' and t-last>.5:
   s=report(player());look(player(),at(s,-140,-300,175),at(s,-30,-107,150),40);stage='shoot_close';last=t
  elif stage=='shoot_close' and t-last>2.2:stage='panel';last=t;shot('Hood_VisorDown_MidWeld')
  elif stage=='panel' and t-last>.5:
   p=player();s=report(p);T.select_welding_station(p,s['id']);look(p,at(s,620,-650,430),at(s,0,0,40),70);stage='shoot_panel';last=t
  elif stage=='shoot_panel' and t-last>2.6:
   stage='leave';last=t;shot('Phase4_Panel_BothAssigned')
  elif stage=='leave' and t-last>.5:
   p=player();s=report(p);left=s['work'];q['notes']['work_before_leave']=left;T.queue_employee_assignment(p,welders[1],3,0,make_guid(plant));look(p,at(s,*WIDE[0]),at(s,*WIDE[1]),WIDE[2]);stage='after_leave';last=t
  elif stage=='after_leave' and t-last>1.4:
   s=report(player());q['notes']['after_leave']=s;q['checks']['progress_kept_rate_100_after_leave']=s['welders'][1]==0 and s['stage']==2 and s['work']>=left-.01 and abs(s['rate']-1.0)<.01 and s['guns_in_hand']==1
   stage='settle';last=t;shot('Phase4_WelderB_LeavingMidUnit')
  elif stage=='settle' and t-last>2:
   p=player();T.select_welding_station(p,report(p)['id']);look(p,at(report(p),620,-650,430),at(report(p),0,0,40),70);stage='shoot_panel_one';last=t
  elif stage=='shoot_panel_one' and t-last>2.6:
   stage='done';last=t;shot('Phase4_Panel_OneWelderLeft')
  elif stage=='done' and t-last>1:
   q['completed']=True;out.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR');return
  q['stage']=stage;out.write_text(json.dumps(q,indent=1))
 except Exception:
  q['errors'].append(traceback.format_exc());out.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
handle=unreal.register_slate_post_tick_callback(tick)
