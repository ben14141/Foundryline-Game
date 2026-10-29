"""Reusable extra manufactured details; helpers and owning groups are supplied by builders."""
def add_details(kind,g):
 box,cyl,label,put,orange,dark,steel,graphite,white,yellow,red=[g[k] for k in ['box','cyl','label','put','orange','dark','steel','graphite','white','yellow','red']]
 import math
 if kind=='PressBrake':
  for side in [-1,1]:
   for z in [1.88,1.94,2.0,2.06,2.12]:put('Body',box('ServicePanelLouver',(-.28,side*1.951,z),(.41,.018,.025),dark,bevel=.004))
   for x in [-.54,-.02]:
    for z in [.96,1.76,2.32]:put('Body',cyl('CoverFastener',(x,side*1.956,z),.009,.018,steel,verts=6))
   put('Body',box('PanelHinge',(-.55,side*1.96,1.14),(.025,.04,.18),steel,bevel=.004))
  for y in [-1.15,-.72,-.29,.14,.57,1.0]:
   put('Body',box('BedWearPad',(.74,y,1.006),(.025,.20,.022),steel,bevel=.003))
  put('Body',box('MachineIDPlate',(.774,1.42,.64),(.012,.27,.12),steel,bevel=.003))
  put('Body',label('MachineSerial','PB-10 / 001',(.782,1.31,.63),(math.pi/2,0,math.pi/2),.023,graphite))
  for side in [-1,1]:
   put('Body',box('LinearEncoderScale',(.742,side*1.63,1.93),(.018,.045,.70),steel,bevel=.002))
   for j in range(24):put('Body',box('EncoderGraduation',(.753,side*1.63,1.61+j*.026),(.004,.035 if j%4==0 else .018,.002),dark))
   put('Ram',box('EncoderReader',(.762,side*1.63,1.92),(.045,.08,.085),graphite,bevel=.004))
   put('Ram',box('GuideWiper',(.699,side*1.59,1.59),(.065,.105,.025),g['rubber'],bevel=.003))
   for y in [side*1.10,side*1.22]:
    put('BackGauge',cyl('GaugeCarriageBearing',(-.12,y,1.15),.028,.075,steel,rot=(0,math.pi/2,0)))
   put('Body',cyl('BackGaugeLeadScrew',(-.24,side*1.15,1.16),.016,.68,steel,rot=(0,math.pi/2,0)))
  for j in range(12):
   y=-1.397+j*.254
   put('LowerDie',box('DieSegmentIdentification',(.616,y,.991),(.006,.08,.020),dark,bevel=.001))
 elif kind=='PlasmaTable':
  for x in [-.97,.97]:
   for y in [-1.73,1.73]:
    put('Body',box('RailEndStop',(x,y,1.15),(.11,.06,.065),dark,bevel=.005))
    put('Body',box('StopBuffer',(x,y*.978,1.15),(.075,.018,.04),g['rubber'],bevel=.004))
   for y in [-1.5,-.75,0,.75,1.5]:
    put('Body',box('SkirtPanelSeam',(x*1.065,y,.53),(.042,.009,.29),dark))
  for y in [-1.1,0,1.1]:
   for yy in [y-.40,y+.40]:
    put('Body',cyl('DrawerScrew',(1.10,yy,.39),.009,.012,steel,rot=(0,math.pi/2,0),verts=6))
  for x in [-.77,.77]:
   for y in [-1.48,0,1.48]:put('SlatBed',box('SlatCassetteLiftTab',(x,y,.94),(.06,.09,.12),steel,bevel=.005))
  for z in [1.38,1.41,1.44,1.47]:put('TorchCarriage',box('ZAxisBellows',(-.30,.335,z),(.17,.04,.018),g['rubber'],bevel=.003))
  put('TorchCarriage',box('TorchHeightSensor',(-.415,.205,1.23),(.045,.06,.11),dark,bevel=.004))
  for z in [1.17,1.21,1.25]:put('Torch',cyl('TorchCoolingRing',(-.30,.175,z),.037,.008,steel,rot=(0,0,0)))
  put('Gantry',box('BridgeServiceCover',(.63,.675,1.48),(.35,.012,.13),graphite,bevel=.004))
  for x in [.49,.77]:put('Gantry',cyl('BridgeCoverScrew',(x,.69,1.48),.009,.012,steel,verts=6))
  for z in [.20,.24,.28,.32,.36,.40,.44]:put('Extraction',box('FilterIntakeLouver',(-1.50,-1.55,z),(.43,.012,.014),dark,bevel=.002))
  put('Extraction',label('ExtractionLabel','FUME FILTER',(-1.73,-1.552,1.16),(math.pi/2,0,0),.055,white))
  put('Body',box('HotWorkWarning',(1.06,-1.0,.56),(.02,.37,.15),yellow,bevel=.003))
  put('Body',label('HotWorkText','HOT WORK',(1.075,-1.16,.54),(math.pi/2,0,math.pi/2),.035,graphite))

  # Rail bearing shoes, bolted axis couplings and replaceable torch wear parts.
  for x in [-.97,.97]:
   for y in [.43,.67]:
    put('Gantry',box('LinearBearingShoe',(x,y,1.105),(.11,.10,.035),steel,bevel=.003))
    put('Gantry',cyl('BearingGreaseNipple',(x,y,1.145),.009,.025,yellow,rot=(0,0,0),verts=8))
  for x in [-.365,-.235]:
   for z in [1.27,1.56]:put('TorchCarriage',box('ZGuideLinearBushing',(x,.265,z),(.045,.055,.075),steel,bevel=.004))
  put('TorchCarriage',cyl('ZMotorCoupler',(-.30,.29,1.59),.026,.065,steel,rot=(0,0,0)))
  for x in [-.37,-.23]:
   for z in [1.18,1.33]:put('Torch',cyl('TorchClampScrew',(x,.153,z),.006,.014,dark,verts=6))
  for x in [-.45,.45]:
   put('PowerUnit',box('PowerLiftingHandle',(-1.42+x*.24,1.0,.755),(.05,.26,.035),g['rubber'],bevel=.008))
