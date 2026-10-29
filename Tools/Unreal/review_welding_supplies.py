"""Single-player rendered review of welding-station supplies (Phase 2): the station is placed, set to make headache racks,
frame sections and light kits are bought from the HUD, then real trucks and forklifts bring all four parts through IN.
Screenshots go to Saved/Screenshots/Reviews; the report goes to Saved/Logs/WeldingSupplyVisualQA.json. No save slots."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools
base=Path(unreal.Paths.project_saved_dir());shots=base/'Screenshots/Reviews';out=base/'Logs/WeldingSupplyVisualQA.json'
start=time.monotonic();q={'checks':{},'notes':{},'shots':[],'errors':[],'samples':[]};cam=None;handle=None
CENTER=unreal.Vector(-1806.12,1823.12,20)
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def player():return next(a for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if a.is_local_controller())
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
def at(s,dx,dy,z):return unreal.Vector(s['x']+dx,s['y']+dy,z)
def full(s):return s and all(v[0]>=v[2]>0 for v in s['bins'].values())
spot=CENTER;stage='start';last=0;shot_at=None
def tick(dt):
 global stage,last,spot,shot_at
 try:
  t=time.monotonic()-start
  if stage not in ('start','pie','place','fixture'):
   s0=report(player())
   if s0 and s0['in_part']>=0 and s0['in_part'] not in q['notes'].setdefault('in_parts',[]):q['notes']['in_parts'].append(s0['in_part'])
  if t>420:raise RuntimeError('Welding supply review timeout at '+stage)
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
   q['checks']['fixture']=T.setup_welding_supply_review(p,s['id']);T.queue_factory_action(p,'StationSupply');stage='buy_frames';last=t
  elif stage=='buy_frames' and t-last>2:T.queue_factory_action(player(),'StationBuy14');stage='buy_lights';last=t
  elif stage=='buy_lights' and t-last>2:T.queue_factory_action(player(),'StationBuy15');stage='ordered';last=t
  elif stage=='ordered' and t-last>2:
   p=player();s=report(p);q['notes']['after_order']=s;q['checks']['recipe_set']=s['recipe']==1 and s['batch']==10
   q['checks']['parts_booked']=all(v[1]>=v[2] for k,v in s['bins'].items() if k in ('14','15'))
   look(p,at(s,620,-650,430),at(s,0,0,40),70);stage='shot_panel';last=t
  elif stage=='shot_panel' and t-last>2.5:shot('WeldingSupplies_Panel_Ordered');stage='run';last=t
  elif stage=='run':
   p=player();T.advance_factory_test(p,4);s=report(p)
   if s['in_part']>=0 and s['in_part'] not in q['notes'].setdefault('in_parts',[]):q['notes']['in_parts'].append(s['in_part'])
   if s['in_part']>=0 and 'in' not in q['notes']:q['notes']['in']=s;look(p,at(s,-470,-360,380),at(s,-230,0,30),62);shot_at=t;stage='shot_in'
   elif full(s):q['notes']['full']=s;stage='settle';last=t
  elif stage=='shot_in' and t-shot_at>1.2:shot('WeldingSupplies_IN_PalletOnPort');shot_at=t;stage='after_in'
  elif stage=='after_in' and t-shot_at>1.5:stage='run'
  elif stage=='settle' and t-last>2:
   p=player();s=report(p);seen=set(q['notes'].get('in_parts',[]))
   q['checks']['all_four_through_in']=seen>={1,10,14,15};q['checks']['bins_full']=full(s);q['checks']['status_ready']=s['supply'].startswith('Supplies ready');q['notes']['views']=0;stage='view';last=t-10
  elif stage=='view':
   s=report(player());V=[('WeldingSupplies_PartsBay_Full',(-40,30,200),(-190,160,50),62),('WeldingSupplies_PartsBay_EyeLevel',(-330,330,150),(-190,160,60),60),('WeldingSupplies_Panel_SuppliesReady',(620,-650,430),(0,0,40),70),('WeldingSupplies_OUT_Cradle',(60,-260,240),(198,0,20),62)]
   i=q['notes']['views']
   if i>=len(V):stage='done';last=t
   elif t-last>4.5:n,c,g,f=V[i];look(player(),at(s,*c),at(s,*g),f);last=t;q['notes']['pending']=n
   elif q['notes'].get('pending') and t-last>2.5:shot(q['notes'].pop('pending'));q['notes']['views']=i+1
  elif stage=='done' and t-last>2:
   q['completed']=True;out.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR');return
  q['stage']=stage;out.write_text(json.dumps(q,indent=1))
 except Exception:
  q['errors'].append(traceback.format_exc());out.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
handle=unreal.register_slate_post_tick_callback(tick)
