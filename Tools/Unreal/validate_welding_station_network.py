"""Four-player listen-server welding-station check (placement, supplies, one-welder assembly), then a fifth late joiner.
No save slots are written."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools
start=time.monotonic();stage=0;last=0;q={'checks':{},'notes':{},'errors':[]};out=Path(unreal.Paths.project_saved_dir())/'Logs/WeldingStationNetworkQA.json'
CENTER=unreal.Vector(-1768.02,1823.12,20)
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def players():return [a for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if a.is_local_controller()]
def reports():return [json.loads(T.welding_station_report(p)) for p in players()]
def staff(p):return json.loads(T.employee_report(p))
def make_guid(text):
 g=unreal.Guid()
 for n,i in zip('abcd',range(0,32,8)):g.set_editor_property(n,int(text.replace('-','')[i:i+8],16))
 return g
def hoods():return [json.loads(T.welding_hood_report(p)) for p in players()]
def host():return next(p for p in players() if p.has_authority())
def commerce(p):return json.loads(T.commerce_report(p))
def order(c,oid):return next((o for o in c['orders'] if o['id']==oid),None)
def guests():return [p for p in players() if not p.has_authority()]
def finish():
 q['completed']=True;out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
def tick(dt):
 global stage,last
 try:
  t=time.monotonic()-start
  if t>900:raise RuntimeError('Welding network timeout at stage '+str(stage))
  if stage==0 and t>3:stage=1;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and t-last>12:
   ps=players();q['checks']['four_players']=len(ps)==4;c=next(p for p in ps if not p.has_authority())
   r=reports();q['notes']['initial']=r[0];q['checks']['no_stations_initially']=all(len(x['stations'])==0 for x in r)
   # A client sends the real station command before the lanes exist: the server refuses it.
   T.queue_welding_station(c,CENTER,0,False);stage=2;last=t
  elif stage==2 and t-last>2:
   c=next(p for p in players() if not p.has_authority())
   q['checks']['unconnected_station_refused']=all(len(x['stations'])==0 for x in reports());q['notes']['refusal']=json.loads(T.welding_station_report(c))['status']
   # Same three paint strokes as the StarterShop automation test, sent by a client.
   T.queue_factory_lane(c,0,unreal.Vector(-2217.6,1275,20),unreal.Vector(-2217.6,2170,20),304.8)
   T.queue_factory_lane(c,1,unreal.Vector(-1455,1214.04,20),unreal.Vector(-810,1214.04,20),121.92)
   T.queue_factory_lane(c,2,unreal.Vector(-1370,1450,20),unreal.Vector(-1370,1450,20),200);stage=3;last=t
  elif stage==3 and t-last>2:
   r=reports();q['notes']['lanes']=[x['lanes'] for x in r];q['checks']['client_paint_replicates']=len(set(x['lanes'] for x in r))==1 and r[0]['lanes']==q['notes']['initial']['lanes']+3
   c=next(p for p in players() if not p.has_authority());T.queue_welding_station(c,CENTER,0,False,True);stage=4;last=t
  elif stage==4 and t-last>2:
   q['checks']['foreign_plant_station_rejected']=all(len(x['stations'])==0 for x in reports())
   c=[p for p in players() if not p.has_authority()]
   # Two clients race for the same spot; exactly one station exists afterwards.
   T.queue_welding_station(c[0],CENTER,0,False);T.queue_welding_station(c[1],CENTER,0,False);stage=5;last=t
  elif stage==5 and t-last>2:
   r=reports();q['notes']['placed']=r[0]['stations'];ids=set(json.dumps(x['stations'],sort_keys=True) for x in r)
   q['checks']['race_one_station_on_all_peers']=len(ids)==1 and len(r[0]['stations'])==1
   q['checks']['station_pose_exact']=len(r[0]['stations'])==1 and abs(r[0]['stations'][0]['x']-CENTER.x)<.1 and abs(r[0]['stations'][0]['y']-CENTER.y)<.1
   q['checks']['late_join_requested']=T.request_late_join();stage=6;last=t
  elif stage==6 and t-last>12:
   r=reports();q['checks']['five_peers']=len(r)==5;q['checks']['late_joiner_sees_station']=len(set(json.dumps(x['stations'],sort_keys=True) for x in r))==1 and len(r[-1]['stations'])==1
   c=next(p for p in players() if not p.has_authority());sid=r[0]['stations'][0]['id'] if r[0]['stations'] else 0
   T.queue_welding_station(c,CENTER,0,False,True,True,sid);stage=7;last=t
  elif stage==7 and t-last>2:
   q['checks']['foreign_remove_rejected']=all(len(x['stations'])==1 for x in reports())
   c=next(p for p in players() if not p.has_authority());r=reports();sid=r[0]['stations'][0]['id'] if r[0]['stations'] else 0
   # Phase 2: a client picks headache racks x10 from the station panel and buys frame sections through the paid supplier flow.
   T.select_welding_station(c,sid);T.queue_factory_action(c,'StationSupply');stage=8;last=t
  elif stage==8 and t-last>2:
   c=next(p for p in players() if not p.has_authority());T.queue_factory_action(c,'StationBuy14');stage=9;last=t
  elif stage==9 and t-last>3:
   r=reports();s=[x['stations'][0] for x in r if x['stations']];q['notes']['supply']=s[0] if s else None
   q['checks']['client_recipe_replicates']=len(s)==len(r) and all(x['recipe']==1 and x['batch']==10 for x in s)
   q['checks']['host_books_client_frames']=bool(s) and s[0]['bins']['14'][1]==12
   q['checks']['member_client_sees_booked_frames']=len(s)>1 and all(x['bins']['14'][1]==12 for x in s[1:])
   c=next(p for p in players() if not p.has_authority());T.queue_welding_station(c,CENTER,0,False,False,True,s[0]['id'] if s else 0);stage=10;last=t
  elif stage==10 and t-last>2:
   q['checks']['station_with_booked_parts_not_removed']=all(len(x['stations'])==1 for x in reports())
   # Phases 3-4: two kits in the bins (host fixture); a client hires three welders.
   sid=reports()[0]['stations'][0]['id'];q['notes']['sid']=sid;T.setup_welding_assembly_review(host(),sid)
   c=guests();plant=make_guid(staff(c[0])['plant'])
   for n in range(3):T.queue_employee_hire(c[0],1,make_guid('484952450000000300000002'+format(n+1,'08x')),plant)
   stage=11;last=t
  elif stage==11 and t-last>3:
   c=guests();plant=make_guid(staff(c[0])['plant']);people=sorted(e['id'] for e in staff(c[0])['employees'] if e['role']==3)
   q['checks']['client_hired_welders']=len(people)>=3
   if len(people)<3:raise RuntimeError('Expected three hired welders: '+json.dumps(people))
   a,b1,b2=people[-3:];q['notes'].update(welder_a=a,racers=[b1,b2])
   # Welder A from one client; then two clients race for Welder B in the same tick.
   T.queue_employee_assignment(c[0],a,1,q['notes']['sid'],plant,0);T.queue_employee_assignment(c[0],b1,1,q['notes']['sid'],plant,1);T.queue_employee_assignment(c[1],b2,1,q['notes']['sid'],plant,1)
   stage=12;last=t
  elif stage==12 and t-last>3:
   s=[x['stations'][0] for x in reports() if x['stations']];w=[x['welders'] for x in s];a=q['notes']['welder_a'];b1,b2=q['notes']['racers']
   q['notes']['slots']=w;winner=w[0][1]
   q['checks']['client_assigned_welder_a_on_every_peer']=all(x[0]==a for x in w)
   q['checks']['slot_b_race_has_one_winner_on_every_peer']=winner in (b1,b2) and all(x==w[0] for x in w)
   loser=b2 if winner==b1 else b1;q['notes']['race']={'winner':winner,'loser':loser}
   q['checks']['race_loser_stays_unassigned_everywhere']=all(any(e['id']==loser and e['assignment']==0 for e in staff(p)['employees']) for p in players())
   stage=13;last=t
  elif stage==13:
   s=[x['stations'][0] for x in reports() if x['stations']]
   if s and (s[0]['stage']<2 or s[0]['at_work']<2):T.advance_factory_test(host(),5)
   if t-last>3 and all(x['stage']==2 and x['at_work']==2 for x in s):
    q['notes']['both_welding']=s[0];q['checks']['every_peer_sees_both_welders_at_170']=len(s)==len(players()) and all(x['rate']==1.7 and x['advancing'] for x in s)
    q['checks']['every_peer_shows_two_guns_in_hand']=all(x['guns_in_hand']==2 for x in s);q['notes']['guns']=[(x['guns_in_hand'],x['sparks']) for x in s]
    h=hoods();q['notes']['hoods_welding']=h;q['checks']['every_peer_shows_two_hoods']=all(x['equipped']==2 for x in h)  # visors are still swinging down here; settled below
    q['checks']['late_join_2_requested']=T.request_late_join();stage=14;last=t
   elif t-last>120:raise RuntimeError('Both welders never welded on every peer: '+json.dumps(s[:2]))
  elif stage==14 and t-last>12:
   ps=players();s=[x['stations'][0] for x in reports() if x['stations']];q['checks']['six_peers']=len(ps)==6
   h=hoods();q['notes']['hoods_late']=h;q['checks']['every_peer_and_late_joiner_see_both_visors_down']=all(x['equipped']==2 and x['closed']==2 for x in h)
   q['checks']['late_joiner_sees_both_welders_and_weld_points']=len(s)==len(ps) and all(x['welders']==s[0]['welders'] and x['at_work']==2 and x['guns_in_hand']==2 for x in s);q['notes']['late']=[(x['at_work'],x['guns_in_hand'],x['sparks']) for x in s]
   q['notes']['work_before_leave']=s[0]['work'];c=guests();T.queue_employee_assignment(c[1],q['notes']['race']['winner'],3,0,make_guid(staff(c[1])['plant']));stage=15;last=t
  elif stage==15 and t-last>3:
   s=[x['stations'][0] for x in reports() if x['stations']];q['notes']['after_leave']=s[0]
   q['checks']['welder_b_leaves_progress_kept_rate_100_everywhere']=all(x['welders'][1]==0 and x['stage']==2 and x['work']>=q['notes']['work_before_leave']-.01 and x['rate']==1.0 and x['guns_in_hand']==1 for x in s)
   h=hoods();q['notes']['hoods_after_leave']=h;q['checks']['released_welder_hood_hidden_everywhere']=all(x['equipped']==1 and x['closed']==1 for x in h)
   c=guests();T.queue_factory_action(c[0],'Run');stage=16;last=t
  elif stage==16 and t-last>2:
   q['notes']['paused_a']=[x['stations'][0]['work'] for x in reports()];stage=17;last=t
  elif stage==17 and t-last>3:
   h=hoods();q['notes']['hoods_paused']=h;q['checks']['paused_visor_raised_everywhere']=all(x['equipped']==1 and x['closed']==0 for x in h)
   b=[x['stations'][0]['work'] for x in reports()];q['checks']['client_pause_keeps_progress_everywhere']=b==q['notes']['paused_a'] and all(x['stations'][0]['stage']==2 for x in reports())
   T.queue_factory_action(guests()[0],'Run');stage=18;last=t
  elif stage==18:
   if t-last>2:T.advance_factory_test(host(),40)
   s=[x['stations'][0] for x in reports() if x['stations']]
   if t-last>4 and all(x['units']>=1 for x in s):
    q['checks']['first_rack_finished_once_on_every_peer']=len(s)==len(players()) and all(x['racks']==x['units']>=1 for x in s);q['notes']['finished']=s[0];stage=19;last=t
   elif t-last>150:raise RuntimeError('Unit never finished on every peer: '+json.dumps(s[:2]))
  elif stage==19:
   T.advance_factory_test(host(),40)
   if t-last>3:
    s=[x['stations'][0] for x in reports()];h=json.loads(T.welding_station_report(host()))['stations'][0];q['notes']['out_after']=[(x['out'],x['units']) for x in s]
    q['checks']['every_finished_rack_exists_once_every_peer_agrees']=all(x['racks']==x['units']==h['units'] for x in s);stage=20;last=t
  # Phase 5: rack orders, collection from OUT, booking and payment, all through client commands.
  elif stage==20:
   c=guests();o=[o for o in commerce(host())['orders'] if o['part']==16 and o['status']==0]
   if not o:raise RuntimeError('No headache-rack offer')
   q['notes']['rack_order']=o[0];oid=o[0]['id']
   for g in c[:2]:T.queue_commerce_action(g,'Order'+str(oid));T.queue_commerce_action(g,'Accept')
   stage=21;last=t
  elif stage==21 and t-last>3:
   oid=q['notes']['rack_order']['id'];cs=[commerce(p) for p in players()]
   q['checks']['rack_order_accepted_once_every_peer']=all(order(c,oid)['status']==1 for c in cs) and sum(1 for o in commerce(host())['orders'] if o['status']==1 and o['id']==oid)==1
   g=guests()[1];T.queue_commerce_action(g,'AssignOrder'+str(oid));stage=22;last=t
  elif stage==22 and t-last>2:
   oid=q['notes']['rack_order']['id'];g=guests()[2];T.queue_commerce_action(g,'StationOrder'+str(oid));stage=23;last=t
  elif stage==23 and t-last>3:
   oid=q['notes']['rack_order']['id'];s=[x['stations'][0] for x in reports()];q['notes']['station_after_order']=s[0]
   q['checks']['client_sets_station_from_order_every_peer']=all(x['recipe']==1 and x['batch']>=1 for x in s);q['notes']['racks_before_order']=s[0]['racks'];stage=24;last=t
  elif stage==24:
   h=host();oid=q['notes']['rack_order']['id'];hs=json.loads(T.welding_station_report(h))['stations'][0]
   if hs['stage']==0:T.setup_welding_assembly_review(h,hs['id'])
   c=commerce(h);staged=sum(a['quantity'] for a in c['pallets'] if a['place']==6 and a['order']==oid)
   if any(a['part']==16 and a['place']==11 and a['reserved'] for a in c['pallets']):q['notes']['out_claimed']=True
   if staged>=order(c,oid)['quantity']:stage=25;last=t
   else:T.advance_factory_test(h,20)
   if t-last>200:raise RuntimeError('Rack order never staged: '+json.dumps(hs))
  elif stage==25 and t-last>3:
   oid=q['notes']['rack_order']['id'];cs=[commerce(p) for p in players()];ids=sorted(a['id'] for a in cs[0]['pallets'] if a['place']==6 and a['order']==oid)
   q['checks']['forklift_collected_out_to_pickup']=q['notes'].get('out_claimed',False) and len(ids)>=1
   q['checks']['every_peer_sees_staged_racks']=all(sorted(a['id'] for a in c['pallets'] if a['place']==6 and a['order']==oid)==ids for c in cs);q['notes']['staged_ids']=ids
   g=guests()[0];T.queue_commerce_action(g,'Tablet');T.queue_commerce_action(g,'Carrier0')
   for i in ids:T.queue_commerce_action(g,'Pallet'+str(i))
   stage=26;last=t
  elif stage==26 and t-last>2:
   g=guests()[0];q['notes']['van_status']=commerce(g)['status'];q['checks']['client_van_refused']="won't fit" in q['notes']['van_status']
   T.queue_commerce_action(g,'Carrier2')
   for i in q['notes']['staged_ids']:T.queue_commerce_action(g,'Pallet'+str(i))
   T.queue_commerce_action(g,'Book');stage=27;last=t
  elif stage==27 and t-last>3:
   oid=q['notes']['rack_order']['id'];cs=[commerce(p) for p in players()]
   q['checks']['client_booked_rigid_truck_every_peer']=all(sum(1 for b in c['bookings'] if not b['inbound'] and b['order']==oid)==1 for c in cs);q['notes']['revenue_before']=commerce(host())['revenue'];stage=28;last=t
  elif stage==28:
   h=host();oid=q['notes']['rack_order']['id']
   if order(commerce(h),oid)['status']==2:stage=29;last=t
   else:T.advance_commerce_test(h,20)
   if t-last>200:raise RuntimeError('Rack order never delivered')
  elif stage==29:
   T.advance_commerce_test(host(),100)
   if t-last>4:
    oid=q['notes']['rack_order']['id'];cs=[commerce(p) for p in players()];o=[order(c,oid) for c in cs];q['notes']['paid']=o[0]
    q['checks']['rack_order_paid_exactly_once_every_peer']=all(x['paid']==x['quantity']*45000 and x['delivered']==x['quantity'] and x['status']==2 for x in o)
    q['checks']['cash_agrees_every_peer']=len(set(c['cash'] for c in cs))==1;hc=commerce(host());q['checks']['host_ledger_valid']=hc.get('authoritative_valid',False) and hc.get('cash_matches_registry',False)
    finish();return

  q['stage']=stage;out.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
