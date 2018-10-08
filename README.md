# mcHFManager
mcHF Windows Update Utility

This is the latest version of the mcHFManager utility that allows
to reflash the firmware of the mcHF. The current version supports
not only the DSP chip (CPU on all revisions before 0.8), but also
the UI board CPU in rev 0.8. All older revisions are supported as
well.

To compile you will need to have Borland Architect 2006, no skinning
is used, so just open the mcHFManager.bdsproj file and Shift-F9 to
compile.

Adding new User Controls is extremely easy as all controls are prepacked
as borland libs, simply you need to drag and drop them to the form,
and the Methods and Properties are self explanatory. The application
uses separate thread to talk to the mcHF via USB, so no blocking of
the UI occurs. Also it is rather easy to add new functionality to the
bootloaders/mcHFManager, although some knowledge of the USB specs is
beneficial as low level control/bulk pipes are used for the transfers
and handling in the bootloader (DSP) is done rather manually.

For any questions, please drop me a mail: djchrismarc at gmail.

73, Chris
M0NKA