"""Fresh starter real RPC sales plus read-only Handyman presentation evidence. No stock grants or simulation fast-forward."""
import unreal,time,json,traceback
from pathlib import Path
T=unreal.FoundrylineNetworkTestTools
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()));out=base/'Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True)
q={'checks':{},'errors':[],'samples':[],'screenshots':[]};stage=0;last=0;start=time.monotonic();ready=None;order=0;camera=None;captured=set();original={};lastsample=0;lastsave=0;appearances={};oldblur=None

def pc():return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def people():return json.loads(T.employee_report(pc()))
def commerce():return json.loads(T.commerce_report(pc()))
def action(s):T.queue_commerce_action(pc(),s)
def factory(s):T.queue_factory_action(pc(),s)
def shot(n):
 path=out/('HandymanStarter_'+n+'.png');unreal.SystemLibrary.execute_console_command(pc().get_world(),'Shot filename="'+str(path)+'"');q['screenshots'].append(str(path))
def aim(e):
 global camera
 if camera is None:camera=T.create_review_camera(pc())
 at=unreal.Vector(*e['position']);seated=e.get('seated',False);eye=at+(unreal.Vector(-180,370,180) if seated else unreal.Vector(240,270,150));target=at+unreal.Vector(-95 if seated else 0,0,135 if seated else 95)
 camera.set_actor_location(eye,False,False);camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye,target),False);camera.get_components_by_class(unreal.CameraComponent)[0].set_field_of_view(60);pc().set_view_target_with_blend(camera,0)
def samples():
 rows=people()['employees'];reports=[]
 for plant in unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineFactoryPlant):
  for c in plant.get_components_by_class(unreal.FoundrylineEmployeeVisualComponent):
   r=json.loads(c.get_presentation_report());r['component']=c.get_path_name();reports.append(r)
 q['samples'].append({'seconds':round(time.monotonic()-ready,2),'employees':rows,'visuals':reports})
 unique=len(reports)==len(rows) and len({r.get('key') for r in reports})==len(rows)
 q['checks']['one_body_per_employee']=q['checks'].get('one_body_per_employee',True) and unique
 q['checks']['real_skeletal_bodies']=q['checks'].get('real_skeletal_bodies',True) and bool(reports) and all(r.get('skeletal') and r.get('mesh','').startswith('/Engine/Transient.FoundrylineWorker_') and r.get('profile','').startswith('/Game/Foundryline/') for r in reports)
 for r in reports:
  key=r.get('key');appearance=(r.get('variant'),r.get('profile'),tuple(r.get('outfit',[])));appearances.setdefault(key,appearance)
  q['checks']['appearance_stable_per_employee']=q['checks'].get('appearance_stable_per_employee',True) and appearances[key]==appearance and r.get('variant') in [0,1,2]
  if r.get('animation'):q['checks']['real_animation_asset']=q['checks'].get('real_animation_asset',False) or r['animation'].startswith('/Game/')
 # Record actual component bounds rather than assuming imported units are correct.
 q.setdefault('body_bounds',{})
 for plant in unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineFactoryPlant):
  for c in plant.get_components_by_class(unreal.SkinnedMeshComponent):
   if not c.is_visible():continue
   origin,extent,radius=unreal.SystemLibrary.get_component_bounds(c);bounds_scale=max(.01,float(c.get_editor_property('bounds_scale')));extent=extent/bounds_scale;height=extent.z*2
   q['body_bounds'][c.get_path_name()]={'origin':[origin.x,origin.y,origin.z],'extent':[extent.x,extent.y,extent.z],'height_cm':height}
   q['checks']['human_scale_bounds']=q['checks'].get('human_scale_bounds',True) and 70<height<260 and extent.x<200 and extent.y<200
 q['checks']['stable_names_ids_jobs']=all(original.get(e['id'])==(e['name'],e['key'],e['role']) for e in rows)
