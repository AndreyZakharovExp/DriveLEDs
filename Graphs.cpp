#include <vcl.h>
#include <math.h>
#include <algorith.h>
#pragma hdrstop

#include "ThorAndor.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TGraphImag *GraphImag;

//добавляем переменные для настройки видео параметров OpenGL
HWND hWnd = NULL;//здесь будет хранится дескриптор окна (ссылка на окно рендеринга)
HINSTANCE hInstance;//здесь будет хранится дескриптор приложения (ссылка на наше приложение)
GLuint PixelFormat;//хранит результат после поиска

static  PIXELFORMATDESCRIPTOR pfd =
{
	sizeof(PIXELFORMATDESCRIPTOR), //зазмер дескриптора данного формата пикселей
	1,                             //номер версии
	PFD_DRAW_TO_WINDOW |           //формат для окна
	PFD_SUPPORT_OPENGL,// |           //формат для OpenGL
	//PFD_DOUBLEBUFFER,              //формат для двойного буфера
	PFD_TYPE_RGBA,                 //требуется RGBA формат //PFD_TYPE_COLORINDEX,//
	24,                            //выбирается бит глубины цвета
	0, 0, 0, 0, 0, 0,              //игнорирование цветовых битов
	0,                             //нет буфера прозрачности
	0,                             //сдвиговый бит игнорируется
	0,                             //нет буфера накопления
	0, 0, 0, 0,                    //биты накопления игнорируются
	32,                            //32 битный Z-буфер (буфер глубины)
	0,                             //нет буфера трафарета
	0,                             //нет вспомогательных буферов
	PFD_MAIN_PLANE,                //главный слой рисования
	0,                             //зарезервировано
	0, 0, 0                        //маски слоя игнорируются
};

//---------------------------------------------------------------------------
__fastcall TGraphImag::TGraphImag(TComponent* Owner)
	: TForm(Owner)
{
	__int32 ii;

	GraphImag->DoubleBuffered = true;
	actNumROI = 0;//acutal number of polygon created ROI

	//make copy of controls after form created
	ATManage->imagObj[0] = LED1Img;//pointer to LED1 image
	ATManage->imagObj[1] = LED2Img;//pointer to LED2 image
	ATManage->imagObj[2] = LED3Img;//pointer to LED3 image
	ATManage->imagObj[3] = LED4Img;//pointer to LED4 image
	ATManage->imagPnlObj[0] = LED1Panel;//pointer to LED1 image-panel
	ATManage->imagPnlObj[1] = LED2Panel;//pointer to LED2 image-panel
	ATManage->imagPnlObj[2] = LED3Panel;//pointer to LED3 image-panel
	ATManage->imagPnlObj[3] = LED4Panel;//pointer to LED4 image-panel
	ATManage->ledOrdLblG[0] = LEDord1LblG;//pointer to LED-name label
	ATManage->ledOrdLblG[1] = LEDord2LblG;//pointer to LED-name label
	ATManage->ledOrdLblG[2] = LEDord3LblG;//pointer to LED-name label
	ATManage->ledOrdLblG[3] = LEDord4LblG;//pointer to LED-name label

	for (ii = 0; ii < NUM_CHANNELS; ii++) // run over bitmaps
	{
		ATManage->imagObj[ii]->Picture->Bitmap->PixelFormat = pf24bit;//pixel format
		ATManage->imagObj[ii]->Stretch = true;//zoomable images
	}
	reSizeCoeff = 1;//resize coefficient (CCD Horizont size / actual image size)
}
//---------------------------------------------------------------------------

