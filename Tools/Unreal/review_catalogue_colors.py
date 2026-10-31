"""Unsaved native visual review for the colorful catalogue; no authored map changes."""
import unreal,time,json,traceback
from pathlib import Path
r=Path(unreal.Paths.project_dir()).resolve();N=unreal.FoundrylineNetworkTestTools;out=r/'Saved/Screenshots/Reviews';start=time.monotonic();last=0;stage=0;group=0;q={'errors':[],'screenshots':[]}
unreal.EditorPythonScripting.set_keep_python_script_alive(True);unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Foundryline/Maps/L_BasicShopStarter')
def pc():
 w=unreal.EditorLevelLibrary.get_pie_worlds(False)[0];return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def tool(a):N.queue_construction_tool(pc(),a,unreal.IntVector(),unreal.IntVector(),0,unreal.LinearColor(1,1,1,1))
def finish():
 (r/'Saved/Logs/CatalogueColorReview.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(pc(),'QUIT_EDITOR')
def tick(dt):
 global stage,last,group
 try:
  now=time.monotonic()
  if now-start>120:raise RuntimeError('Timed out '+str(stage))
  if stage==0 and now-start>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=now
  elif stage==1 and now-last>12:
   N.queue_commerce_action(pc(),'Office');stage=2;last=now
  elif stage==2 and now-last>2:N.queue_commerce_action(pc(),'PropertyView');stage=3;last=now
  elif stage==3 and now-last>3:tool('HUD:CatalogOpen');stage=4;last=now
  elif stage==4 and now-last>2:
   tool('HUD:CatalogGroup'+['Expansion','Production','Storage','Office','Site','Logistics','Decorations'][group]);stage=5;last=now
  elif stage==5 and now-last>2:
   name='Catalogue_Color_'+['Expansion','Production','Storage','Office','Site','Logistics','Decorations'][group];unreal.SystemLibrary.execute_console_command(pc(),'Shot filename="'+str(out/(name+'.png'))+'"');q['screenshots'].append(name);q[name]=json.loads(json.loads(N.pavement_report(pc()))['catalog']);stage=6;last=now
  elif stage==6 and now-last>2:
   group+=1
   if group<7:stage=4;last=now
   else:
    q['sign_in_decorations']=any(b.get('item')=='Company sign' for b in q['Catalogue_Color_Decorations']['buttons'])
    q['sign_removed_from_expansion']=not any(b.get('item')=='Company sign' for b in q['Catalogue_Color_Expansion']['buttons'])
    if not q['sign_in_decorations'] or not q['sign_removed_from_expansion']:q['errors'].append('Sign category mismatch')
    tool('HUD:CatalogSearch')
    for k in 'SIGN':tool('Key:'+k)
    stage=7;last=now
  elif stage==7 and now-last>2:
   c=json.loads(json.loads(N.pavement_report(pc()))['catalog']);q['search_single_result']=c['search']=='sign' and c['matches']==1
   b=next(b for b in c['buttons'] if b['action']=='CatalogClear');N.queue_construction_tool(pc(),'WheelClick',unreal.IntVector(round(b['x']),round(b['y']),0),unreal.IntVector(),0,unreal.LinearColor(1,1,1,1));stage=8;last=now
  elif stage==8 and now-last>2:
   c=json.loads(json.loads(N.pavement_report(pc()))['catalog']);q['clear_search_button']=c['search']=='' and c['open']
   b=[b for b in c['buttons'] if b.get('item')=='Company sign'][-1];N.queue_construction_tool(pc(),'WheelClick',unreal.IntVector(round(b['x']),round(b['y']),0),unreal.IntVector(),0,unreal.LinearColor(1,1,1,1));stage=9;last=now
  elif stage==9 and now-last>2:
   p=json.loads(N.pavement_report(pc()));c=json.loads(p['catalog']);q['detail_select_button']=p['tool']=='Sign' and not c['open']
   for k in ['search_single_result','clear_search_button','detail_select_button']:
    if not q[k]:q['errors'].append(k+' failed')
   q['complete']=True;finish()
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
