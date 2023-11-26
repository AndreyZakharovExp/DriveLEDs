//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
//---------------------------------------------------------------------------

USEFORM("ThorAndor.cpp", ATManage);
USEFORM("BrtTimeCours.cpp", RoiGraph);
USEFORM("Graphs.cpp", GraphImag);
//---------------------------------------------------------------------------
WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
	try
	{
		Application->Initialize();
		Application->MainFormOnTaskBar = true;
		Application->CreateForm(__classid(TATManage), &ATManage);
		Application->CreateForm(__classid(TRoiGraph), &RoiGraph);
		Application->CreateForm(__classid(TGraphImag), &GraphImag);
		Application->Run();
	}
	catch (Exception &exception)
	{
		Application->ShowException(&exception);
	}
	catch (...)
	{
		try
		{
			throw Exception("");
		}
		catch (Exception &exception)
		{
			Application->ShowException(&exception);
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
