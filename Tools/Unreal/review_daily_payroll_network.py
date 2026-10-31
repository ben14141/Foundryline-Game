"""Four peers: real business flow, paid idle/unassigned staff, one midnight bill."""
from pathlib import Path
import unreal
base=Path(unreal.Paths.project_dir()).resolve()/'Tools/Unreal/review_rack_dispatch.py'
exec(compile(base.read_text().split('FLOW=flow()')[0],str(base),'exec'))
OUT=R/'Saved/Logs/DailyPayrollNetworkQA.json';BASE_FLOW=flow

def flow():
 yield from BASE_FLOW();q['completed']=False
 ps=players();h=host();c=next(p for p in ps if p!=h)
 N.setup_operations_review(c,1);yield 1;action(c,'Office');yield 1
 before=state();action(c,'HirePerson');yield 1
 check('client_hires_without_upfront_salary_charge',state()['cash']==before['cash'])
 yield 1;check('unassigned_hire_in_daily_payroll_on_four_peers',all(state(p)['payroll']==36000 for p in ps))
 before=state();q['before_midnight']=before;boundary=(int(before['clock']//1440)+1)*1440
 factory(h,'Run');yield .5
 for _ in range(60):
  if state()['clock']>=boundary+1:break
  advance();yield .2
 if state()['running']:factory(h,'Run');yield 1
 after=state();q['after_midnight']=after
 bill=after['wages_paid']-before['wages_paid']
 check('idle_and_unassigned_workers_earn_salary',bill>0 and after['clock']>=boundary)
 check('payday_debits_the_shared_company_once',before['cash']-after['cash']==bill)
 check('new_day_accrual_is_smaller_than_daily_budget',0<=after['wages_accrued']<after['payroll'])
 yield 1;check('four_peers_agree_daily_bill_and_liability',all(all(state(p)[k]==after[k] for k in ['cash','wages_paid','wages_accrued','payroll']) for p in ps))
 clock=state()['clock'];paid=state()['wages_paid'];pending=state()['wages_accrued'];yield 1
 check('pause_freezes_clock_and_payroll_on_all_peers',all(state(p)['clock']==clock and state(p)['wages_paid']==paid and state(p)['wages_accrued']==pending for p in ps))
 q['completed']=True
FLOW=flow()
exec(compile(base.read_text().split('FLOW=flow()')[1],str(base),'exec'))
