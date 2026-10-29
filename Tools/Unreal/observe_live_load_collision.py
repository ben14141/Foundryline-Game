"""Read-only collision observer during the real purchasing/production flow."""
import unreal,time,json,traceback,builtins
from pathlib import Path
T=unreal.FoundrylineNetworkTestTools
out=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))/'Logs/LiveLoadCollisionQA.json'
q={'completed':False,'checks':{'actual_stock_pallet_blocks':False,'actual_carried_load_blocks':False,'unused_fork_bodies_disabled':False},'hits':[],'samples':0,'errors':[]}
start=time.monotonic();last=0
def finish():q['completed']=True;out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
def tick(dt):
 global last
 try:
  elapsed=time.monotonic()-start
  if elapsed>300:finish();return
  if elapsed-last<.5:return
  last=elapsed
  worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
  if not worlds:return
  p=next((p for w in worlds for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller() and p.get_controlled_pawn() and p.get_controlled_pawn().get_class().get_name()=='FoundrylineWalkCharacter'),None)
  if not p:return
  c=json.loads(T.commerce_report(p));locations={x['id']:x['place'] for x in c.get('pallets',[])};q['samples']+=1
  for actor in unreal.GameplayStatics.get_all_actors_of_class(p.get_world(),unreal.FoundrylineFactoryPlant):
   boxes=actor.get_components_by_class(unreal.BoxComponent)
   if any('Fork1_' in b.get_name() and b.get_collision_enabled()==unreal.CollisionEnabled.NO_COLLISION for b in boxes):q['checks']['unused_fork_bodies_disabled']=True
   for b in boxes:
    name=b.get_name()
    if not name.startswith('Pallet') or b.get_collision_enabled()==unreal.CollisionEnabled.NO_COLLISION:continue
    try:pid=int(name.split('_')[0][6:])
    except Exception:continue
    key='actual_carried_load_blocks' if locations.get(pid)==5 else 'actual_stock_pallet_blocks'
    if q['checks'][key]:continue
    center,extent,_=unreal.SystemLibrary.get_component_bounds(b)
    if center.z-extent.z>200 or center.z+extent.z<=22:continue
    for axis,sign in [(0,1),(0,-1),(1,1),(1,-1)]:
     a=[center.x,center.y,112];a[axis]+=sign*([extent.x,extent.y][axis]+95);end=unreal.Vector(center.x,center.y,112)
     hit=json.loads(T.probe_player_collision(p,unreal.Vector(*a),end))
     if hit['blocked'] and not hit['penetrating'] and hit['component']==b.get_path_name():q['checks'][key]=True;q['hits'].append({'pallet':pid,'place':locations.get(pid),'hit':hit});break
  out.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
builtins.foundryline_stop_load_collision_probe=finish
