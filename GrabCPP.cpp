// -----------------------------------------------------------------------------------------
// Sapera++ console grab example
// 
//    This program shows how to grab images from a camera into a buffer in the host
//    computer's memory, using Sapera++ Acquisition and Buffer objects, and a Transfer 
//    object to link them.  Also, a View object is used to display the buffer.
//
// -----------------------------------------------------------------------------------------

// Disable deprecated function warnings with Visual Studio 2005\

/// Modified for Use with OpenCV 
/// Comments with "///" indicate changes from original code 

#if defined(_MSC_VER) && _MSC_VER >= 1400
#pragma warning(disable: 4995)
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "stdio.h"
#include "conio.h"
#include "math.h"
#include "sapclassbasic.h"
#include "ExampleUtils.h"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <fstream>

using namespace cv;
using namespace std;


// Restore deprecated function warnings with Visual Studio 2005
#if defined(_MSC_VER) && _MSC_VER >= 1400
#pragma warning(default: 4995)
#endif

// Static Functions
static void XferCallback(SapXferCallbackInfo *pInfo);
static BOOL GetOptions(int argc, char *argv[], char *acqServerName, UINT32 *pAcqDeviceIndex, char *configFileName);
static BOOL GetOptionsFromCommandLine(int argc, char *argv[], char *acqServerName, UINT32 *pAcqDeviceIndex, char *configFileName);

/// Globals 
Mat Img;

/// Address for UDP Socket


int main(int argc, char* argv[])
{
   UINT32   acqDeviceNumber;
   char*    acqServerName = new char[CORSERVER_MAX_STRLEN];
   char*    configFilename = new char[MAX_PATH];

   printf("Sapera Console Grab Example (C++ version)\n");

   // Call GetOptions to determine which acquisition device to use and which config
   // file (CCF) should be loaded to configure it.
   // Note: if this were an MFC-enabled application, we could have replaced the lengthy GetOptions 
   // function with the CAcqConfigDlg dialog of the Sapera++ GUI Classes (see GrabMFC example)
   if (!GetOptions(argc, argv, acqServerName, &acqDeviceNumber, configFilename))
   {
      printf("\nPress any key to terminate\n");
      CorGetch();
      return 0;
   }

  

   

   SapAcquisition Acq;
   SapAcqDevice AcqDevice;
   SapBufferWithTrash Buffers;
   SapTransfer AcqToBuf = SapAcqToBuf(&Acq, &Buffers);
   SapTransfer AcqDeviceToBuf = SapAcqDeviceToBuf(&AcqDevice, &Buffers);
   SapTransfer* Xfer = NULL;
   SapView View;
   SapLocation loc(acqServerName, acqDeviceNumber);

   if (SapManager::GetResourceCount(acqServerName, SapManager::ResourceAcq) > 0)
   {
      Acq = SapAcquisition(loc, configFilename);
      Buffers = SapBufferWithTrash(2, &Acq);
      View = SapView(&Buffers, SapHwndAutomatic);
      AcqToBuf = SapAcqToBuf(&Acq, &Buffers, XferCallback, &View);
      Xfer = &AcqToBuf;

      // Create acquisition object
      if (!Acq.Create())
         goto FreeHandles;

   }

   else if (SapManager::GetResourceCount(acqServerName, SapManager::ResourceAcqDevice) > 0)
   {
      if (strcmp(configFilename, "NoFile") == 0)
         AcqDevice = SapAcqDevice(loc, FALSE);
      else
         AcqDevice = SapAcqDevice(loc, configFilename);

      Buffers = SapBufferWithTrash(2, &AcqDevice);
      View = SapView(&Buffers, SapHwndAutomatic);
      AcqDeviceToBuf = SapAcqDeviceToBuf(&AcqDevice, &Buffers, XferCallback, &View);
      Xfer = &AcqDeviceToBuf;

      // Create acquisition object
      if (!AcqDevice.Create())
         goto FreeHandles;
   }

   ///Initialize MAT Container for OpenCV to correct width and heigh, filled with zeros 
   Img = Mat::zeros(Buffers.GetHeight(), Buffers.GetWidth(), CV_8UC1);

   // Create buffer object
   if (!Buffers.Create())
      goto FreeHandles;
   

   // Create transfer object
   if (Xfer && !Xfer->Create())
      goto FreeHandles;

   // Create view object
   if (!View.Create())
      goto FreeHandles;
   
   // Start continous grab
  
   Xfer->Grab();
   
   ///Initialize MAT Container for OpenCV to correct width and heigh, filled with zeros  (need 2nd time to prevent being skipped by goto)
   Img = Mat::zeros(Buffers.GetHeight(), Buffers.GetWidth(), CV_8UC1);

   printf("Press any key to stop grab\n");
   CorGetch();

   // Stop grab
   Xfer->Freeze();
   if (!Xfer->Wait(5000))
      printf("Grab could not stop properly.\n");

FreeHandles:

   printf("Press any key to terminate\n");

   CorGetch();

   //unregister the acquisition callback
   Acq.UnregisterCallback();

   // Destroy view object
   if (!View.Destroy()) return FALSE;

   // Destroy transfer object
   if (Xfer && *Xfer && !Xfer->Destroy()) return FALSE;

   // Destroy buffer object
   if (!Buffers.Destroy()) return FALSE;

   // Destroy acquisition object
   if (!Acq.Destroy()) return FALSE;

   // Destroy acquisition object
   if (!AcqDevice.Destroy()) return FALSE;

   return 0;
}

