//---------------------------------------------------------------------------

#ifndef GraphsH
#define GraphsH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <VCLTee.TeeFilters.hpp>
#include <Vcl.Menus.hpp>
#include <Vcl.Dialogs.hpp>
#include <Vcl.ComCtrls.hpp>

//---------------------------------------------------------------------------
#include <gl\gl.h>
#include <gl\glu.h>
#include <gl\glaux.h>
//---------------------------------------------------------------------------
#include <windows.h>

//---------------------------------------------------------------------------
class TGraphImag : public TForm
{
__published:	// IDE-managed Components
	TImage *LED1Img;
	TImage *LED2Img;
	TImage *LED3Img;
	TImage *LED4Img;
	TPopupMenu *MImagMenu;
	TMenuItem *SaveClk;
	TMenuItem *DeleteROI;
	TMenuItem *DeleteAllROI;
	TLabel *LEDord1LblG;
	TLabel *LEDord2LblG;
	TLabel *LEDord3LblG;
	TLabel *LEDord4LblG;
	TScrollBar *BrightPorog;
	TPanel *LED1Panel;
	TPanel *LED2Panel;
	TPanel *LED3Panel;
	TPanel *LED4Panel;
	void __fastcall LEDImgMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall LEDImgMouseUp(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall SaveClkClick(TObject *Sender);
	void __fastcall DeleteROIClick(TObject *Sender);
	void __fastcall DeleteAllROIClick(TObject *Sender);
	void __fastcall LEDImgMouseMove(TObject *Sender, TShiftState Shift, int X, int Y);
	void __fastcall BrightPorogChange(TObject *Sender);
	void __fastcall FormResize(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall ReDrawImagesROIs(TObject *Sender);
	void __fastcall FormActivate(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TGraphImag(TComponent* Owner);
	bool PointInPolygon(__int32 ix, __int32 iy, __int32 cfROI);
	//point within contour
	bool freeROIflag,//true if user draw free contour
		 rectROIflag;//true if user draw rectangular contour
	__int32 actNumROI,//acutal number of polygon created ROI
			*roiMask[roiMaxN],//pointers to ROI-masks (array of indexes)
			roiToDel,//number of existing ROI to be delete
			freeROI[roiMaxN][2][1000];//freeform ROIs
	TRect rectROI;//rectangular ROIs
	HGLRC hRC[NUM_CHANNELS];//постоянный контекст рендеринга
	HDC hndlDC[NUM_CHANNELS];//приватный контекст устройства GDI
//	unsigned short *copyRawImgs;//[NUM_CHANNELS];//copy of source images shown at the moment (taken from camera or loaded from storage)
	float reSizeCoeff;//resize coefficient (CCD Horizont size / actual image size)
};
//---------------------------------------------------------------------------
extern PACKAGE TGraphImag *GraphImag;
//---------------------------------------------------------------------------
#endif
