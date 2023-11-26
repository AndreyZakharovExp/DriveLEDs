//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "ThorAndor.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TRoiGraph *RoiGraph;
//---------------------------------------------------------------------------
__fastcall TRoiGraph::TRoiGraph(TComponent* Owner)
	: TForm(Owner)
{
	__int32 ii, jj;

	LedRoi->RemoveAllSeries();//remover all series
	for (ii = 0; ii < NUM_CHANNELS; ii++) //rum over image-objects
	{
		for (jj = 0; jj < roiMaxN; jj++) //run over ROIs
		{
			LedRoi->AddSeries(ATManage->brtnessImROItime[ii][jj]);//add the series to chart
			LedRoi->AddSeries(ATManage->brtnessImROIframe[ii][jj]);//add the series to chart
		}
	}
	for (ii = 0; ii < 2; ii++) //rum over image-objects
	{
		for (jj = 0; jj < roiMaxN; jj++) //run over ROIs
		{
			LedRoi->AddSeries(ATManage->ratioImROItime[ii][jj]);//add the series to chart
			LedRoi->AddSeries(ATManage->ratioImROIframe[ii][jj]);//add the series to chart
		}
	}
	RoiGraph->tagsNum = 0;//virtually delete all previous tags
}
//---------------------------------------------------------------------------

void __fastcall TRoiGraph::TmFrmRwRtClick(TObject *Sender)
{
	//change x-scale units (time or frame numbers)
	TMenuItem *copyH;

	copyH = (TMenuItem*)Sender;
	copyH->Checked = !(copyH->Checked);//invert property
	ResetVisibility();//update lines visibility
}
//---------------------------------------------------------------------------

void TRoiGraph::ResetVisibility()
{
	__int32 ii, jj;
	if (RawData->Checked) //raw data to be plotted
	{
		for (ii = 0; ii < NUM_CHANNELS; ii++) //rum over image-objects
		{
			for (jj = 0; jj < GraphImag->actNumROI; jj++) //run over ROIs
			{
				if (ATManage->plotLEDflg[ii]->Checked) //show the LED
				{
					if (TimeScl->Checked) //time-scale
					{
						ATManage->brtnessImROItime[ii][jj]->Visible = true;//show as time
						ATManage->brtnessImROIframe[ii][jj]->Visible = false;//hide frame numbers
					}
					else //frames-scale
					{
						ATManage->brtnessImROItime[ii][jj]->Visible = false;//hide time
						ATManage->brtnessImROIframe[ii][jj]->Visible = true;//show as frames
					}
				}
				else //hide the LED
				{
					ATManage->brtnessImROItime[ii][jj]->Visible = false;//hide time
					ATManage->brtnessImROIframe[ii][jj]->Visible = false;//hide frame numbers
				}
			}
		}
		for (ii = 0; ii < 2; ii++) //rum over ratios
		{
			for (jj = 0; jj < GraphImag->actNumROI; jj++) //run over ROIs
			{
				ATManage->ratioImROItime[ii][jj]->Visible = false;//hide time
				ATManage->ratioImROIframe[ii][jj]->Visible = false;//hide frame numbers
			}
		}
		LedRoi->Title->Caption = "Brightness, percent";//raw data (brightness)
	}
	else //ratios to be plotted
	{
		for (ii = 0; ii < 2; ii++) //rum over ratios
		{
			for (jj = 0; jj < GraphImag->actNumROI; jj++) //run over ROIs
			{
				if (TimeScl->Checked) //time-scale
				{
					ATManage->ratioImROItime[ii][jj]->Visible = true;//show as time
					ATManage->ratioImROIframe[ii][jj]->Visible = false;//hide frame numbers
				}
				else //frames-scale
				{
					ATManage->ratioImROItime[ii][jj]->Visible = false;//hide time
					ATManage->ratioImROIframe[ii][jj]->Visible = true;//show as frames
				}
			}
		}
		for (ii = 0; ii < NUM_CHANNELS; ii++) //rum over image-objects
		{
			for (jj = 0; jj < GraphImag->actNumROI; jj++) //run over ROIs
			{
				ATManage->brtnessImROItime[ii][jj]->Visible = false;//hide time
				ATManage->brtnessImROIframe[ii][jj]->Visible = false;//hide frame numbers
			}
		}
		LedRoi->Title->Caption = "Ratios";//ratios
	}
	if (TimeScl->Checked) //time-scale
	{
		LedRoi->BottomAxis->Title->Caption = "Time, s";//time unites
	}
	else //frames-scale
	{
		LedRoi->BottomAxis->Title->Caption = "Frame, #";//frame unites
	}
}
//---------------------------------------------------------------------------

