//---------------------------------------------------------------------------

#ifndef LedCamControlH
#define LedCamControlH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
//---------------------------------------------------------------------------
class DeviceControl : public TThread
{
private:
protected:
	void __fastcall Execute();
	void __fastcall SynchroDisplayImage();
	void __fastcall SynchroProgress();
	void __fastcall SynchroErrorMsg();
public:
	__fastcall DeviceControl(bool CreateSuspended);
	bool stst;//start-stop flag (true - acquisition on progress, false - acquisition is stopped)
	__int32 mlc,//main loop counter
			useLEDsNum,//number of needed LEDs
			ledOrder[NUM_CHANNELS],//order of used LEDs
			loopNum,//number loops (times)
			framPrd,//period between 3-color frames (seconds)
			ledOnExpDelay;//LEDon-exposition delay (milliseconds)
	__int64 ledOnDur[NUM_CHANNELS];//duration of LED-on period (ms)
	float vLEDbright[NUM_CHANNELS],//brightness of used LEDs in volts
		  exposToLED[NUM_CHANNELS],//expositions to each LED (second)
		  *imTStamps,//timestamps of acquired images (seconds from acquisition start)
		  *ringExposTime;//array of exposure times
	ViSession thorInstr;//thorlabs instrument handle
	UnicodeString errorMsgTxt;//text of error message
};
//---------------------------------------------------------------------------
#endif
