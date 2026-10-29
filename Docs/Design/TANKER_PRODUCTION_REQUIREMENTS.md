# Straight-barrel carbon-steel tanker: playable prototype

The authoritative bill is `FFoundrylineFactoryCatalog::TankerBill()`. Fixed machine
recipes are in `DefaultPrograms()`; players choose parts and batch sizes.
These are game balancing values, not a pressure-vessel fabrication specification.

## Product and space

All player-facing distances, dimensions, widths, heights and vehicle speeds use
feet/inches and mph. Stored geometry stays in real centimetres. Machine display
textures also use inches/feet. Floor lane width controls adjust in whole feet.

- Barrel diameter: 235 cm. Tank including both heads: approximately 1191 cm.
- Product footprint reserves the full trailer frame, approximately 1330 x 260 cm.
- Resized tanker-bay preset: 60 ft x 30 ft overall, including supplies. Minimum clear shop height: 450 cm.
- The 14 ft internal supply strip contains all six full-size pallet bays. The opposite side provides welder access; the tanker remains at full scale.
- At least one welder with a connected pedestrian path and one forklift driver
  on a supply route with connected forklift lanes. More workers count only after
  reaching their work positions.

The internal buffer contains separate named bays sized for the actual load:
brackets 180 x 180 cm, frame rails 180 x 360 cm, shells 180 x 210 cm, whole heads
285 x 285 cm, running gear 160 x 160 cm, and fittings 140 x 140 cm. These are
internal dimensions; their UI dimensions are displayed in feet and inches.
The bay positions and forklift destinations come from the same catalog layout.

## Bill and process chain

| Final part | Per tanker | Plasma time / steel | Next process |
| --- | ---: | --- | --- |
| Bent bracket | 24 | 8 s / 5 lb per blank | Press brake: 8 s per bracket |
| Frame rail | 12 | 15 s / 52 lb per blank | Press brake: 16 s per rail |
| Curved shell section | 48 | 12 s / 96 lb per blank | Roller: 18 s per section |
| Complete F&D head | 2 | Purchased complete | Weld to barrel; no partial heads |
| Running gear kit | 1 | Purchased | Install on frame |
| Valve and fittings kit | 1 | Purchased | Install after barrel and heads |

Eight longitudinal rings with six 60-degree sectors per ring make the shell.
The flat blanks fit the single 60 x 120 inch stock format. Whole heads are
purchased initially; the plasma and press brake do not produce dished heads.
Purchased-head stock uses a dedicated floor cradle rather than a shallow rack shelf.

Each part has a predetermined time at each required machine and predetermined
steel pounds per blank. Players cannot change these specifications. The press
and roller preserve the blank's recorded steel weight.
Material is reserved/consumed when processing begins. One finished part appears
only after that operation's timer finishes. Disabling a machine finishes the
current part, then prevents another cycle. Pausing the plant freezes all work.

The plasma draws whole sheets, preserves usable table remainder, and records
insufficient remnants as scrap. Minimum programmed cutting consumption for the
default bill is 5352 lb, excluding remnants and bought heads. Actual delivered
sheet count depends on remnants and the processing sequence.

## Transport, assembly and ownership

One pallet contains one part type with exact quantities in state. Mesh counts
are representative. Requests reserve source quantities and destination capacity.
Forklifts pick up, carry and unload exact quantities; cargo is never independently
spawned without deducting the source. Outputs go to assigned storage; intermediate
blanks go directly to the next machine. Linked routes can serve several sources
and destinations. Crossing and vehicle waits preserve all stock reservations.
The starter's shared freight corridor leases a loaded trip and return leg to one
driver at a time, rotating waiting drivers. This is intentionally conservative;
independent simultaneous corridor reservations for larger plants remain future work.

Welders reserve incremental assemblies, install parts over time and stop if the
next required parts are missing. Installed counts drive solid versus hologram
presentation. Finished products require inspection before transfer to pickup.

The current session is one shared cooperative company/plant. Only joined,
non-spectating players can issue validated production commands through their
own controller. Host-only saves include property, programs, pallets, reservations,
in-flight cargo, floor lanes, routes, employees and product progress. Camera,
roof, overlay and computer-menu preferences are local to each player.

## Controls

### Product work-zone presets

The tanker is the largest planned product class. The currently implemented product
is the tanker; smaller products will get their own smaller layouts, rather than
using tanker dimensions or shrinking the product mesh.

| Tanker preset | Complete footprint, including supplies |
| --- | --- |
| Compact | 66 ft x 54 ft |
| Standard | 72 ft x 60 ft |
| Tanker bay (resized large preset) | 60 ft x 30 ft |

Choose Floor / Routes, then New Tanker Work Zone. Select a preset, use R to
rotate in 90-degree steps and V to change the delivery side, then click once.
The full-size product and internal bays follow the cursor. Blue means the
footprint fits; red explains the obstruction, floor limit or paused-state check.
Right-click cancels placement. Lanes continue to use click and drag.

One orange perimeter encloses product space, clearance and the internal supplies.
The internal part-sized bays remain blue and carry the same zone number. Selecting
the perimeter or supplies opens that zone's inventory. The server creates the zone
and adds it to the chosen supply route in one operation; overlapping simultaneous
placements are rejected. Painted lanes still need to reach the supplies for actual
forklift access. No pallet or product scale changes with preset size.

