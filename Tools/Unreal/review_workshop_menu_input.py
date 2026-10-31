"""Unsaved four-player workshop-menu input review; never changes authored maps or saved companies."""
import unreal,time,json,traceback
from pathlib import Path
r=Path(unreal.Paths.project_dir()).resolve();out=r/'Saved/Screenshots/Reviews';N=unreal.FoundrylineNetworkTestTools;start=time.monotonic();last=0;stage=0;q={'checks':{},'errors':[],'screenshots':[]}
unreal.EditorPythonScripting.set_keep_python_script_alive(True);unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Foundryline/Maps/L_BasicShopStarter')
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def pc(w):return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def host():return next(pc(w) for w in worlds() if unreal.GameplayStatics.get_game_mode(w))
def ui(p=None):return json.loads(N.operations_ui_report(p or host()))
def cash(p=None):return json.loads(N.commerce_report(p or host()))['cash']
def action(a,p=None):N.queue_commerce_action(p or host(),a)
def check(k,v):
 q['checks'][k]=bool(v)
 if not v:q['errors'].append(k+' failed: '+json.dumps(ui()))
def geom():
 c=json.loads(json.loads(N.pavement_report(host()))['catalog']);w,h=c['canvas_width'],c['canvas_height'];s=min((h-64)/1000,(w-64)/1740);return ((w/s-1740)/2,(h/s-1000)/2,s,w,h)
def click(a,b):
 x,y,s,w,h=geom();vw,vh=host().get_viewport_size();f=min(vw/w,vh/h);sx=round((vw-w*f)/2+a*s*f);sy=round((vh-h*f)/2+b*s*f);N.queue_construction_tool(host(),'WheelClick',unreal.IntVector(sx,sy,0),unreal.IntVector(),0,unreal.LinearColor(1,1,1,1))
def nav(i):
 x,y,*_=geom();click(x+120,y+88+26+i*66+27)
def card(i):
 x,y,*_=geom();cw=1740-304-56;tw=(cw-36)/3;click(x+332+(i%3)*(tw+18)+tw/2,y+110+86+288+(i//3)*208+95)
def shot(n):
 q['screenshots'].append(n);unreal.SystemLibrary.execute_console_command(host(),'Shot filename="'+str(out/(n+'.png'))+'"')
def finish():
 q['completed']=True;q['elapsed_seconds']=round(time.monotonic()-start,2);q['final_ui']=[ui(pc(w)) for w in worlds()];(r/'Saved/Logs/WorkshopMenuInputQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
def tick(dt):
 global stage,last
 try:
  now=time.monotonic()
  if now-start>180:raise RuntimeError('Timed out at stage '+str(stage))
  if stage==0 and now-start>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=now
  elif stage==1 and now-last>14:
   if len(worlds())<4:return
   for w in worlds():N.setup_operations_review(pc(w),1);action('Office',pc(w))
   stage=2;last=now
  elif stage==2 and now-last>3:
   check('four_players_open_home',len(worlds())==4 and all(ui(pc(w))['screen']==1 and ui(pc(w))['page']==9 for w in worlds()));q['cash_before']=cash();shot('Workshop_Menu_Home');stage=3;last=now
  elif stage==3 and now-last>2:card(4);stage=4;last=now
  elif stage==4 and now-last>2:
   check('home_team_card_uses_actual_mouse_input',ui()['page']==7);check('peer_navigation_stays_independent',all(ui(pc(w))['page']==9 for w in worlds() if pc(w)!=host()));shot('Workshop_Menu_Team');stage=5;last=now
  elif stage==5 and now-last>2:nav(1);stage=6;last=now
  elif stage==6 and now-last>2:check('orders_navigation',ui()['page']==0);shot('Workshop_Menu_Orders');stage=7;last=now
  elif stage==7 and now-last>2:nav(3);stage=8;last=now
  elif stage==8 and now-last>2:check('production_navigation',ui()['page']==10);shot('Workshop_Menu_Production');stage=9;last=now
  elif stage==9 and now-last>2:nav(0);stage=10;last=now
  elif stage==10 and now-last>2:card(1);stage=11;last=now
  elif stage==11 and now-last>2:check('home_steel_card_uses_actual_mouse_input',ui()['page']==3);shot('Workshop_Menu_Steel');stage=12;last=now
  elif stage==12 and now-last>2:nav(6);stage=13;last=now
  elif stage==13 and now-last>2:check('money_navigation',ui()['page']==5);shot('Workshop_Menu_Money');stage=14;last=now
  elif stage==14 and now-last>2:nav(4);stage=15;last=now
  elif stage==15 and now-last>2:check('stock_and_freight_navigation',ui()['page']==4);shot('Workshop_Menu_Freight');stage=16;last=now
  elif stage==16 and now-last>2:nav(0);stage=17;last=now
  elif stage==17 and now-last>2:
   x,y,*_=geom();click(x+120,y+909);stage=18;last=now
  elif stage==18 and now-last>3:
   check('planner_button_enters_property_view',ui()['screen']==0 and ui()['property_access'] and not ui()['walking']);check('planner_does_not_close_peer_computers',all(ui(pc(w))['screen']==1 for w in worlds() if pc(w)!=host()));N.queue_construction_tool(host(),'Key:Q',unreal.IntVector(),unreal.IntVector(),0,unreal.LinearColor(1,1,1,1));stage=19;last=now
  elif stage==19 and now-last>2:
   check('property_handoff_opens_matching_catalogue',json.loads(json.loads(N.pavement_report(host()))['catalog'])['open']);shot('Workshop_Menu_Planning');stage=19.5;last=now
  elif stage==19.5 and now-last>2:N.queue_construction_tool(host(),'Key:Q',unreal.IntVector(),unreal.IntVector(),0,unreal.LinearColor(1,1,1,1));stage=20;last=now
  elif stage==20 and now-last>2:action('ReturnOffice');stage=21;last=now
  elif stage==21 and now-last>3:
   check('return_to_first_person_restores_input',ui()['walking'] and not ui()['ignore_move'] and not ui()['ignore_look'] and ui()['roofs']);check('browsing_spends_no_cash',cash()==q['cash_before']);N.setup_operations_review(host(),1);stage=22;last=now
  elif stage==22 and now-last>2:shot('Workshop_Menu_Desk');stage=23;last=now
  elif stage==23 and now-last>2:finish()
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
