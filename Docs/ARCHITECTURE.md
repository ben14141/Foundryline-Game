# Foundryline repository architecture

Source: Master Plan v1.1, sections 20-23 (pages 35-38). The plan's example runtime
module name ManufacturingGame is named Foundryline in this project. Its 19 domain
directories are preserved, within standard Unreal Public/Private visibility folders.
These are system boundaries in one runtime module initially, not 19 empty build
modules. Split a domain into its own module when dependency/build needs justify it.

## Layout

```text
Foundryline.uproject
Config/                         Project-specific engine and game settings
Source/
  Foundryline.Target.cs          Game target
  FoundrylineEditor.Target.cs    Editor target
  Foundryline/
    Foundryline.Build.cs         Runtime dependencies
    Public/<System>/            Shared headers and data contracts
    Private/<System>/           System implementations
    Private/Tests/              Unreal automation tests
Content/
  Factory/                      Original Fab Factory LowPoly package; preserve paths
  Foundryline/<System>/         Game-owned definitions, presentation and content
  Foundryline/Maps/             Flat forest/grass maps and preserved setup maps
Docs/
  ARCHITECTURE.md                System boundaries and conventions
  Architecture/                 Technical contracts and limitations
  Design/                       Master plan, branding and UI specification
  Planning/                     Approved scope and milestone status
Tools/                          Read-only repository checks and inventory tools
```

## Domain responsibilities

| System | Owns | Current implementation |
| --- | --- | --- |
| Core | Module startup, shared IDs/types, events, simulation time | Runtime module entry |
| Grid | Coordinates, snapping, cell layers, occupancy queries | Centimetre grid math, bounds and snapping |
| Construction | Placement, surfaces, floors/walls, validation, demolition | Definitions, records, atomic actions and draft planning |
| Buildings | Building identity, footprints, edge walls, doors and generated visuals | Modular wall/floor/door practice via Construction; enclosure rules pending |
| Zones | Work-zone definitions, capabilities, minimum size and access | Reserved |
| Pathing | Employee/vehicle networks, route queries, congestion, invalidation | Paved driving maps, articulated paths, shared node graph and congestion costs |
| Employees | Company-scoped named roster, free test hiring, assignments, walking and production staff gates | [Employee foundation](Architecture/EMPLOYEES.md), [hiring](Architecture/EMPLOYEE_HIRING.md), [specific jobs](Architecture/EMPLOYEE_JOBS.md); payroll, skills and shifts remain later work |
| Production | Recipes, operations, dependencies, dynamic work and rework | Fixed recipes, machine cycles, linked racks and assembly; see Architecture/FIRST_ORDER_LOOP.md |
| Logistics | Freight, vehicles, receiving/shipping, staging and parking operations | Trip planning and server traffic coordinator; live fleet pending |
| Inventory | Quantities, containers, buffers, reservations and transfers | Exact pallet quantities, reservations, racks and forklift transfers |
| Orders | Customers' contracts, work orders, quotes and payment milestones | Fixed offers, acceptance, deadlines, manifests and delivery payment terms |
| Economy | Cash ledger, payroll, costs, receivables/payables and financing | Company cash, purchasing/carrier fees, operating costs and delivery payments; financing deferred |
| Management | Policies, delegation, authority limits and manager decisions | Reserved |
| Companies | Company/plant ownership, property and organization records | Server business registry, membership/invitations, exclusive plots and scoped permissions; see Architecture/BUSINESS_FOUNDATION.md |
| Competition | Competitor economy, bidding and acquisition opportunities | Reserved |
| World | Town composition, terrain/surfaces, parcels and scene presentation | Camera pawn, game mode and ground-picking math; uncompiled |
| UI | Camera interaction presentation, HUD, inspectors, overlays and screens | First-person HUD, office-gated Property View and company ERP modules; see Architecture/BUSINESS_ERP_UI.md |
| SaveSystem | Versioned snapshots, serialization, load validation and migrations | Placement snapshots and explicit async save-slot APIs |
| Networking | Server-validated commands and replicated construction state | Owning-controller construction and door RPCs; peer-local visuals |

## Dependency rules

- Core is independent of gameplay domains. Grid depends only on engine/core types.
- Construction consumes Grid and Companies ownership data. Its object records are
  runtime types, not types owned by the save system.
- SaveSystem composes domain records; records never depend on USaveGame.
- Networking translates authenticated player requests into trusted action context.
  The local single-player path uses the same authoritative construction service.
- UI and visual actors consume state; they never own inventory, money or production.
- Inventory is exact in simulation; meshes are approximate visual representations.
- Pathing consumes local geometry-change information; cosmetic edits must not cause
  global navigation rebuilds. Pathing and collision integration are not built yet.
- Runtime systems do not depend on UnrealEd or editor-only tools.
- No per-frame whole-town simulation; eventual simulation scheduling belongs to Core.

## Naming and content rules

- C++ types use the Foundryline prefix, Unreal F/U/A/E conventions and generated
  headers last. Use domain-qualified includes, not private relative include paths.
- Centimetres are the common physical unit. Snap classes never resize meshes.
- Currency uses integer cents. Do not introduce floating-point account balances.
- Stable definition IDs are explicit names; runtime entities use persistent GUIDs.
- Suggested authored assets: DA_ (definition), BP_ (visual actor), WBP_ (UI),
  M_/MI_ (material/instance), SM_ (static mesh), SK_ (skeletal mesh), T_ (texture),
  L_ (map). Imported vendor names remain unchanged.
- Keep direct Fab imports at original mount paths to preserve package references.
  Game-specific variants and wrappers belong under Content/Foundryline.
- Do not move .uasset files with filesystem tools to reorganize dependencies.
  Use Unreal migration/rename tools when package references need changing.
- No unlicensed purchased asset content is published to a public repository.
  Review vendor redistribution permissions before any push of Content/Factory.

## Mandatory design constraints

Multiplayer is mandatory, with more than two players supported in the design.
Local four-player construction mirroring and mixed-view state tests now run;
production player limits and large moving-object performance remain unmeasured. Roofs default hidden in top-down view with an optional show/hide control and
are mandatory in first-person view. True 1:1 scale, Lumen, 60+ FPS target, broad zoom, grid snapping and
hologram planning previews remain requirements, not verified completion claims.
The first scene and runtime foundations must be accepted before deeper production,
markets or acquisitions are implemented.


Construction state is server-owned and mirrored to client-local visuals. Camera
mode and cutaway roofs are per player. Moving vehicles, employees and machinery
must use a separate bounded simulation/replication path rather than rebuilding
construction. See Docs/Architecture/PROPERTY_EDITING.md for implementation limits.


Shared construction now uses owning-controller RPCs and final server validation;
local previews do not reserve footprints. Construction selections and camera/roof
views stay per player. The cooperative starter company context is server-owned.
Save/load stays with the host; undo/redo is standalone until per-player histories
exist. See Docs/Planning/SHARED_CONSTRUCTION_TEST_REPORT.md for acceptance scope.

Driving surfaces, parking targets, local logistics views and server-owned truck
zone metadata are documented in Docs/Architecture/DRIVING_AND_LOGISTICS.md.
Route previews use independent immutable snapshots; live traffic remains separate.

The playable factory loop now uses a separate server-owned bounded plant simulation
with pallet inventories, fixed developer-authored recipes, machine jobs, forklift
routes and incremental tanker assembly. Replicated snapshots drive local visual
interpolation and machine effects. Host saves include property and in-flight plant
state; camera, roof, computer menus and colored overlays stay local. See
Docs/Design/TANKER_PRODUCTION_REQUIREMENTS.md for rules and prototype limits.
