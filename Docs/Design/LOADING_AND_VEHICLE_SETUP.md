# Loading and vehicle setup

Vehicle choice follows the product, rather than the building tier. Dispatch will
need the packaged product's dimensions, weight, handling needs, and compatible
vehicle capacities. Select a suitable vehicle from that data; do not assume that
every small shop uses small trucks or that every delivery requires a semi.
Automatic product-based dispatch and the live delivery fleet are future work.

The starter property has two grade-level loading doors, a compact paved yard,
a wall-side reversing apron, and one editable truck entrance. A 3.5 m door is an
opening and loading position, not a raised platform or an internal ramp.

Construction setup lives under Build > Site > Loading:

1. Draw pavement beside the shop.
2. Connect it to the street with an adjustable truck driveway.
3. Click an exterior shop wall to place a loading door.

All three tools have hologram previews. Sections can be moved, resized, removed,
and expanded through validated construction actions. Solo history supports undo
and redo. Network undo of another player's actions is not implemented.

Resizing painted pavement clips driving permissions to its new footprint and
keeps retained route points at their world positions. Move or remove a route point
before shrinking away its pavement. Rejected edits leave the property unchanged.

Route testing offers a passenger car, an 8 m rigid box truck, and an articulated
semi. The box truck is 2.4 m wide and uses a 7 m minimum rear-axle turn radius.
These are explicit prototype dimensions, not claims about every real vehicle.
Freight permission zones apply to both truck types. Trailer articulation applies
only to semis. Each loading check uses the chosen vehicle's actual rear overhang
and swept body. The vehicle choices and previews are local to each player;
construction and permitted driving areas remain server-authoritative.

Road dimensions remain physical centimetres. Resize adjustable truck access
without stretching a vehicle or changing marked street lane widths. Build a larger
yard and entrance when a product's vehicle requires more turning or staging space.

Pavement uses hold-and-drag placement, with live dimensions and release to build.
Size presets are removed from the placement panel. Marked roads retain their lane
widths; truck access has an adjustable width while dragging sets its run.

Offices may now stand independently on owned land or connect to a shop. Their
minimum footprint is 4 x 4 m, with a required exterior door and a shop door when
attached. Shops and offices retain normal overlap and setback validation.

Building order is symmetric: placing a shop against an existing office creates
the shared-wall office doorway and preserves or adds an exterior entrance.
These changes are derived and validated atomically on the construction authority.