void __fastcall TRoiGraph::TagTextKeyPress(TObject *Sender, System::WideChar &Key)
{
	//show tag on graphs
	__int32 ii, jj, k,
			frmN;//frame index (count)
	float timeStmp;//last timestampe

	if ((Key == '\r') && (TagText->Text.Length() > 0)) //нажали enter - добавляем метку на график с главными амплитудами (пока только туда)
	{
		frmN = -1;//frame index (count)
		timeStmp = -1;//last timestampe
		for (ii = 0; ii < NUM_CHANNELS; ii++) //rum over image-objects
		{
			for (jj = 0; jj < roiMaxN; jj++) //run over ROIs
			{
				//time-scale
				k = ATManage->brtnessImROItime[ii][jj]->Count();//last point number
				if (k > 0) //data exist
					if (ATManage->brtnessImROItime[ii][jj]->XValues->operator [](k - 1) > timeStmp)
						timeStmp = ATManage->brtnessImROItime[ii][jj]->XValues->operator [](k - 1);//last timestampe

				//frames-scale
				k = ATManage->brtnessImROIframe[ii][jj]->Count();//last point number
				if (k > 0) //data exist
					if (ATManage->brtnessImROIframe[ii][jj]->XValues->operator [](k - 1) > frmN)
						frmN = ATManage->brtnessImROIframe[ii][jj]->XValues->operator [](k - 1);//number of point on the graph
			}
		}

		if ((frmN >= 0) && (tagsNum < 200) && (frmN > -1) && (timeStmp > -1)) //point on graph exist
		{
			pntTags[tagsNum].textMark = TagText->Text;//text of tag
			pntTags[tagsNum].frmInd = frmN;//number of point on the graph
			pntTags[tagsNum].timeStmp = timeStmp;//timestampe of the point
			TagText->Text = "";//clear field
			tagsNum++;//add new tag
			LedRoiAfterDraw(this);//draw all tags again
		}
	}
}
//---------------------------------------------------------------------------