Preset, rotation, delivery side and stock ownership persist in the plant save and
replicated state. Existing saved custom layouts retain their geometry and inventory;
the organized testing shop now uses the resized Tanker bay preset. Resizing occupied zones is not introduced here.

### Organized test shop

The test shop is 52 x 50 metres at 1:1 scale (about 170 ft 7 in x 164 ft).
The attached office remains separate, with two ground-level loading doors and an
expanded apron. The 60 ft x 30 ft tanker zone connects to the east loading wall; plasma,
brake and roller cells run along the west side. Floor geometry, machines, rack
positions and navigation share `FFoundrylineWorkshopLayout`.

Each machine has a three-level input rack and a three-level output rack. The
plasma input holds raw sheets and its feed pallet. Its output stores bracket,
rail and shell blanks. Brake supply levels hold bracket and rail blanks; brake
output levels hold bent brackets and finished rails. Roller supply holds shell
blanks and its feed pallet; roller output holds formed shells. Separate kit
racking and a floor cradle area hold bought-in running gear, fittings and whole heads.

Forklifts move cut blanks through the producing machine's output rack and the
receiving machine's input rack before feeding the machine. Final parts travel
from machine output racks to the assigned work-zone supplies. No decorative
stock is duplicated: all displayed pallets come from authoritative quantities.
Output dispatch batches to pallet capacity and flushes the last partial batch
when demand is covered. Raw feed replenishes by a sheet pallet rather than two sheets.

The Storage menu pages the thirteen named positions and shows available/incoming
quantities. Machine slot part assignments are fixed. Test layout saves use version
5; older prototype saves remain backed up but are not loaded into this new geometry.

Machine cells use orange diagonal floor hatching. Pallet racks, machine output
pickup pads, work-zone supplies and receiving use blue hatching; the informational
overlay adds light color fills. Forklift corridor markings remove internal lines
where corridors join, including T and cross junctions. Personnel walkways create
white striped crossings automatically at perpendicular forklift intersections.
People wait before entering a crossing when a forklift is approaching, then cross
after it clears. Forklifts retain priority, with immediate physical separation as
a safety stop. Crossing geometry is derived/cached from saved lane geometry.

The tanker work zone touches the pickup-door threshold. Its product sits near the door,
with the pickup tractor centred immediately outside. All six pallet bays are inside
the same zone boundary. Smaller presets can remain freestanding. A four-metre-wide clearance
aligns assembly with that opening, with a marked tractor pickup bay on the paved
apron. The other loading opening aligns with raw-sheet receiving. Both openings
retain five-metre clearance and road/slab height. These are tested placement and
access provisions; automatic commercial semi dispatch and product transfer onto
customer trucks remain a separate logistics milestone.

- P or Production: production sidebar. Click machines or use their sidebar buttons.
- Walk to a console and look at it: E opens its computer; Escape closes it.
- Click batch quantity to type, then Enter; +/- also adjust batch size. Run Selected
  Program sends part and quantity to the server. Time and weight are read-only.
  Auto follows product demand; Manual uses
  the selected batch quantity.
- Requirements shows the bill, process times, dimensions, equipment and staffing.
- Storage receives sheets and purchased kits and assigns empty storage positions.
- Floor / Routes paints persistent lanes and toggles the informational overlay.
- Orange boundaries, diagonal hatch fills and stencils identify work zones; blue hatched, named bays identify
  parts and receiving; double yellow lines/arrows identify forklifts; dashed green
  lines identify personnel; white zebra stripes identify crossings. The overlay
  adds translucent color fills, a legend, labels and selected-part bay dimensions without changing routes.
  Delivery bays use each part's footprint rather than one shared rectangle size.
  Player-facing dimensions use feet and inches; Unreal simulation remains centimeters at 1:1 scale.
- Save Plant / Load Plant use the host's FoundrylineFactory01 slot.

People currently use temporary worker proxies. Commercial ordering, payment,
specialized head forming and real worker animations are later systems.






### Standard welding equipment and paired bays

Every work-zone preset includes a full-scale MIG cart, gas bottle, gun and leads.
The cart stands behind the trailer, inside the zone and clear of its exit; its
physical collision also blocks navigation. Idle tools rest on the cart. When the
assigned worker reaches the product, the gun and routed lead appear at the work
point; sparks and light follow actual replicated assembly activity. There is no
automatic demonstration cycle in production zones. The placement hologram includes
the standard welder. Equipment is derived from saved zone geometry for every peer.

The stocked practice shop now has two matching 60 ft x 30 ft tanker bays, one at
each large loading door. The second bay mirrors its supply strip toward the
central receiving/storage space. That wall has exactly two 5 m truck doors, one
4 m forklift door centred between them, and one personnel door. Each bay has its
own six staged-parts pallets and assigned welder; the Production sidebar's zone
selector switches the selected job. General heads, kits and sheet receiving stay
between the bays, outside their reserved areas. Idle-zone buffers do not count
as shared supply for another zone's production demand.
