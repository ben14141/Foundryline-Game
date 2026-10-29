"""Temporary rendered PIE route editing review. No map/property save."""
import unreal,time
N=unreal.FoundrylineNetworkTestTools
started=time.monotonic();stage=0
points=[('NetDraw',(3000,800)),('NetDraw',(3000,2600)),('NetDraw',(4800,2600)),('NetSelect',(3000,2600)),('NetStaging',None),('NetReverse',None),('NetDraw',None),('NetDraw',(1200,2600)),('NetReverse',None),('NetSelect',(3000,800)),('NetStart',None),('NetSelect',(4800,2600)),('NetFinish',None),('NetCheck',None)]
def pc():
 for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
  for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController):
   if p.is_local_controller():return p
def tick(dt):
 global stage
 t=time.monotonic()-started
 if stage==0 and t>4:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1
 elif stage==1 and t>12:
  p=pc()
  if p:
   N.queue_logistics_action(p,'NetPanel',unreal.Vector(),False);N.queue_truck_zone(p,unreal.IntVector(-400,-2400,0),unreal.IntVector(6000,5300,0),False);stage=2
 elif stage>=2 and stage<len(points)+2 and t>16+(stage-2):
  a,xy=points[stage-2];N.queue_logistics_action(pc(),a,unreal.Vector(*(xy or (0,0)),0),xy is not None);stage+=1
 elif stage==len(points)+2 and t>34:
  unreal.log('ROUTE_NETWORK_UI '+pc().get_network_summary());unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
