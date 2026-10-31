"""Unsaved four-player review of current recipes, full pallet stacks and marked light kits."""
import unreal,time,json,traceback
from pathlib import Path
r=Path(unreal.Paths.project_dir()).resolve();out=r/'Saved/Screenshots/Reviews';N=unreal.FoundrylineNetworkTestTools;levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/Foundryline/Maps/L_FirstShopStarter');unreal.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();last=0;stage=0;q={'checks':{},'errors':[]};camera=None
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def pc(w):return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def host():return next(pc(w) for w in worlds() if unreal.GameplayStatics.get_game_mode(w))
def check(k,v):
 q['checks'][k]=bool(v)
 if not v:q['errors'].append(k)
def aim(at,target):
 global camera
 if not camera:camera=N.create_review_camera(host())
 camera.set_actor_location_and_rotation(unreal.Vector(*at),unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*at),unreal.Vector(*target)),False,True);host().set_view_target_with_blend(camera,0)
def shot(name):unreal.SystemLibrary.execute_console_command(host(),'Shot filename="'+str(out/(name+'.png'))+'"')
def finish():
 q['completed']=True;(r/'Saved/Logs/CurrentPartsQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
def tick(dt):
 global stage,last
 try:
  now=time.monotonic()
  if now-start>160:raise RuntimeError('Current parts review timed out at '+str(stage))
  if stage==0 and now-start>3:levels.editor_request_begin_play();stage=1;last=now
  elif stage==1 and now-last>16:
   if len(worlds())!=4:return
   for w in worlds():
    d=json.loads(N.commerce_report(pc(w)));check(w.get_path_name()+'_starter_offers_only_current_plasma_parts',all(o['part'] in [1,10] for o in d['orders'] if o['status']==0))
   check('host_enters_planning',N.setup_operations_review(host(),0));check('server_prepares_unsaved_pallet_samples',N.setup_operations_review(host(),5));stage=2;last=now
  elif stage==2 and now-last>4:
   expected={1:50,9:50,10:100,15:8};counts=[]
   for w in worlds():
    d=json.loads(N.commerce_report(pc(w)));stock={p['part']:p for p in d['pallets'] if p['part'] in expected};counts.append({p:v['quantity'] for p,v in stock.items()})
    check(w.get_path_name()+'_replicated_counts_and_positive_unit_weights',all(p in stock and stock[p]['quantity']==n and stock[p]['unit_lb']>0 for p,n in expected.items()))
    rendered={}
    for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineFactoryPlant):
     for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
      mesh=c.static_mesh
      if not mesh:continue
      for key,name in [(1,'BracketBlank'),(9,'BentBracket'),(10,'MountingPlate'),(15,'LightKitBox')]:
       if mesh.get_name()=='SM_'+name:rendered[key]=rendered.get(key,0)+c.get_instance_count()
    check(w.get_path_name()+'_actual_full_stack_instances',all(rendered.get(p)==n for p,n in expected.items()))
   check('all_four_players_share_exact_pallet_stock',all(c==counts[0] for c in counts));q['pallet_counts']=counts
   mesh=unreal.load_asset('/Game/Foundryline/Products/Components/LightKit/SM_LightKitBox');check('identified_carton_has_all_materials',mesh and all(m.material_interface for m in mesh.get_editor_property('static_materials')))
   aim((-1200,950,650),(-1150,1650,80));unreal.SystemLibrary.execute_console_command(host(),'ShowHUD');stage=3;last=now
  elif stage==3 and now-last>3:shot('CurrentParts_FullPallets');stage=4;last=now
  elif stage==4 and now-last>2:aim((-350,1330,250),(-550,1650,60));stage=5;last=now
  elif stage==5 and now-last>3:shot('LightKits_InGame');stage=6;last=now
  elif stage==6 and now-last>2:finish()
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
