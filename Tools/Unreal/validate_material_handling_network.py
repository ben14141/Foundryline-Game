import unreal,json,time,traceback
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve();report=root/'Saved/Logs/MaterialHandlingNetworkQA.json';N=unreal.FoundrylineNetworkTestTools
q={'stage':0,'checks':{},'errors':[],'completed':False};start=time.monotonic();driver=None;motion='';last=0
E=unreal.FoundrylineSheetPlace
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def pc(w):return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylinePlayerController) if p.is_local_controller())
def cell(w):return unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineMaterialHandlingCell)[0]
def write():report.write_text(json.dumps(q,indent=2))
def cmd(p,a):return N.queue_material_control(p,a,0,0,0)
def counts(w):return [s.count for s in cell(w).stock]
def visual_counts(w):return sorted(c.get_instance_count() for c in cell(w).get_components_by_class(unreal.InstancedStaticMeshComponent))
def done():
 q['completed']=True;write();unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'QUIT_EDITOR')
def tick(dt):
 global driver,motion,last
 try:
  t=time.monotonic()-start
  if driver and t-last>.1:
   last=t;value=1 if motion=='approach' and cell(driver.get_world()).pose.position.x>100 else 0;N.queue_material_control(driver,0,value,0,0)
  s=q['stage'];schedule=[4,14,23,27,29,35,37,40,46,48,55,57,60,66,68]
  if s>=len(schedule) or t<schedule[s]:return
  if s==0:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  else:
   ws=worlds();host=next(w for w in ws if unreal.GameplayStatics.get_game_mode(w));clients=sorted([w for w in ws if w!=host],key=lambda w:w.get_path_name())
   if s==1:
    q['checks']['four_players']=len(ws)==4;q['checks']['twenty_exact_sheets_all_peers']=all(counts(w)==[10,10] for w in ws);q['checks']['only_three_visuals_per_load']=all(visual_counts(w)==[3,3] for w in ws);driver=pc(clients[0]);cmd(driver,10);motion='approach'
   elif s==2:
    motion='';q['checks']['client_drives_forklift']=all(cell(w).pose.position.x<150 for w in ws);cmd(driver,1)
   elif s==3:
    q['checks']['pickup_is_shared_without_losing_stock']=all(cell(w).carried==0 and counts(w)==[10,10] for w in ws);cmd(pc(clients[1]),10);N.setup_material_fixture(pc(host),1)
   elif s==4:
    q['checks']['other_player_cannot_steal_driver']=all(cell(w).driver and cell(w).driver.get_player_name()==driver.player_state.get_player_name() for w in ws);cmd(driver,4)
   elif s==5:
    q['checks']['rack_storage_and_lift_replicate']=all(cell(w).carried==-1 and cell(w).stock[0].place==E.RACK and cell(w).stock[0].rack_level==2 and cell(w).pose.lift>299 and counts(w)==[10,10] for w in ws);cmd(driver,1)
   elif s==6:
    q['checks']['rack_retrieval_replicates']=all(cell(w).carried==0 and counts(w)==[10,10] for w in ws);N.setup_material_fixture(pc(host),2)
   elif s==7:cmd(driver,5)
   elif s==8:
    q['checks']['roller_input_transfer_replicates']=all(cell(w).carried==-1 and cell(w).stock[0].place==E.ROLLER_INPUT for w in ws);cmd(driver,6)
   elif s==9:
    q['checks']['roller_reserves_exactly_one_sheet']=all(counts(w)==[9,10] and cell(w).roller_remaining>0 for w in ws);cmd(driver,6);cmd(pc(clients[1]),6)
   elif s==10:
    q['checks']['one_output_and_no_duplicate_consumption']=all(counts(w)==[9,10] and cell(w).rolled_blanks==1 and cell(w).roller_remaining==0 for w in ws);q['checks']['simplified_visuals_preserve_nine_sheet_count']=all(visual_counts(w)==[3,3] for w in ws);cmd(driver,11);driver=None
   elif s==11:cmd(pc(clients[1]),10)
   elif s==12:
    q['checks']['driver_handoff_shared']=all(cell(w).driver and cell(w).driver.get_player_name()==pc(clients[1]).player_state.get_player_name() for w in ws);q['checks']['late_join_requested']=N.request_late_join()
   elif s==13:
    q['checks']['fifth_player_gets_stock_storage_output_and_visuals']=len(ws)==5 and all(counts(w)==[9,10] and cell(w).stock[0].place==E.ROLLER_INPUT and cell(w).rolled_blanks==1 and visual_counts(w)==[3,3] for w in ws);q['checks']['client_cannot_teleport_test_fixture']=not N.setup_material_fixture(pc(clients[0]),0)
   elif s==14:done();return
  q['stage']+=1
  if s>0:q['last_state']=[{'counts':counts(w),'carried':cell(w).carried,'lift':cell(w).pose.lift,'output':cell(w).rolled_blanks,'feedback':cell(w).feedback} for w in worlds()]
  write()
 except Exception:q['errors'].append(traceback.format_exc());done()
handle=unreal.register_slate_post_tick_callback(tick);write()