void __fastcall TGraphImag::LEDImgMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y)
{
	//start frame dragging
	__int32 ii, r;

	if ((actNumROI < roiMaxN) && (Button == mbLeft) && Shift.Contains(ssShift)) //rectangular ROI creation allowed
	{
		freeROIflag = false;//true if user draw free contour
		rectROIflag = true;//true if user draw rectangular contour
		rectROI.Left = X * reSizeCoeff;//x-coordinate of left top corner
		rectROI.Top = Y * reSizeCoeff;//y-coordinate of left top corner
	}
	else if ((actNumROI < roiMaxN) && (Button == mbLeft) && !Shift.Contains(ssShift)) //freeform ROI creation allowed
	{
		freeROIflag = true;//true if user draw free contour
		rectROIflag = false;//true if user draw rectangular contour
		freeROI[actNumROI][0][0] = 1;//points number in freeform ROIs
		freeROI[actNumROI][1][0] = 1;//points number in freeform ROIs
		freeROI[actNumROI][0][1] = X * reSizeCoeff;//countour X-points of freeform ROIs
		freeROI[actNumROI][1][1] = Y * reSizeCoeff;//countour Y-points of freeform ROIs
	}

	//prepare right-button menu
	roiToDel = -1;//number of existing ROI to be delete
	if  (Button == mbRight) //right-button down
	{
		for (ii = 0; ii < actNumROI; ii++) //run over ROIs
		{
			if (PointInPolygon(X * reSizeCoeff, Y * reSizeCoeff, ii)) //point within contour
				roiToDel = ii;//number of existing ROI to be delete
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TGraphImag::LEDImgMouseMove(TObject *Sender, TShiftState Shift, int X, int Y)
{
	__int32 ii, r,
			imN;//image index
	float h,//horizontal resolution
		  v;//vertical resolution

	h = float(ATManage->bHorzRes);//horizontal resolution
	v = float(ATManage->bVertRes);//vertical resolution

	if ((freeROI[actNumROI][0][0] < 990) && freeROIflag) //user is drawing free line
	{
		freeROI[actNumROI][0][0]++;//points number in freeform ROIs
		freeROI[actNumROI][1][0]++;//points number in freeform ROIs
		freeROI[actNumROI][0][freeROI[actNumROI][0][0]] = X * reSizeCoeff;//countour X-points of freeform ROIs
		freeROI[actNumROI][1][freeROI[actNumROI][1][0]] = Y * reSizeCoeff;//countour Y-points of freeform ROIs

		//draw free form
		for (imN = 0; imN < ATManage->acqParams.getImgNum; imN++) //run over all acquired images (different LEDs)
		{
			wglMakeCurrent(GraphImag->hndlDC[imN], GraphImag->hRC[imN]);//задаём текущий контекст рисования
			//glViewport(0, 0, ATManage->imagPnlObj[imN]->Width, ATManage->imagPnlObj[imN]->Height);//set correct paint range

			//draw edge of the ROI
			glBegin(GL_LINE_STRIP);//plot polygon
			r = freeROI[actNumROI][0][0];//vertexis index
			glColor3f(1, 0, 0);//set pixel color
			glVertex2f(2 * float(freeROI[actNumROI][0][r - 1]) / h - 1, 1 - 2 * float(freeROI[actNumROI][1][r - 1]) / v);//set vertex
			glVertex2f(2 * float(freeROI[actNumROI][0][r]) / h - 1, 1 - 2 * float(freeROI[actNumROI][1][r]) / v);//set vertex
			glEnd();

			SwapBuffers(GraphImag->hndlDC[imN]);//обновляем буфер (не двойная буферизация) //plot via OpenGL
		}
	}
	else if (rectROIflag) //rectangular ROI
	{
		/*
		rectROI.Left = X;//x-coordinate of left top corner
		rectROI.Top = Y;//y-coordinate of left top corner
		*/
		for (imN = 0; imN < ATManage->acqParams.getImgNum; imN++) //run over all acquired images (different LEDs)
		{
			wglMakeCurrent(GraphImag->hndlDC[imN], GraphImag->hRC[imN]);//задаём текущий контекст рисования
			//glViewport(0, 0, ATManage->imagPnlObj[imN]->Width, ATManage->imagPnlObj[imN]->Height);//set correct paint range
			glBegin(GL_LINE_LOOP);//plot polygon
			glColor3f(1, 0, 0);//set pixel color
			glVertex2f(2 * float(rectROI.Left) / h - 1, 1 - 2 * float(rectROI.Top) / v);//set pixel
			glVertex2f(2 * float(X) / h - 1, 1 - 2 * float(rectROI.Top) / v);//set pixel
			glVertex2f(2 * float(X) / h - 1, 1 - 2 * float(Y) / v);//set pixel
			glVertex2f(2 * float(rectROI.Left) / h - 1, 1 - 2 * float(Y) / v);//set pixel
			glEnd();

			SwapBuffers(GraphImag->hndlDC[imN]);//обновляем буфер (не двойная буферизация) //plot via OpenGL
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TGraphImag::LEDImgMouseUp(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y)
{
	//end of frame dragging (create ROI)
	__int32 ii, r, ix, iy,
			ifrp,//counter of full image points
			imN;//image index


	/*
	freeROI[actNumROI][0][0]++;//points number in freeform ROIs
	freeROI[actNumROI][1][0]++;//points number in freeform ROIs
	freeROI[actNumROI][0][freeROI[actNumROI][0][0]] = X;//countour X-points of freeform ROIs
	freeROI[actNumROI][1][freeROI[actNumROI][1][0]] = Y;//countour Y-points of freeform ROIs
	*/

	if ((actNumROI < roiMaxN) && (Button == mbLeft) && !freeROIflag) //ROI creation allowed
	{
		if (X > rectROI.Left)
			rectROI.Right = X * reSizeCoeff;//x-coordinate of right bottom corner
		else
		{
			rectROI.Right = rectROI.Left;//x-coordinate of right bottom corner
			rectROI.Left = X * reSizeCoeff;//x-coordinate of left top corner
		}

		if (Y > rectROI.Top)
			rectROI.Bottom = Y * reSizeCoeff;//y-coordinate of right bottom corner
		else
		{
			rectROI.Bottom = rectROI.Top;//y-coordinate of right bottom corner
			rectROI.Top = Y * reSizeCoeff;//y-coordinate of left top corner
        }

		//repcreat roi as freeroi
		freeROI[actNumROI][0][0] = 4;//points number in freeform ROIs
		freeROI[actNumROI][1][0] = 4;//points number in freeform ROIs
		freeROI[actNumROI][0][1] = rectROI.Left;//countour X-points of freeform ROIs
		freeROI[actNumROI][1][1] = rectROI.Top;//countour Y-points of freeform ROIs
		freeROI[actNumROI][0][2] = rectROI.Right;//countour X-points of freeform ROIs
		freeROI[actNumROI][1][2] = rectROI.Top;//countour Y-points of freeform ROIs
		freeROI[actNumROI][0][3] = rectROI.Right;//countour X-points of freeform ROIs
		freeROI[actNumROI][1][3] = rectROI.Bottom;//countour Y-points of freeform ROIs
		freeROI[actNumROI][0][4] = rectROI.Left;//countour X-points of freeform ROIs
		freeROI[actNumROI][1][4] = rectROI.Bottom;//countour Y-points of freeform ROIs

		rectROIflag = false;//true if user draw rectangular contour
	}
	else if ((actNumROI < roiMaxN) && (Button == mbLeft) && freeROIflag) //freeform ROI creation allowed
	{
		freeROIflag = false;//true if use draw free contour
	}

	r = 0;//number of points within current ROI
	for (iy = 0; iy < ATManage->bVertRes; iy++) //run over rows
	{
		for (ix = 0; ix < ATManage->bHorzRes; ix++) //run over colums
		{
			if (PointInPolygon(ix, iy, actNumROI)) //point within contour
				r++;//point in polygon
		}
	}

	if (r > 0) //nonempty ROI
	{
		roiMask[actNumROI] = new __int32[1 + r];//memory preallocation for ROI-masks (array of indexes)
		roiMask[actNumROI][0] = r;//number of points in ROI
		r = 1;//number of point in roiMask
		ifrp = 0;//counter of full image points
		for (iy = 0; iy < ATManage->bVertRes; iy++) //run over rows
		{
			for (ix = 0; ix < ATManage->bHorzRes; ix++) //run over colums
			{
				if (PointInPolygon(ix, iy, actNumROI)) //point within contour
				{
					roiMask[actNumROI][r] = ifrp;//remember current point as point inside current ROI
					r++;//point in polygon
				}
				ifrp++;//next point of full image
			}
		}
		actNumROI++;//acutal number of created ROI

		ATManage->ProgList->Lines->Add(IntToStr(actNumROI) + " ROI created");
		ATManage->ProgList->Lines->Add(IntToStr(roiMask[actNumROI - 1][0]) + " (" + IntToStr(roiMask[actNumROI - 1][1]) + "/" + IntToStr(roiMask[actNumROI - 1][2]) + ")");
	}

	for (imN = 0; imN < ATManage->acqParams.getImgNum; imN++) //run over all acquired images (different LEDs)
	{
		wglMakeCurrent(GraphImag->hndlDC[imN], GraphImag->hRC[imN]);
		glClearColor(0, 0, 0, 0);//очистка экрана в черный цвет
	}
	ReDrawImagesROIs(this);//redraw images and ROIs
}
//---------------------------------------------------------------------------

bool TGraphImag::PointInPolygon(__int32 ix, __int32 iy, __int32 cfROI)
{
	//ix, iy - point to solve in or out of contour
	//cfROI - number of current freeform ROI

	__int32 ii,
			x1, x2, xE, y1, y2;
	float intrsct;//number intersections
	bool inp;//point in polygon

	/*
	freeROI[actNumROI][0][0]++;//points number in freeform ROIs
	freeROI[actNumROI][1][0]++;//points number in freeform ROIs
	freeROI[actNumROI][0][freeROI[actNumROI][0][0]] = X;//countour X-points of freeform ROIs
	freeR[actNumROI][1][freeROI[actNumROI][1][0]] = Y;//countour Y-points of freeform ROIs
	*/
	intrsct = 0;//number intersections
	for (ii = 1; ii < freeROI[cfROI][0][0]; ii++) //run over polygon vertex
	{
		x1 = freeROI[cfROI][0][ii];//edge x1
		y1 = freeROI[cfROI][1][ii];//edge y1
		x2 = freeROI[cfROI][0][ii + 1];//edge x2
		y2 = freeROI[cfROI][1][ii + 1];//edge y2

		if (((y1 < iy) && (y2 >= iy)) || ((y1 >= iy) && (y2 < iy))) //first condition
		{
			xE = x1 + (iy - y1) * (x2 - x1) / (y2 - y1);
			if (ix < xE) //last condition
				intrsct++;//one more intersection
		}
	}
	//final edge
	ii = freeROI[cfROI][0][0];//number of vertex in current polygon
	y1 = freeROI[cfROI][1][ii];//edge y1
	x1 = freeROI[cfROI][0][ii];//edge x1
	y2 = freeROI[cfROI][1][1];//edge y2
	x2 = freeROI[cfROI][0][1];//edge x2
	if (((y1 < iy) && (y2 >= iy)) || ((y1 >= iy) && (y2 < iy))) //first condition
	{
		xE = x1 + (iy - y1) * (x2 - x1) / (y2 - y1);
		if (ix < xE) //last condition
			intrsct++;//one more intersection
	}

	y1 = floor(intrsct / 2);
	intrsct = intrsct / 2;
	inp = ((intrsct > 0) && ((intrsct - y1) > 0.3));
	return inp;
}
//---------------------------------------------------------------------------

void __fastcall TGraphImag::SaveClkClick(TObject *Sender)
{
	//save clicked image
}
//---------------------------------------------------------------------------

void __fastcall TGraphImag::DeleteROIClick(TObject *Sender)
{
	//delete clicked ROI
	__int32 ii, jj,
			imN;//image index


	/*
	freeROI[actNumROI][0][0]++;//points number in freeform ROIs
	freeROI[actNumROI][1][0]++;//points number in freeform ROIs
	freeROI[actNumROI][0][freeROI[actNumROI][0][0]] = X;//countour X-points of freeform ROIs
	freeROI[actNumROI][1][freeROI[actNumROI][1][0]] = Y;//countour Y-points of freeform ROIs
	*/
	ATManage->ProgList->Lines->Add("ROI" + IntToStr(roiToDel + 1) + " deleted");
	if (roiToDel >= 0) //delete single ROI
	{
		for (ii = roiToDel; ii < actNumROI; ii++) //run over ROIs after deleted
		{
			for (jj = 0; jj <= freeROI[ii + 1][0][0]; jj++) //run over vertexes
			{
				freeROI[ii][0][jj] = freeROI[ii + 1][0][jj];//copy next ROI
				freeROI[ii][1][jj] = freeROI[ii + 1][1][jj];//copy next ROI
			}
		}
		actNumROI--;//reduce ROI number
	}
	for (imN = 0; imN < ATManage->acqParams.getImgNum; imN++) //run over all acquired images (different LEDs)
	{
		wglMakeCurrent(GraphImag->hndlDC[imN], GraphImag->hRC[imN]);
		glClearColor(0, 0, 0, 0);//очистка экрана в черный цвет
	}
	ReDrawImagesROIs(this);//redraw images and ROIs
}
//---------------------------------------------------------------------------

void __fastcall TGraphImag::DeleteAllROIClick(TObject *Sender)
{
	//delete all ROIs
	__int32 ii,
			imN;//image index

	//for (ii = 0; ii < actNumROI; ii++) //run over ROIs after deleted
		//erase ROI
	actNumROI = 0;//delete all ROI

	for (imN = 0; imN < ATManage->acqParams.getImgNum; imN++) //run over all acquired images (different LEDs)
	{
		wglMakeCurrent(GraphImag->hndlDC[imN], GraphImag->hRC[imN]);
		glClearColor(0, 0, 0, 0);//очистка экрана в черный цвет
	}
	ReDrawImagesROIs(this);//redraw images and ROIs
}
//---------------------------------------------------------------------------

void __fastcall TGraphImag::BrightPorogChange(TObject *Sender)
{
	//change brightness magnification
	ATManage->brtMag = 1 + (BrightPorog->Position - 1) * 0.2;//brightness amplification
	ReDrawImagesROIs(this);//redraw images and ROIs
}
//---------------------------------------------------------------------------

void __fastcall TGraphImag::FormResize(TObject *Sender)
{
	//change iamge positions when window resized
	__int32 ii,
			imWd,//new width of each image
			imHt;//new height of each image

	if (ATManage->acqParams.getImgNum > 0) //any image is drawn
	{
		imWd = (GraphImag->Width - 20 - 10 - ATManage->acqParams.getImgNum) / ATManage->acqParams.getImgNum;//new width of each image
		imHt = __int32(float(imWd) * (ATManage->bVertRes / ATManage->bHorzRes));//new height of each image

		if (GraphImag->Height < 100) //too small window
		{
			GraphImag->OnResize = NULL;//switch off reaction
			GraphImag->Height = 100;//smallest height of window
			GraphImag->OnResize = GraphImag->FormResize;//switch off reaction
		}

		if (GraphImag->Height < (imHt + 55)) //window smaller
		{
			imHt = GraphImag->Height - 55;//new height of each image
			imWd = __int32(float(imHt) * (ATManage->bHorzRes / ATManage->bVertRes));//new width of each image
        }

		//set images sizes and visibility
		for (ii = 0; ii < NUM_CHANNELS; ii++) //run over needed image-objects
		{
			//set image size
			if (ii < ATManage->acqParams.getImgNum) //image in use
			{
				ATManage->imagPnlObj[ii]->Top = 20;//vertical position
				ATManage->imagPnlObj[ii]->Left = ii * (imWd + 1) + 20;//horizontal position
				ATManage->imagPnlObj[ii]->Width = imWd;//horizontal size of image
				ATManage->imagPnlObj[ii]->Height = imHt;//vertical size of image

				ATManage->ledOrdLblG[ii]->Left = ii * (imWd + 1) + (imWd / 2) - 10;//horizontal position
			}
			else //not used image
			{
				ATManage->imagPnlObj[ii]->Top = 20;//vertical position
				ATManage->imagPnlObj[ii]->Left = (ATManage->acqParams.getImgNum - 1) * imWd;//horizontal position
			}
		}
		BrightPorog->Height = imHt;//scrolbar size
		reSizeCoeff = float(ATManage->bHorzRes) / float(imWd);//resize coefficient
		ReDrawImagesROIs(this);//redraw images and ROIs
	}
}
//---------------------------------------------------------------------------

void __fastcall TGraphImag::FormCreate(TObject *Sender)
{
	__int32 ii;

	for (ii = 0; ii < NUM_CHANNELS; ii++) //run over panels
	{
		hndlDC[ii] = GetDC(ATManage->imagPnlObj[ii]->Handle);//получаем ссылку на окно куда будем рисовать
		PixelFormat = ChoosePixelFormat(hndlDC[ii], &pfd);//задаём параметр пикселя
		SetPixelFormat(hndlDC[ii], PixelFormat, &pfd);//задаём формат
		hRC[ii] = wglCreateContext(hndlDC[ii]);//создаём контекст рисования
	}
}
//---------------------------------------------------------------------------

void __fastcall TGraphImag::ReDrawImagesROIs(TObject *Sender)
{
	__int32 x, y, ii, r, jj,
			imN,//image index
			radr;//raw images address
	float c,//color for OpenGL
		  brtM,//brightness magnitude
		  h,//horizontal resolution
		  v;//vertical resolution
	bool inp;

	h = float(ATManage->bHorzRes);//horizontal resolution (non resize image width)
	v = float(ATManage->bVertRes);//vertical resolution (non resize image high)
	brtM = float(ATManage->brtMag);//brightness magnitude

	radr = 0;//raw images addres
	for (imN = 0; imN < ATManage->acqParams.getImgNum; imN++) //run over all acquired images (different LEDs)
	{
		wglMakeCurrent(GraphImag->hndlDC[imN], GraphImag->hRC[imN]);//задаём текущий контекст рисования
		glViewport(0, 0, ATManage->imagPnlObj[imN]->Width, ATManage->imagPnlObj[imN]->Height);//set correct paint range
		glPointSize(1.2 * float(ATManage->imagPnlObj[imN]->Width) / h);//expand points also

        //redraw original image
		glBegin(GL_POINTS);//plot via OpenGL
		for (y = 0; y < ATManage->bVertRes; y++) //run over rows
		{
			for (x = 0; x < ATManage->bHorzRes; x++) //run over colums
			{
				//plot via OpenGL
				c = brtM * ATManage->rawImgsSat[radr];//float(ATManage->rawImgs[radr]) / ATManage->saturatLevel;//float color
				glColor3f(c, c, c);//set pixel color
				glVertex2f(2 * float(x) / h - 1, 1 - 2 * float(y) / v);//set pixel
				radr++;//next pixel
			}
		}
		glEnd();//plot via OpenGL

		//redraw all ROIs
		for (ii = 0; ii < actNumROI; ii++) //run over ROIs
		{
			glBegin(GL_LINE_LOOP);//plot polygon
			for (r = 1; r <= freeROI[ii][0][0]; r++) //run over vertexis
			{
				glColor3f(1, 0, 0);//set pixel color
				glVertex2f(2 * float(freeROI[ii][0][r]) / h - 1, 1 - 2 * float(freeROI[ii][1][r]) / v);//set pixel
			}
			glEnd();


//			//===== fill poligon for checking =====//
//			r = 0;
//			glBegin(GL_POINTS);//plot via OpenGL
//			for (y = 0; y < ATManage->bVertRes; y++) //run over rows
//			{
//				for (x = 0; x < ATManage->bHorzRes; x++) //run over colums
//				{
//					inp = false;
//					for (jj = 1; ((jj < roiMask[ii][0]) && (!inp)); jj++)
//						if (r == roiMask[ii][jj])
//							inp = true;
//					if (inp)
//					{
//						//plot via OpenGL
//						glColor3f(1, 0, 0);//set pixel color
//						glVertex2f(2 * float(x) / h - 1, 1 - 2 * float(y) / v);//set pixel
//					}
//					r++;
//				}
//			}
//			glEnd();
//			//===== end of (fill poligon for checking) =====//


		}
		SwapBuffers(GraphImag->hndlDC[imN]);//обновляем буфер (не двойная буферизация) //plot via OpenGL
	}
}
//---------------------------------------------------------------------------

void __fastcall TGraphImag::FormActivate(TObject *Sender)
{
	ReDrawImagesROIs(this);//redraw images and ROIs
}
//---------------------------------------------------------------------------

