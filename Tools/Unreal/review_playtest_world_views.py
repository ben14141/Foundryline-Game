"""Camera-only world review in an existing fresh PIE session. No inventory/save edits."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()));out=base/'Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True);log=base/'Logs/PlaytestWorldCameraQA.json';start=time.monotonic();last=0;stage=0;index=0;camera=None;old=None;oldblur=0;q={'screenshots':[],'errors':[],'checks':{}}
views=[
 ('ShopInterior',(-300,1900,220),(-1350,800,150),75),
 ('PlasmaDetail',(-1800,1450,240),(-1300,800,110),65),
 ('RawRacking',(-250,100,190),(-700,500,200),75),
 ('FinishedRacking',(-240,1550,200),(-700,1000,200),75),
 ('ForkliftParking',(300,1200,210),(100,500,130),65),
 ('OfficeCorridor',(-2500,1350,190),(-2500,800,160),70),
 ('LoadingThreshold',(800,1700,190),(-200,1700,140),75),
 ('ExteriorOffice',(-3700,-100,190),(-3000,700,180),70),
 ('CeilingLighting',(-1800,1000,180),(-900,1200,500),75),
 ('GroundVegetation',(5200,-1000,190),(6500,0,150),75)]
def pc():return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def aim():
 n,p,t,f=views[index];camera.set_actor_location(unreal.Vector(*p),False,False);camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*p),unreal.Vector(*t)),False);camera.get_components_by_class(unreal.CameraComponent)[0].set_field_of_view(f)
def finish():
 if old:pc().set_view_target_with_blend(old,0)
 if camera:camera.destroy_actor()
 unreal.SystemLibrary.execute_console_command(pc().get_world(),'r.MotionBlurQuality '+str(oldblur));q['completed']=True
 for n,_,_,_ in views:
  fs=list(out.glob('WorldQA_'+n+'*.png'))
  if fs:q['screenshots'].append(str(max(fs,key=lambda p:p.stat().st_mtime)))
 log.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(dt):
 global last,stage,index,camera,old,oldblur
 try:
  elapsed=time.monotonic()-start
  if elapsed>130:raise RuntimeError('Camera-only review timeout at view '+str(index))
  if stage==0:
   if not unreal.EditorLevelLibrary.get_pie_worlds(False):return
   p=pc();old=p.get_view_target();camera=unreal.FoundrylineNetworkTestTools.create_review_camera(p)
   if not camera:raise RuntimeError("PIE review camera unavailable")
   p.set_view_target_with_blend(camera,0)
   try:oldblur=unreal.SystemLibrary.get_console_variable_int_value('r.MotionBlurQuality')
   except Exception:oldblur=0
   unreal.SystemLibrary.execute_console_command(p.get_world(),'r.MotionBlurQuality 0');aim();last=elapsed;stage=1
   lights=[];missing=[]
   for a in unreal.GameplayStatics.get_all_actors_of_class(p.get_world(),unreal.Actor):
    for c in a.get_components_by_class(unreal.LightComponent):
     if c.is_visible():
      try:lights.append({'type':c.get_class().get_name(),'intensity':c.get_editor_property('intensity')})
      except Exception:pass
    if isinstance(a,(unreal.FoundrylineFactoryPlant,unreal.FoundrylinePlasmaTable)):
     for c in a.get_components_by_class(unreal.StaticMeshComponent):
      mesh=c.get_editor_property('static_mesh')
      if mesh:
       for i in range(c.get_num_materials()):
        if not c.get_material(i):missing.append(mesh.get_name())
   q['visible_lights']=lights;q['checks']['active_light_sources_present']=bool(lights);q['missing_equipment_materials']=missing;q['checks']['equipment_meshes_have_materials']=not missing
  elif stage==1 and elapsed-last>3:
   unreal.SystemLibrary.execute_console_command(pc().get_world(),'Shot filename="'+str(out/('WorldQA_'+views[index][0]+'.png'))+'"');last=elapsed;stage=2
  elif stage==2 and elapsed-last>2:
   index+=1
   if index>=len(views):finish();return
   aim();last=elapsed;stage=1
  q['view']=views[index][0] if index<len(views) else 'done';log.write_text(json.dumps(q,indent=2))
 except Exception:
  q['errors'].append(traceback.format_exc())
  try:finish()
  except Exception:log.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.EditorPythonScripting.set_keep_python_script_alive(False)
handle=unreal.register_slate_post_tick_callback(tick)
