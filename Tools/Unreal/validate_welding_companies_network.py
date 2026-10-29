"""Five-player listen-server check of welding stations in independent companies (Phase 6).
Two players found their own businesses (ExerciseBusinessFoundation: A and B on their own plots, a third player joins A), the
host and the fifth player stay in the starter co-op. Each independent company places its own station, hires and assigns its
own welder and builds a rack in its own plant through real client commands; commands aimed at another company's plant
(place, remove, configure, assign a welder) are refused without changing it. No save slots are written."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools
start=time.monotonic();stage='start';last=0;q={'checks':{},'notes':{},'errors':[]};out=Path(unreal.Paths.project_saved_dir())/'Logs/WeldingCompaniesNetworkQA.json'
CENTER=unreal.Vector(-1768.02,1823.12,20);OTHER=unreal.Vector(-700,1000,20)
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def players():return [a for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if a.is_local_controller()]
def host():return next(p for p in players() if p.has_authority())
def commerce(p):return json.loads(T.commerce_report(p))
def welding(p):return json.loads(T.welding_station_report(p))
def staff(p):return json.loads(T.employee_report(p))
def make_guid(text):
 g=unreal.Guid();h=text.replace('-','')
 for n,i in zip('abcd',range(0,32,8)):g.set_editor_property(n,int(h[i:i+8],16))
 return g
def check(n,v):q['checks'][n]=bool(v)
def finish():
 q['completed']=True;out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
groups={}
def members(key):return [p for p in players() if commerce(p)['company']==groups[key]['company']]
def go(s,t):
 global stage,last
 stage=s;last=t
def tick(dt):
 global stage,last
 try:
  t=time.monotonic()-start
  if t>600:raise RuntimeError('Welding companies network timeout at '+stage)
  if stage=='start' and t>3:go('pie',t);unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage=='pie' and t-last>16:
   check('five_players',len(players())==5);setup=json.loads(T.exercise_business_foundation(host(),0));setup.pop('create_error',None);q['notes']['foundation']=setup
   check('two_independent_businesses_founded',setup.get('two_independent_businesses'));go('companies',t)
  elif stage=='companies' and t-last>14:
   rs=[(p,commerce(p)) for p in players()];by={}
   for p,r in rs:by.setdefault(r['company'],{'plant':r['plant'],'players':[]})['players'].append(p)
   starter=commerce(host())['company'];indep=[c for c in by if c!=starter]
   check('three_companies',len(by)==3);check('host_and_one_client_stay_in_starter',len(by[starter]['players'])==2)
   check('independent_company_sizes_two_and_one',sorted(len(by[c]['players']) for c in indep)==[1,2])
   a=next(c for c in indep if len(by[c]['players'])==2);b=next(c for c in indep if len(by[c]['players'])==1)
   groups['A']={'company':a,'plant':by[a]['plant']};groups['B']={'company':b,'plant':by[b]['plant']};groups['S']={'company':starter,'plant':by[starter]['plant']}
   q['notes']['groups']={k:{'company':v['company'],'plant':v['plant']} for k,v in groups.items()}
   for k in 'AB':
    p=members(k)[0]
    T.queue_factory_lane(p,0,unreal.Vector(-2217.6,1275,20),unreal.Vector(-2217.6,2170,20),304.8)
    T.queue_factory_lane(p,1,unreal.Vector(-1455,1214.04,20),unreal.Vector(-810,1214.04,20),121.92)
    T.queue_factory_lane(p,2,unreal.Vector(-1370,1450,20),unreal.Vector(-1370,1450,20),200)
   go('place',t)
  elif stage=='place' and t-last>4:
   for k in 'AB':T.queue_welding_station(members(k)[0],CENTER,0,False)
   go('placed',t)
  elif stage=='placed' and t-last>4:
   ra=[welding(p) for p in members('A')];rb=[welding(p) for p in members('B')];rs=[welding(p) for p in members('S')]
   check('each_company_places_its_own_station',all(len(r['stations'])==1 for r in ra+rb));check('starter_plant_has_no_station',all(len(r['stations'])==0 for r in rs))
   check('company_members_see_the_same_station',len(set(json.dumps(r['stations'][0]['x']) for r in ra))==1)
   groups['A']['station']=ra[0]['stations'][0]['id'];groups['B']['station']=rb[0]['stations'][0]['id']
   # B aims real station commands at A's plant: remove A's station, add a second one, set A's batch. The fifth player (starter co-op) tries too.
   pb=members('B')[0];ps=next(p for p in members('S') if not p.has_authority());plant_a=make_guid(groups['A']['plant'])
   T.queue_welding_command_for_plant(pb,plant_a,'Remove',groups['A']['station'],CENTER,0);T.queue_welding_command_for_plant(pb,plant_a,'Add',0,OTHER,0)
   T.queue_welding_command_for_plant(pb,plant_a,'Configure',groups['A']['station'],CENTER,7);T.queue_welding_command_for_plant(ps,plant_a,'Configure',groups['A']['station'],CENTER,9)
   go('foreign',t)
  elif stage=='foreign' and t-last>4:
   ra=[welding(p) for p in members('A')];a=ra[0]['stations']
   check('foreign_company_cannot_remove_add_or_configure_a_station',all(len(r['stations'])==1 and r['stations'][0]['id']==groups['A']['station'] and r['stations'][0]['recipe']==0 for r in ra))
   q['notes']['foreign_status']=[welding(p)['status'] for p in members('B')+members('S')]
   # A's second member (who joined A) sets A's batch through the same command: allowed.
   pa2=members('A')[1];T.queue_welding_command_for_plant(pa2,make_guid(groups['A']['plant']),'Configure',groups['A']['station'],CENTER,1);T.queue_welding_command_for_plant(members('B')[0],make_guid(groups['B']['plant']),'Configure',groups['B']['station'],CENTER,1)
   go('configured',t)
  elif stage=='configured' and t-last>3:
   check('company_member_configures_own_station',all(welding(p)['stations'][0]['recipe']==1 and welding(p)['stations'][0]['batch']==1 for p in members('A')+members('B')))
   for i,k in enumerate('AB'):T.queue_employee_hire(members(k)[0],1,make_guid('48495245000000060000000'+str(i+1)+format(7,'08x')),make_guid(groups[k]['plant']))
   go('hired',t)
  elif stage=='hired' and t-last>3:
   for k in 'AB':
    p=members(k)[0];people=[e for e in staff(p)['employees'] if e['role']==3];q['notes']['staff_'+k]=staff(p)['employees']
    if not people:raise RuntimeError('No hired person in company '+k)
    groups[k]['welder']=max(people,key=lambda e:e['id'])['id']
   # B first tries to put its own new hire on A's station, then each company assigns its own.
   T.queue_employee_assignment(members('B')[0],groups['B']['welder'],1,groups['A']['station'],make_guid(groups['A']['plant']));go('foreign_assign',t)
  elif stage=='foreign_assign' and t-last>3:
   check('foreign_welder_assignment_refused',all(welding(p)['stations'][0]['welders']==[0,0] for p in members('A')))
   for k in 'AB':T.queue_employee_assignment(members(k)[0],groups[k]['welder'],1,groups[k]['station'],make_guid(groups[k]['plant']))
   go('assigned',t)
  elif stage=='assigned' and t-last>3:
   check('each_company_assigns_its_own_welder',all(welding(p)['stations'][0]['welders'][0]==groups[k]['welder'] for k in 'AB' for p in members(k)))
   for k in 'AB':check('fixture_'+k,T.setup_welding_assembly_review_for_plant(host(),groups[k]['station'],make_guid(groups[k]['plant'])));T.queue_factory_action(members(k)[0],'Speed8')
   go('build',t)
  elif stage=='build':
   ra=welding(members('A')[0])['stations'][0];rb=welding(members('B')[0])['stations'][0];q['notes']['build']={'A':ra,'B':rb}
   if ra['units']>=1 and rb['units']>=1:go('built',t)
   elif t-last>300:raise RuntimeError('Racks never finished: '+json.dumps(q['notes']['build']))
  elif stage=='built' and t-last>3:
   for k in 'AB':check('company_'+k+'_builds_one_rack_seen_by_its_members',all(welding(p)['stations'][0]['units']==1 and welding(p)['stations'][0]['racks']==1 for p in members(k)))
   check('starter_plant_still_has_no_station_or_racks',all(len(welding(p)['stations'])==0 for p in members('S')))
   cs={k:[commerce(p)['cash'] for p in members(k)] for k in 'ABS'};q['notes']['cash']=cs
   check('members_agree_on_their_company_cash',all(len(set(v))==1 for v in cs.values()))
   ws={k:[(commerce(p)['payroll'],commerce(p)['wages_paid']) for p in members(k)] for k in 'AB'};q['notes']['wages']=ws
   check('each_company_pays_its_own_welder',all(len(set(v))==1 and v[0][0]>0 and v[0][1]>0 for v in ws.values()))
   check('host_ledger_valid',commerce(host()).get('authoritative_valid',False))
   finish();return
  q['stage']=stage;out.write_text(json.dumps(q,indent=2))
 except Exception:
  q['errors'].append(traceback.format_exc());out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
handle=unreal.register_slate_post_tick_callback(tick)
