"""Single-player rendered review of one-welder assembly (Phase 3): the station is placed with two kits in its bins, set to
make headache racks from the HUD, a welder is hired and assigned to it through the real employee commands, then the plant
runs: fit-up, the rack on the fixture, welding at machine A, lights, the move to OUT and the finished rack on OUT.
Screenshots go to Saved/Screenshots/Reviews; the report goes to Saved/Logs/WeldingAssemblyVisualQA.json. No save slots."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools
base=Path(unreal.Paths.project_saved_dir());shots=base/'Screenshots/Reviews';out=base/'Logs/WeldingAssemblyVisualQA.json'
start=time.monotonic();q={'checks':{},'notes':{},'shots':[],'errors':[],'samples':[]};cam=None;handle=None
CENTER=unreal.Vector(-1806.12,1823.12,20)
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def player():return next(a for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if a.is_local_controller())
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
 taken.add(name);path=str(shots/(name+'.png'));unreal.AutomationLibrary.take_high_res_screenshot(1920,1080,path);q['shots'].append(path)
def report(p):
 r=json.loads(T.welding_station_report(p));return r['stations'][0] if r['stations'] else None
def staff(p):return json.loads(T.employee_report(p))
def at(s,dx,dy,z):return unreal.Vector(s['x']+dx,s['y']+dy,z)
# Moments to capture: (name, condition on the station report, camera offset, target offset, fov).
MOMENTS=[
 ('Phase3_FitUp_KitParts',lambda s:s['stage']==1 and 12.5<=s['work']<15,(40,260,300),(0,-10,92),55),
 ('Phase3_RackOnFixture',lambda s:s['stage']==2 and s['work']>=40,(170,-240,235),(0,0,118),55),
 ('Phase3_WelderWorking_MachineA',lambda s:s['stage']==2 and s['work']>=55,(-300,-330,175),(-70,-70,105),58),
 ('Phase3_Panel_Welding',lambda s:s['stage']==2 and s['work']>=62,(620,-650,430),(0,0,40),70),
 ('Phase3_FinalAssembly_Lights',lambda s:s['stage']==3 and s['work']>=10,(-20,-230,215),(0,0,125),55),
 ('Phase3_RackMovingToOut',lambda s:s['stage']==4 and s['work']>=.2,(80,-390,300),(110,0,110),60),
 ('Phase3_FinishedRackOnOUT',lambda s:s['stage']==0 and s['out']==1,(60,-260,240),(198,0,20),62),
]
spot=CENTER;stage='start';last=0;moment=0;welder=None;plant=None
def tick(dt):
 global stage,last,spot,moment,welder,plant
 try:
  t=time.monotonic()-start
  if t>520:raise RuntimeError('Welding assembly review timeout at '+stage)
  if stage=='start' and t>3:stage='pie';last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage=='pie' and t-last>14:
   p=player();T.queue_property_view(p)
   T.queue_factory_lane(p,0,unreal.Vector(-2217.6,1275,20),unreal.Vector(-2217.6,2170,20),304.8)
   T.queue_factory_lane(p,1,unreal.Vector(-1455,1214.04,20),unreal.Vector(-810,1214.04,20),121.92)
   T.queue_factory_lane(p,2,unreal.Vector(-1394,1450,20),unreal.Vector(-1394,1450,20),200);stage='place';last=t
  elif stage=='place' and t-last>4:
   p=player();best=None
   for dx in [0,1,-1,2,-2]:
    for dy in [0,1,-1,2,-2]:
     c=unreal.Vector(CENTER.x+dx*15.24,CENTER.y+dy*15.24,20)
     if json.loads(T.preview_welding_station(p,c,0,False))['valid']:best=c;break
    if best:break
   spot=best or CENTER;q['notes']['commit']=json.loads(T.preview_welding_station(p,spot,0,False,True));T.queue_factory_action(p,'ToolStation');stage='fixture';last=t
  elif stage=='fixture' and t-last>3:
   p=player();s=report(p);q['checks']['station_placed']=bool(s)
   if not s:raise RuntimeError('Station was not placed')
   q['checks']['bins_filled']=T.setup_welding_assembly_review(p,s['id']);T.queue_factory_action(p,'StationSupply');stage='hire';last=t
  elif stage=='hire' and t-last>2:
   p=player();plant=staff(p)['plant'];q['notes']['before_hire']=report(p);T.queue_employee_hire(p,1,make_guid('484952450000000300000001'+format(7,'08x')),make_guid(plant));stage='assign';last=t
  elif stage=='assign' and t-last>2:
   p=player();people=[e for e in staff(p)['employees'] if e['role']==3];q['checks']['welder_hired']=bool(people)
   if not people:raise RuntimeError('No hired person to assign')
   welder=max(people,key=lambda e:e['id'])['id'];s=report(p);T.queue_employee_assignment(p,welder,1,s['id'],make_guid(plant));stage='assigned';last=t
  elif stage=='assigned' and t-last>2:
   p=player();e=next(e for e in staff(p)['employees'] if e['id']==welder);s=report(p);q['notes']['assigned']={'employee':e.get('job'),'assignment':e['assignment'],'station':s}
   q['checks']['welder_assigned_to_station']=e['assignment']==s['id'] and e['role']==1;q['checks']['recipe_set']=s['recipe']==1 or s['stage']>0
   look(p,at(s,-330,-420,260),at(s,-30,-107,90),62);stage='run';last=t
  elif stage=='run':
   p=player();s=report(p);q['samples'].append({k:s[k] for k in ('stage','work','advancing','welder_ready','out','units','supply')}) if len(q['samples'])<4000 else None
   if s['stage']>0:q['checks']['unit_started_with_welder']=q['checks'].get('unit_started_with_welder',True) and (s['welder_ready'] or s['stage']==4)
   if s['stage']==2 and s['advancing']:q['checks']['weld_advances']=True
   if moment>=len(MOMENTS):stage='settle';last=t
   else:
    n,cond,c,g,f=MOMENTS[moment]
    if cond(s):look(p,at(s,*c),at(s,*g),f);q['notes'][n]=s;stage='shoot';last=t
    else:T.advance_factory_test(p,2 if s['stage'] in (1,2,3,4) else 4)
  # Advance first: the screenshot call can tick Slate again before it returns.
  elif stage=='shoot' and t-last>2.6:name=MOMENTS[moment][0];moment+=1;stage='run';last=t;shot(name)
  elif stage=='settle':
   p=player();T.advance_factory_test(p,4)
   if t-last>4:
    s=report(p);q['notes']['final']=s;outs=[x['out'] for x in q['samples']]
    q['checks']['one_rack_on_out']=s['out']==1 and max(outs)<=1;q['checks']['one_unit_counted']=s['units']==1;q['checks']['second_unit_waits_for_out']=s['stage']==0 and 'OUT cradle full' in s['supply']
    q['checks']['all_moments_shot']=len(q['shots'])==len(MOMENTS);stage='done';last=t
  elif stage=='done' and t-last>1:
   q['completed']=True;out.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR');return
  q['stage']=stage;out.write_text(json.dumps(q,indent=1))
 except Exception:
  q['errors'].append(traceback.format_exc());out.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
handle=unreal.register_slate_post_tick_callback(tick)
