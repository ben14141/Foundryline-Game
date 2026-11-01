# Windows playtest

Pull main, then double-click **Play-Foundryline.cmd** at the repository root.
It downloads the Windows playtest from this repository's GitHub Release, checks
its SHA-256 checksum, installs under LocalAppData/Foundryline/Playtest, and starts
the game. Later launches reuse the verified installed version. An internet
connection is needed for the initial download and release lookup.

No Unreal Editor, Visual Studio, private asset backup, or separate Fab purchases
are required to play this compiled build. Windows x64 and a DX12-capable GPU are
required. If Windows reports a missing Visual C++ runtime, run
Windows/Prerequisites/vc_redist.x64.exe from the extracted download, then launch
again. This is Microsoft's x64 redistributable supplied with the engine.

You can also download Foundryline-Windows.zip from the GitHub Release, extract
the entire archive, and run Windows/Foundryline.exe. Keep all accompanying
folders. Do not run the executable inside the zip.

This is an unsigned development playtest. Local co-op foundations have been
tested, but internet connectivity, discovery, firewall/NAT traversal, and
packaged internet multiplayer are not established by this release.

## Working on the source

The public Git checkout still excludes purchased source assets and local maps.
Opening the uproject is a developer workflow and requires authorized content
restoration. Downloading the playtest does not install editable Unreal assets.
The included assets may only be used as incorporated in Foundryline; do not
extract or redistribute them separately. See the
[Fab license](https://www.fab.com/eula) and [repository content](REPOSITORY_CONTENT.md).