void __fastcall TRoiGraph::LedRoiAfterDraw(TObject *Sender)
{
	//Redraw all entered tags
	__int32 ii, jj, k, z,
			x0, x1,//х - координата для надписи
			y0, y1,//y - координата для надписи
			fH,//высота надписи
			fW,//ширина надписи
			pnG;//number of point on the graph

	for (k = 0; k < tagsNum; k++) //run over tags
	{
		for (ii = 0; ii < NUM_CHANNELS; ii++) //rum over image-objects
		{
			for (jj = 0; jj < roiMaxN; jj++) //run over ROIs
			{
				//time-scale, brightness
				z = ATManage->brtnessImROItime[ii][jj]->Count();//last point on a graph
				if ((ATManage->brtnessImROItime[ii][jj]->Visible) && //visible line
					(z > 0) && (pntTags[k].frmInd < z)) //data exist
				{
					x0 = ATManage->brtnessImROItime[ii][jj]->CalcXPosValue(pntTags[k].timeStmp);//x0 координата надписи
					y0 = ATManage->brtnessImROItime[ii][jj]->CalcYPos(pntTags[k].frmInd);//y0 координата надписи

					fH = LedRoi->Canvas->TextHeight(pntTags[k].textMark);//высота надписи
					fW = __int32((float)LedRoi->Canvas->TextWidth(pntTags[k].textMark) * 1.15);//ширина надписи
					x1 = x0 - floor(fW / 2);//x1 координата надписи
					y1 = y0 - (2 * fH);//y1 координата надписи

					LedRoi->Canvas->Rectangle(x1, y1, x1 + fW, y1 + fH);//рамка для надписи
					LedRoi->Canvas->TextOutW(x1 + 2, y1, pntTags[k].textMark);//сама надпись
					LedRoi->Canvas->Line(x0, y0 - fH, x0, y0);//стрелка (указывает на точку, к которой привязана надпись)
				}

				//frames-scale, brightness
				z = ATManage->brtnessImROIframe[ii][jj]->Count();//last point on a graph
				if ((ATManage->brtnessImROIframe[ii][jj]->Visible) && //visible line
					(z > 0)) //data exist
				{
					x0 = ATManage->brtnessImROItime[ii][jj]->CalcXPosValue(pntTags[k].frmInd);//x0 координата надписи
					y0 = ATManage->brtnessImROIframe[ii][jj]->CalcYPos(pntTags[k].frmInd);//y0 координата надписи

					fH = LedRoi->Canvas->TextHeight(pntTags[k].textMark);//высота надписи
					fW = __int32((float)LedRoi->Canvas->TextWidth(pntTags[k].textMark) * 1.15);//ширина надписи
					x1 = x0 - floor(fW / 2);//x1 координата надписи
					y1 = y0 - (2 * fH);//y1 координата надписи

					LedRoi->Canvas->Rectangle(x1, y1, x1 + fW, y1 + fH);//рамка для надписи
					LedRoi->Canvas->TextOutW(x1 + 2, y1, pntTags[k].textMark);//сама надпись
					LedRoi->Canvas->Line(x0, y0 - fH, x0, y0);//стрелка (указывает на точку, к которой привязана надпись)
				}

				//time-scale, ratios
				if (ii < 2) //two ratios only
				{
					z = ATManage->ratioImROItime[ii][jj]->Count();//last point on a graph
					if ((ATManage->ratioImROItime[ii][jj]->Visible) && //visible line
						(z > 0)) //data exist
					{
						x0 = ATManage->ratioImROItime[ii][jj]->CalcXPosValue(pntTags[k].timeStmp);//x0 координата надписи
						y0 = ATManage->ratioImROItime[ii][jj]->CalcYPos(pntTags[k].frmInd);//y0 координата надписи

						fH = LedRoi->Canvas->TextHeight(pntTags[k].textMark);//высота надписи
						fW = __int32((float)LedRoi->Canvas->TextWidth(pntTags[k].textMark) * 1.15);//ширина надписи
						x1 = x0 - floor(fW / 2);//x1 координата надписи
						y1 = y0 - (2 * fH);//y1 координата надписи

						LedRoi->Canvas->Rectangle(x1, y1, x1 + fW, y1 + fH);//рамка для надписи
						LedRoi->Canvas->TextOutW(x1 + 2, y1, pntTags[k].textMark);//сама надпись
						LedRoi->Canvas->Line(x0, y0 - fH, x0, y0);//стрелка (указывает на точку, к которой привязана надпись)
					}

					//frames-scale, ratios
					z = ATManage->ratioImROIframe[ii][jj]->Count();//last point on a graph
					if ((ATManage->ratioImROIframe[ii][jj]->Visible) && //visible line
						(z > 0)) //data exist
					{
						x0 = ATManage->ratioImROIframe[ii][jj]->CalcXPosValue(pntTags[k].frmInd);//x0 координата надписи
						y0 = ATManage->ratioImROIframe[ii][jj]->CalcYPos(z - 1);//y0 координата надписи

						fH = LedRoi->Canvas->TextHeight(pntTags[k].textMark);//высота надписи
						fW = __int32((float)LedRoi->Canvas->TextWidth(pntTags[k].textMark) * 1.15);//ширина надписи
						x1 = x0 - floor(fW / 2);//x1 координата надписи
						y1 = y0 - (2 * fH);//y1 координата надписи

						LedRoi->Canvas->Rectangle(x1, y1, x1 + fW, y1 + fH);//рамка для надписи
						LedRoi->Canvas->TextOutW(x1 + 2, y1, pntTags[k].textMark);//сама надпись
						LedRoi->Canvas->Line(x0, y0 - fH, x0, y0);//стрелка (указывает на точку, к которой привязана надпись)
					}
				} //end of (if (ii < 2) //two ratios only)
			} //end of (for (jj = 0; jj < roiMaxN; jj++) //run over ROIs)
		} //end of (for (ii = 0; ii < NUM_CHANNELS; ii++) //rum over image-objects)
	} //end of (for (k = 0; k < tagsNum; k++) //run over tags)
}
//---------------------------------------------------------------------------