def finish():
 q['completed']=True;q['stage']=stage
 try:
  unreal.GameplayStatics.set_game_paused(pc().get_world(),False)
  if oldblur is not None:unreal.SystemLibrary.execute_console_command(pc().get_world(),'r.MotionBlurQuality '+str(oldblur))
  pc().set_view_target_with_blend(pc().get_controlled_pawn(),0)
 except Exception:pass
 (base/'Logs/HandymanStarterNativeQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(dt):
 global stage,last,ready,order,original,lastsample,lastsave,oldblur
 try:
  now=time.monotonic();t=now-start
  if t>190 or ready is not None and now-ready>150:raise RuntimeError('Handyman starter timeout stage '+str(stage))
  if stage==0 and t>3:
   if unreal.EditorLevelLibrary.get_pie_worlds(False):unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play();stage=-1
   else:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1
   last=now
  elif stage==-1 and now-last>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=now
  elif stage==1 and now-last>12:
   oldblur=unreal.SystemLibrary.get_console_variable_int_value('r.MotionBlurQuality');unreal.SystemLibrary.execute_console_command(pc().get_world(),'r.MotionBlurQuality 0');p=people();c=commerce();ready=now;original={e['id']:(e['name'],e['key'],e['role']) for e in p['employees']};q['checks']['fresh_owned_staff_no_free_stock']=p['count']==2 and c['cash']==400000 and not c['pallets'];T.setup_operations_review(pc(),1);action('Office');stage=2;last=now
  elif stage==2 and now-last>1:
   order=next(o['id'] for o in commerce()['orders'] if o['part']==1);action('Order'+str(order));action('Accept');stage=3;last=now
  elif stage==3 and now-last>1:action('Assign');stage=4;last=now
  elif stage==4 and now-last>1:action('Page3');action('SheetsMinus');stage=5;last=now
  elif stage==5 and now-last>1:action('BuySteel');stage=6;last=now
  elif stage==6 and now-last>1:action('ProgramOrder');stage=7;last=now
  elif stage==7 and now-last>1:factory('Apply');stage=8;last=now
  elif stage==8:
   if now-last<.5:return
   factory('CloseComputer')
   e=next(e for e in people()['employees'] if e['role']==0)
   if e['moving']:q['checks']['actual_operator_walk']=True;aim(e);captured.add('walk');stage=81;last=now
   elif now-last>8:action('Speed8');stage=9;last=now
  elif stage==81 and now-last>.7:shot('Walking');action('Speed8');stage=9;last=now
  elif stage==9:
   p=people();e=next(e for e in p['employees'] if e['role']==0)
   if e['working'] and 'work' not in captured:captured.add('work');q['checks']['real_plasma_work']=True;aim(e);stage=91;last=now
   elif sum(x['quantity'] for x in commerce()['pallets'] if x['place']==6)==40:stage=10;last=now
  elif stage==91 and now-last>.7:shot('OperatingPlasma');stage=92;last=now
  elif stage==92:
   e=next(e for e in people()['employees'] if e['role']==2)
   if e['moving'] or now-last>6:aim(e);stage=93;last=now
  elif stage==93 and now-last>.7:shot('ForkliftDriver');stage=9;last=now
  elif stage==10:
   q['checks']['forty_real_brackets_staged']=True;action('Tablet');action('Page8');action('Carrier1')
   for p in commerce()['pallets']:
    if p['place']==6:action('Pallet'+str(p['id']))
   stage=11;last=now
  elif stage==11 and now-last>1:action('Book');stage=12;last=now
  elif stage==12:
   c=commerce();o=next(o for o in c['orders'] if o['id']==order)
   if o['delivered']==40:
    q['checks']['real_delivery_payment']=o['paid']==44000;q['final_commerce']=c;q['final_people']=people();pc().set_view_target_with_blend(pc().get_controlled_pawn(),0);stage=121;last=now
  elif stage==121 and now-last>1:T.setup_operations_review(pc(),1);action('Office');action('Page5');stage=13;last=now
  elif stage==13 and now-last>1:shot('PaidAccount');stage=14;last=now
  elif stage==14 and now-last>1:finish();return
  if ready is not None and now-lastsample>.2:samples();lastsample=now
  if now-lastsave>2:q['stage']=stage;(base/'Logs/HandymanStarterNativeQA.json').write_text(json.dumps(q,indent=2));lastsave=now
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
