//---------------------------------------------------------------------------

#include <vcl.h>
#include "ThorAndor.h"
#pragma hdrstop

#pragma package(smart_init)

//---------------------------------------------------------------------------
__fastcall DeviceControl::DeviceControl(bool CreateSuspended)
	: TThread(CreateSuspended)
{
}
//---------------------------------------------------------------------------

void __fastcall DeviceControl::SynchroDisplayImage()
{
	ATManage->DisplayImage();//draw images
}
//---------------------------------------------------------------------------

void __fastcall DeviceControl::SynchroProgress()
{
	//show process progress
	if (mlc >= loopNum) //acquisition complete
	{
		if (stst) //stop-button was not clicked
			ATManage->StartProgClick(this);//click stop-button
	}
	else //acquisition in progress
		ATManage->ProgList->Lines->Add(IntToStr(mlc + 1) + " done");
}
//---------------------------------------------------------------------------

void __fastcall DeviceControl::SynchroErrorMsg()
{
	ATManage->ProgList->Lines->Add(errorMsgTxt);//print error message
}
//---------------------------------------------------------------------------

void __fastcall DeviceControl::Execute()
{
	//thead executing operations with camera (Andor) and LED-switcher (Thorlabs DC4100)
	__int32 jj,
			imInd,//images counter
			corTime;//correction time (ms)
	unsigned int uiErr;//driver message code
	__int64 tactps,//CPU clock frequency (tacts per second)
			firstTact,//tact correspondint to acquisition begin
			tactLEDon,//tact of CPU in LED-on period
			curntTact,//current tact of CPU
			nextTact,//next tact of CPU (when LED will be switched off)
			ledOffTact,//tact when LED must be switched-off
			corTact1,//correction tact1
			corTact2,//correction tact2
			ledExpDelay;//LEDon-exposition delay (tacts)
	float realLEDbright;//actual brightness of used LED (volts)


	/* ===== log-file ===== */
//	FILE *fileStrm;//stream for output data to file
	/* ===== log-file ===== */


	while (!Terminated)
	{
		if (imTStamps) //timestamps of acquired images
			delete[] imTStamps;//delete previous timestamps
		imTStamps = new float[loopNum * useLEDsNum];//timestamps of acquired images (seconds from acquisition start)

		while (!QueryPerformanceFrequency((_LARGE_INTEGER*)&tactps)) {};//CPU clock frequency (tacts per second)

		StartAcquisition();//start acquisition on camera

		if (!ATManage->Live->Tag) //LEDs + acquisition
		{
			ledExpDelay = (ledOnExpDelay * tactps) / 1e3;//LEDon-exposition delay (tacts per delay) delay between LED-on and aquisition of camera
			QueryPerformanceCounter((_LARGE_INTEGER*)&firstTact);//tact correspondint to acquisition begin
			imInd = 0;//images counter


			/* ===== log-file ===== */
//			fileStrm = fopen("C:\\Users\\User\\Pictures\\DLlog.txt", "w");//create file
//			fprintf(fileStrm, "tacts per second %d\n", tactps);
//			fprintf(fileStrm, "event description\ttimestamp, tacts\n");
//			errorMsgTxt = "Log-file created";//text of error message
//			Synchronize(&SynchroErrorMsg);//pring error message
			/* ===== log-file ===== */


			//begin acquisition in main loop
			for (mlc = 0; (mlc < loopNum) && stst; mlc++) //repeat acquisition in main loop
			{
				QueryPerformanceCounter((_LARGE_INTEGER*)&corTact1);//current tact of CPU
				for (jj = 0; jj < useLEDsNum; jj++) //run over comand within a loop
				{
//                    //= set individual exposition time =//
//					//ATManage->acqParams.exposTime = exposToLED[ledOrder[jj]];//individual exposition to the LED (seconds)
//					uiErr = SetExposureTime(exposToLED[ledOrder[jj]]);//set exposition time
//					if (uiErr != DRV_SUCCESS)
//					{
//						errorMsgTxt = "!Exposition time error";//text of error message
//						Synchronize(&SynchroErrorMsg);//pring error message
//					}
//					//= end of (set individual exposition time) =//

					/* ===== log-file ===== */
//					QueryPerformanceCounter((_LARGE_INTEGER*)&curntTact);//current tact of CPU
//					fprintf(fileStrm, "LED %d ON-command send, \t%d\n", ledOrder[jj], curntTact - firstTact);//
					/* ===== log-file ===== */

					//= switch LED on =//
					TLDC4100_setConstCurrent(thorInstr, ledOrder[jj], vLEDbright[jj]);//switch the LED on by setting current (amperes)
					//= wait for requested brightness of the LED =//
					realLEDbright = -1;//not vLEDbright[jj]
					while (realLEDbright != vLEDbright[jj]) //wait for requested brightness of the LED
						TLDC4100_getConstCurrent(thorInstr, ledOrder[jj], &realLEDbright);//get actual brightness of used LED (volts)
					QueryPerformanceCounter((_LARGE_INTEGER*)&curntTact);//current tact of CPU
					//= end of (wait for requested brightness of the LED) =//
					//= end  of (switch LED on) =//

					tactLEDon = (ledOnDur[ledOrder[jj]] * tactps) / 1e3;//tact of CPU in LED-on period
					ledOffTact = curntTact + tactLEDon;//next tact of CPU (when LED will be switched off)

					/* ===== log-file ===== */
//					fprintf(fileStrm, "LED %d status ON, \t%d\n", ledOrder[jj], curntTact - firstTact);//
					/* ===== log-file ===== */

					//= wait for LEDon-exposition delay =//
					if (ledExpDelay > 1) //large delay
					{
						nextTact = curntTact + ledExpDelay;//next tact of CPU (when LED will be switched off)
						while (curntTact < nextTact) //wait for LEDon-exposition delay
							QueryPerformanceCounter((_LARGE_INTEGER*)&curntTact);//current tact of CPU
					}
					//= end of (wait for LEDon-exposition delay) =//


					/* ===== log-file ===== */
//					QueryPerformanceCounter((_LARGE_INTEGER*)&curntTact);//current tact of CPU
//					fprintf(fileStrm, "ready to start expos, \t%d\n", curntTact - firstTact);//
					/* ===== log-file ===== */

					//= single image acquizition =//
					uiErr = SendSoftwareTrigger();//send an event to the camera to take an acquisition (in "software" trigger mode)
					while (uiErr != DRV_SUCCESS) //send an event to the camera to take an acquisition (in "software" trigger mode)
					{
						errorMsgTxt = "!Software trigger sending error " + IntToStr(__int32(uiErr));//text of error message
						Synchronize(&SynchroErrorMsg);
						uiErr = SendSoftwareTrigger();//send an event to the camera to take an acquisition (in "software" trigger mode)
					}
					QueryPerformanceCounter((_LARGE_INTEGER*)&curntTact);//current tact of CPU

					/* ===== log-file ===== */
//					fprintf(fileStrm, "expos command sent, \t%d\n", curntTact - firstTact);//
					/* ===== log-file ===== */

					imTStamps[imInd] = float(curntTact - firstTact) / float(tactps);//timestamps of acquired images (seconds from acquisition start)
					imInd++;//next image
					//= end of (single image acquizition) =//

					//try to wait for end of current exposition (or by GetAcquisitionProgress; or by GetStatus and SetDriverEvent)
					WaitForAcquisition();//wait for image acquired

//					//= wait for LEDon time end =//
//					while (curntTact < ledOffTact) //wait for pause (instead of Sleep(ledOnDur))
//						QueryPerformanceCounter((_LARGE_INTEGER*)&curntTact);//current tact of CPU
//					//= end of (wait for LEDon time end) =//


					/* ===== log-file ===== */
//					fprintf(fileStrm, "LED %d OFF-command send, \t%d\n", ledOrder[jj], curntTact - firstTact);//
					/* ===== log-file ===== */

					//= switch LED off =//
					TLDC4100_setConstCurrent(thorInstr, ledOrder[jj], 0);//switch the LED off by setting current to 0A
//					//= wait for requested brightness of the LED =//
//					realLEDbright = -1;//not vLEDbright[jj]
//					while (realLEDbright != 0) //wait for requested brightness of the LED
//						TLDC4100_getConstCurrent(thorInstr, ledOrder[jj], &realLEDbright);//get actual brightness of used LED (volts)
//					//= end of (wait for requested brightness of the LED) =//
					//= end  of (switch LED on) =//

					/* ===== log-file ===== */
//					QueryPerformanceCounter((_LARGE_INTEGER*)&curntTact);//current tact of CPU
//					fprintf(fileStrm, "LED %d status OFF, \t%d\n", ledOrder[jj], curntTact - firstTact);//
					/* ===== log-file ===== */

				} //end of (for (jj = 0; jj < useLEDsNum; jj++) //run over comand within a loop)
				Synchronize(&SynchroDisplayImage);//read out and display images
				Synchronize(&SynchroProgress);//show experiment progress
				QueryPerformanceCounter((_LARGE_INTEGER*)&corTact2);//current tact of CPU

				if ((mlc < (loopNum - 1)) && stst) //pause for all steps excepting the last one
				{
					corTime = __int32(1000 * float(corTact2 - corTact1) / float(tactps));//correction time (ms)
					if ((framPrd - corTime - 100) > 0) //long no-action time
						Sleep(framPrd - corTime - 100);//pause between measurements (frame rate)

					nextTact = corTact1 + (__int64(framPrd / 1000) * tactps);//next tact of CPU (when new cycle start)
					while ((curntTact < nextTact) && stst) //wait for pause (instead of Sleep(ledOnDur))
						QueryPerformanceCounter((_LARGE_INTEGER*)&curntTact);//current tact of CPU
				}
			} //end of (for (mlc = 0; (mlc < loopNum) && stst; mlc++) //repeat acquisition in main loop)

			//switch off all LEDs
			for (jj = 0; jj < NUM_CHANNELS; jj++) //run over all LEDs
			{
				TLDC4100_setConstCurrent(thorInstr, jj, 0);//set current (amperes)
				TLDC4100_setLedOnOff(thorInstr, jj, 0);//switch LED off
			}
			Synchronize(&SynchroProgress);//show experiment progress

			/* ===== log-file ===== */
//			fclose(fileStrm);//close file
			/* ===== log-file ===== */
		}
		else //live video
		{
			while (ATManage->Live->Tag) //acquisition not stopped
			{
				WaitForAcquisition();//wait for next acquisition
				Synchronize(&SynchroDisplayImage);//read out and display images
			}
		}
		AbortAcquisition();//abort the current acquisition on camera
		Suspend();//suspend device controlling thread until new command
    }
}
//---------------------------------------------------------------------------
