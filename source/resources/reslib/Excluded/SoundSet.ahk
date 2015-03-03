SoundSet(aSetting, aComponentType, aControlType, aComponentInstance:=1, aMixerID:=0){
; If the caller specifies NULL for aSetting, the mode will be "Get".  Otherwise, it will be "Set".
	static MIXERCONTROL_CONTROLTYPE_INVALID:=0xFFFFFFFF,MMSYSERR_NOERROR:=0,MIXER_GETLINEINFOF_COMPONENTTYPE:=3
				,MIXER_GETLINEINFOF_DESTINATION:=0,MIXER_GETLINEINFOF_SOURCE:=1,MIXER_GETLINECONTROLSF_ONEBYTYPE:=2
				,MIXER_GETCONTROLDETAILSF_VALUE:=0,mxcaps:=Struct(_MIXERCAPS),ml:=Struct(_MIXERLINE)
				,mc := Struct(_MIXERCONTROL) ; MSDN: "No initialization of the buffer pointed to by [pamxctrl below] is required"
				,mlc := Struct(_MIXERLINECONTROLS),mcd := Struct(_MIXERCONTROLDETAILS),mcdMeter:=Struct("DWORD dwValue")
				,MIXERCONTROL_CT_CLASS_SWITCH:=0x20000000,MIXERCONTROL_CT_SC_SWITCH_BOOLEAN:=0,MIXERCONTROL_CT_UNITS_BOOLEAN:=0x00010000
				,MIXERCONTROL_CONTROLTYPE_BOOLEAN:=MIXERCONTROL_CT_CLASS_SWITCH | MIXERCONTROL_CT_SC_SWITCH_BOOLEAN | MIXERCONTROL_CT_UNITS_BOOLEAN
				,MIXERCONTROL_CONTROLTYPE_ONOFF:=MIXERCONTROL_CONTROLTYPE_BOOLEAN + 1
				,MIXERCONTROL_CONTROLTYPE_MUTE:=MIXERCONTROL_CONTROLTYPE_BOOLEAN + 2
				,MIXERCONTROL_CONTROLTYPE_MONO:=MIXERCONTROL_CONTROLTYPE_BOOLEAN + 3
				,MIXERCONTROL_CONTROLTYPE_LOUDNESS:=MIXERCONTROL_CONTROLTYPE_BOOLEAN + 4
				,MIXERCONTROL_CONTROLTYPE_STEREOENH:=MIXERCONTROL_CONTROLTYPE_BOOLEAN + 5
				,MIXERCONTROL_CONTROLTYPE_BASSBOOST:=MIXERCONTROL_CONTROLTYPE_BOOLEAN + 0x00002277
	static MIXERLINE_COMPONENTTYPE_DST_FIRST:=0x00000000
				,MIXERLINE_COMPONENTTYPE_DST_MASTER:=MIXERLINE_COMPONENTTYPE_DST_FIRST + 4
				,MIXERLINE_COMPONENTTYPE_DST_SPEAKERS:=MIXERLINE_COMPONENTTYPE_DST_FIRST + 4
				,MIXERLINE_COMPONENTTYPE_DST_HEADPHONES:=MIXERLINE_COMPONENTTYPE_DST_FIRST + 5
				,MIXERLINE_COMPONENTTYPE_SRC_FIRST:=0x00001000
				,MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED:=MIXERLINE_COMPONENTTYPE_SRC_FIRST + 0
				,MIXERLINE_COMPONENTTYPE_SRC_DIGITAL:=MIXERLINE_COMPONENTTYPE_SRC_FIRST + 1
				,MIXERLINE_COMPONENTTYPE_SRC_LINE:=MIXERLINE_COMPONENTTYPE_SRC_FIRST + 2
				,MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE:=MIXERLINE_COMPONENTTYPE_SRC_FIRST + 3
				,MIXERLINE_COMPONENTTYPE_SRC_SYNTH:=MIXERLINE_COMPONENTTYPE_SRC_FIRST + 4
				,MIXERLINE_COMPONENTTYPE_SRC_CD:=MIXERLINE_COMPONENTTYPE_SRC_FIRST + 5
				,MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE:=MIXERLINE_COMPONENTTYPE_SRC_FIRST + 6
				,MIXERLINE_COMPONENTTYPE_SRC_PCSPEAKER:=MIXERLINE_COMPONENTTYPE_SRC_FIRST + 7
				,MIXERLINE_COMPONENTTYPE_SRC_WAVE:=MIXERLINE_COMPONENTTYPE_SRC_FIRST + 8
				,MIXERLINE_COMPONENTTYPE_SRC_AUX:=MIXERLINE_COMPONENTTYPE_SRC_FIRST + 9
				,MIXERLINE_COMPONENTTYPE_SRC_ANALOG:=MIXERLINE_COMPONENTTYPE_SRC_FIRST + 10
	static MIXERCONTROL_CT_CLASS_FADER:=0x50000000,MIXERCONTROL_CT_UNITS_UNSIGNED:=0x00030000
				,MIXERCONTROL_CT_CLASS_SLIDER:=0x40000000,MIXERCONTROL_CT_UNITS_SIGNED:=0x00020000
				,MIXERCONTROL_CONTROLTYPE_FADER:=MIXERCONTROL_CT_CLASS_FADER | MIXERCONTROL_CT_UNITS_UNSIGNED
				,MIXERCONTROL_CONTROLTYPE_VOLUME:=MIXERCONTROL_CONTROLTYPE_FADER + 1
				,MIXERCONTROL_CONTROLTYPE_VOL:=MIXERCONTROL_CONTROLTYPE_FADER + 1
				,MIXERCONTROL_CONTROLTYPE_SLIDER:=MIXERCONTROL_CT_CLASS_SLIDER | MIXERCONTROL_CT_UNITS_SIGNED
				,MIXERCONTROL_CONTROLTYPE_PAN:=MIXERCONTROL_CONTROLTYPE_SLIDER + 1
				,MIXERCONTROL_CONTROLTYPE_QSOUNDPAN:=MIXERCONTROL_CONTROLTYPE_SLIDER + 2
				,MIXERCONTROL_CONTROLTYPE_BASS:=MIXERCONTROL_CONTROLTYPE_FADER + 2
				,MIXERCONTROL_CONTROLTYPE_TREBLE:=MIXERCONTROL_CONTROLTYPE_FADER + 3
				,MIXERCONTROL_CONTROLTYPE_EQUALIZER:=MIXERCONTROL_CONTROLTYPE_FADER + 4

	;~ #define SOUND_MODE_IS_SET aSetting ; Boolean: i.e. if it's not NULL, the mode is "SET".
	;~ double setting_percent
	;~ Var *output_var
  ;~ output_var = NULL ; To help catch bugs.
  ;~ setting_percent = ATOF(aSetting)
  if (aSetting < -100)
    setting_percent := -100
  else if (aSetting > 100)
    setting_percent := 100
  else setting_percent:=aSetting
	
	If (aComponentType="N/A")
		aComponentType:=MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED
	else if InStr(".Master.Speakers.Headphones.","." aComponentType ".")
		aComponentType:=MIXERLINE_COMPONENTTYPE_DST_%aComponentType%
	else if InStr(".Digital.Line.Microphone.Synth.CD.Telephone.PCSpeaker.Wave.Aux.Analog.","." aComponentType ".")
		aComponentType:=MIXERLINE_COMPONENTTYPE_SRC_%aComponentType%
	else aComponentType:=0
	
	If (Type(aControlType)="Integer")
		aControlType:=aControlType
	else if InStr(".Vol.Volume.OnOff.Mute.Mono.Loudness.StereoEnh.BassBoost.Pan.QSoundpan.Bass.treble.Equalizer.","." aControlType ".")
		aControlType:= MIXERCONTROL_CONTROLTYPE_%aControlType%
	else aControlType:=MIXERCONTROL_CONTROLTYPE_INVALID
	;~ ListVars
	;~ MsgBox
		
	;~ else ; The mode is GET.
	;~ {
		;~ output_var = OUTPUT_VAR
		;~ output_var->Assign() ; Init to empty string regardless of whether we succeed here.
	;~ }

	; Rare, since load-time validation would have caught problems unless the params were variable references.
	; Text values for ErrorLevels should be kept below 64 characters in length so that the variable doesn't
	; have to be expanded with a different memory allocation method:
	if (aControlType == MIXERCONTROL_CONTROLTYPE_INVALID || aComponentType == MIXERLINE_COMPONENTTYPE_DST_UNDEFINED)
		return "Invalid Control Type or Component Type"
	
	; Open the specified mixer ID:
	;~ HMIXER hMixer
	if DllCall("Winmm\mixerOpen","PTR*",hMixer,"Int", aMixerID,"PTR", 0,"PTR", 0,"Int", 0)
		return "Can't Open Specified Mixer"

	; Find out how many destinations are available on this mixer (should always be at least one):
	;~ int dest_count
	;~ MIXERCAPS mxcaps
	mxcaps.Fill()
	if !DllCall("Winmm\mixerGetDevCaps","PTR",hMixer,"PTR", mxcaps[],"Int", sizeof(mxcaps))
		dest_count := mxcaps.cDestinations
	else
		dest_count := 1  ; Assume it has one so that we can try to proceed anyway.

	; Find specified line (aComponentType + aComponentInstance):
	ml.Fill()
	ml.cbStruct := sizeof(ml)
	if (aComponentInstance = 1)  ; Just get the first line of this type, the easy way.
	{
		ml.dwComponentType := aComponentType
		if DllCall("Winmm\mixerGetLineInfo","PTR",hMixer,"PTR", ml[],"Int", MIXER_GETLINEINFOF_COMPONENTTYPE)
		{
			DllCall("Winmm\mixerClose","PTR",hMixer)
			return "Mixer Doesn't Support This Component Type"
		}
	}
	else
	{
		; Search through each source of each destination, looking for the indicated instance
		; number for the indicated component type:
		;~ int source_count
		;~ bool found = false
		found_instance:=0
		d:=0
		Loop % dest_count {
		;~ for (int d = 0, found_instance = 0 d < dest_count && !found ++d) ; For each destination of this mixer.
		;~ {
			ml.dwDestination := d := A_Index-1
			if DllCall("Winmm\mixerGetLineInfo","PTR",hMixer,"PTR", ml[], MIXER_GETLINEINFOF_DESTINATION)
				; Keep trying in case the others can be retrieved.
				continue
			source_count := ml.cConnections  ; Make a copy of this value so that the struct can be reused.
			Loop % ml.cConnections {
			;~ for (int s = 0 s < source_count && !found ++s) ; For each source of this destination.
			;~ {
				ml.dwDestination := d ; Set it again in case it was changed.
				ml.dwSource := A_Index-1
				if DllCall("Winmm\mixerGetLineInfo","PTR",hMixer,"PTR", ml[], MIXER_GETLINEINFOF_SOURCE)
					; Keep trying in case the others can be retrieved.
					continue
				; This line can be used to show a soundcard's component types (match them against mmsystem.h):
				;MsgBox(ml.dwComponentType)
				if (ml.dwComponentType = aComponentType)
				{
					++found_instance
					if (found_instance = aComponentInstance){
						found := true
						break 2
					}
				}
			} ; inner for()
		} ; outer for()
		if (!found)
		{
			DllCall("Winmm\mixerClose","PTR",hMixer)
			return "Mixer Doesn't Have That Many of That Component Type"
		}
	}

	; Find the mixer control (aControlType) for the above component:
  mc.Fill()
	mlc.Fill()
	mlc.cbStruct := sizeof(mlc)
	mlc.pamxctrl := mc[]
	mlc.cbmxctrl := sizeof(mc)
	mlc.dwLineID := ml.dwLineID
	mlc.dwControlType := aControlType
	mlc.cControls := 1
	if DllCall("Winmm\mixerGetLineControls","PTR",hMixer,"PTR", mlc[],"Int", MIXER_GETLINECONTROLSF_ONEBYTYPE)
	{
		DllCall("Winmm\mixerClose","PTR",hMixer)
		return "Component Doesn't Support This Control Type"
	}
	; Does user want to adjust the current setting by a certain amount?
	; For v1.0.25, the first char of RAW_ARG is also checked in case this is an expression intended
	; to be a positive offset, such as +(var + 10)
	adjust_current_setting := SubStr(aSetting,1,1) = "-" || SubStr(aSetting,1,1) = "+"
	
	; These are used in more than once place, so always initialize them here:
	mcd.Fill()
	mcdMeter.Fill()
	mcd.cbStruct := sizeof(mcd)
	mcd.dwControlID := mc.dwControlID
	mcd.cChannels := 1 ; MSDN: "when an application needs to get and set all channels as if they were uniform"
	mcd.paDetails := mcdMeter[]
	mcd.cbDetails := 4
	
	; Get the current setting of the control, if necessary:
	if (adjust_current_setting)
	{
		if DllCall("Winmm\mixerGetControlDetails","PTR",hMixer,"PTR", mcd[],"Int", MIXER_GETCONTROLDETAILSF_VALUE)
		{
			DllCall("Winmm\mixerClose","PTR",hMixer)
			return "Can't Get Current Setting"
		}
	}

	If ( aControlType = MIXERCONTROL_CONTROLTYPE_BASSBOOST
		|| aControlType > MIXERCONTROL_CONTROLTYPE_BOOLEAN && aControlType <= MIXERCONTROL_CONTROLTYPE_STEREOENH)
		control_type_is_boolean := true
	else ; For all others, assume the control can have more than just ON/OFF as its allowed states.
		control_type_is_boolean := false
	
  if (control_type_is_boolean){
    if (adjust_current_setting) ; The user wants this toggleable control to be toggled to its opposite state:
      mcdMeter.dwValue := (mcdMeter.dwValue > mc.dwMinimum) ? mc.dwMinimum : mc.dwMaximum
    else ; Set the value according to whether the user gave us a setting that is greater than zero:
      mcdMeter.dwValue := (setting_percent > 0) ? mc.dwMaximum : mc.dwMinimum
  }
  else ; For all others, assume the control can have more than just ON/OFF as its allowed states.
  {
    ; Make this an __int64 vs. DWORD to avoid underflow (so that a setting_percent of -100
    ; is supported whenever the difference between Min and Max is large, such as MAXDWORD):
    specified_vol := (mc.dwMaximum - mc.dwMinimum) * (setting_percent / 100)
    if (adjust_current_setting)
    {
      ; Make it a big int so that overflow/underflow can be detected:
      vol_new := mcdMeter.dwValue + specified_vol
      if (vol_new < mc.dwMinimum)
        vol_new := mc.dwMinimum
      else if (vol_new > mc.dwMaximum)
        vol_new := mc.dwMaximum
      mcdMeter.dwValue := vol_new
    }
    else
      mcdMeter.dwValue := specified_vol ; Due to the above, it's known to be positive in this case.
  }
  result := DllCall("Winmm\mixerSetControlDetails","PTR",hMixer,"PTR", mcd[],"Int", MIXER_GETCONTROLDETAILSF_VALUE)
  DllCall("Winmm\mixerClose","PTR",hMixer)
  return result = MMSYSERR_NOERROR ? 0 : "Can't Change Setting"
}
