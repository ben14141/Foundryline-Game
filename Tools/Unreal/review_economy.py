"""Economy v1 rendered review: place a welding station, hire and assign a welder, let game time pass with the plant running,
then shoot the office Team screen (each person's wage and the plant payroll), the Money page (payroll per hour and the running
Wages line) and Home (the idle-payroll nudge to take another order).
Screenshots go to Saved/Screenshots/Reviews (Economy_*); the report to Saved/Logs/EconomyVisualQA.json. No save slots."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools
base=Path(unreal.Paths.project_saved_dir());shots=base/'Screenshots/Reviews';out=base/'Logs/EconomyVisualQA.json'
start=time.monotonic();q={'checks':{},'notes':{},'shots':[],'errors':[]};handle=None;taken=set()
CENTER=unreal.Vector(-1768.02,1823.12,20)
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def player():return next(a for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if a.is_local_controller())
def make_guid(text):
 g=unreal.Guid()
 for n,i in zip('abcd',range(0,32,8)):g.set_editor_property(n,int(text.replace('-','')[i:i+8],16))
 return g
def shot(name):
 # Marked before the call: taking a screenshot ticks Slate (and this callback) again before it returns.
 if name in taken:return
 taken.add(name);path=str(shots/(name+'.png'));q['shots'].append(path);unreal.AutomationLibrary.take_high_res_screenshot(1920,1080,path)
def commerce(p):return json.loads(T.commerce_report(p))
def staff(p):return json.loads(T.employee_report(p))
stage='start';last=0;sid=0;plant=None
def go(s,t):
 global stage,last
 stage=s;last=t
def tick(dt):
 global sid,plant
 try:
  t=time.monotonic()-start
  if t>400:raise RuntimeError('Economy review timeout at '+stage)
  if stage=='start' and t>3:go('pie',t);unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage=='pie' and t-last>14:
   p=player();T.queue_property_view(p)
   T.queue_factory_lane(p,0,unreal.Vector(-2217.6,1275,20),unreal.Vector(-2217.6,2170,20),304.8)
   T.queue_factory_lane(p,1,unreal.Vector(-1455,1214.04,20),unreal.Vector(-810,1214.04,20),121.92)
   T.queue_factory_lane(p,2,unreal.Vector(-1370,1450,20),unreal.Vector(-1370,1450,20),200);go('place',t)
  elif stage=='place' and t-last>4:p=player();T.preview_welding_station(p,CENTER,0,False,True);T.queue_factory_action(p,'ToolStation');go('hire',t)
  elif stage=='hire' and t-last>3:
   p=player();sid=json.loads(T.welding_station_report(p))['stations'][0]['id'];plant=staff(p)['plant'];T.queue_employee_hire(p,1,make_guid('4849524500000007000000010000000a'),make_guid(plant));go('assign',t)
  elif stage=='assign' and t-last>2.5:
   p=player();people=[e for e in staff(p)['employees'] if e['role']==3];T.queue_employee_assignment(p,max(people,key=lambda e:e['id'])['id'],1,sid,make_guid(plant),0);go('run',t)
  elif stage=='run' and t-last>2.5:
   p=player();c=commerce(p);q['notes']['before']={'cash':c['cash'],'payroll':c['payroll'],'wages':c['wages_paid']}
   if not c['running']:T.queue_factory_action(p,'Run')
   go('pass',t)
  elif stage=='pass':
   p=player();T.advance_commerce_test(p,20)
   if t-last>6:
    c=commerce(p);q['notes']['after']={'cash':c['cash'],'payroll':c['payroll'],'wages':c['wages_paid'],'clock':c['clock']}
    q['checks']['payroll_is_48_per_hour']=c['payroll']==4800;q['checks']['wages_paid']=c['wages_paid']>q['notes']['before']['wages'];q['checks']['host_valid']=c.get('authoritative_valid',False)
    T.queue_factory_action(p,'Run');action=T.queue_commerce_action;action(p,'Office');action(p,'Page7');go('team',t)
  elif stage=='team' and t-last>2.5:go('team2',t);shot('Economy_Team_PayrollAndRates')
  elif stage=='team2' and t-last>1:p=player();T.queue_commerce_action(p,'Page5');go('money',t)
  elif stage=='money' and t-last>2.5:go('home',t);shot('Economy_Money_WagesLine')
  elif stage=='home' and t-last>1:p=player();T.queue_commerce_action(p,'Page9');go('home_shot',t)
  elif stage=='home_shot' and t-last>2.5:go('done',t);shot('Economy_Home_PayrollNudge')
  elif stage=='done' and t-last>1.5:
   q['completed']=True;out.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR');return
  q['stage']=stage;out.write_text(json.dumps(q,indent=1))
 except Exception:
  q['errors'].append(traceback.format_exc());out.write_text(json.dumps(q,indent=1));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
handle=unreal.register_slate_post_tick_callback(tick)
