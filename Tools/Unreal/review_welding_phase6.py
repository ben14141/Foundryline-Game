"""Phase 6 rendered player flow from the small cut-parts shop, with real player commands and no stock fixtures:
paint and place the station, take a headache-rack order on Sales, hire and assign one welder, set the station from the
order, buy frame sections and light kits from the station panel, buy steel and cut the plates and brackets on the plasma,
build, ship and get paid; then hire a second welder (Welder B), take the next rack order and run it again.
Screenshots go to Saved/Screenshots/Reviews (Phase6_*); the report goes to Saved/Logs/WeldingPhase6VisualQA.json."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools
base=Path(unreal.Paths.project_saved_dir());shots=base/'Screenshots/Reviews';out=base/'Logs/WeldingPhase6VisualQA.json'
start=time.monotonic();q={'checks':{},'notes':{},'shots':[],'errors':[],'timeline':[]};cam=None;handle=None
CENTER=unreal.Vector(-1768.02,1823.12,20)
BRACKET=1;PLATE=10;FRAMES=14;LIGHTS=15;RACK=16;PICKUP=6;STATION_IN=10
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
taken=set()
def shot(name):
 # Marked before the call: taking a screenshot ticks Slate (and this callback) again before it returns.
 if name in taken:return
 taken.add(name);path=str(shots/(name+'.png'));q['shots'].append(path);unreal.AutomationLibrary.take_high_res_screenshot(1920,1080,path)
def station(p):
 r=json.loads(T.welding_station_report(p));return r['stations'][0] if r['stations'] else None
def commerce(p):return json.loads(T.commerce_report(p))
def staff(p):return json.loads(T.employee_report(p))
def action(p,s):T.queue_commerce_action(p,s)
def order(c,oid):return next((o for o in c['orders'] if o['id']==oid),None)
def at(s,dx,dy,z):return unreal.Vector(s['x']+dx,s['y']+dy,z)
stage='start';last=0;sid=0;plant=None;oid=0;run=0;want={PLATE:0,BRACKET:0};queued={PLATE:0,BRACKET:0};pending_shot=None;shot_at=0;moments=set();operate_at=0
def go(s,t):
 global stage,last
 stage=s;last=t;q['timeline'].append((round(t,1),s))
def queue_shot(name,cam_args,t):
 # Pause time, frame the moment, shoot, then resume (stage 'shoot' below).
 global pending_shot,shot_at
 if name in moments:return False
 moments.add(name);p=player();T.queue_factory_action(p,'Run');look(p,*cam_args);pending_shot=name;shot_at=t;return True
def operate(p,t):
 # What a player does from the panels: buy missing frames/lights at the station, cut the plates and brackets the order
 # needs on the plasma, and book the rigid truck when racks are at the loading bay.
 global operate_at
 if t-operate_at<1.0:return
 operate_at=t;s=station(p);c=commerce(p)
 if 'Missing frame' in s['supply']:T.select_welding_station(p,sid);T.queue_factory_action(p,'StationBuy'+str(FRAMES))
 if 'Missing light' in s['supply']:T.select_welding_station(p,sid);T.queue_factory_action(p,'StationBuy'+str(LIGHTS))
 m=next((m for m in c['machines'] if m['id']==1),None)
 if m and m['batch_remaining']<=0 and m['remaining']<=0:
  for part in (PLATE,BRACKET):
   if queued[part]<want[part]:T.queue_plasma_program(p,part,want[part]-queued[part]);queued[part]=want[part];q['notes'].setdefault('programs',[]).append((part,want[part]));break
 o=order(c,oid)
 if o and o['status']==1:
  staged=[a for a in c['pallets'] if a['place']==PICKUP and a['order']==oid and not a['booking']]
  booked=sum(i['quantity'] for b in c['bookings'] if not b['inbound'] and b['order']==oid for i in b['manifest'])
  if staged and (sum(a['quantity'] for a in staged)+booked+o['delivered']>=o['quantity'] or len(staged)>=2):
   action(p,'Tablet');action(p,'Carrier2')
   for a in staged:action(p,'Pallet'+str(a['id']))
   action(p,'Book');action(p,'Close')
def take_order(p):
 global oid,want
 c=commerce(p);offers=[o for o in c['orders'] if o['part']==RACK and o['status']==0]
 if not offers:return False
 oid=offers[0]['id'];action(p,'Order'+str(oid));action(p,'Accept');action(p,'AssignOrder'+str(oid));return True
def plan_parts(p):
 # Plates and brackets this order needs beyond rack stock; steel is bought for them (one sheet cuts many small parts).
 global want
 o=order(commerce(p),oid);n=o['quantity'];c=commerce(p)
 have={part:sum(a['quantity']-a['reserved'] for a in c['pallets'] if a['part']==part and a['place']==1) for part in (PLATE,BRACKET)}
 want[PLATE]=queued[PLATE]+max(0,2*n-have[PLATE]);want[BRACKET]=queued[BRACKET]+max(0,4*n-have[BRACKET]);q['notes'].setdefault('wanted',[]).append(dict(want))
 action(p,'BuySheets2');action(p,'BuySteel')
def tick(dt):
 global stage,last,sid,plant,run,pending_shot
 try:
  t=time.monotonic()-start
  if t>1500:raise RuntimeError('Phase 6 review timeout at '+stage)
  if pending_shot and stage not in ('shoot','resume'):go('shoot',t);return
  if stage=='shoot':
   if t-last>2.5:name=pending_shot;go('resume',t);shot(name)
   return
  if stage=='resume':
   if t-last>1:p=player();T.queue_factory_action(p,'Run');T.queue_factory_action(p,'Speed8');pending_shot=None;go(q['notes'].pop('return_to','run'),t)
   return
  if stage=='start' and t>3:go('pie',t);unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage=='pie' and t-last>14:
   p=player();T.queue_property_view(p);c=commerce(p);q['notes']['start']={'cash':c['cash'],'pallets':len(c['pallets'])}
   q['checks']['small_shop_starts_without_racks_or_station']=station(p) is None and not any(o['part']==RACK for o in c['orders'])
   T.queue_factory_lane(p,0,unreal.Vector(-2217.6,1275,20),unreal.Vector(-2217.6,2170,20),304.8)
   T.queue_factory_lane(p,1,unreal.Vector(-1455,1214.04,20),unreal.Vector(-810,1214.04,20),121.92)
   T.queue_factory_lane(p,2,unreal.Vector(-1370,1450,20),unreal.Vector(-1370,1450,20),200);go('place',t)
  elif stage=='place' and t-last>4:
   p=player();q['notes']['commit']=json.loads(T.preview_welding_station(p,CENTER,0,False,True));T.queue_factory_action(p,'ToolStation');go('placed',t)
  elif stage=='placed' and t-last>3:
   p=player();s=station(p);q['checks']['station_placed']=bool(s)
   if not s:raise RuntimeError('Station was not placed')
   sid=s['id'];plant=staff(p)['plant'];action(p,'Office');go('order',t)
  elif stage=='order' and t-last>2:
   p=player()
   if take_order(p):go('hire',t)
   else:
    if not commerce(p)['running']:T.queue_factory_action(p,'Run')
    T.advance_commerce_test(p,1)
  elif stage=='hire' and t-last>2.5:
   p=player();q['checks']['first_order_accepted']=order(commerce(p),oid)['status']==1
   T.queue_employee_hire(p,1,make_guid('484952450000000600000001'+format(1,'08x')),make_guid(plant));go('assign',t)
  elif stage=='assign' and t-last>2.5:
   p=player();people=[e for e in staff(p)['employees'] if e['role']==3]
   if not people:raise RuntimeError('No hired person to assign')
   q['notes']['welder_a']=max(people,key=lambda e:e['id'])['id'];T.queue_employee_assignment(p,q['notes']['welder_a'],1,sid,make_guid(plant),0);go('set',t)
  elif stage=='set' and t-last>2.5:p=player();action(p,'StationOrder'+str(oid));go('parts',t)
  elif stage=='parts' and t-last>2.5:
   p=player();s=station(p);q['checks']['station_set_from_first_order']=s['recipe']==1 and s['batch']>=1;plan_parts(p);action(p,'Close');T.select_welding_station(p,sid)
   look(p,at(s,620,-650,430),at(s,0,0,40),70);go('panel',t)
  elif stage=='panel' and t-last>3:p=player();T.queue_factory_action(p,'Speed8');run=1;q['notes']['run1_start']=t;go('run',t);shot('Phase6_StationPanel_BuyFramesAndLights')
  elif stage=='run':
   p=player();s=station(p);c=commerce(p);o=order(c,oid);operate(p,t)
   m=next((m for m in c['machines'] if m['id']==1),None)
   if m and m['remaining']>0 and m['part']==PLATE:q['notes']['return_to']='run';queue_shot('Phase6_PlasmaCuttingPlates',(unreal.Vector(-1300+420,800-520,420),unreal.Vector(-1300,800,80),60),t)
   if any(a['place']==STATION_IN for a in c['pallets']):q['notes']['return_to']='run';queue_shot('Phase6_SuppliesOnIN',(at(s,-470,-430,300),at(s,-200,0,60),60),t)
   if s['stage']==2 and s['advancing'] and s['work']>40 and (run==1 or s['at_work']==2):q['notes']['return_to']='run';queue_shot('Phase6_Welder%s_Welding'%('A' if run==1 else 'sAB'),(at(s,250,-430,240),at(s,-60,-40,100),55),t)
   if s['racks']>=1 and s['out']>=1:q['notes']['return_to']='run';queue_shot('Phase6_Run%d_RackOnOUT'%run,(at(s,560,-360,270),at(s,205,-20,60),55),t)
   if o['status']==2:
    q['notes']['run%d_paid'%run]=o;q['checks']['order_%d_paid_once'%run]=o['paid']==o['quantity']*45000 and o['delivered']==o['quantity'];q['notes']['run%d_seconds'%run]=t-q['notes']['run%d_start'%run]
    action(p,'Office');action(p,'Page2');action(p,'Order'+str(oid));go('paid_shot',t)
   else:T.advance_factory_test(p,6)
  elif stage=='paid_shot' and t-last>2.5:go('after_paid',t);shot('Phase6_Run%d_OrderPaid'%run)
  elif stage=='after_paid' and t-last>1:
   p=player()
   if run==2:action(p,'Page9');go('final',t)
   else:
    action(p,'Close');T.queue_employee_hire(p,1,make_guid('484952450000000600000002'+format(2,'08x')),make_guid(plant));go('assign_b',t)
  elif stage=='assign_b' and t-last>2.5:
   p=player();people=[e for e in staff(p)['employees'] if e['role']==3]
   if not people:raise RuntimeError('No second hire to assign')
   q['notes']['welder_b']=max(people,key=lambda e:e['id'])['id'];T.queue_employee_assignment(p,q['notes']['welder_b'],1,sid,make_guid(plant),1);action(p,'Office');go('order2',t)
  elif stage=='order2':
   p=player()
   if take_order(p):go('set2',t)
   else:T.advance_commerce_test(p,2)
  elif stage=='set2' and t-last>2.5:p=player();action(p,'StationOrder'+str(oid));go('parts2',t)
  elif stage=='parts2' and t-last>2.5:
   p=player();s=station(p);q['checks']['second_welder_assigned']=s['welders'][1]==q['notes']['welder_b'];plan_parts(p);action(p,'Close');T.queue_factory_action(p,'Speed8');run=2;q['notes']['run2_start']=t;go('run',t)
  elif stage=='final' and t-last>2.5:
   go('final_done',t);shot('Phase6_Overview_AfterTwoOrders');p=player();c=commerce(p);q['notes']['end_cash']=c['cash'];q['checks']['made_money']=c['cash']>q['notes']['start']['cash'];q['checks']['host_valid']=c.get('authoritative_valid',False)
   action(p,'Close');go('done',t)
  elif stage=='final_done':pass
  elif stage=='done' and t-last>1:
   q['completed']=True;out.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR');return
  q['stage']=stage;out.write_text(json.dumps(q,indent=1))
 except Exception:
  q['errors'].append(traceback.format_exc());out.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
handle=unreal.register_slate_post_tick_callback(tick)
