//---------------------------------------------------------------------------

#include <vcl.h>
#include <cstdio.h>
#pragma hdrstop

#include "stdafx.h"
#include "ThorAndor.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TATManage *ATManage;
HINSTANCE visaModul,//visa modul (thorlabs dc4100)
		  atmcd32d;//andor modul
ViSession thorInstr = VI_NULL;//thorlabs instrument handle

//types for visa functions (thorlabs)
typedef ViStatus _VI_FUNC __stdcall (*visaOpenDefaultRM)(ViPSession vi);
typedef ViStatus _VI_FUNC __stdcall (*visaFindRsrc)(ViSession sesn, ViString expr, ViPFindList vi, ViPUInt32 retCnt, ViChar _VI_FAR desc[]);
typedef ViStatus _VI_FUNC __stdcall (*visaFindNext)(ViFindList vi, ViChar _VI_FAR desc[]);
typedef ViStatus _VI_FUNC __stdcall (*visaParseRsrc)(ViSession rmSesn, ViRsrc rsrcName, ViPUInt16 intfType, ViPUInt16 intfNum);
typedef ViStatus _VI_FUNC __stdcall (*visaParseRsrcEx)(ViSession rmSesn, ViRsrc rsrcName, ViPUInt16 intfType, ViPUInt16 intfNum, ViChar _VI_FAR rsrcClass[], ViChar _VI_FAR expandedUnaliasedName[], ViChar _VI_FAR aliasIfExists[]);
typedef ViStatus _VI_FUNC __stdcall (*visaOpen)(ViSession sesn, ViRsrc name, ViAccessMode mode, ViUInt32 timeout, ViPSession vi);
typedef ViStatus _VI_FUNC __stdcall (*visaGetAttribute)(ViObject vi, ViAttr attrName, void _VI_PTR attrValue);
typedef ViStatus _VI_FUNC __stdcall (*visaClose)(ViObject vi);

//visa functions (thorlabs)
visaOpenDefaultRM vi32OpenDefaultRM = NULL;
visaFindRsrc vi32FindRsrc = NULL;
visaFindNext vi32FindNext = NULL;
visaParseRsrc vi32ParseRsrc = NULL;
visaParseRsrcEx vi32ParseRsrcEx = NULL;
visaOpen vi32Open = NULL;
visaGetAttribute vi32GetAttribute = NULL;
visaClose vi32Close = NULL;
ViChar rscStr[VI_FIND_BUFLEN];//resource string
TIFF *tiffImg;//pointer to tiff-image object

//prototypes of functions using visa
static ViStatus readInstrData(ViSession resMgr, ViRsrc rscStr, ViChar *name, ViChar *descr, ViAccessMode *lockState);
static ViStatus CleanupScan(ViSession resMgr, ViFindList findList, ViStatus err);
void error_exit(ViStatus err, UnicodeString funcName);

