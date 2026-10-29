"""Single-player rendered review of Phase 5 (commercial integration) through the real controller commands:
place the station, accept a headache-rack order on Sales and send it to the loading bay, staff the station (hire and assign
a welder; a forklift driver if none), set the station from the order (StationOrder shortcut), run the plant until the
forklift collects the OUT pallets to the pickup zone, book the truck on the loading tablet (the pallet van is refused
first), and run until the order is delivered and paid. A full OUT pallet (SetWeldingOutStacks review fixture) then shows
the forklift taking it to finished-goods storage. Screenshots go to Saved/Screenshots/Reviews (Phase5_*); the report
goes to Saved/Logs/WeldingPhase5VisualQA.json. No save slots."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools
base=Path(unreal.Paths.project_saved_dir());shots=base/'Screenshots/Reviews';out=base/'Logs/WeldingPhase5VisualQA.json'
start=time.monotonic();q={'checks':{},'notes':{},'shots':[],'errors':[]};cam=None;handle=None
CENTER=unreal.Vector(-1768.02,1823.12,20)
RACK=16;OUT=11;FORKS=5;PICKUP=6;STORED=1
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def player():return next(a for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if a.is_local_controller())
def make_guid(text):
 g=unreal.Guid()
 for n,i in zip('abcd',range(0,32,8)):g.set_editor_property(n,int(text.replace('-','')[i:i+8],16))
 return g
def look(p,loc,target,fov=60):
 global cam
 if not cam:cam=T.create_review_camera(p)
 cam.set_actor_location_and_rotation(loc,unreal.MathLibrary.find_look_at_rotation(loc,target),False,False);cam.camera_component.set_field_of_view(fov);p.set_view_target_with_blend(cam,0)
def shot(name):
 if any(name in s for s in q['shots']):return
 path=str(shots/(name+'.png'));unreal.AutomationLibrary.take_high_res_screenshot(1920,1080,path);q['shots'].append(path)
def station(p):
 r=json.loads(T.welding_station_report(p));return r['stations'][0] if r['stations'] else None
def commerce(p):return json.loads(T.commerce_report(p))
def staff(p):return json.loads(T.employee_report(p))
def action(p,s):T.queue_commerce_action(p,s)
def racks(c,place):return sum(a['quantity'] for a in c['pallets'] if a['part']==RACK and a['place']==place)
def order(c,oid):return next((o for o in c['orders'] if o['id']==oid),None)
spot=CENTER;stage='start';last=0;oid=0;sid=0;plant=None;wait_for=None;after=None;hold=0
def go(s,t):
 global stage,last
 stage=s;last=t
def tick(dt):
 global stage,last,spot,oid,sid,plant,wait_for,after,hold
 try:
  t=time.monotonic()-start
  if t>900:raise RuntimeError('Phase 5 review timeout at '+stage)
  if stage=='start' and t>3:go('pie',t);unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage=='pie' and t-last>14:
   p=player();T.queue_property_view(p);c=commerce(p);q['checks']['no_rack_offer_before_station']=not any(o['part']==RACK for o in c['orders'])
   T.queue_factory_lane(p,0,unreal.Vector(-2217.6,1275,20),unreal.Vector(-2217.6,2170,20),304.8)
   T.queue_factory_lane(p,1,unreal.Vector(-1455,1214.04,20),unreal.Vector(-810,1214.04,20),121.92)
   T.queue_factory_lane(p,2,unreal.Vector(-1370,1450,20),unreal.Vector(-1370,1450,20),200);go('place',t)
  elif stage=='place' and t-last>4:
   p=player();best=None
   for dx in [0,1,-1,2,-2]:
    for dy in [0,1,-1,2,-2]:
     c=unreal.Vector(CENTER.x+dx*15.24,CENTER.y+dy*15.24,20)
     if json.loads(T.preview_welding_station(p,c,0,False))['valid']:best=c;break
    if best:break
   spot=best or CENTER;q['notes']['commit']=json.loads(T.preview_welding_station(p,spot,0,False,True));T.queue_factory_action(p,'ToolStation');go('placed',t)
  elif stage=='placed' and t-last>3:
   p=player();s=station(p);q['checks']['station_placed']=bool(s)
   if not s:raise RuntimeError('Station was not placed')
   sid=s['id'];T.setup_welding_assembly_review(p,sid);T.advance_commerce_test(p,1);go('offer',t)
  elif stage=='offer' and t-last>2:
   p=player();c=commerce(p);offers=[o for o in c['orders'] if o['part']==RACK and o['status']==0];q['checks']['rack_offer_with_station']=bool(offers)
   if not offers:raise RuntimeError('No headache-rack offer appeared')
   oid=offers[0]['id'];q['notes']['order']=offers[0];action(p,'Office');action(p,'Page0');action(p,'Order'+str(oid));go('sales',t)
  elif stage=='sales' and t-last>2.5:shot('Phase5_SalesOrder_Offer');go('accept',t)
  elif stage=='accept' and t-last>1.5:p=player();action(p,'Order'+str(oid));action(p,'Accept');go('assign',t)
  elif stage=='assign' and t-last>2:p=player();action(p,'AssignOrder'+str(oid));go('hire',t)
  elif stage=='hire' and t-last>2:
   p=player();c=commerce(p);q['checks']['order_accepted_and_at_bay']=order(c,oid)['status']==1 and c['pickup_order']==oid
   plant=staff(p)['plant'];T.queue_employee_hire(p,1,make_guid('484952450000000500000001'+format(9,'08x')),make_guid(plant));go('welder',t)
  elif stage=='welder' and t-last>2:
   p=player();people=[e for e in staff(p)['employees'] if e['role']==3]
   if not people:raise RuntimeError('No hired person to assign')
   w=max(people,key=lambda e:e['id'])['id'];q['notes']['welder']=w;T.queue_employee_assignment(p,w,1,sid,make_guid(plant));go('driver',t)
  elif stage=='driver' and t-last>2:
   p=player();emp=staff(p)['employees'];q['notes']['staff']=[(e['id'],e['role'],e['assignment']) for e in emp]
   if not any(e['role']==2 and e['assignment'] for e in emp):
    T.queue_employee_hire(p,2,make_guid('484952450000000500000002'+format(9,'08x')),make_guid(plant));go('driver_assign',t)
   else:go('overview',t)
  elif stage=='driver_assign' and t-last>2:
   p=player();people=[e for e in staff(p)['employees'] if e['role']==3];c=commerce(p)
   if people and c['drivers']:T.queue_employee_assignment(p,max(people,key=lambda e:e['id'])['id'],2,c['drivers'][0]['id'],make_guid(plant))
   go('overview',t)
  elif stage=='overview' and t-last>2.5:p=player();action(p,'Page9');go('overview_shot',t)
  elif stage=='overview_shot' and t-last>2:shot('Phase5_Overview_SetStation');go('station_order',t)
  elif stage=='station_order' and t-last>1.5:p=player();action(p,'StationOrder'+str(oid));go('production',t)
  elif stage=='production' and t-last>2:
   p=player();s=station(p);q['checks']['station_set_from_order']=s['recipe']==1 and s['batch']==order(commerce(p),oid)['quantity'];action(p,'Page10');go('production_shot',t)
  elif stage=='production_shot' and t-last>2:shot('Phase5_Production_Station');go('close',t)
  elif stage=='close' and t-last>1.5:
   p=player();action(p,'Close');T.queue_factory_action(p,'Speed8');s=station(p);look(p,unreal.Vector(s['x']+620,s['y']-420,330),unreal.Vector(s['x']+250,s['y'],60),62);go('run',t)
  elif stage=='run':
   p=player();s=station(p);c=commerce(p)
   if s['stage']==0:T.setup_welding_assembly_review(p,sid)
   o=order(c,oid);staged=racks(c,PICKUP)
   reserved=[a for a in c['pallets'] if a['part']==RACK and a['place']==OUT and a['reserved']>0];onforks=[a for a in c['pallets'] if a['part']==RACK and a['place']==FORKS]
   if (reserved or onforks) and 'collect_seen_at' not in q['notes']:q['notes']['collect_seen_at']=t;q['checks']['forklift_collects_from_out']=True
   # The moment the forklift lifts a pallet off OUT: pause time, aim at that forklift, shoot, then carry on.
   if onforks and not any('Phase5_ForkliftCollectingFromOUT' in x for x in q['shots']) and hold==0:
    T.queue_factory_action(p,'Run');d=next((d for d in c['drivers'] if d['id']==onforks[0]['owner']),c['drivers'][0]);q['notes']['collecting_driver']=d
    look(p,unreal.Vector(d['x']+320,d['y']-380,280),unreal.Vector(d['x']+90,d['y'],90),62);hold=1;last=t
   elif hold==1 and t-last>2.5:shot('Phase5_ForkliftCollectingFromOUT');hold=3;last=t
   elif hold==3 and t-last>1.5:T.queue_factory_action(p,'Run');hold=2;s2=station(p);look(p,unreal.Vector(s2['x']+620,s2['y']-420,330),unreal.Vector(s2['x']+250,s2['y'],60),62)
   if staged>=o['quantity']:q['notes']['staged']=[a for a in c['pallets'] if a['place']==PICKUP];go('staged',t)
   elif hold not in (1,3):T.advance_factory_test(p,6)
  elif stage=='staged' and t-last>1:
   p=player();c=commerce(p);pid=[a['id'] for a in c['pallets'] if a['place']==PICKUP and a['order']==oid];q['notes']['pickup_pallets']=pid
   x=[a for a in c['pallets'] if a['place']==PICKUP][0];look(p,unreal.Vector(x['x']+420,x['y']-380,300),unreal.Vector(x['x'],x['y'],60),60);go('staged_shot',t)
  elif stage=='staged_shot' and t-last>2:shot('Phase5_RacksAtLoadingBay');go('van',t)
  elif stage=='van' and t-last>1.5:
   p=player();action(p,'Tablet');action(p,'Carrier0')
   for i in q['notes']['pickup_pallets']:action(p,'Pallet'+str(i))
   go('van_shot',t)
  elif stage=='van_shot' and t-last>2:
   p=player();c=commerce(p);q['notes']['van_status']=c['status'];q['checks']['van_refused_on_tablet']=not any(not b['inbound'] for b in c['bookings']) and "won't fit" in c['status'];shot('Phase5_Dispatch_VanRefused');go('truck',t)
  elif stage=='truck' and t-last>1.5:
   p=player();action(p,'Carrier2')
   for i in q['notes']['pickup_pallets']:action(p,'Pallet'+str(i))
   go('truck_shot',t)
  elif stage=='truck_shot' and t-last>2:shot('Phase5_DispatchManifest');go('book',t)
  elif stage=='book' and t-last>1.5:p=player();action(p,'Book');go('booked',t)
  elif stage=='booked' and t-last>2.5:
   p=player();c=commerce(p);b=[b for b in c['bookings'] if not b['inbound']];q['checks']['rack_pickup_booked']=len(b)==1 and sorted(i['pallet'] for i in b[0]['manifest'])==sorted(q['notes']['pickup_pallets'])
   q['notes']['revenue_before']=c['revenue'];action(p,'Page4');go('booked_shot',t)
  elif stage=='booked_shot' and t-last>2:shot('Phase5_Dispatch_Booked');go('deliver',t)
  elif stage=='deliver':
   p=player();c=commerce(p);o=order(c,oid)
   if o['status']==2:q['notes']['paid']=o;go('settle',t)
   else:T.advance_commerce_test(p,20)
  elif stage=='settle' and t-last>2:
   p=player();T.advance_commerce_test(p,200);go('paid',t)
  elif stage=='paid' and t-last>2:
   p=player();c=commerce(p);o=order(c,oid);q['checks']['paid_exactly_once']=o['paid']==o['quantity']*45000 and o['delivered']==o['quantity'];q['notes']['revenue_after']=c['revenue']
   q['checks']['revenue_counted_once']=c['revenue']-q['notes']['revenue_before']==(0 if o['payment']==0 else o['quantity']*45000) or c['revenue']-q['notes']['revenue_before']==o['quantity']*45000
   q['checks']['host_valid']=c.get('authoritative_valid',False);action(p,'Office');action(p,'Page2');action(p,'Order'+str(oid));go('paid_shot',t)
  elif stage=='paid_shot' and t-last>2.5:shot('Phase5_Payment_OrderComplete');go('money',t)
  elif stage=='money' and t-last>1.5:p=player();action(p,'Page5');go('money_shot',t)
  elif stage=='money_shot' and t-last>2.5:shot('Phase5_Payment_Ledger');go('stock',t)
  elif stage=='stock' and t-last>1.5:
   p=player();action(p,'Close');q['checks']['stock_fixture']=T.set_welding_out_stacks(p,sid,5,0);go('stock_run',t)
  elif stage=='stock_run':
   p=player();c=commerce(p);stored=[a for a in c['pallets'] if a['part']==RACK and a['place']==STORED]
   if stored and not any(a['part']==RACK and a['place'] in (OUT,FORKS) for a in c['pallets']):
    a=stored[0];q['notes']['stored']=a;q['checks']['full_pallet_to_storage']=a['quantity']==5;look(p,unreal.Vector(a['x']-380,a['y']-380,320),unreal.Vector(a['x'],a['y'],a['z']+40),58);go('stock_shot',t)
   else:T.advance_factory_test(p,6)
  elif stage=='stock_shot' and t-last>2.5:shot('Phase5_RackInStorage');go('stock_ui',t)
  elif stage=='stock_ui' and t-last>1.5:p=player();action(p,'Office');action(p,'Page11');go('stock_ui_shot',t)
  elif stage=='stock_ui_shot' and t-last>2.5:shot('Phase5_Stock_FinishedProducts');go('done',t)
  elif stage=='done' and t-last>1:
   q['completed']=True;out.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR');return
  q['stage']=stage;out.write_text(json.dumps(q,indent=1))
 except Exception:
  q['errors'].append(traceback.format_exc());out.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
handle=unreal.register_slate_post_tick_callback(tick)
