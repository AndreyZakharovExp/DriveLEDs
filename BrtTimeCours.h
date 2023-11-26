//---------------------------------------------------------------------------

#ifndef BrtTimeCoursH
#define BrtTimeCoursH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <VCLTee.Chart.hpp>
#include <VCLTee.Series.hpp>
#include <VCLTee.TeEngine.hpp>
#include <VCLTee.TeeProcs.hpp>
#include <Vcl.Menus.hpp>
#include <Bde.DBTables.hpp>
#include <Data.DB.hpp>
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
struct markers //structure for user's tags
{
	UnicodeString textMark;//text of tag
	__int32 frmInd;//number frame (count)
	float timeStmp;//timestampe (s)
};

//---------------------------------------------------------------------------
class TRoiGraph : public TForm
{
__published:	// IDE-managed Components
	TChart *LedRoi;
	TMainMenu *ImagesMenu;
	TMenuItem *XScale;
	TMenuItem *TimeScl;
	TMenuItem *FrameScl;
	TMenuItem *Data1;
	TMenuItem *RawData;
	TMenuItem *Ratios;
	TLabel *Label1;
	TEdit *TagText;
	void __fastcall TmFrmRwRtClick(TObject *Sender);
	void __fastcall TagTextKeyPress(TObject *Sender, System::WideChar &Key);
	void __fastcall LedRoiAfterDraw(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TRoiGraph(TComponent* Owner);
	void ResetVisibility();
	markers pntTags[200];//array with tages
	__int32 tagsNum;//number of tags entered
};
//---------------------------------------------------------------------------
extern PACKAGE TRoiGraph *RoiGraph;
//---------------------------------------------------------------------------
#endif