static void XferCallback(SapXferCallbackInfo *pInfo)
{
   ///*** BEGIN MODIFICATION FOR OPENCV ***
   SapView *pView = (SapView *)pInfo->GetContext();
   PUINT8 pData;
   double largest_area = 0;
   int largest_contour_index = 0;
   const char *path = "C:\\Users\\Bobby\\Documents\\GitHub\\GLO_Tracking\\Data.txt"; ///path to write data to 
   std::ofstream file(path); 
   double distx; ///distance from center, declared as double bc it is converted to degrees in future (often less than 1 )
   double disty;
   int x, y;
   Point TL;
   Point Br;
   Point framcenter; ///OpenCV variables for modification of Mat object 
   Rect bounding_rect;
   Point center;
   vector<vector<Point>> contours;
   vector<Vec4i> hierarchy;
   framcenter.x = 640; ///this represents the true center of the fram if 1280x1024
   framcenter.y = 512; 
   TL.x = framcenter.x - ((405 / 2) + 0.005/ 0.0000617284);
   TL.y = framcenter.y + ((405 / 2) + 0.005/ 0.0000617284);
   Br.x = framcenter.x + ((405 / 2) + 0.005/ 0.0000617284);
   Br.y = framcenter.y - ((405 / 2) + 0.005/ 0.0000617284);
   // refresh view
   ///pView->Show();  no longer want to waste resources viewing with sapera window, using openCV window 
  
   SapBuffer *Buf = pView->GetBuffer(); ///retrieve current buffer
   Buf->GetAddress((void**)&pData); ///retrieve pointer to intensity matrix values from Buffer
   int height = Buf->GetHeight(); ///height and width of current frame grabbed 
   int width = Buf->GetWidth();


   for (int i = 0; i < height; i++) ///loop through the size of the whole image and store intensity values in the Mat container Img
   {
	   for (int j = 0; j < width; j++)
	   {
		   Img.at<uchar>(i, j) = *pData;
		   pData++; ///increment through each memory address of pData 
	   }
   }
   Buf->ReleaseAddress((void**)&pData); ///release pointer to intensity matrix 
  
   /// GaussianBlur(Img, Img, Size(3, 3), 0, 0);
   ///This can be used to filter the image Size(x,x) changes the kernal size 

   threshold(Img,Img, 20, 255, THRESH_BINARY); ///Threshold the gray (change 3rd parameter based on brightness of "sun")
   findContours(Img, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_TC89_L1); /// Find the contours in the image
   
   for (int i = 0; i< contours.size(); i++) /// iterate through each contour. 
   {
	   double a = contourArea(contours[i], false);  ///  Find the area of contour

	   if (a>largest_area) 
	   {
		   largest_area = a;
		   largest_contour_index = i;                ///Store the index of largest contour
		   bounding_rect = boundingRect(contours[i]); /// Find the bounding rectangle for biggest contour   
		  // cout << i << "\n";
		  //cout << contours.size() << "\n";
	   }
   }
  
   rectangle(Img, bounding_rect, Scalar(80,80 ,80), 5, 2, 0); ///draw a rectangle around the disk of the sun 
   rectangle(Img, TL,Br, Scalar(200, 200, 200), 5, 2, 0); ///rectangle to show 0.1 deg

   center = (bounding_rect.br() + bounding_rect.tl())*0.5; ///find center of the circle (tl= topleft, br=botttomright)
   circle(Img, center, 5, Scalar(180, 180, 180), 2); /// draw center on sun
   circle(Img, framcenter, 5, Scalar(100, 100, 100), 2); /// draw center of frame   

  
   
   x = center.x;
   y = center.y;

   /// Point rad = (bounding_rect.tl()-bounding_rect.br());
   ///cout << rad;
  
   distx = x - framcenter.x;
   disty = y - framcenter.y;

   distx *= 0.0000617284;  ///change pixel scaling (this is for .025/405 deg/pixel, default is .001 degree per pixel) 
   disty *= 0.0000617284;

   /*
   distx *= 0.001241;  ///change pixel scaling (this is for .53/427 deg/pixel, default is .001 degree per pixel) 
   disty *= 0.001241;
   */

   file << distx << "," << disty; ///write center location to file to be grabbed by python
   file.close();

   //cout << "Distance from center of Frame: " << (center.x - frame_center.x) << "," << (center.y - frame_center.y) << "\n";

   imshow("frame", Img);  /// Shows the current frame through the OpenCV window 
   waitKey(1);
   
   ///*** END OF MODIFICATION FOR OPENCV ***///

   // refresh framerate
   static float lastframerate = 0.0f;
   SapTransfer* pXfer = pInfo->GetTransfer();
   if (pXfer->UpdateFrameRateStatistics())
   {
	   /// export the array of values here 
      SapXferFrameRateInfo* pFrameRateInfo = pXfer->GetFrameRateStatistics();
      float framerate = 0.0f;

	  
      if (pFrameRateInfo->IsLiveFrameRateAvailable())
         framerate = pFrameRateInfo->GetLiveFrameRate();

      // check if frame rate is stalled
      if (pFrameRateInfo->IsLiveFrameRateStalled())
      {
         printf("Live frame rate is stalled.\n");
      }
      // update FPS only if the value changed by +/- 0.1
      else if ((framerate > 0.0f) && (abs(lastframerate - framerate) > 0.1f))
      {
         printf("Grabbing at %.1f frames/sec\n", framerate);
         lastframerate = framerate;
      }
   }
}

