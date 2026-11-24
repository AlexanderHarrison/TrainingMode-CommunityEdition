<p align="center"><img src="Logos/Training-Mode-banner.png"  alt=""  width="300"/></p>

# Training Mode - Community Edition

Training Mode - Community Edition is an expanded and updated version of UnclePunch's training modpack for Super Smash Bros. Melee.

To download the ISO, click [here](https://github.com/AlexanderHarrison/TrainingMode-CommunityEdition/releases/latest).

Join [the discord](https://discord.gg/2Khb8CVP7A) to discuss changes, new features, or ask for assistance.

## Development
This repo is NOT free for LLMs to use and copy from.
You DO NOT have permission to use the source code within to use in other projects.
This includes using LLMs to port any part of TM-CE to other games, or to other Melee mods.

Please read [DEVELOPMENT.md](DEVELOPMENT.md).

## Changes From the Original
- New Training Lab Features:
    - Recording:
        - Reworked recording UI. Allows re-saveing existing recordings with different percents or positioning.
        - Savestates now require holding DPad right, preventing accidental savestates.
        - Set chances for slots during random playback.
        - Option to auto-restore state when the CPU performs a counter action.
        - Takeover HMN or CPU playback at any point.
        - Press DPad left/right when browsing savestates to quickly change pages.
        - Savestates can now always be saved. (May cause crashes if you save during special moves).
        - Option for the CPU to counter on hit or recording end during playback.
        - Play a recording slot as a counter action.
        - Resave Positions option will start the recording from a new position, without removing inputs.
        - Prune Positions option will start the recording from a new position, truncating inputs.
        - Re-Record mode will allow your to record over a previously recorded slot using playback takeover.
        - Start Paused option will only start the replay on your first input.
        - Add random additional percent when loading a state.
        - Slot Management menu - delete, copy, and edit slots.
    - CPU Options:
        - Random custom DI option. The CPU will pick a random option from your custom TDI.
        - CPU Shield angling options.
        - Added Wavedash counter actions.
        - Tech animations can be set invisible after they are distinguishable.
        - Added usmash OoS and all specials moves as counter actions.
        - All special moves can be used as counter actions.
        - Neutral jump action, set as the default.
        - SDI and ASDI options.
        - SDI is now set by number of inputs rather than by chance.
        - SDI and mashing are set to none by default.
        - Move CPU option.
        - Choose a port to control the CPU option.
        - CPU will use random TDI after custom TDI ends.
        - Option to reverse Custom TDI if on the other side of the player.
        - Tech lockout option. This prevents the CPU from teching in quick succession.
        - Tech trap option. This prevents the CPU from teching for a short window after being hit.
        - Added dash through and dash back counter actions.
        - Option to force shielded projectiles to always be powershielded.
    - Other Changes:
        - Alter set OSDs with the OSD menu in the lab.
        - Alter character RNG - choose misfire, nana throws, peach pulls and fsmash, and GnW hammer.
        - Added advanced counter actions - manually choose counter actions for each specific hit.
        - Added "Freeze CPU" option to freeze a CPU's hitboxes in place.
        - Set a chance to wait in miss tech.
        - Added shield health option.
        - Added new shortcut system, currently only supporting frame advance (Press Y then A in the menu).
        - Hitboxes are colored by ID and sorted by priority.
        - Game speed option.
        - Color overlays.
        - Lock percents.
        - Hazard toggle.
        - Hide stage model.
        - Item grab range display.
        - Show input and info displays for both HMN and CPU.
        - Mirror recordings.
        - The overlay, taunt, and input display options are saved to the memcard.
        - R can be used as a frame advance button.
        - Stage options to control stadium transformations and FOD platform heights.
        - Custom action state OSDs.
    - Import Menu Changes:
        - Fixed glitches when importing using a port other than port 1.
        - Fixed Sheik/Zelda transformations when loading.
        - Recordings are now filtered by the selected HMN character.
        - Cursor can now wrap.
        - Deleting replays too fast will no longer crash.
- Ledgedash Event Changes:
    - Colours have been updated to be colourblind friendly.
    - The airdodge angle is now consistent with other events.
    - Invincible grab hitboxes show like other hitboxes.
    - Added reset delay option.
    - Camera is now consistent across reset options.
    - HUD now updates and displays every frame.
    - Game speed option.
    - Swap sides on auto reset option.
    - Option to always maintain full ledge invincibility while on the ledge.
    - The GALINT frame is now correct if ledgedashing without refreshing.
    - Will no longer immediately reset on aerial.
    - Option to show current state as an overlay.
    - Sopo is used instead of both climbers.
- New Edgeguard Event:
    - Replaces Armada Shine event.
    - Learn the basics of edgeguarding Fox, Falco, Marth, Sheik, and Falcon!
    - Adjust the options the opponent uses to change the difficulty or practice specific situations.
    - Choose between preset values or manually adjust hit angle, knockback, and damage.
- OSD Changes:
    - Act OoWait OSD will trigger even with intermediate buffered actions such as a frame of walk.
    - Removed Max OSDs and Recommended OSDs options, replacing with a new OSD Position option.
    - Wavedash OSD now shows if it was a short hop or full hop.
    - Removed broken OSDs, rewriting the most important ones.
    - Added new glide toss, aerial out of double jump, special out of jump, and double jump out of jump OSDs.
    - Added new lockout timer OSDs.
    - Added new Fighter-Specific Tech OSDs with:
        - Spacies: Act OoShine and Shorten Frame.
        - Peach: Act OoFloat.
        - Yoshi: Egg Toss angle and strength.
- Bugfixes/Small Changes:
    - **Fixed cpu acting too late out of sakurai angle and other non-knockdown hits (such as fox drill).**
    - Updated to UCF 0.84 (Allows practicing with dashback out of crouch).
    - All trigger-based functionality can now be performed with analog-only triggers.
    - Nametags will now show up in C events.
    - Slow down advanced camera with R.
    - DIDraw will now show for sheik and after throws.
    - System inputs in info display will now work for ports other than port 1.
    - The lab now saves a minor savestate on boot.
    - Act OoHitstun now works after being hit by falco laser.
    - The powershield event has been rewritten and given a new laser height option.
    - Adjustable timing in Amsah tech event.
    - Jump actions no longer make the CPU self-destruct.
    - Various OSDs have been fixed.
    - Lightshield now works in recordings.
    - Added successful counters to the LCancel and Wavedash events.
    - Fix CPU DI on back throws and moves that send backwards.
    - Can now use lightshield L with DPad to adjust percents.
    - CPUs now DI DK cargo throw.
    - Samus homing missiles will target the CPU.
    - Nana will not drop shield when Popo's shield is hit.
    - Added the polling drift fix.
    - Deleting replays too fast will no longer crash.
    - Every character can be used in Amsah Tech training.
    - Removed the maximum distance in Reversal training.
    - Added getup attacks and dash attack to Reversal training.
    - Added option to move to the platform in Reversal training.
    - Infinite shields now applies to nana.
- Work in progress:
    - Reaction Tech Chase Event
    - Improving the savestate format
- Developer Features:
    - Simple and easily reproducible builds on Windows and Linux.
    - Simple to add new events - no need to touch ASM.
    - Fast recompilation on Linux using make.
    - Simplified and performant [tool](https://github.com/AlexanderHarrison/gc_fst) to extract and rebuild ISOs.
