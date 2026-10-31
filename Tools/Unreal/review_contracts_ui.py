"""Four-player contract UI review: real row/accept clicks and real workstation purchase unlock assembly offers."""
import unreal,time,json,traceback
from pathlib import Path
r=Path(unreal.Paths.project_dir()).resolve();prefix=(r/'Tools/Unreal/review_workshop_menu_input.py').read_text().split('def tick(dt):')[0].replace('WorkshopMenuInputQA.json','ContractsUIQA.json');exec(compile(prefix,'menu_review_helpers','exec'),globals())
def data(p=None):return json.loads(N.commerce_report(p or host()))
def tool(name,at=(0,0)):
 N.queue_construction_tool(host(),name,unreal.IntVector(*at,0),unreal.IntVector(*at,0),0,unreal.LinearColor(1,1,1,1))
def tick(dt):
 global stage,last
 try:
  now=time.monotonic()
  if now-start>190:raise RuntimeError('Contract review timeout at '+str(stage))
  if stage==0 and now-start>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=now
  elif stage==1 and now-last>14:
   if len(worlds())<4:return
   for w in worlds():N.setup_operations_review(pc(w),1);action('Office',pc(w))
   stage=2;last=now
  elif stage==2 and now-last>2:
   check('four_players_open_workshop',len(worlds())==4 and all(ui(pc(w))['screen']==1 for w in worlds()));q['cash_before']=cash();nav(1);stage=3;last=now
  elif stage==3 and now-last>2:
   rows=[o for o in data()['orders'] if o['status']==0];q['selected_contract']=rows[1]['id'];q['selected_part']=rows[1]['part'];x,y,*_=geom();click(x+332+80,y+244+144+68);stage=4;last=now
  elif stage==4 and now-last>2:
   check('actual_product_card_click_selects_contract',ui()['selected_order']==q['selected_contract']);check('contract_navigation_is_local',all(ui(pc(w))['page']==9 for w in worlds() if pc(w)!=host()));shot('Contracts_CutPart');stage=5;last=now
  elif stage==5 and now-last>2:
   x,y,*_=geom();lw=(1740-304-56)*.46;rx=x+332+lw+22;rw=(1740-304-56)-lw-22;click(rx+rw/2,y+902);stage=6;last=now
  elif stage==6 and now-last>3:
   check('accept_click_is_server_validated_and_shared',all(any(o['id']==q['selected_contract'] and o['status']==1 for o in data(pc(w))['orders']) for w in worlds()));check('successful_accept_opens_active_contract',ui()['page']==2 and ui()['selected_order']==q['selected_contract']);check('accepting_completion_paid_contract_spends_no_cash',cash()==q['cash_before']);shot('Contracts_Active');stage=7;last=now
  elif stage==7 and now-last>2:action('PropertyView');stage=8;last=now
  elif stage==8 and now-last>2:tool('HUD:CatalogOpen');stage=9;last=now
  elif stage==9 and now-last>1:tool('HUD:CatalogGroupProduction');stage=10;last=now
  elif stage==10 and now-last>1:tool('HUD:CatalogItem0');stage=11;last=now
  elif stage==11 and now-last>2:tool('PaveMove',(-1300,900));stage=12;last=now
  elif stage==12 and now-last>2:
   p=json.loads(N.pavement_report(host()));check('assembly_station_has_valid_footprint',p['factory_preview_valid']);tool('PaveDown',(-1300,900));tool('PaveUp',(-1300,900));stage=13;last=now
  elif stage==13 and now-last>3:
   check('normal_station_purchase_replicates',all(json.loads(N.pavement_report(pc(w)))['factory_stations']==1 for w in worlds()));N.queue_factory_action(host(),'Run');stage=14;last=now
  elif stage==14 and now-last>2:N.advance_commerce_test(host(),20);stage=15;last=now
  elif stage==15 and now-last>2:
   if data()['running']:N.queue_factory_action(host(),'Run')
   assembly=next((o for o in data()['orders'] if o['part']==16 and o['status']==0),None);check('real_workstation_unlocks_assembly_contracts',assembly is not None)
   q['assembly_contract']=assembly['id'] if assembly else 0;N.setup_operations_review(host(),1);action('Office');stage=16;last=now
  elif stage==16 and now-last>2:
   action('Page0');rows=[o for o in data()['orders'] if o['status']==0];index=next(i for i,o in enumerate(rows) if o['id']==q['assembly_contract']);q['assembly_row']=index%4
   for i in range(index//4):action('Next')
   stage=16.5;last=now
  elif stage==16.5 and now-last>2:
   x,y,*_=geom();click(x+332+80,y+244+144*q['assembly_row']+68);stage=17;last=now
  elif stage==17 and now-last>2:
   check('assembly_detail_selects_real_order',ui()['selected_order']==q['assembly_contract'] and q['assembly_contract']>0);shot('Contracts_Assembly');stage=18;last=now
  elif stage==18 and now-last>2:nav(2);stage=19;last=now
  elif stage==19 and now-last>2:check('steel_purchase_page_reachable',ui()['page']==3);shot('Contracts_SteelSupplier');stage=20;last=now
  elif stage==20 and now-last>2:nav(3);stage=21;last=now
  elif stage==21 and now-last>2:check('production_page_reachable',ui()['page']==10);shot('Contracts_Production');stage=22;last=now
  elif stage==22 and now-last>2:action('Page6');stage=23;last=now
  elif stage==23 and now-last>2:shot('Contracts_Equipment');stage=24;last=now
  elif stage==24 and now-last>2:check('peer_screens_stay_independent',all(ui(pc(w))['page']==9 for w in worlds() if pc(w)!=host()));finish()
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
