"""Read-only native observer; run inside the existing editor PIE session.
No game commands, inventory changes, save, camera movement or extra editor launch.
"""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();last=0;observed=0;out=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))/'Logs/PlaytestWorldVisualProbe.json'
q={'checks':{k:False for k in ['forklift_travels_on_automated_route','racking_and_pallet_materials_assigned','plasma_actually_cuts_with_native_sparks','plasma_torch_moves','forklift_root_stays_on_ground_while_lifting','forklift_mast_raises_for_rack_or_truck']},'errors':[],'samples':0,'torch_positions':[],'fork_positions':[],'fork_lift_range':[],'cutting_samples':0,'max_sparks':0,'rack_components':0,'carrier_seen':False};fork_start={};torch_first=None;liftlo=1e9;lifthi=-1e9;rootlo=1e9;roothi=-1e9

def vec(v):return [round(v.x,3),round(v.y,3),round(v.z,3)]
def prop(o,n,default=None):
 try:return o.get_editor_property(n)
 except Exception:return default

def tick(dt):
 global last,observed,torch_first,liftlo,lifthi,rootlo,roothi
 try:
  now=time.monotonic();elapsed=now-start
  if now-last<.15:return
  last=now;worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
  if worlds:
   w=worlds[0];plants=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineFactoryPlant)
   if plants:
    plant=plants[0];q['samples']+=1
    for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylinePlasmaTable):
     p=a.get_torch_tip();t=vec(p)
     if torch_first is None:torch_first=p
     if len(q['torch_positions'])<60 and (not q['torch_positions'] or q['torch_positions'][-1]!=t):q['torch_positions'].append(t)
     if a.is_cutting():q['cutting_samples']+=1
    pcs=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController)
    if pcs:
     m=json.loads(unreal.FoundrylineNetworkTestTools.equipment_motion_report(pcs[0]));q['max_sparks']=max(q['max_sparks'],m.get('sparks',0))
    for c in plant.get_components_by_class(unreal.StaticMeshComponent):
     mesh=c.get_editor_property('static_mesh')
     if not mesh or not c.is_visible():continue
     name=mesh.get_name()
     if name=='SM_Forklift_Body':
      root=c.get_attach_parent();p=root.get_editor_property('relative_location');rootlo=min(rootlo,p.z);roothi=max(roothi,p.z);key=root.get_name()
      if key not in fork_start:fork_start[key]=tuple(vec(p))
      first=fork_start[key];d2=(p.x-first[0])**2+(p.y-first[1])**2+(p.z-first[2])**2
      if d2>10000:
       q['checks']['forklift_travels_on_automated_route']=True
       if len(q.setdefault('moved_fork_samples',[]))<12:q['moved_fork_samples'].append(vec(p))
      if len(q['fork_positions'])<60:q['fork_positions'].append(vec(p))
     if name=='SM_Forklift_Carriage':
      car=c.get_attach_parent();p=car.get_editor_property('relative_location');liftlo=min(liftlo,p.z);lifthi=max(lifthi,p.z)
    racks=0;missing=[]
    for c in plant.get_components_by_class(unreal.StaticMeshComponent):
     mesh=prop(c,'static_mesh')
     if mesh and ('Rack_' in mesh.get_name() or 'Pallet_' in mesh.get_name()):
      racks+=1
      for i in range(c.get_num_materials()):
       if not c.get_material(i):missing.append(mesh.get_name())
    q['rack_components']=max(q['rack_components'],racks);q['checks']['racking_and_pallet_materials_assigned']=racks>0 and not missing
    q['carrier_seen']|=bool(unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineCarrierSceneActor))
    q['checks']['plasma_actually_cuts_with_native_sparks']=q['cutting_samples']>1 and q['max_sparks']>0
    q['checks']['plasma_torch_moves']=len({tuple(v) for v in q['torch_positions']})>3
    if rootlo<1e8:q['checks']['forklift_root_stays_on_ground_while_lifting']=abs(rootlo-20)<.5 and abs(roothi-20)<.5;q['fork_root_z_range']=[rootlo,roothi]
    if liftlo<1e8:q['fork_lift_range']=[liftlo,lifthi];q['checks']['forklift_mast_raises_for_rack_or_truck']=lifthi-liftlo>50
  if elapsed>220:
   q['completed']=True;unreal.unregister_slate_post_tick_callback(handle)
  out.write_text(json.dumps(q,indent=2))
 except Exception:
  q['errors'].append(traceback.format_exc());out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