//---------------------------------------------------------------------------
__fastcall TATManage::TATManage(TComponent* Owner)
	: TForm(Owner)
{
	UnicodeString visaDname;//visa driver adress
				  //atmDname;//andor driver adress
	unsigned int uiErr;
	__int32 ii, jj;
	__int32 adcChanNum;

	ViStatus 		err = VI_SUCCESS; 		   // error variable
	ViSession   	resMgr = VI_NULL;		   // resource manager
	ViFindList		findList = VI_NULL; 	   // find list
	ViChar*			rscPtr = VI_NULL;		   // pointer to resource string
	ViUInt32    	cnt = 0;				   // counts found devices
	ViChar			alias[DC4100_BUFFER_SIZE]; // string to copy the alias of a device to
	ViChar			name[DC4100_BUFFER_SIZE];  // string to copy the name of a device to
	ViAccessMode	lockState; 				   // lock state of a device

	visaDname = "c:\\Windows\\System32\\visa32.dll";//driver adress
	visaModul = LoadLibrary(visaDname.w_str());//driver enter point

	//atmDname = "c:\\Windows\\System32\\atmcd32d.dll";//driver adress
	//atmcd32d = LoadLibrary(atmDname.w_str());//driver enter point
	cam_handle = NULL;//camera handle

	if (visaModul) //driver was loaded
	{
		vi32OpenDefaultRM = (visaOpenDefaultRM)GetProcAddress(visaModul, "viOpenDefaultRM");
		vi32FindRsrc = (visaFindRsrc)GetProcAddress(visaModul, "viFindRsrc");
		vi32FindNext = (visaFindNext)GetProcAddress(visaModul, "viFindNext");
		vi32ParseRsrc = (visaParseRsrc)GetProcAddress(visaModul, "viParseRsrc");
		vi32ParseRsrcEx = (visaParseRsrcEx)GetProcAddress(visaModul, "viParseRsrcEx");
		vi32Open = (visaOpen)GetProcAddress(visaModul, "viOpen");
		vi32GetAttribute = (visaGetAttribute)GetProcAddress(visaModul, "viGetAttribute");
		vi32Close = (visaClose)GetProcAddress(visaModul, "viClose");

		//=== find and connect to Thorlabs DC4100 ===//
		if (vi32OpenDefaultRM)
			err = vi32OpenDefaultRM(&resMgr);

		err = vi32FindRsrc(resMgr, DC4100_FIND_PATTERN, &findList, &cnt, rscStr);
		while (cnt) //run over found devices
		{
			//read information
			if (readInstrData(resMgr, rscStr, name, alias, &lockState) == VI_SUCCESS)
			{
				//check for the devices name
				if (strstr(alias, "DC4100"))// || strstr(alias, "DC4104"))
				{
					// If found we store the device description pointer
					rscPtr = rscStr;
					ProgList->Lines->Add(alias);//show name of found instrument
					CleanupScan(resMgr, findList, VI_SUCCESS);
					break;
				}
			}
			cnt--;//count down
			if (cnt)
			{
				//find next device
				if ((err = vi32FindNext(findList, rscStr)))
					CleanupScan(resMgr, findList, err);
			}
		}

		if (rscPtr) //LED-controller detected
		{
			StartProg->Enabled = true;
			err = TLDC4100_init(rscStr, VI_OFF, VI_OFF, &thorInstr);//try to open DC4100
			if (err)
				error_exit(err, "TLDC4100_init");

			for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED channels
				TLDC4100_setLedOnOff(thorInstr, ii, 0);//turn off the LED

			err = TLDC4100_setOperationMode(thorInstr, MODUS_CONST_CURRENT);//set the modus to "constant current"
			if (err)
				error_exit(err, "TLDC4100_setOperationMode");

			err = TLDC4100_setSelectionMode(thorInstr, MULTI_SELECT);//SINGLE_SELECT);//set selection mode
			if(err)
				error_exit(err, "TLDC4100_setSelectionMode");
		}
		else  //LED-controller error
			ProgList->Lines->Add("LED-controller error");//no LED-controller
		//=== end of find and connect to Thorlabs DC4100 ===//
	}
	else //LED-controller error
		ProgList->Lines->Add("no LED-controller");//no LED-controller

	bHorzRes = 512;//default binned horizontal resolution
	bVertRes = 512;//default binned vertical resolution
	//=== find and connect to Andor camera ===//
	//uiErr = atm32GetAvailableCameras(&i_numCams);//number of installed cameras
	uiErr = GetCameraHandle(0, &cam_handle);//get the handle for the first installed camera
	if (uiErr == DRV_SUCCESS) //camara was found
	{
		//uiErr = SetCurrentCamera(cam_handle);//not need for single camera
		uiErr = Initialize("c:\\Program Files (x86)\\Andor Bioimaging\\Common Files\\Detector.ini");//initialize camera
		if (uiErr == DRV_SUCCESS)
			ProgList->Lines->Add("Andor was initialized");
		else
		{
			ProgList->Lines->Add("Andor initialization problem");
			ProgList->Lines->Add("Initialize: " + IntToStr((int)uiErr));
		}

		uiErr = GetNumberADChannels(&adcChanNum);//get number of ADC channels
		if (uiErr != DRV_SUCCESS)
			ProgList->Lines->Add("GetNumberADChannels: " + IntToStr((int)uiErr));
		else
			ProgList->Lines->Add("NumberADChannels: " + IntToStr(adcChanNum));

		uiErr = GetDetector(&hRes, &vRes);//get detector size
		if (uiErr != DRV_SUCCESS)
			ProgList->Lines->Add("GetDetector: " + IntToStr((int)uiErr));
		else
		{
			ProgList->Lines->Add("Detector: " + IntToStr(hRes) + "x" + IntToStr(vRes));
			bHorzRes = hRes;//default binned horizontal resolution
			bVertRes = vRes;//default binned vertical resolution
		}

		uiErr = GetBitDepth(0, &adcDepth);//find the maximum possible counts per pixel, i.e. the saturation level
		if (uiErr != DRV_SUCCESS)
			ProgList->Lines->Add("\r\nGetBitDepth: " + IntToStr((int)uiErr));
		else
			ProgList->Lines->Add("\r\nBitDepth: " + IntToStr(adcDepth));

		saturatLevel = 1;//saturation level
		for(ii = 0; ii < adcDepth; ii++) //run over digits
			saturatLevel *= 2;//calculate saturation level

		//use Andor capibilities to test if camera has a shutter
		AndorCapabilities* caps = new AndorCapabilities();
		caps->ulSize = sizeof(AndorCapabilities);

		uiErr = GetCapabilities(caps);//additional capabilities
		if (uiErr != DRV_SUCCESS)
			ProgList->Lines->Add("\r\nGetCapabilities: " + IntToStr((int)uiErr));
		else
			ProgList->Lines->Add("\r\nCapabilities: " + IntToStr((int)caps));
		if (caps->ulFeatures & AC_FEATURES_SHUTTER)//if a shutter is present
			uiErr = SetShutter(1, 1, 10, 10);//if a shutter is present set is to always open
		//if (caps->ulTriggerModes & AC_TRIGGERMODE_CONTINUOUS)
		//	ProgList->Lines->Add("software trigger is available");
		rawImgs = NULL;//no image data
		rawImgsSat = NULL;//array for image related to saturation level
	}
	else //camera error
	{
		ProgList->Lines->Add("camera not found");
		ProgList->Lines->Add("GetCameraHandle: " + IntToStr((int)uiErr));
	}
	//=== end of find and connect to Andor camera ===//

	//LEDs and camera control thread
	ledCamThread = new DeviceControl(true);//creat LEDs and camera control thread
	ledCamThread->thorInstr = thorInstr;//copy of handle of thorlabs instrument (DC100 LED switcher)
	ledCamThread->stst = false;//start-stop flag (true - acquisition on progress, false - acquisition is stopped)
	ledCamThread->imTStamps = NULL;//timestamps of acquired images (seconds from experiment start)

	imGlbNum = 1;//global number of saved image for current session

	//make copy of controls after form created
	ledCheck[0] = UseLED1;//pointer to use-LED checkbox
	ledCheck[1] = UseLED2;//pointer to use-LED checkbox
	ledCheck[2] = UseLED3;//pointer to use-LED checkbox
	ledCheck[3] = UseLED4;//pointer to use-LED checkbox
	ledSN[0] = LED1sn;//pointer to LED-order edit
	ledSN[1] = LED2sn;//pointer to LED-order edit
	ledSN[2] = LED3sn;//pointer to LED-order edit
	ledSN[3] = LED4sn;//pointer to LED-order edit
	vLEDbright[0] = LED1bright;//pointer to LED-voltage edit
	vLEDbright[1] = LED2bright;//pointer to LED-voltage edit
	vLEDbright[2] = LED3bright;//pointer to LED-voltage edit
	vLEDbright[3] = LED4bright;//pointer to LED-voltage edit
	durLEDon[0] = LED1OnDur;//pointer to LED-On duration edit
	durLEDon[1] = LED2OnDur;//pointer to LED-On duration edit
	durLEDon[2] = LED3OnDur;//pointer to LED-On duration edit
	durLEDon[3] = LED4OnDur;//pointer to LED-On duration edit
	camExposToLED[0] = ExposToLED1;//pointer to camera exposition time to LED edit
	camExposToLED[1] = ExposToLED2;//pointer to camera exposition time to LED edit
	camExposToLED[2] = ExposToLED3;//pointer to camera exposition time to LED edit
	camExposToLED[3] = ExposToLED4;//pointer to camera exposition time to LED edit
	plotLEDflg[0] = PlotLED1;//pointer to plot-LED checkbox
	plotLEDflg[1] = PlotLED2;//pointer to plot-LED checkbox
	plotLEDflg[2] = PlotLED3;//pointer to plot-LED checkbox
	plotLEDflg[3] = PlotLED4;//pointer to plot-LED checkbox

	linClrs[0] = (TColor)RGB(255, 0, 0);//red (LED1)
	linClrs[1] = (TColor)RGB(0, 0, 0);//black (LED2)
	linClrs[2] = (TColor)RGB(0, 179, 51);//dark green (LED3)
	linClrs[3] = (TColor)RGB(0, 0, 255);//blue (LED4)
	//reserve colors
	linClrs[4] = (TColor)RGB(255, 0, 255);//magenta
	linClrs[5] = (TColor)RGB(204, 204, 0);//sandy-yellow
	linClrs[6] = (TColor)RGB(0, 255, 255);//cyan
	linClrs[7] = (TColor)RGB(0, 255, 0);//green;
	linClrs[8] = (TColor)RGB(128, 128, 128);//grey
	linClrs[9] = (TColor)RGB(0, 128, 255);//sky blue

	TSeriesPointerStyle pntrStl[10];
	pntrStl[0] = psCircle;//circle
	pntrStl[1] = psTriangle;//triangle
	pntrStl[2] = psDiamond;//diamond
	pntrStl[3] = psRectangle;//square
	pntrStl[4] = psDownTriangle;//down triangle
	pntrStl[5] = psRightTriangle;//right triangle
	pntrStl[6] = psHexagon;//hexagon

	for (ii = 0; ii < NUM_CHANNELS; ii++) //rum over image-objects
	{
		for (jj = 0; jj < roiMaxN; jj++) //run over ROIs
		{
			brtnessImROItime[ii][jj] = new TLineSeries(this);//create all series
			brtnessImROItime[ii][jj]->Visible = false;//invisible points initially
			brtnessImROItime[ii][jj]->LinePen->Width = 2;//line width
			brtnessImROItime[ii][jj]->Pointer->Visible = true;//show labels
			brtnessImROItime[ii][jj]->Pointer->Style = pntrStl[jj];//label style
			brtnessImROItime[ii][jj]->Title = "LED" + IntToStr(ii + 1) + " ROI" + IntToStr(jj + 1);//name of line
			brtnessImROItime[ii][jj]->LinePen->Color = linClrs[ii];//line color
			brtnessImROItime[ii][jj]->Color = linClrs[ii];//line color

			brtnessImROIframe[ii][jj] = new TLineSeries(this);//create all series
			brtnessImROIframe[ii][jj]->Visible = false;//invisible points initially
			brtnessImROIframe[ii][jj]->LinePen->Width = 2;//line width
			brtnessImROIframe[ii][jj]->Pointer->Visible = true;//show labels
			brtnessImROIframe[ii][jj]->Pointer->Style = pntrStl[jj];//label style
			brtnessImROIframe[ii][jj]->Title = "LED" + IntToStr(ii + 1) + " ROI" + IntToStr(jj + 1);//name of line
			brtnessImROIframe[ii][jj]->LinePen->Color = linClrs[ii];//line color
			brtnessImROIframe[ii][jj]->Color = linClrs[ii];//line color
		}
	}
	for (ii = 0; ii < 2; ii++) //rum over image-objects
	{
		for (jj = 0; jj < roiMaxN; jj++) //run over ROIs
		{
			ratioImROItime[ii][jj] = new TLineSeries(this);//create all series
			ratioImROItime[ii][jj]->Visible = false;//invisible points initially
			ratioImROItime[ii][jj]->LinePen->Width = 2;//line width
			ratioImROItime[ii][jj]->Pointer->Visible = true;//show labels
			ratioImROItime[ii][jj]->Pointer->Style = pntrStl[jj];//label style
			ratioImROItime[ii][jj]->Title = "RATIO" + IntToStr(ii + 1) + " ROI" + IntToStr(jj + 1);//name of line
			ratioImROItime[ii][jj]->LinePen->Color = linClrs[ii];//line color
			ratioImROItime[ii][jj]->Color = linClrs[4 + ii];//line color

			ratioImROIframe[ii][jj] = new TLineSeries(this);//create all series
			ratioImROIframe[ii][jj]->Visible = false;//invisible points initially
			ratioImROIframe[ii][jj]->LinePen->Width = 2;//line width
			ratioImROIframe[ii][jj]->Pointer->Visible = true;//show labels
			ratioImROIframe[ii][jj]->Pointer->Style = pntrStl[jj];//label style
			ratioImROIframe[ii][jj]->Title = "RATIO" + IntToStr(ii + 1) + " ROI" + IntToStr(jj + 1);//name of line
			ratioImROIframe[ii][jj]->LinePen->Color = linClrs[ii];//line color
			ratioImROIframe[ii][jj]->Color = linClrs[4 + ii];//line color
		}
	}
	readLEDn[0] = NULL;//array for original numbers of the LED
	readLEDn[1] = NULL;//array for numbers of corresponding image
	readLEDn[2] = NULL;//array for numbers of filename
}
//---------------------------------------------------------------------------

