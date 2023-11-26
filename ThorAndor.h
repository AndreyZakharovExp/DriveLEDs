//---------------------------------------------------------------------------

#ifndef ThorAndorH
#define ThorAndorH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.Dialogs.hpp>
#include <Vcl.Buttons.hpp>
#include <Vcl.FileCtrl.hpp>
#include <tiffio.h>
#include "ATMCD32D.H"
#include "visa.h" //we need visa (typically found in your VXIPNP\include directory)
#include "TLDC4100.h" //the device driver header

#define roiMaxN 5 //maximal ROIs number
#include "LedCamControl.h"
#include "Graphs.h"
#include "BrtTimeCours.h"

//acquisition parameters struct
struct AcqParams //acquisition parameters structure
{
	int   readMode;//read mode ("image", "single-track", "multi-track" or other)
	int   acquisitMode;//acquisition mode ("single scan", "kinetics, "run till abort", "accumulate" or other)
	float exposTime;//exposition time (seconds)
	int	  trigMode;//trigger mode
	int   getImgNum;//number of images to be scanned befor read out
	float kineticCycTm;//kinetic cycle time (seconds)
};

//---------------------------------------------------------------------------
class TATManage : public TForm
{
__published:	// IDE-managed Components
	TMemo *ProgList;
	TButton *LoadSettings;
	TButton *StartProg;
	TOpenDialog *LoadSettDlg;
	TBitBtn *Snap;
	TEdit *Reps;
	TLabel *Label1;
	TLabel *Label2;
	TEdit *FramePrd;
	TLabel *Label5;
	TLabel *Label6;
	TLabel *Label7;
	TButton *Live;
	TGroupBox *LEDsConfig;
	TGroupBox *CamConfig;
	TGroupBox *AcqPrm;
	TLabel *Label10;
	TEdit *BinSize;
	TLabel *Label11;
	TCheckBox *UseLED1;
	TEdit *LED1sn;
	TLabel *Label12;
	TEdit *LED1bright;
	TLabel *Label13;
	TLabel *Label14;
	TEdit *LED2bright;
	TEdit *LED2sn;
	TCheckBox *UseLED2;
	TEdit *LED3bright;
	TEdit *LED3sn;
	TCheckBox *UseLED3;
	TEdit *LED4bright;
	TEdit *LED4sn;
	TCheckBox *UseLED4;
	TCheckBox *ManualSwitch;
	TCheckBox *SavFlag;
	TLabel *Label15;
	TLabel *Label16;
	TCheckBox *PlotLED1;
	TCheckBox *PlotLED2;
	TCheckBox *PlotLED3;
	TCheckBox *PlotLED4;
	TButton *LoadRecalcImages;
	TOpenDialog *LoadImagesDlg;
	TLabel *Label17;
	TLabel *Label18;
	TLabel *Label19;
	TLabel *Label20;
	TEdit *LED1OnDur;
	TEdit *LED2OnDur;
	TEdit *LED3OnDur;
	TEdit *LED4OnDur;
	TEdit *ExposToLED1;
	TEdit *ExposToLED2;
	TEdit *ExposToLED3;
	TEdit *ExposToLED4;
	TButton *SaveSettings;
	TSaveDialog *SaveSettDlg;
	TButton *UserSelDir;
	TFileListBox *SelFileList;
	TLabel *Label4;
	TLabel *Label8;
	TEdit *LEdelay;
	void __fastcall LoadSettingsClick(TObject *Sender);
	void __fastcall StartProgClick(TObject *Sender);
	void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
	void __fastcall SnapClick(TObject *Sender);
	void __fastcall LiveClick(TObject *Sender);
	void __fastcall IntPrmKeyPress(TObject *Sender, System::WideChar &Key);
	void __fastcall FloatPrmKeyPress(TObject *Sender, System::WideChar &Key);
	void __fastcall LEDsnChange(TObject *Sender);
	void __fastcall ManualSwitchClick(TObject *Sender);
	void __fastcall BinSizeChange(TObject *Sender);
	void __fastcall LEDonDurChange(TObject *Sender);
	void __fastcall ExposTimeChange(TObject *Sender);
	void __fastcall LoadRecalcImagesClick(TObject *Sender);
	void __fastcall SaveSettingsClick(TObject *Sender);
	void __fastcall UserSelDirClick(TObject *Sender);
	void __fastcall FramePrdChange(TObject *Sender);
	void __fastcall PlotLEDClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TATManage(TComponent* Owner);
	void PreparCamera();
	void PreparImages();
	void DisplayImage();
	void SaveDigBrtData();
	void PreparLines();
	long cam_handle;//handle of the camera
	unsigned short *rawImgs;//array for image
	__int32 imGlbNum,//global number of saved image for current session
			*readLEDn[3],//all LED numbers was read [0 - original numbers of the LED, 1 - numbers of corresponding image, 2 - numbers of filename]
			adcDepth,//ADC bit depth (camera parameter)
			clrDepth,//color bit depth of image to be loaded
			unqLedN[NUM_CHANNELS + 1];//unique LED numbers, [0 - number of unique LEDs, 1-4 - number of LEDs]
	UnicodeString baseName,//name of file based on date
				  *selFiles,//selected tiff-files
				  rawText;//suffix (indicat about save after acquisition)
	unsigned long imSize;//image size (pixels)
	int adcChanNum,//number of ADC channels
		hRes,//full horizontal resolution of CCD detector
		vRes,//full vertical resolution of CCD detector
		bHorzRes,//binned horizontal resolution
		bVertRes;//binned vertical resolution
	AcqParams acqParams;//acquisition parameters
	float saturatLevel,//saturation level
		  *rawImgsSat,//array for image related to saturation level
		  brtMag;//brightness amplification
	DeviceControl *ledCamThread;//handle of thread controlling LEDs and camera
	TCheckBox *ledCheck[NUM_CHANNELS],//array of pointers to use-LED checkboxes
			  *plotLEDflg[NUM_CHANNELS];//array of pointers to plot-LED checkboxes
	TEdit *ledSN[NUM_CHANNELS],//array of pointers to LED-order edits
		  *vLEDbright[NUM_CHANNELS],//array of pointers to LED-voltage edits
		  *durLEDon[NUM_CHANNELS],//array of pointers to LED-On duration edit
		  *camExposToLED[NUM_CHANNELS];//array of pointers to camera exposition time to LED edit
	TImage *imagObj[NUM_CHANNELS];//array of pointers to imabes-objects on GraphImag form
	TPanel *imagPnlObj[NUM_CHANNELS];//array of pointers to imabes-panel-objects on GraphImag form
	TLineSeries *brtnessImROItime[NUM_CHANNELS][roiMaxN];//pointer to lines of brightness in ROI for each image (time-scale)
	TLineSeries *brtnessImROIframe[NUM_CHANNELS][roiMaxN];//pointer to lines of brightness in ROI for each image (frame-scale)
	TLineSeries *ratioImROItime[2][roiMaxN];//pointer to lines of ratio in ROI for each image (time-scale) 590(LED1)/455(LED4) - Cl; 505(LED3)/455(LED4) - pH
	TLineSeries *ratioImROIframe[2][roiMaxN];//pointer to lines of ratio in ROI for each image (frame-scale) 590(LED1)/455(LED4) - Cl; 505(LED3)/455(LED4) - pH
	TLabel *ledOrdLblG[NUM_CHANNELS];//copy of label objects
	TColor linClrs[10];//constant colors for lines
	TDateTime CurrentDateTime;//current date and time
};
//---------------------------------------------------------------------------
extern PACKAGE TATManage *ATManage;
//---------------------------------------------------------------------------
#endif