static BOOL GetOptions(int argc, char *argv[], char *acqServerName, UINT32 *pAcqDeviceIndex, char *configFileName)
{
   // Check if arguments were passed
   if (argc > 1)
      return GetOptionsFromCommandLine(argc, argv, acqServerName, pAcqDeviceIndex, configFileName);
   else
      return GetOptionsFromQuestions(acqServerName, pAcqDeviceIndex, configFileName);
}

static BOOL GetOptionsFromCommandLine(int argc, char *argv[], char *acqServerName, UINT32 *pAcqDeviceIndex, char *configFileName)
{
   // Check the command line for user commands
   if ((strcmp(argv[1], "/?") == 0) || (strcmp(argv[1], "-?") == 0))
   {
      // print help
      printf("Usage:\n");
      printf("GrabCPP [<acquisition server name> <acquisition device index> <config filename>]\n");
      return FALSE;
   }

   // Check if enough arguments were passed
   if (argc < 4)
   {
      printf("Invalid command line!\n");
      return FALSE;
   }

   // Validate server name
   if (SapManager::GetServerIndex(argv[1]) < 0)
   {
      printf("Invalid acquisition server name!\n");
      return FALSE;
   }

   // Does the server support acquisition?
   int deviceCount = SapManager::GetResourceCount(argv[1], SapManager::ResourceAcq);
   int cameraCount = SapManager::GetResourceCount(argv[1], SapManager::ResourceAcqDevice);

   if (deviceCount + cameraCount == 0)
   {
      printf("This server does not support acquisition!\n");
      return FALSE;
   }

   // Validate device index
   if (atoi(argv[2]) < 0 || atoi(argv[2]) >= deviceCount + cameraCount)
   {
      printf("Invalid acquisition device index!\n");
      return FALSE;
   }

   ///if (cameraCount == 0)
   {
      // Verify that the specified config file exist
      OFSTRUCT of = { 0 };
      if (OpenFile(argv[3], &of, OF_EXIST) == HFILE_ERROR)
      {
         printf("The specified config file (%s) is invalid!\n", argv[3]);
         return FALSE;
      }
   }

   // Fill-in output variables
   CorStrncpy(acqServerName, argv[1], CORSERVER_MAX_STRLEN);
   *pAcqDeviceIndex = atoi(argv[2]);
   ///if (cameraCount == 0)
      CorStrncpy(configFileName, argv[3], MAX_PATH);

   return TRUE;
}

