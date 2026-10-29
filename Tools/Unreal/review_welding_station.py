"""Single-player rendered review of the welding station: red/blue placement preview, then the placed station.
Screenshots go to Saved/Screenshots/Reviews. Lanes and the station come from the real controller commands; no save slots."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools
base=Path(unreal.Paths.project_saved_dir());shots=base/'Screenshots/Reviews';out=base/'Logs/WeldingStationVisualQA.json'
start=time.monotonic();stage=0;last=0;q={'checks':{},'notes':{},'shots':[],'errors':[]};cam=None
CENTER=unreal.Vector(-1806.12,1823.12,20)
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def player():return next(a for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if a.is_local_controller())
def look(p,loc,target,fov=60):
 global cam
 if not cam:cam=T.create_review_camera(p)
 cam.set_actor_location_and_rotation(loc,unreal.MathLibrary.find_look_at_rotation(loc,target),False,False);cam.camera_component.set_field_of_view(fov);p.set_view_target_with_blend(cam,0)
def shot(name):
 path=str(shots/(name+'.png'));unreal.AutomationLibrary.take_high_res_screenshot(1920,1080,path);q['shots'].append(path)
def finish():
 q['completed']=True;out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
aim=None;spot=CENTER;done=set()
def placed():
 r=json.loads(T.welding_station_report(player()))['stations'];return r[0] if r else {'x':spot.x,'y':spot.y}
def commit(p):
 q['notes']['commit']=json.loads(T.preview_welding_station(p,spot,0,False,True));T.queue_factory_action(p,'ToolStation');T.queue_factory_action(p,'Toggle')
def view_placed(p):
 commit(p) if 'commit' not in q['notes'] else None;look(p,unreal.Vector(spot.x+620,spot.y-650,430),unreal.Vector(spot.x,spot.y,40),70)
STEPS=[
 (lambda p:look(p,unreal.Vector(-1050,420,820),unreal.Vector(-1300,800,60),66),'WeldingStation_Preview_Invalid',(lambda:unreal.Vector(-1300,800,20),0,False)),
 (lambda p:look(p,unreal.Vector(spot.x+620,spot.y-650,430),unreal.Vector(spot.x,spot.y,40),70),'WeldingStation_Preview_Valid',(lambda:spot,0,False)),
 (lambda p:look(p,unreal.Vector(spot.x+620,spot.y-650,430),unreal.Vector(spot.x,spot.y,40),70),'WeldingStation_Preview_Rotated_Swapped',(lambda:spot,1,True)),
 (view_placed,'WeldingStation_Placed_Overview',None),
 (lambda p:look(p,unreal.Vector(placed()['x']-110,placed()['y']-150,170),unreal.Vector(placed()['x']+80,placed()['y']+40,95),75),'WeldingStation_Placed_EyeHeight_WelderA',None),
 (lambda p:look(p,unreal.Vector(placed()['x']-120,placed()['y']+60,260),unreal.Vector(placed()['x']-200,placed()['y']-170,60),70),'WeldingStation_Placed_MachineA_IN',None),
 (lambda p:look(p,unreal.Vector(placed()['x']+90,placed()['y']-60,260),unreal.Vector(placed()['x']+200,placed()['y']+170,60),70),'WeldingStation_Placed_MachineB_OUT',None),
]
def tick(dt):
 global stage,last,aim,spot
 try:
  t=time.monotonic()-start
  if t>200:raise RuntimeError('Welding visual timeout at stage '+str(stage))
  if stage==0 and t>3:stage=1;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and t-last>14:
   p=player();T.queue_property_view(p)
   T.queue_factory_lane(p,0,unreal.Vector(-2217.6,1275,20),unreal.Vector(-2217.6,2170,20),304.8)
   T.queue_factory_lane(p,1,unreal.Vector(-1455,1214.04,20),unreal.Vector(-810,1214.04,20),121.92)
   T.queue_factory_lane(p,2,unreal.Vector(-1394,1450,20),unreal.Vector(-1394,1450,20),200);stage=2;last=t
  elif stage==2 and t-last>4:
   p=player();q['notes']['invalid']=json.loads(T.preview_welding_station(p,unreal.Vector(-1300,800,20),0,False));q['checks']['burn_table_preview_red']=not q['notes']['invalid']['valid']
   best=None
   for dx in [0,1,-1,2,-2]:
    for dy in [0,1,-1,2,-2]:
     r=json.loads(T.preview_welding_station(p,unreal.Vector(CENTER.x+dx*15.24,CENTER.y+dy*15.24,20),0,False))
     if r['valid']:best=(dx,dy,r);break
    if best:break
   q['notes']['valid']=best[2] if best else r;q['checks']['connected_preview_blue']=bool(best);o=best[:2] if best else (0,0)
   spot=unreal.Vector(CENTER.x+o[0]*15.24,CENTER.y+o[1]*15.24,20);aim=None;stage=3;last=t
  elif stage>=3 and stage<3+len(STEPS):
   step=STEPS[stage-3];p=player()
   if aim is None:step[0](p);aim=True;last=t
   if step[2]:T.preview_welding_station(p,step[2][0](),step[2][1],step[2][2]);T.aim_factory_brush(p,step[2][0]())
   if step[2] and t-last>2.2 and 'hint_'+step[1] not in q['notes']:q['notes']['hint_'+step[1]]=p.get_factory_feedback() or json.loads(T.preview_welding_station(p,step[2][0](),step[2][1],step[2][2]))['hint']
   if t-last>2.5 and step[1] not in done:shot(step[1]);done.add(step[1])
   if t-last>4.5:aim=None;stage+=1;last=t
  elif stage==3+len(STEPS) and t-last>2:q['notes']['placed']=json.loads(T.welding_station_report(player()))['stations'];q['checks']['placed_through_ui_commit']=len(q['notes']['placed'])==1;finish();return
  q['stage']=stage;out.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
