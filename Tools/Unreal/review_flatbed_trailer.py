"""Render the imported flatbed trailer on its own: a blank level with sun, sky and a floor, the trailer assembled from
manifest.json (body, wheels on their axle centres, raised legs) and coupled to the Megapoly tractor at its fifth wheel.
Screenshots go to Saved/Screenshots/Reviews (FlatbedTrailer_*); report: Saved/Logs/FlatbedTrailerReview.json.
Run: UnrealEditor Foundryline.uproject -RenderOffscreen -ExecutePythonScript=<abs path to this file>
"""
import unreal,json,time,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
SAVED=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()));SHOTS=SAVED/'Screenshots/Reviews';OUT=SAVED/'Logs/FlatbedTrailerReview.json'
man=json.loads((ROOT/'SourceAssets/Vehicles/FlatbedTrailer/manifest.json').read_text());D='/Game/Foundryline/Vehicles/FlatbedTrailer/'
q={'placed':[],'shots':[],'errors':[]};start=time.monotonic();stage='wait';last=0;handle=None
A=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);U=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
def mesh(path,loc,rot=unreal.Rotator(0,0,0),scale=None):
 a=A.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*loc),rot);m=unreal.load_asset(path);a.static_mesh_component.set_static_mesh(m)
 if scale:a.set_actor_scale3d(unreal.Vector(*scale))
 q['placed'].append([path,bool(m)]);return a
def build():
 unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).new_level('/Temp/FlatbedTrailerReview')
 A.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,500),unreal.Rotator(roll=0,pitch=-40,yaw=-35)).light_component.set_intensity(8)
 A.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,500)).light_component.set_editor_property('real_time_capture',True)
 A.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector(0,0,0))
 mesh('/Engine/BasicShapes/Plane.Plane',(0,0,0),scale=(120,60,1))
 mesh(D+'SM_Trailer_Body',(0,0,0))
 for n,p in man['wheels'].items():mesh(D+'SM_'+n,p)
 for n,p in man['legs_rest'].items():mesh(D+'SM_'+n,p)
 # tractor: the trailer kingpin sits on the fifth wheel 42 cm ahead of the tractor origin
 for n,p in [('Tractor_Body',(0,0,0)),('Tractor_Wheel_0',(0.00926,95.22259,45.53076)),('Tractor_Wheel_1',(0.00921,-95.59264,45.53089)),('Tractor_Wheel_2',(367.83292,114.05562,45.51796)),('Tractor_Wheel_3',(367.83297,-114.08374,45.5181))]:
  mesh('/Game/Foundryline/Vehicles/Semi/Megapoly/SM_'+n+'.SM_'+n,(p[0]-42,p[1],p[2]))
def view(loc,target):
 rot=unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*loc),unreal.Vector(*target));U.set_level_viewport_camera_info(unreal.Vector(*loc),rot)
VIEWS=[('Hitched',(900,1700,650),(-600,0,100)),('Rear',(-2300,800,300),(-1300,0,90)),('Wheels',(-1250,420,70),(-1250,0,70)),('Side',(-700,2700,300),(-700,0,110))]
def tick(dt):
 global stage,last
 try:
  t=time.monotonic()-start
  if t>300:raise RuntimeError('timeout at '+stage)
  if stage=='wait' and t>5:stage='0';last=t
  elif stage.isdigit():
   i=int(stage)
   if i>=len(VIEWS):stage='done';last=t
   elif t-last>4:
    n,loc,tg=VIEWS[i];view(loc,tg)
    if t-last>8:
     p=str(SHOTS/('FlatbedTrailer_'+n+'.png'));unreal.AutomationLibrary.take_high_res_screenshot(1600,900,p);q['shots'].append(p);stage=str(i+1);last=t
  elif stage=='done' and t-last>6:
   OUT.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor();return
 except Exception:
  q['errors'].append(traceback.format_exc());OUT.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
build()   # a new level from inside a Slate tick crashes the editor
handle=unreal.register_slate_post_tick_callback(tick)
