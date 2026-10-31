# Computer Planning View: tool wheel

The tool wheel is a quick-access control inside the office computer's Planning View. It is unavailable while walking, on the ERP desktop, in a machine screen, or while directly operating a vehicle. The existing inspectors remain available for dimensions and detailed settings.

Click the active-tool bar at the bottom of the map, or press Tab, to open the wheel. Its center always names the current tool, including tools chosen through the sidebar. Choose a category, then a tool. Tool selection closes the wheel so the map stays visible. Click the center to go back, or right-click/Escape to close. Clicking outside closes it without placing anything.

| Category | Tools |
| --- | --- |
| Pavement | Rectangle, Custom Shape, Paint, Edit, Erase, Sidewalk |
| Roads | Straight, Road Spline, Truck Spline, Entrance, Edit Road, Markings |
| Logistics | Overlay, Draw Route, Edit Route, Driving Zones, Parking overlay, Trip Check |
| Paint submenu | Parking, white/yellow/dashed lines, arrows, loading bay preset, keep-clear hatching, stop lines |

Entrance opens pavement selection; select a road to adjust its entrance radius in the compact inspector. Trip Check opens the guided entrance/destination/exit workflow. Parking in Logistics displays parking information; parking paint is in the Paint submenu.

A draft must be finished or cancelled before changing tools through the wheel. Wheel input is handled before map construction or route drawing. Polar hit testing uses the same sectors as rendering, and viewport conversion accounts for camera letterboxing. Hover transitions, white panels, grey text and orange accents follow the current Foundryline UI.

Wheel pages, active highlighting and open/closed state belong to each player's HUD. Switching tools must not change another player's tools or mutate shared construction. Existing placement and logistics commands continue through their authenticated company/plant validation paths.

Validation tools: `Tools/Unreal/review_planning_wheel.py` drives the wheel using actual mouse input, checks first-person gating and local tool state, then checks shared pavement and late-join replication. Unit and full-suite evidence remains separate from rendered acceptance.

## Acceptance - 2026-10-30

Editor compilation and all 151 automated tests passed. The focused wheel review passed all 16 checks using actual pointer input: computer-planning gating, category/submenu selection, current tool display, Back, closing without changing tools, avoiding accidental map placement, per-player tool isolation and fifth-player late join. Reports: `Saved/Logs/WheelQuickQA.json` and `Saved/Logs/WheelQuickReview.log`. Reviewed screenshots: `Saved/Screenshots/Reviews/Planning_Wheel_Main00001.png` and `Planning_Wheel_Roads00001.png`.

Road Spline selects a 24 ft two-way road; Truck Spline selects a 30 ft two-way road. Click successive points and press Enter to finish. The inspector exposes point editing and the turning check. Back from road markings returns to Roads; Back from pavement paint returns to Pavement.

Loading Bay selects the complete wall-snapped starter preset, including its door, apron and fixed markings. It no longer selects custom loading paint. See LOADING_BAY_PRESETS.md for placement and upgrades.