void __fastcall TATManage::LoadSettingsClick(TObject *Sender)
{
	//load DriveLEDs settings from lcf-file
	__int32 ii,
			progPrms[27];//parameters array
	unsigned long int bytesRead;
	bool readCor;//read state
	UnicodeString bfrStr;//buffer string
	HANDLE hFile;//handle of file with parameter

	if (LoadSettDlg->Execute())
	{
		hFile = CreateFile(LoadSettDlg->FileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);//хэндл файла, в который сохран€ютс€ данные

		for (ii = 0; ii < 27; ii++) //run over parameters to be loaded
		{
			progPrms[ii] = 0;
			readCor = ReadFile(hFile, &progPrms[ii], sizeof(__int32), &bytesRead, 0);
			if (!readCor) //error of read
				break;
		}
		CloseHandle(hFile);//close the file
		if (readCor) //correct read of parameters
		{
			BinSize->Text = IntToStr(progPrms[0]);//binning
			Reps->Text = IntToStr(progPrms[1]);//loops number
			FramePrd->Text = IntToStr(progPrms[2]);//one lap duration (ms)
			for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED channels
				ledCheck[ii]->Checked = bool(progPrms[3 + ii]);//if LED is in use
			for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED channels
			{
				if (progPrms[7 + ii] < 0) //is empty string
					ledSN[ii]->Text = "";//no serial number
				else
					ledSN[ii]->Text = IntToStr(progPrms[7 + ii]);//serial number
			}
			for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED channels
			{
				if (progPrms[11 + ii] < 0) //is empty string
					vLEDbright[ii]->Text = "";//no LED brightness (voltage)
				else
					vLEDbright[ii]->Text = IntToStr(progPrms[11 + ii]);//LED brightness (voltage)
			}
			for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED channels
			{
				if (progPrms[15 + ii] < 0) //is empty string
					durLEDon[ii]->Text = "";//no LED-on duration
				else
					durLEDon[ii]->Text = IntToStr(progPrms[15 + ii]);//LED-on duration
			}
			for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED channels
			{
				if (progPrms[19 + ii] < 0) //is empty string
					camExposToLED[ii]->Text = "";//no exposition time
				else
					camExposToLED[ii]->Text = IntToStr(progPrms[19 + ii]);//exposition time
			}
			for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED channels
				plotLEDflg[ii]->Checked = bool(progPrms[23 + ii]);//if data is plotted
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TATManage::SaveSettingsClick(TObject *Sender)
{
	//save DriveLEDs settings to lcf-file
	__int32 ii,
			progPrms[27];//parameters array
	unsigned long int totWrt,//bytes wrote total
					  m_nWritten;//bytes wrote
	HANDLE hFile;//handle of file with parameter

	if (SaveSettDlg->Execute())
	{
		if (SaveSettDlg->FileName.Pos(".lcf") < 1) //no extension
			SaveSettDlg->FileName = SaveSettDlg->FileName + ".lcf";//add extension
		hFile = CreateFile(SaveSettDlg->FileName.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);//handle of file with parameter

		progPrms[0] = StrToInt(BinSize->Text);//binning
		progPrms[1] = StrToInt(Reps->Text);//loops number
		progPrms[2] = StrToInt(FramePrd->Text);//one lap duration (ms)
		for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED channels
			progPrms[3 + ii] = StrToInt(__int32(ledCheck[ii]->Checked));//if LED is in use
		for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED channels
		{
			if (ledSN[ii]->Text.Length() < 1) //is empty string
				progPrms[7 + ii] = -1;//serial number
			else
				progPrms[7 + ii] = StrToInt(ledSN[ii]->Text);//serial number
		}
		for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED channels
		{
			if (vLEDbright[ii]->Text.Length() < 1) //is empty string
				progPrms[11 + ii] = -1;//LED brightness (voltage)
			else
				progPrms[11 + ii] = StrToInt(vLEDbright[ii]->Text);//LED brightness (voltage)
		}
		for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED channels
		{
            if (durLEDon[ii]->Text.Length() < 1) //is empty string
				progPrms[15 + ii] = -1;//LED-on duration
			else
				progPrms[15 + ii] = StrToInt(durLEDon[ii]->Text);//LED-on duration
		}
		for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED channels
		{
			if (camExposToLED[ii]->Text.Length() < 1) //is empty string
				progPrms[19 + ii] = -1;//exposition time
			else
				progPrms[19 + ii] = StrToInt(camExposToLED[ii]->Text);//exposition time
		}
		for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED channels
			progPrms[23 + ii] = StrToInt(__int32(plotLEDflg[ii]->Checked));//if data is plotted

		//write all
		totWrt = 0;//bytes wrote total
		m_nWritten = 0;//bytes wrote
		for (ii = 0; ii < 27; ii++) //run over parameters to be saved
		{
			WriteFile(hFile, &progPrms[ii], sizeof(__int32), &m_nWritten, 0);
			totWrt += m_nWritten;//total bytes wrote
		}
		CloseHandle(hFile);//close file
	}
}
//---------------------------------------------------------------------------

void __fastcall TATManage::StartProgClick(TObject *Sender)
{
	//execute program for LEDs
	__int32 ii, jj, z, r, x, imN,
			useLEDnum,//unique commands for LEDs
			imCount,//number of images requested
			ordEnt[NUM_CHANNELS];//order entered by user
	__int64 maxExpos;//maximal exposition time
	AnsiString flNm,//buffer string with filename
			   bufStr;//buffer string
	float brtImRoi;//brightness within ROI
	unsigned int uiErr;//driver message code

	if (LoadRecalcImages->Tag == 0) //acquisition mode
	{
		if (!ledCamThread->stst) //start acquisition
		{
            RoiGraph->tagsNum = 0;//virtually delete all previous tags
			//ledCheck - array of pointers to LED-check boxes (UseLED1)
			//ledSN - array of pointers to LED-order edits (LED1sn)
			//vLEDbright - array of pointers to LED-voltage edits (LED1bright)
			for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED-illuminator channels
				ordEnt[ii] = 0;//prepare order checking
			useLEDnum = 0;//counter of needed LEDs
			for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED-illuminator channels
			{
				if (ledCheck[ii]->Checked) //LED #ii is in use and raw data requested
				{
					jj = StrToInt(ledSN[ii]->Text);//serial number of LED #ii
					ledCamThread->ledOrder[jj - 1] = ii;//number of LED to be switched on in jj order
					ledCamThread->vLEDbright[jj - 1] = float(StrToInt(vLEDbright[ii]->Text)) / 1e3;//LED voltage
					ordEnt[useLEDnum] = jj;//order entered by user
					useLEDnum++;//increase counter of LEDs in use
				}
			}

			//check order correctness
			for (jj = 0; jj < useLEDnum; jj++) //run over used LEDs
				for (ii = 1; ii <= useLEDnum; ii++) //run over used LEDs
					if (ii == ordEnt[jj]) //good number
						ordEnt[jj] = 0;//flag of order correctness
			z = 0;//flag of order correctness
			for (ii = 0; ii < useLEDnum; ii++) //run over used LEDs
				z += ordEnt[ii];//flag of order correctness
			if (z != 0) //wrong LEDs order
			{
				ProgList->Lines->Add("WRONG LEDs ORDER! CORRECT MANUALLY!");//error message
				return;
			}

			ledCamThread->useLEDsNum = useLEDnum;//number of needed LEDs
			ledCamThread->loopNum = StrToInt(Reps->Text);//number loops (times)
			ledCamThread->framPrd = StrToInt(FramePrd->Text);//period between 3-color frames (ms)

			StartProg->Caption = "Stop";//change button name
			StartProg->Tag = 1;//acquisition on progress
			LEDsConfig->Enabled = false;//LED changes not allowed
			CamConfig->Enabled = false;//camera changes not allowed
			Snap->Enabled = false;//snap not allowed
			Live->Enabled = false;//live stream not allowed
			GraphImag->BrightPorog->Enabled = false;//disable brightness threshold changes
			ledCamThread->stst = true;//start-stop flag (true - acquisition on progress, false - acquisition is stopped)

			PreparLines();//refresh line series

			//set all LEDs off
			for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LEDs
			{
				ledCamThread->ledOnDur[ii] = __int64(StrToInt(durLEDon[ii]->Text));//duration of LED-on period (ms)
				ledCamThread->exposToLED[ii] = float(StrToInt(camExposToLED[ii]->Text)) / 1e3;//expositions to each LED (seconds)
				TLDC4100_setConstCurrent(thorInstr, ii, 0);//set current (amperes)
				TLDC4100_setLedOnOff(thorInstr, ii, 0);//switch off
			}
			for (jj = 0; jj < useLEDnum; jj++) //run over need LEDs
				TLDC4100_setLedOnOff(thorInstr, ledCamThread->ledOrder[jj], 1);//switch LED on
			ledCamThread->ledOnExpDelay = StrToInt(LEdelay->Text);//LEDon-exposition delay (milliseconds)

			Sleep(200);//pause for LEDs reaction
			acqParams.acquisitMode = 5;//1 - "single scan"; 2 - "accumulate"; 3 -  "kinetic"; 5 - "run till abort"
			maxExpos = 0;//maximal exposition time
			if (ledCamThread->ringExposTime) //previous array exist
			{
				delete ledCamThread->ringExposTime; ledCamThread->ringExposTime = NULL;//delete array of exposure times
			}
			ledCamThread->ringExposTime = new float[useLEDnum];//array of exposure times
			for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LEDs
			{
				if (ledCheck[ii]->Checked) //LED #ii is in use
				{
					jj = StrToInt(ledSN[ii]->Text);//serial number of LED #ii
					ledCamThread->ringExposTime[jj - 1] = ledCamThread->exposToLED[ii];//next exposition time
					if (ledCamThread->ledOnDur[ii] > maxExpos) //maximal duration
						maxExpos = ledCamThread->ledOnDur[ii];//maximal exposition time
				}
			}
			acqParams.kineticCycTm = 0;//float(maxExpos) / 1e3;//kinetic cycle time (seconds)
			acqParams.trigMode = 10;////0 - "internal"; 1 - "external"; 10 - "software"
			acqParams.getImgNum = useLEDnum;//number of images to be scanned befor read out
			PreparCamera();//apply configuration of camera
			uiErr = SetRingExposureTimes(useLEDnum, ledCamThread->ringExposTime);//set an array of exposure times
            if (uiErr != DRV_SUCCESS)
				ProgList->Lines->Add("!Ring exposition times error");//text of error message
			else
			{
				bufStr = "exposition times:";//text of error message
				for (ii = 0; ii < useLEDnum; ii++) //run over LEDs
					bufStr += IntToStr(__int32(1e3 * ledCamThread->ringExposTime[ii])) + ", ";
				ProgList->Lines->Add(bufStr);//text of error message
			}

			ProgList->Lines->Add("acquisition start");
			ledCamThread->Resume();//resume device controlling thread
		} //end of (if (!ledCamThread->stst) //start acquisition)
		else //stop acquisition
		{
			ledCamThread->stst = false;//start-stop flag (true - acquisition on progress, false - acquisition is stopped)
			StartProg->Caption = "Start";//change button name
			StartProg->Tag = 0;//acquisition stopped
			LEDsConfig->Enabled = true;//LED changes allowed
			CamConfig->Enabled = true;//camera changes allowed
			Snap->Enabled = true;//snap allowed
			Live->Enabled = true;//live stream allowed
			GraphImag->BrightPorog->Enabled = true;//enable brightness threshold changes
			ProgList->Lines->Add("acquisition successful end");

			//= save digital brightness data if requested =//
			if ((SavFlag->Checked) && (Live->Tag == 0)) //save digital data
			{
                rawText = "_raw";//suffix (indicat about save after acquisition)
				SaveDigBrtData();//save digital data
			}
			//= end of save digital brightness data if requested =//
		} //end of (else //stop acquisition)
	} //end of (if (LoadRecalcImages->Tag == 0) //acquisition mode)
	else //(LoadRecalcImages->Tag == 1) //recalculation mode
	{
		if (LoadImagesDlg->Tag == 1) //good images was detected
		{
			PreparLines();//refresh line series
			imCount = SelFileList->Tag;//number of images requested
			for (ii = 0; ii < imCount; ii++) //run over images (files)
			{
				if (readLEDn[1][ii] > -1) //valide image
				{
					flNm = selFiles[readLEDn[2][ii]];//file to be read
					tiffImg = TIFFOpen(flNm.c_str(), "r");//read next image
					if (tiffImg != NULL) //success read
					{
						for (z = 0; z < bVertRes; z++) //run over rows
							TIFFReadScanline(tiffImg, &rawImgs[z * bHorzRes], z, 0);//read a line from current imadge

						for (r = 0; r < GraphImag->actNumROI; r++) //run over ROIs
						{
							brtImRoi = 0;//brightness
							for (x = 1; x < (GraphImag->roiMask[r][0] + 1); x++) //run over points in ROI-mask
								brtImRoi += rawImgs[GraphImag->roiMask[r][x]];//brightness of the point within the ROI

							brtImRoi *= 100;//percent
							brtImRoi /= (GraphImag->roiMask[r][0] * saturatLevel);//average intesity (over points within the ROI), percents of saturation

							brtnessImROItime[readLEDn[0][ii] - 1][r]->AddXY(ledCamThread->imTStamps[ii], brtImRoi);//add point (time-brightness)
							brtnessImROIframe[readLEDn[0][ii] - 1][r]->AddXY(ceil(readLEDn[1][ii] / acqParams.getImgNum), brtImRoi);//add point (frame-brightness)
						}
						TIFFClose(tiffImg);//close the file
					} //end of (if (tiffImg != NULL) //success read)
				}
			} //end of (for (ii = 0; ii < imCount; ii++) //run over images (files))

			for (ii = 0; ii < floor(imCount / acqParams.getImgNum); ii++) //run over measures (frames)
			{
				for (r = 0; r < GraphImag->actNumROI; r++) //run over ROIs
				{
					if ((brtnessImROItime[0][r]->Count() >= (ii + 1)) && (brtnessImROItime[3][r]->Count() >= (ii + 1))) //data exist
					{
						brtImRoi = brtnessImROItime[0][r]->YValues->operator [](ii) / brtnessImROItime[3][r]->YValues->operator [](ii);//ratio1 (LED1/LED4)
						ratioImROItime[0][r]->AddXY(brtnessImROItime[0][r]->XValues->operator [](ii), brtImRoi);//add point (time-ratio1)
						ratioImROIframe[0][r]->AddXY(ii, brtImRoi);//add point (frame-ratio1)
					}

					if ((brtnessImROItime[2][r]->Count() >= (ii + 1)) && (brtnessImROItime[3][r]->Count() >= (ii + 1))) //data exist
					{
						brtImRoi = brtnessImROItime[2][r]->YValues->operator [](ii) / brtnessImROItime[3][r]->YValues->operator [](ii);//ratio2 (LED3/LED4)
						ratioImROItime[1][r]->AddXY(brtnessImROItime[2][r]->XValues->operator [](ii), brtImRoi);//add point (time-ratio2)
						ratioImROIframe[1][r]->AddXY(ii, brtImRoi);//add point (frame-ratio2)
					}
				}
			} //end of (for (ii = 0; ii < floor(imCount / acqParams.getImgNum); ii++) //run over measures (frames))
			rawText = "";//suffix (indicat about save after acquisition)
			SaveDigBrtData();//save new data
		} //end of (if (LoadImagesDlg->Tag == 1) //good images was detected)
	} //end of (else //(LoadRecalcImages->Tag == 1) //recalculation mode)
}
//---------------------------------------------------------------------------

void TATManage::PreparCamera()
{
	//apply configuration of camera
	__int32 ii, x, y;
	unsigned int uiErr;//driver message code
	int pixImgPrm[6];//scanning part of CCD matrix
	long bufCap;//number of images the circular buffer can store based on the current acquisition settings
	float maxExpos;//maximal exposition time

	acqParams.readMode  = 4;//"image" read mode
	maxExpos = 0;//maximal exposition time
	for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LEDs
		if ((ledCheck[ii]->Checked) && ((float(StrToInt(camExposToLED[ii]->Text)) / 1e3) > maxExpos)) //LED #ii exposition time
			maxExpos = float(StrToInt(camExposToLED[ii]->Text)) / 1e3;//maximal exposition time (secods)
	acqParams.exposTime = maxExpos;//exposition (seconds)

	pixImgPrm[0] = StrToInt(BinSize->Text);//horizontal binning
	pixImgPrm[1] = pixImgPrm[0];//vertical binning
	pixImgPrm[2] = 1;//first pixel to be scanned horizontally (from 1)
	pixImgPrm[3] = hRes;//last pixel to be scanned horizontally (from 1)
	pixImgPrm[4] = 1;//first pixel to be scanned vertically (from 1)
	pixImgPrm[5] = vRes;//last pixel to be scanned vertically (from 1)

	bHorzRes = (pixImgPrm[3] - pixImgPrm[2] + 1) / pixImgPrm[0];//binned horizontal resolution
	bVertRes = (pixImgPrm[5] - pixImgPrm[4] + 1) / pixImgPrm[1];//binned vertical resolution
	imSize = bHorzRes * bVertRes;//image size (pixels)

	//prepare graphics objects
	PreparImages();//set images sizes

	ProgList->Clear();//clear message box
	ProgList->Lines->Add("detector " + IntToStr(hRes) + " x " + IntToStr(vRes));
	ProgList->Lines->Add("binned detector " + IntToStr(bHorzRes) + " x " + IntToStr(bVertRes));

	//set acquisition paramaters
	uiErr = SetReadMode(acqParams.readMode);
	if (uiErr != DRV_SUCCESS)
		ProgList->Lines->Add("SetReadMode: " + IntToStr((int)uiErr));

	uiErr = SetImage(pixImgPrm[0], pixImgPrm[1], //binning
					 pixImgPrm[2], pixImgPrm[3], //left-right edges (ROI)
					 pixImgPrm[4], pixImgPrm[5]);//upper-lower edges (ROI)
	if (uiErr != DRV_SUCCESS)
		ProgList->Lines->Add("SetImage: " + IntToStr((int)uiErr));

	uiErr = SetAcquisitionMode(acqParams.acquisitMode);//set acquisition mode
	if (uiErr != DRV_SUCCESS)
		ProgList->Lines->Add("SetAcquisitionMode: " + IntToStr((int)uiErr));

	uiErr = SetTriggerMode(acqParams.trigMode);//set trigger mode
	if (uiErr != DRV_SUCCESS)
		ProgList->Lines->Add("SetTriggerMode: " + IntToStr((int)uiErr));

//	uiErr = SetNumberAccumulations(acqParams.getImgNum);//number of images to be scanned befor read out)
//	if (uiErr != DRV_SUCCESS)
//		ProgList->Lines->Add("SetNumberAccumulations: " + IntToStr((int)uiErr));

	uiErr = SetExposureTime(acqParams.exposTime);//set exposition time
	if (uiErr != DRV_SUCCESS)
		ProgList->Lines->Add("SetExposureTime: " + IntToStr((int)uiErr));

	uiErr = SetKineticCycleTime(acqParams.kineticCycTm);//minimal time between acquisition (seconds)
	if (uiErr != DRV_SUCCESS)
		ProgList->Lines->Add("SetKineticCycleTime: " + IntToStr((int)uiErr));

//	uiErr =	GetSizeOfCircularBuffer(&bufCap);//number of images the circular buffer can store based on the current acquisition settings
//	if (uiErr != DRV_SUCCESS)
//		ProgList->Lines->Add("GetSizeOfCircularBuffer: " + IntToStr((int)uiErr));
//	else
//		ProgList->Lines->Add("buffer can store " + IntToStr(int(bufCap)) + " images");

	//string filename
	CurrentDateTime = Now();//current date and time
	baseName = "c:\\Users\\User\\Pictures\\TA_";//path
	baseName += CurrentDateTime.FormatString("YYYY-MM-DD_hh-nn-ss");//add date and time
}
//---------------------------------------------------------------------------

void TATManage::PreparImages()//__int32 imN)
{
	//set image-objects sizes (on GraphImag form)
	//imN - number of images to be used
	__int32 ii, jj;

	GraphImag->OnResize = NULL;//switch off reaction
	GraphImag->Height = 100;//smallest verticatl size of window
	if ((55 + bVertRes) > 100) //image larger
		GraphImag->Height = 55 + bVertRes;//verticatl size of window
	GraphImag->Width = acqParams.getImgNum * (bHorzRes + 1) + 20 + 10;//horizontal size of window
	GraphImag->OnResize = GraphImag->FormResize;//switch off reaction
	GraphImag->reSizeCoeff = 1;//resize coefficient (CCD Horizont size / actual image size)
	GraphImag->BrightPorog->Height = bVertRes;//scrolbar size

	for (ii = 0; ii < NUM_CHANNELS; ii++) //run over needed image-objects
	{
		//set image size
		if (ii < acqParams.getImgNum) //image in use
		{
			imagPnlObj[ii]->Visible = true;//visible image
			imagPnlObj[ii]->Enabled = true;//activate image object
			imagPnlObj[ii]->Top = 20;//vertical position
			imagPnlObj[ii]->Left = ii * (bHorzRes + 1) + 20;//horizontal position
			imagPnlObj[ii]->Width = bHorzRes;//horizontal size of image
			imagPnlObj[ii]->Height = bVertRes;//vertical size of image

			ledOrdLblG[ii]->Visible = true;//visible label
			ledOrdLblG[ii]->Left = ii * (bHorzRes + 1) + (bHorzRes / 2) - 10;//horizontal position
			ledOrdLblG[ii]->Caption = "LED " + IntToStr(ledCamThread->ledOrder[ii] + 1);//number of LED switched in order ii
		}
		else //not used image
		{
			imagPnlObj[ii]->Enabled = false;//inactivate image object
			imagPnlObj[ii]->Visible = false;//invisible image
			imagPnlObj[ii]->Top = 20;//vertical position
			imagPnlObj[ii]->Left = (ATManage->acqParams.getImgNum - 1) * bHorzRes;//horizontal position

			ledOrdLblG[ii]->Visible = false;//invisible label
		}
	}

	if (rawImgs) //the array was created earlier
		delete[] rawImgs;//delete previous data
	rawImgs = new unsigned short[bHorzRes * bVertRes * acqParams.getImgNum];//preallocation memory for image

	if (rawImgsSat) // the array was created earlier
		delete[] rawImgsSat;//delete previous data
	rawImgsSat = new float[bHorzRes * bVertRes * acqParams.getImgNum];//preallocation memory for image

	brtMag = 1 + (GraphImag->BrightPorog->Position - 1) * 0.2;//brightness amplification
}
//---------------------------------------------------------------------------

void TATManage::DisplayImage()
{
	//get images from CCD (if need), draw images on a form
	__int32 ii, x, y, imN, r, m,
			radr;//raw images address
	float brtImRoi,//brightness within ROI
		  intensity;//pixel brightness
//	__int64 tactps,//CPU clock frequency (tacts per second)
//			corTact1,//correction tact1
//			corTact2;//correction tact2
	long frstIm,//index of the first available image in the circular buffer
		 lastIm,//index of the last available image in the circular buffer
		 validFrstIm,//index of the first valid image.
		 validLastIm;//index of the last valid image.
	UnicodeString tmStr;//time string
	AnsiString flNm;//filename

//	QueryPerformanceFrequency((_LARGE_INTEGER*)&tactps);//CPU clock frequency (tacts per second)
//	QueryPerformanceCounter((_LARGE_INTEGER*)&corTact1);//current tact of CPU
	if (StartProg->Tag) //acquisition on progress
	{
		GetNumberNewImages(&frstIm, &lastIm);//get indexis of first and last new images
		while ((lastIm - frstIm + 1) < acqParams.getImgNum) //wait requested images
			GetNumberNewImages(&frstIm, &lastIm);//get indexis of first and last new images
			//GetNumberAvailableImages(&frstIm, &lastIm);//get indexis of first and last available images

		GetImages16(lastIm - acqParams.getImgNum + 1, lastIm, rawImgs, imSize * acqParams.getImgNum, &validFrstIm, &validLastIm);//get all newly acquired images
		if ((validLastIm - validFrstIm + 1) != acqParams.getImgNum) //bad image acquisition
		{
			ProgList->Lines->Add("wrong new images indexes");
			ProgList->Lines->Add(IntToStr(int(frstIm)) + ", " + IntToStr(int(lastIm)) + ", " + IntToStr(int(validFrstIm)) + ", " + IntToStr(int(validLastIm)));
		}
	}
	else //if (Live->Tag == 1) //images acquiring in live-mode or snap
		GetMostRecentImage16(rawImgs, imSize);//get latest image (for live)

//	QueryPerformanceCounter((_LARGE_INTEGER*)&corTact2);//current tact of CPU
//	ProgList->Lines->Add(IntToStr(((1000 * (corTact2 - corTact1)) / tactps)) + " ms");

	radr = 0;//raw images address
	for (imN = 0; imN < acqParams.getImgNum; imN++) //run over all acquired images (different LEDs)
	{
//		QueryPerformanceCounter((_LARGE_INTEGER*)&corTact1);//current tact of CPU

		for (y = 0; y < bVertRes; y++) //run over rows
		{
			for (x = 0; x < bHorzRes; x++) //run over colums
			{
				rawImgsSat[radr] = float(rawImgs[radr]) / saturatLevel;//set pixel (gray)
				radr++;//next pixel
			}
		}

//		QueryPerformanceCounter((_LARGE_INTEGER*)&corTact2);//current tact of CPU
//		ProgList->Lines->Add(IntToStr(((1000 * (corTact2 - corTact1)) / tactps)) + " ms");

		//= brightness and ratios in ROIs calculation =//
		ii = ledCamThread->ledOrder[imN];//LED number
		if (StartProg->Tag) //acquisition on progress
		{
			for (r = 0; r < GraphImag->actNumROI; r++) //run over ROIs
			{
				brtImRoi = 0;//brightness
				for (x = 1; x < (GraphImag->roiMask[r][0] + 1); x++) //run over points in ROI-maks
					brtImRoi += rawImgs[(imN * imSize) + GraphImag->roiMask[r][x]];//brightness of the point within the ROI

				brtImRoi *= 100;//percent
				brtImRoi /= (GraphImag->roiMask[r][0] * saturatLevel);//average intesity (over points within the ROI), percents of saturation

				brtnessImROItime[ii][r]->AddXY(ledCamThread->imTStamps[(acqParams.getImgNum * ledCamThread->mlc) + imN], brtImRoi);//add point (time-brightness)
				brtnessImROIframe[ii][r]->AddXY(ledCamThread->mlc, brtImRoi);//add point (frame-brightness)
			}
		}
		//= end of brightness in ROIs calculation =//

		//= save images if requested =//
		if ((StartProg->Tag) && (SavFlag->Checked) && (Live->Tag == 0)) //save image (not live, acquisition on progress)
		{
			tmStr = FloatToStr(ledCamThread->imTStamps[(acqParams.getImgNum * ledCamThread->mlc) + imN]);//timedata as string
			x = tmStr.Pos(",");//comma postion (decimal point)
			tmStr.SetLength(x + 4);//4 digits after decimal point

			flNm = baseName + "_LED" + IntToStr(ledCamThread->ledOrder[imN] + 1) + "_" + tmStr + ".tiff";//name of file to be saved
			tiffImg = TIFFOpen(flNm.c_str(), "w");//creat tiff-imadge
			if (tiffImg != NULL) //file was created
			{
				//set up the image tags
				TIFFSetField(tiffImg, TIFFTAG_IMAGEWIDTH, bHorzRes);//horizontal size of window
				TIFFSetField(tiffImg, TIFFTAG_IMAGELENGTH, bVertRes);//verticatl size of window
				TIFFSetField(tiffImg, TIFFTAG_BITSPERSAMPLE, adcDepth);//ADC bit depth
				TIFFSetField(tiffImg, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
				TIFFSetField(tiffImg, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
				TIFFSetField(tiffImg, TIFFTAG_SAMPLESPERPIXEL, 1);
				TIFFSetField(tiffImg, TIFFTAG_ROWSPERSTRIP, 1);
				TIFFSetField(tiffImg, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
				TIFFSetField(tiffImg, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);
				TIFFSetField(tiffImg, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

				//now go line by line to write out the image data
				for (y = 0; y < bVertRes; y++) //run over rows
					TIFFWriteScanline(tiffImg, &rawImgs[(imN * bVertRes * bHorzRes) + y * bHorzRes], y, 0);//now actually write the row of pixels

				TIFFClose(tiffImg);//close the file
			}
			else //saving problem
			{
				ProgList->Lines->Add("error: TIFFOpen");//saving problem
			}
			imGlbNum++;//global number of saved image for current session
		}
		//= end of save images if requested =//
	}

	GraphImag->ReDrawImagesROIs(this);//redraw images and ROIs

	//calculate and plot rations
	if (StartProg->Tag) //acquisition on progress
	{
		ii = (acqParams.getImgNum * ledCamThread->mlc) + acqParams.getImgNum - 1;//use timestamp of the last image
		for (r = 0; r < GraphImag->actNumROI; r++) //run over ROIs
		{
			//ratio-1 (LED1/LED4)
			if (brtnessImROItime[0][r]->Count() < brtnessImROItime[3][r]->Count()) //smallest index
				m = brtnessImROItime[0][r]->Count();//smallest index
			else
				m = brtnessImROItime[3][r]->Count();//smallest index

			//if ((brtnessImROItime[0][r]->Count() >= (ledCamThread->mlc + 1)) && (brtnessImROItime[3][r]->Count() >= (ledCamThread->mlc + 1))) //data exist
			if ((m > 0) && (m > ratioImROItime[0][r]->Count())) //data exist
			{
				//brtImRoi = brtnessImROItime[0][r]->YValues->operator [](ledCamThread->mlc) / brtnessImROItime[3][r]->YValues->operator [](ledCamThread->mlc);//ratio1 (LED1/LED4)
				m--;//count to index
				brtImRoi = brtnessImROItime[0][r]->YValues->operator [](m) / brtnessImROItime[3][r]->YValues->operator [](m);//ratio1 (LED1/LED4)
				//ratioImROItime[0][r]->AddXY(ledCamThread->imTStamps[ii], brtImRoi);//add point (time-ratio1)
				ratioImROItime[0][r]->AddXY(brtnessImROItime[0][r]->XValues->operator [](m), brtImRoi);//add point (time-ratio1)
				//ratioImROIframe[0][r]->AddXY(ledCamThread->mlc, brtImRoi);//add point (frame-ratio1)
				ratioImROIframe[0][r]->AddXY(brtnessImROIframe[0][r]->XValues->operator [](m), brtImRoi);//add point (frame-ratio1)
			}

			//ratio-2 (LED3/LED4)
			if (brtnessImROItime[2][r]->Count() < brtnessImROItime[3][r]->Count()) //smallest index
				m = brtnessImROItime[2][r]->Count();//smallest index
			else
				m = brtnessImROItime[3][r]->Count();//smallest index

			//if ((brtnessImROItime[2][r]->Count() >= (ledCamThread->mlc + 1)) && (brtnessImROItime[3][r]->Count() >= (ledCamThread->mlc + 1))) //data exist
			if ((m > 0) && (m > ratioImROItime[1][r]->Count())) //data exist
			{
				//brtImRoi = brtnessImROItime[2][r]->YValues->operator [](ledCamThread->mlc) / brtnessImROItime[3][r]->YValues->operator [](ledCamThread->mlc);//ratio2 (LED3/LED4)
				m--;//count to index
				brtImRoi = brtnessImROItime[2][r]->YValues->operator [](m) / brtnessImROItime[3][r]->YValues->operator [](m);//ratio2 (LED3/LED4)
				//ratioImROItime[1][r]->AddXY(ledCamThread->imTStamps[ii], brtImRoi);//add point (time-ratio2)
				ratioImROItime[1][r]->AddXY(brtnessImROItime[2][r]->XValues->operator [](m), brtImRoi);//add point (time-ratio2)
				//ratioImROIframe[1][r]->AddXY(ledCamThread->mlc, brtImRoi);//add point (frame-ratio2)
				ratioImROIframe[1][r]->AddXY(brtnessImROIframe[2][r]->XValues->operator [](m), brtImRoi);//add point (frame-ratio2)
			}
		} //end of (for (r = 0; r < GraphImag->actNumROI; r++) //run over ROIs)
	} //if (StartProg->Tag) //acquisition on progress
}
//---------------------------------------------------------------------------

void __fastcall TATManage::SnapClick(TObject *Sender)
{
	//make single image acquisition
	unsigned int uiErr;

	acqParams.acquisitMode = 1;//1 - "single scan"; 2 - "accumulate"; 3 -  "kinetic"; 5 - "run till abort"
	acqParams.trigMode = 0;//0 - "internal"; 1 - "external"; 10 - "software"
	acqParams.kineticCycTm = 0.3;//kinetic cycle time (seconds)
	acqParams.getImgNum = 1;//number of images to be scanned befor read out
	PreparCamera();//apply configuration of camera
	ledOrdLblG[0]->Visible = false;//invisible label

	//get aquisiton
	uiErr = StartAcquisition();//start acquisition on camera
	uiErr = WaitForAcquisition();//wait for acquisition complete
	DisplayImage();//show image
}
//---------------------------------------------------------------------------

void __fastcall TATManage::LiveClick(TObject *Sender)
{
	//live from the camera
	__int32 ii;
	float maxExpos;//maximal exposition time

	if (Live->Tag == 0) //start live demonstration
	{
		Snap->Enabled = false;//desabled
		StartProg->Enabled = false;//desabled
		LoadRecalcImages->Enabled = false;//desabled

		Live->Tag = 1;//live vision on progress
		Live->Caption = "Idle";//change button function
		acqParams.acquisitMode = 5;//1 - "single scan"; 2 - "accumulate"; 3 -  "kinetic"; 5 - "run till abort"
		maxExpos = 0;//maximal exposition time
		for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LEDs
			if ((float(StrToInt(camExposToLED[ii]->Text)) / 1e3) > maxExpos) //LED #ii exposition time
				maxExpos = float(StrToInt(camExposToLED[ii]->Text)) / 1e3;//maximal exposition time (secods)
		acqParams.kineticCycTm = 0;//maxExpos;//kinetic cycle time (seconds)
		acqParams.trigMode = 0;//0 - "internal"; 1 - "external"; 10 - "software"
		acqParams.getImgNum = 1;//number of images to be scanned befor read out
		PreparCamera();//apply configuration of camera
		ledOrdLblG[0]->Visible = false;//invisible label
		//GraphImag->LED1Img->Anchors << akLeft << akTop << akRight << akBottom;
		GraphImag->BorderStyle << bsSizeable;//window can be resized
		ledCamThread->Resume();//resume device controlling thread
	}
	else //end live demonstration
	{
		Live->Tag = 0;//live vision is stopped
		Live->Caption = "Live";//change button function
		//GraphImag->LED1Img->Anchors << akLeft << akTop;
		GraphImag->BorderStyle << bsSingle;//unresizable window

		Snap->Enabled = true;//enabled
		StartProg->Enabled = true;//enabled
		LoadRecalcImages->Enabled = true;//enabled
	}
}
//---------------------------------------------------------------------------

void error_exit(ViStatus err, UnicodeString funcName)
{
	ViChar ebuf[DC4100_ERR_DESCR_BUFFER_SIZE];

	TLDC4100_error_message(thorInstr, err, ebuf);
	ATManage->ProgList->Lines->Add("DC100 error: " + funcName);//print error
}
//---------------------------------------------------------------------------

static ViStatus readInstrData(ViSession resMgr, ViRsrc rscStr, ViChar *name, ViChar *descr, ViAccessMode *lockState)
{
	//Read instrument data. Reads the data from the opened VISA instrument session.
	//Return value: VISA library error code.
	//Note: For Serial connections we use the VISA alias as name and the 'VI_ATTR_INTF_INST_NAME' as description.

	#define DEF_VI_OPEN_TIMEOUT	1500 				// open timeout in milliseconds
	#define DEF_INSTR_NAME			"Unknown"		// default device name
	#define DEF_INSTR_ALIAS			""				// default alias
	#define DEF_LOCK_STATE			VI_NO_LOCK		// not locked by default

	ViStatus err;
	ViSession thorInstr;
	ViUInt16 intfType, intfNum;
	ViInt16 ri_state, cts_state, dcd_state;

	//default values
	strcpy(name,  DEF_INSTR_NAME);
	strcpy(descr, DEF_INSTR_ALIAS);
	*lockState = DEF_LOCK_STATE;

	//get alias
	if ((err = vi32ParseRsrcEx(resMgr, rscStr, &intfType, &intfNum, VI_NULL, VI_NULL, name)) < 0)
		return err;

	if (intfType != VI_INTF_ASRL)
		return VI_ERROR_INV_RSRC_NAME;

	//open resource
	err = vi32Open(resMgr, rscStr, VI_NULL, DEF_VI_OPEN_TIMEOUT, &thorInstr);
	if (err == VI_ERROR_RSRC_BUSY)
	{
		// Port is open in another application
		if (strncmp(name, "LPT", 3) == 0)
		{
			// Probably we have a LPT port - do not show this
			return VI_ERROR_INV_OBJECT;
		}
		// display port as locked
		*lockState = VI_EXCLUSIVE_LOCK ;
		return VI_SUCCESS;
	}
	if (err)
		return err;

	//get attribute data
	err = vi32GetAttribute(thorInstr, VI_ATTR_RSRC_LOCK_STATE, lockState);
	if (!err)
		err = vi32GetAttribute(thorInstr, VI_ATTR_INTF_INST_NAME,  descr);

	//get wire attributes to exclude LPT...
	if (!err)
		err = vi32GetAttribute(thorInstr, VI_ATTR_ASRL_RI_STATE,   &ri_state);
	if (!err)
		err = vi32GetAttribute(thorInstr, VI_ATTR_ASRL_CTS_STATE,  &cts_state);
	if (!err)
		err = vi32GetAttribute(thorInstr, VI_ATTR_ASRL_DCD_STATE,  &dcd_state);
	if (!err)
	{
		if ((ri_state == VI_STATE_UNKNOWN) && (cts_state == VI_STATE_UNKNOWN) && (dcd_state == VI_STATE_UNKNOWN))
			err = VI_ERROR_INV_OBJECT;
	}

	//closing
	vi32Close(thorInstr);
	return err;
}
//---------------------------------------------------------------------------

static ViStatus CleanupScan(ViSession resMgr, ViFindList findList, ViStatus err)
{
	//Cleanup scan. Frees the data structures used for scanning instruments.
	//Return value: value passed via the 'err' parameter.

	if (findList != VI_NULL)
		vi32Close(findList);
	if (resMgr != VI_NULL)
		vi32Close(resMgr);
	return err;
}
//---------------------------------------------------------------------------

void __fastcall TATManage::FormCloseQuery(TObject *Sender, bool &CanClose)
{
	//close the GUI
	if (visaModul)
	{
		if (thorInstr != VI_NULL)
			TLDC4100_close(thorInstr);//close device
		FreeLibrary(visaModul);//release the driver
		visaModul = NULL;
	}
	if (atmcd32d)
	{
		FreeLibrary(atmcd32d);//release the driver
		atmcd32d = NULL;
	}
	if (cam_handle)
		ShutDown();

	if (rawImgs) //data array exist
	{
		delete[] rawImgs;
		rawImgs = NULL;
	}

	if (rawImgsSat) //data array exist
	{
		delete[] rawImgsSat;
		rawImgsSat = NULL;
	}

	if (readLEDn[0])
	{
		delete[] readLEDn[0]; readLEDn[0] = NULL;//delete array for original numbers of the LED
		delete[] readLEDn[1]; readLEDn[1] = NULL;//delete array for numbers of corresponding image
		delete[] readLEDn[2]; readLEDn[2] = NULL;//delete array for numbers of filename
	}
}
//---------------------------------------------------------------------------

void __fastcall TATManage::IntPrmKeyPress(TObject *Sender, System::WideChar &Key)
{
	//check entered symbol
	if (((Key < '0') || (Key > '9')) && (Key != '\b')) //incorrect symbol
		Key = '\0';//delete incorrect symbol
}
//---------------------------------------------------------------------------

void __fastcall TATManage::FloatPrmKeyPress(TObject *Sender, System::WideChar &Key)
{
	//check entered symbol
	if (((Key < '0') || (Key > '9')) && (Key != '\b') && (Key != ',')) //incorrect symbol
		Key = '\0';//delete incorrect symbol
}
//---------------------------------------------------------------------------

void __fastcall TATManage::LEDsnChange(TObject *Sender)
{
	//check LED on-off order
	//ledCheck - array of pointers to LED-check boxes (UseLED1)
	//ledSN - array of pointers to LED-order edits (LED1sn)
	//vLEDbright - array of pointers to LED-voltage edits (LED1bright)

	__int32 ii, z;

	for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED-channels
	{
		if (ledCheck[ii]->Checked)
		{
			if (ledSN[ii]->Text.Length() < 1) //LED #ii in use but without serial number
				ledSN[ii]->Text = IntToStr(4);//IntToStr(NUM_CHANNELS);//serial number of LED #ii
			else
			{
				z = StrToInt(ledSN[ii]->Text);//numberic value
				if (z < 1) //too small
					ledSN[ii]->Text = IntToStr(1);//serial number of LED #ii
				if (z > NUM_CHANNELS) //too large
					ledSN[ii]->Text = IntToStr(NUM_CHANNELS);//serial number of LED #ii
			}
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TATManage::ManualSwitchClick(TObject *Sender)
{
	//switch LEDs manually
	__int32 ii;
	unsigned short stat;

	if (ManualSwitch->Checked) //try manually switch on-off LEDs
	{
		for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED channels
		{
			TLDC4100_getLedOnOff(thorInstr, ii, &stat);//get LED status
			if (ledCheck[ii]->Checked) //LED #ii is in use
			{
				TLDC4100_setConstCurrent(thorInstr, ii, float(StrToInt(vLEDbright[ii]->Text)) / 1e3);//set current (amperes)
				if (~stat) //LED is switced off
					TLDC4100_setLedOnOff(thorInstr, ii, 1);//turn on the LED
			}
			else //LED #ii have to be turned off
			{
				if (stat) //LED is switced on
					TLDC4100_setLedOnOff(thorInstr, ii, 0);//turn off the LED
			}
		}
	}
	else //end manual switching
	{
    	for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED channels
		{
			TLDC4100_getLedOnOff(thorInstr, ii, &stat);//get LED status
			if (stat) //LED is switced on
			{
                TLDC4100_setConstCurrent(thorInstr, ii, 0);//set current (amperes)
				TLDC4100_setLedOnOff(thorInstr, ii, 0);//turn off the LED
			}
		}
	}
}
//---------------------------------------------------------------------------

void TATManage::SaveDigBrtData()
{
	//save digital data
	__int32 ii, imN, r,
			t0,//time of record start (second from day beding)
			pN,//number of experimental points
			ledN,//LED index
			crntTagI;//number of current tag
	FILE *fileStrm;//stream for output data to file
	AnsiString bufStr;//buffer string
	bool prntFlg;//print flag for tags
	float timeStmp;//current timestampe
	//double dStartT;//difference between start timestampes dStartT = double((startTact - ledCamThread->firstTact) / ledCamThread->tactps);//difference between start timestampes (seconds)
	UnicodeString strTmStamp;//current timestamp (string, hh-mm-ss)
	TDateTime dateTime;//current date and time

	ii = baseName.Length();//length of date-time string
	t0 = StrToInt(baseName.SubString(ii - 7, 2)) * 3600 + //hours
		 StrToInt(baseName.SubString(ii - 4, 2)) * 60 + //minutes
		 StrToInt(baseName.SubString(ii - 1, 2));//seconds
	for (imN = 0; imN < acqParams.getImgNum; imN++) //run over all acquired images (different LEDs)
	{
		ledN = ledCamThread->ledOrder[imN];//original LED number
		bufStr = baseName + "_LED" + IntToStr(ledN + 1) + rawText+ ".txt";//pathname for data from current LED
		fileStrm = fopen(bufStr.c_str(), "w");//create file

		fprintf(fileStrm, "frame\ttime(h-m-s)\ttime(s)");//head line
		for (r = 0; r < GraphImag->actNumROI; r++) //run over ROIs
			fprintf(fileStrm, "\tROI%d", r + 1);//head line
		fprintf(fileStrm, "\ttags");
		fprintf(fileStrm, "\n");

		crntTagI = 0;//number of current tag
		prntFlg = false;//print flag for tags

		pN = brtnessImROItime[ledN][0]->XValues->Count;//points number
		for (ii = 0; ii < pN; ii++) //run over brigntness points
		{
			fprintf(fileStrm, "%d", ii + 1);//frame number
			timeStmp = brtnessImROItime[ledN][0]->XValues->operator [](ii);//current timestamp (seconds)

			dateTime.Val = double(t0 + timeStmp) / double(24 * 3600);//plus "timeStmp" second
			strTmStamp = dateTime.FormatString("hh-nn-ss");//formated string
			bufStr = strTmStamp;//copy as AnsiString
			fprintf(fileStrm, "\t%s", bufStr);//current timestamp (string, hh-mm-ss)

			fprintf(fileStrm, "\t%6.3f", timeStmp);//current timestamp (seconds)
			for (r = 0; r < GraphImag->actNumROI; r++) //run over ROIs
			{
				fprintf(fileStrm, "\t%6.6f", brtnessImROItime[ledN][r]->YValues->operator [](ii));//brightness value
			}
			if ((RoiGraph->tagsNum > 0) && (crntTagI < RoiGraph->tagsNum)) //tag was entered
			{
				if (timeStmp >= RoiGraph->pntTags[crntTagI].timeStmp)// || //tag on time
					prntFlg = true;//print flag for tags
			}
			if (prntFlg) //tag exist and on time
			{
				fprintf(fileStrm, "\t");
				bufStr = RoiGraph->pntTags[crntTagI].textMark;//copy as nonUnicode
				fprintf(fileStrm, (char*)bufStr.c_str());
				crntTagI++;//next tag
				prntFlg = false;//print flag for tags
			}
			else
				fprintf(fileStrm, "\t - ");

			fprintf(fileStrm, "\n");//next line
		}
		fclose(fileStrm);//close file with data from current LED
	}

	//also save user's tag in a separated file
//	if (RoiGraph->tagsNum > 0) //tag was entered
//	{
//		bufStr = baseName + "Tags.txt";//pathname for data from current LED
//		fileStrm = fopen(bufStr.c_str(), "w");//create file
//		for (ii = 0; ii < RoiGraph->tagsNum; ii++)
//		{
//
//		}
//    }
}
//---------------------------------------------------------------------------

void __fastcall TATManage::BinSizeChange(TObject *Sender)
{
	//binning was changed
	GraphImag->DeleteAllROIClick(this);//delete all ROIs
}
//---------------------------------------------------------------------------

void __fastcall TATManage::LEDonDurChange(TObject *Sender)
{
	//check duration of LED-on period
	__int32 n;//led number
	TEdit *ledOnDr;//control handle

	ledOnDr = (TEdit *)Sender;//control handle
	n = ledOnDr->Tag;//led number
//	if (StrToInt(ledOnDr->Text) < StrToInt(camExposToLED[n]->Text))
//		ledOnDr->Text = camExposToLED[n]->Text;//set minimal valid LED-on time
}
//---------------------------------------------------------------------------

void __fastcall TATManage::ExposTimeChange(TObject *Sender)
{
	//check exposition time
	__int32 ii, minOnDur,
			n;//led number
	TEdit *exposTm;//control handle

	exposTm = (TEdit *)Sender;//control handle
	n = exposTm->Tag;//led number
//	if (StrToInt(durLEDon[n]->Text) < StrToInt(exposTm->Text))
//		exposTm->Text = durLEDon[n]->Text;//set maximal valid exposition time
}
//---------------------------------------------------------------------------

void __fastcall TATManage::LoadRecalcImagesClick(TObject *Sender)
{
	//load previously acquired images
	__int32 ii, jj, z, z2, k, r, x, y,
			radr,//raw images address
			imCount[2];//number of images requested [requested, actual]
	AnsiString flNm,//buffer string with filename
			   bufStr;//buffer string
	UnicodeString baseDir,//selected directory
				  bfrS;//buffer string
	bool uf,//unique found
		 anySelect;//files of directory was selected
	float c;//color for OpenGL

	if (LoadRecalcImages->Tag == 0) //recalculation mode requested
	{
		LoadRecalcImages->Tag = 1;//change button mode
		LoadRecalcImages->Caption = "Record";//change button mode
		LoadRecalcImages->Hint = "press to return to acquisition mode";//change hint
		GraphImag->BrightPorog->Position = 1;//set minimal value
		UserSelDir->Visible = false;//switch off dir-select button
		RoiGraph->tagsNum = 0;//virtually delete all previous tags

		anySelect = false;
		if (UserSelDir->Tag == 1) //read all files in a directory
		{
			baseDir = "C:\\";//start directory
			anySelect = SelectDirectory(baseDir, TSelectDirOpts() << sdPrompt, 0);//directory selection
			SelFileList->Directory = baseDir;//directory with experiment data
			SelFileList->Update();//accept new directory for FileList
			imCount[0] = SelFileList->Count;//number of images requested
		}
		else //read some files
		{
			anySelect = LoadImagesDlg->Execute();//file selection
			imCount[0] = LoadImagesDlg->Files->Count;//number of images requested
		}
		anySelect = (imCount[0] > 0);//some files was selected

		if (anySelect) //open images for recalculation
		{
			//creat list of actual files
			if (selFiles) //list exist
			{
				delete[] selFiles; selFiles = NULL;//
			}
			selFiles = new UnicodeString[imCount[0]];//creat list of tiff-files
			z = 0;//tiff files counter
			for (ii = 0; ii < imCount[0]; ii++) //run over files
			{
				if (UserSelDir->Tag == 1) //read all files in a directory
					bfrS = SelFileList->Directory + "\\" + SelFileList->Items->operator [](ii);//condidate file
				else //user's selection
					bfrS = LoadImagesDlg->Files->operator [](ii);//condidate file

				bufStr = bfrS.SubString(bfrS.Length() - 6, 7);//extention
				if (bufStr.Pos(".tif") > 1) //tiff only
				{
					selFiles[z] = bfrS;//tiff-file (full pathname)
					z++;//tiff-files counter
				}
			}
			imCount[0] = z;//actual files number (number of images requested)
		} //end of (if (anySelect) //open images for recalculation)

		SelFileList->Tag = imCount[0];//number of images requested
		LoadImagesDlg->Tag = 0;//no good images was detected
		if (anySelect) //open images for recalculation
		{
			if (readLEDn[0]) //array exist
			{
				delete[] readLEDn[0]; readLEDn[0] = NULL;//delete array for original numbers of the LED
				delete[] readLEDn[1]; readLEDn[1] = NULL;//delete array for numbers of corresponding image
				delete[] readLEDn[2]; readLEDn[2] = NULL;//delete array for numbers of filename
			}
			if (ledCamThread->imTStamps) //array exist
			{
				delete[] ledCamThread->imTStamps; ledCamThread->imTStamps = NULL;//delete array
			}

			readLEDn[0] = new __int32[imCount[0]];//original numbers of the LED
			readLEDn[1] = new __int32[imCount[0]];//numbers of corresponding image
			readLEDn[2] = new __int32[imCount[0]];//numbers of filename
			ledCamThread->imTStamps = new float[imCount[0]];//timestamps of acquired images (seconds from acquisition start)

			imCount[1] = 0;//number of actual (good) images
			unqLedN[0] = 0;//number of unique LEDs
			for (ii = 0; ii < imCount[0]; ii++) //run over files
			{
				flNm = selFiles[ii];//current file
				z = flNm.Pos("_LED");//input position
				if ((flNm.Pos("TA_") > 1) && (z > 1)) //good file
				{
					readLEDn[2][imCount[1]] = ii;//number of filename
					bufStr = flNm.SubString(z + 4, 1);//LED number as string
					readLEDn[0][imCount[1]] = StrToInt(bufStr);//LED number
					z2 = flNm.Pos(".tif");//input position
					bufStr = flNm.SubString(z + 6, z2 - (z + 6));//timestamp as string
					ledCamThread->imTStamps[imCount[1]] = StrToFloat(bufStr);//timestamp as number
					if (unqLedN[0] == 0) //first number
					{
						unqLedN[1] = readLEDn[0][imCount[1]];//unique LED numbers
						unqLedN[0] = 1;//number of unique LEDs
					}
					else
					{
						uf = true;//unique found
						for (jj = 0; jj < unqLedN[0]; jj++) //run over unique leds
						{
							if (unqLedN[jj + 1] == readLEDn[0][imCount[1]]) //non unique led
								uf = false;//unique found
						}
						if (uf) //unique found
						{
							unqLedN[unqLedN[0] + 1] = readLEDn[0][imCount[1]];//unique LED numbers
							unqLedN[0]++;//number of unique LEDs
						}
					}
					imCount[1]++;//number of actual (good) images
				} //end of (if ((flNm.Pos("TA_") > 1) && (z > 1)) //good file)
			} //end of (for (ii = 0; ii < imCount[0]; ii++) //run over files)

			//sort by timestamps
			for (z = 0; z < imCount[0]; z++) //run over all files
				readLEDn[1][z] = -1;//initially unenumerated
			for (ii = 0; ii < imCount[1]; ii++) //run over actual images
			{
				for (r = 0; r < imCount[1]; r++) //run over actual (good) images
				{
					if (readLEDn[1][r] < 0) //is unenumerated timestamp
					{
						z = r;//first unenumerated timestamp
						break;
					}
				}
				for (r = 0; r < imCount[1]; r++) //run over timestamps
				{
					if ((readLEDn[1][r] < 0) && (ledCamThread->imTStamps[r] < ledCamThread->imTStamps[z])) //looking for minimal
						z = r;//number of minimal among unenumerated timestampes
				}
				readLEDn[1][z] = ii;//number of the timestampt
			}

			//sort by LED number (make ascending order)
			for (z = 0; z < (unqLedN[0] - 1); z++) // run over used LEDs
			{
				for (k = (z + 1); k < unqLedN[0]; k++) // run over used LEDs
					if (unqLedN[z + 1] > unqLedN[k + 1]) //wrong order
					{
						ii = unqLedN[z + 1];
						unqLedN[z + 1] = unqLedN[k + 1];
						unqLedN[k + 1] = ii;
					}
			}

			if (unqLedN[0] > 0) //good images was detected
			{
				GraphImag->DeleteAllROIClick(this);//delete all previous ROI

				tiffImg = NULL;//file handle
				ii = 0;//file index
				while ((tiffImg == NULL) && (ii < imCount[0])) //find first "right" tiff-file
				{
					flNm = selFiles[ii];//current file
					z = flNm.Pos("_LED");//input position
					if ((flNm.Pos("TA_") > 1) && (z > 1)) //good file
						tiffImg = TIFFOpen(flNm.c_str(), "r");//read an image
					ii++;//next file
				}
				if (tiffImg != NULL) //success load
				{
					TIFFGetField(tiffImg, TIFFTAG_IMAGEWIDTH, &bHorzRes);//horizontal size of window
					TIFFGetField(tiffImg, TIFFTAG_IMAGELENGTH, &bVertRes);//verticatl size of window
					TIFFGetField(tiffImg, TIFFTAG_BITSPERSAMPLE, &clrDepth);//ADC bit depth
					TIFFClose(tiffImg);//close the file

					saturatLevel = 1;//saturation level
					for(ii = 0; ii < clrDepth; ii++) //run over digits
						saturatLevel *= 2;//calculate saturation level
				}
				else //no file was loaded
				{
					ATManage->ProgList->Lines->Add("!!! images read error !!!");//print error message
					return;//out of LoadRecalcImages
				}
				acqParams.getImgNum = unqLedN[0];//number of unique LEDs

				for (ii = 0; ii < unqLedN[0]; ii++) //run over files
					ledCamThread->ledOrder[ii] = unqLedN[ii + 1] - 1;//number of LED to be switched-on in jj order
				PreparImages();//format window and controls

				LoadImagesDlg->Tag = 1;//good images was detected
				radr = 0;//raw images address
				for (jj = 0; jj < unqLedN[0]; jj++) //run over unique LED images
				{
					for (ii = 0; ii < imCount[0]; ii++) //run over all files
					{
						flNm = selFiles[ii];//selected file
						z = flNm.Pos("_LED" + IntToStr(unqLedN[jj + 1]));//input position
						if ((flNm.Pos("TA_") > 1) && (z > 1)) //wanted LED was detected
						{
							tiffImg = TIFFOpen(flNm.c_str(), "r");//read next image
							for (y = 0; y < bVertRes; y++) //run over rows
							{
								TIFFReadScanline(tiffImg, &rawImgs[(jj * bVertRes * bHorzRes) + y * bHorzRes], y, 0);//read a line from current imadge
								for (x = 0; x < bHorzRes; x++) //run over colums
								{
									rawImgsSat[radr] = float(rawImgs[radr]) / saturatLevel;//array for image related to saturation level
									radr++;//next pixel
								}
							}
							TIFFClose(tiffImg);//close the file
							break;//out of (for ii)
						}
					} // end of (for (ii = 0; ii < imCount[0]; ii++) //run over all files)
				} //end of (for (jj = 0; jj < unqLedN[0]; jj++) //run over unique LED images)
				GraphImag->ReDrawImagesROIs(this);//redraw images and ROIs

				//set name for data-file
				z = flNm.Pos("_LED") - 1;//input position
				baseName = flNm.SetLength(z);//path with date and time string only

			} //end of (if (unqLedN[0] > 0) //good images was detected)
		} //end of (if (anySelect) //open images for recalculation)
	} // end of (if (LoadRecalcImages->Tag == 0) //recalculation mode requested)
	else //(LoadRecalcImages->Tag == 1) //acquisition mode requested
	{
		LoadRecalcImages->Tag = 0;//change button mode
		LoadRecalcImages->Caption = "Files reCalc";//change button mode
		LoadRecalcImages->Hint = "press to load images and recalculate";//change hint
		UserSelDir->Visible = true;//switch on dir-select button
	}
	UserSelDir->Tag = 0;//flag of directory selection
}
//---------------------------------------------------------------------------

void TATManage::PreparLines()
{
	//prepare LineSeries for new data acquisition
	__int32 ii, jj;

	for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED-illuminator channels
		for (jj = 0; jj < roiMaxN; jj++) //run over ROIs
		{
			brtnessImROItime[ii][jj]->Clear();//delete previous data
			brtnessImROItime[ii][jj]->Visible = false;//invisible line
			brtnessImROIframe[ii][jj]->Clear();//delete previous data
			brtnessImROIframe[ii][jj]->Visible = false;//invisible line
		}
	for (ii = 0; ii < 2; ii++) //run over LED-illuminator channels
		for (jj = 0; jj < roiMaxN; jj++) //run over ROIs
		{
			ratioImROItime[ii][jj]->Clear();//delete previous data
			ratioImROItime[ii][jj]->Visible = false;//invisible line
			ratioImROIframe[ii][jj]->Clear();//delete previous data
			ratioImROIframe[ii][jj]->Visible = false;//invisible line
		}
	for (ii = 0; ii < NUM_CHANNELS; ii++) //run over LED-illuminator channels
	{
		if (ledCheck[ii]->Checked) //LED #ii is in use and raw data requested
		{
			for (jj = 0; jj < roiMaxN; jj++) //run over possible ROIs
			{
				if ((plotLEDflg[ii]->Checked) && (jj < GraphImag->actNumROI) && (RoiGraph->RawData->Checked)) //plot the ROI of the LED
				{
					if (RoiGraph->TimeScl->Checked) //time-scale
						brtnessImROItime[ii][jj]->Visible = true;//visible line
					else
						brtnessImROIframe[ii][jj]->Visible = true;//visible line
				}
			}
		}
	}
	for (ii = 0; ii < 2; ii++) //run over ratios
	{
		for (jj = 0; jj < roiMaxN; jj++) //run over possible ROIs
		{
			if ((jj < GraphImag->actNumROI) && (RoiGraph->Ratios->Checked)) //plot the ratio of the ROI
			{
				if (RoiGraph->TimeScl->Checked) //time-scale
					ratioImROItime[ii][jj]->Visible = true;//visible line
				else
					ratioImROIframe[ii][jj]->Visible = true;//visible line
			}
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TATManage::UserSelDirClick(TObject *Sender)
{
	//Select directory
	UserSelDir->Tag = 1;//flag of directory selection
	LoadRecalcImagesClick(this);//read files
}
//---------------------------------------------------------------------------

void __fastcall TATManage::FramePrdChange(TObject *Sender)
{
	ledCamThread->framPrd = StrToInt(FramePrd->Text);//period between 3-color frames (ms)
}
//---------------------------------------------------------------------------

void __fastcall TATManage::PlotLEDClick(TObject *Sender)
{
	//change lines visibility
	RoiGraph->ResetVisibility();//update lines visibility
}
//---------------------------------------------------------------------------

