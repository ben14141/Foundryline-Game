"""Paid office desks: catalogue, two actual planner purchases and office terminal access.
Fresh unsaved PIE; no grants, authored maps or player saves are written.
"""
import unreal,time,json,traceback
from pathlib import Path
R=Path(unreal.Paths.project_dir()).resolve();N=unreal.FoundrylineNetworkTestTools;L=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
t=unreal.AssetImportTask();t.filename=str(R/'SourceAssets/UI/BuildCatalogue/T_Catalog_OfficeDesk.png');t.destination_path='/Game/Foundryline/UI/BuildCatalogue';t.destination_name='T_Catalog_OfficeDesk';t.automated=True;t.replace_existing=True;t.save=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t]);a=unreal.load_asset(t.destination_path+'/'+t.destination_name);a.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON);a.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);a.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS);unreal.EditorAssetLibrary.save_loaded_asset(a)
L.load_level('/Game/Foundryline/Maps/L_FirstShopStarter');unreal.EditorPythonScripting.set_keep_python_script_alive(True)
q={'checks':{},'errors':[],'positions':[]};start=time.monotonic();last=start;index=-1;before=0;camera=None

def world():return next(w for w in unreal.EditorLevelLibrary.get_pie_worlds(False) if unreal.GameplayStatics.get_game_mode(w))
def pc():return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(world(),unreal.PlayerController) if p.is_local_controller())
def action(s):N.queue_commerce_action(pc(),s)
def factory(s):N.queue_factory_action(pc(),unreal.Name(s))
def tool(s):N.queue_construction_tool(pc(),unreal.Name(s),unreal.IntVector(),unreal.IntVector(),0,unreal.LinearColor(1,1,1,1))
def cash():return json.loads(N.commerce_report(pc()))['cash']
def check(n,v):q['checks'][n]=bool(v)
def desks():return [d for d in unreal.GameplayStatics.get_all_actors_of_class(world(),unreal.FoundrylineOperationsTerminal) if any((d.get_actor_location()-unreal.Vector(x,y,20)).length()<70 for x,y in q['positions'])]
def shot(n):unreal.SystemLibrary.execute_console_command(pc(),'Shot filename="'+str(R/'Saved/Screenshots/Reviews'/('OfficeDesk_'+n+'.png'))+'"')
def aim():
 global camera
 camera=N.create_review_camera(pc());target=unreal.Vector(-3000,800,20);at=target+unreal.Vector(900,-1400,1500);camera.set_actor_location_and_rotation(at,unreal.MathLibrary.find_look_at_rotation(at,target),False,True);pc().set_view_target_with_blend(camera,0)
def pause():
 global before
 if json.loads(N.commerce_report(pc()))['running']:factory('Run')
 before=cash()
def place():
 factory('ShopItem7')
 for x in range(-3400,-2400,150):
  for y in range(250,1800,150):
   p=unreal.Vector(x,y,20);preview=json.loads(N.preview_factory_brush(pc(),unreal.Name('Equipment'),p,p,False,False))
   if preview['valid']:
    result=json.loads(N.preview_factory_brush(pc(),unreal.Name('Equipment'),p,p,False,True));check('purchase_command_'+str(len(q['positions'])),result['sent']);q['positions'].append([x,y]);return
 raise RuntimeError('No legal office workstation position')
def bought():
 check('two_extra_desks',len(desks())==2);check('charged_exactly_1000',cash()==before-100000)
 for i,d in enumerate(desks()):check('desk_owned_by_plant_'+str(i),d.get_owner() is not None)
 factory('ToolDone');shot('Placed')
def finish():
 q['completed']=True;q['elapsed_seconds']=time.monotonic()-start;(R/'Saved/Logs/OfficeDeskNativeQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(pc() if unreal.EditorLevelLibrary.get_pie_worlds(False) else None,'QUIT_EDITOR')
steps=[(18,lambda:N.setup_operations_review(pc(),1)),(1,lambda:action('Office')),(1,lambda:action('PropertyView')),(2,pause),(1,aim),(1,lambda:tool('HUD:CatalogOpen')),(1,lambda:tool('HUD:CatalogGroupOffice')),(2,lambda:shot('Catalogue')),(1,lambda:tool('HUD:CatalogClose')),(1,lambda:factory('ShopItem7')),(1,place),(2,lambda:factory('StationRotate')),(1,place),(2,bought),(1,lambda:N.setup_operations_review(pc(),1)),(1,lambda:action('Office')),(2,lambda:check('new_computer_opens_desktop',json.loads(N.operations_ui_report(pc()))['screen']==1)),(1,lambda:shot('Computer')),(1,finish)]
def tick(dt):
 global index,last
 try:
  now=time.monotonic()
  if now-start>240:raise RuntimeError('Review timeout at '+str(index))
  if index<0:
   if now-start>3:L.editor_request_begin_play();index=0;last=now
   return
  delay,f=steps[index]
  if now-last<delay:return
  f();index+=1;last=now
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
