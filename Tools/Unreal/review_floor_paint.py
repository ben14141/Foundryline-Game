"""Native floor decal review: real floor, depth occlusion, stable world coordinates."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.project_saved_dir());out=base/'Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True)
q={'checks':{},'errors':[]};start=time.monotonic();stage=0;last=0

def pc():return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def plant():return unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineFactoryPlant)[0]
def act(a):unreal.FoundrylineNetworkTestTools.queue_factory_action(pc(),a)
def camera(location,target):
 unreal.FoundrylineNetworkTestTools.setup_workshop_review(pc(),True);c=pc().get_view_target();c.set_actor_location(location,False,False);c.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location,target),False)
def shot(name):unreal.AutomationLibrary.take_high_res_screenshot(1920,1200,str(out/('FloorPaint_'+name+'.png')))
def tick(dt):
 global stage,last
 try:
  t=time.monotonic()-start
  if stage==0 and t>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=t
  elif stage==1 and t-last>14:
   camera(unreal.Vector(2200,6500,7000),unreal.Vector(800,2600,100));stage=2;last=t
  elif stage==2 and t-last>4:
   ds=plant().get_components_by_class(unreal.DecalComponent);paint=[d for d in ds if d.get_name()=='FactorySurfacePaint'];q['checks']['one_floor_paint_layer']=len(paint)==1
   if paint:
    d=paint[0];v=d.get_editor_property('decal_size');q['checks']['paint_at_slab_top']=abs(d.get_world_location().z-20)<.01;q['checks']['shallow_projection']=abs(v.x-1)<.01
   q['checks']['no_floating_floor_text']=not plant().get_components_by_class(unreal.TextRenderComponent)
   stage=3;last=t;shot('Overview');act('Driver');act('Driver');act('Driver')
  elif stage==3 and t-last>3:camera(unreal.Vector(800,1100,190),unreal.Vector(1300,1750,20));stage=4;last=t
  elif stage==4 and t-last>4:stage=5;last=t;shot('Floor_Close')
  elif stage==5 and t-last>3:camera(unreal.Vector(1000,2400,5000),unreal.Vector(100,2500,20));stage=50;last=t
  elif stage==50 and t-last>4:
   components=plant().get_components_by_class(unreal.StaticMeshComponent)
   bodies=[c for c in components if c.get_editor_property('static_mesh') and c.get_editor_property('static_mesh').get_name()=='SM_Forklift_Body']
   q['checks']['four_parked_forklifts']=len(bodies)==4
   fits=True;q['parking_mesh_bounds']=[]
   for body in bodies:
    root=body.get_attach_parent();home=body.get_world_location();bounds=[]
    for c in components:
     parent=c.get_attach_parent()
     while parent and parent!=root:parent=parent.get_attach_parent()
     if parent!=root:continue
     lo,hi=c.get_local_bounds();tr=c.get_world_transform()
     for x in (lo.x,hi.x):
      for y in (lo.y,hi.y):
       for z in (lo.z,hi.z):bounds.append(unreal.MathLibrary.transform_location(tr,unreal.Vector(x,y,z)))
    q['parking_mesh_bounds'].append({'home':[home.x,home.y],'min':[min(v.x for v in bounds),min(v.y for v in bounds)],'max':[max(v.x for v in bounds),max(v.y for v in bounds)]})
    fits &= all(home.x-100<=v.x<=home.x+100 and home.y-280<=v.y<=home.y+220 for v in bounds)
   q['checks']['full_forklift_meshes_fit_parking_paint']=bool(fits)
   stage=51;last=t;shot('Four_Parking_Bays')
  elif stage==51 and t-last>3:camera(unreal.Vector(2200,6500,7000),unreal.Vector(800,2600,100));act('Toggle');act('Page2');act('Overlay');stage=6;last=t
  elif stage==6 and t-last>4:stage=62;last=t;shot('Overlay')
  elif stage==62 and t-last>3:stage=60;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()
  elif stage==60 and t-last>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=61;last=t
  elif stage==61 and t-last>14:act('Toggle');act('Page2');act('Overlay');act('Save');stage=7;last=t
  elif stage==7 and t-last>2:
   q['checks']['two_bays_preserved']=plant().get_zone_count()==2;q['completed']=True;(base/'Logs/FloorPaintVisualQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);return
  if t>140:raise RuntimeError('Paint review timed out at '+str(stage))
  (base/'Logs/FloorPaintVisualQA.json').write_text(json.dumps({**q,'stage':stage},indent=2))
 except Exception:
  q['errors'].append(traceback.format_exc());(base/'Logs/FloorPaintVisualQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
